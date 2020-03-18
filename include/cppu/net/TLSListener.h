#pragma once

#include "TCPListener.h"
#include "TLSSocket.h"

#include "TLSKey.h"
#include "TLSCertificate.h"

namespace bear
{
	using namespace bear;
	#include <bearssl.h>
}

namespace cppu
{
	namespace net
	{
		class TLSListener
		{
		private:
			std::mutex lock;
			asio::ip::tcp::acceptor acceptor;
			asio::ip::tcp::socket nextSocket;
			std::deque<asio::ip::tcp::socket> acceptedSockets;
			
			TLSCertificate certificate;
			TLSKey key;

			void AcceptNext()
			{
				acceptor.async_accept(nextSocket, std::bind(&TLSListener::AcceptHandler, this, std::placeholders::_1));
			}

			virtual void AcceptHandler(const asio::error_code& error)
			{
				if (!error)
				{
					std::lock_guard<std::mutex> lk(lock);
					acceptedSockets.emplace_front(std::move(nextSocket));
				}

				AcceptNext();
			}

		public:
			TLSListener()
				: acceptor(net::GetContext())
				, nextSocket(net::GetContext())
			{
			}

			void SetCertificate(const TLSCertificate& certificate, const TLSKey& key)
			{
				this->certificate = certificate;
				this->key = key;
			}

			ErrorCode Listen(const EndPoint& endpoint)
			{
				if (!acceptor.is_open())
				{
					asio::ip::tcp::endpoint tcpEndPoint(endpoint.endPoint.address(), endpoint.endPoint.port());

					try
					{
						acceptor.open(tcpEndPoint.protocol());
						acceptor.bind(tcpEndPoint);
						acceptor.listen();

						//AcceptNext();

						return Error::NONE;
					}
					catch (asio::error_code error)
					{
						acceptor.close();
						return error.value();
					}
				}
				else
					return Error::LISTENER_ALREADY_LISTENING;
			}

			bool IsOpen() const
			{
				return acceptor.is_open();
			}

			ErrorCode SetBlocking(bool enabled)
			{
				asio::error_code error;
				acceptor.non_blocking(!enabled, error);
				return static_cast<ErrorCode>(error.value());
			}

			bool GetBlocking() const
			{
				return !acceptor.non_blocking();
			}

			ErrorCode Close()
			{
				if (acceptor.is_open())
				{
					asio::error_code error;
					acceptor.close(error);
					return static_cast<ErrorCode>(error.value());
				}

				return Error::NONE;
			}

			bool Accept(TLSSocket& socket)
			{
				try
				{
					acceptor.accept(socket.socket);

					// verify
					unsigned char iobuf[BR_SSL_BUFSIZE_BIDI];
					
					socket.isServer = true;
					bear::br_ssl_server_init_full_rsa(&socket.sc, &certificate.data, 1, &key.rsaKey);	// init minimal ssl server with RSA

					/*
					* Set the I/O buffer to the provided array. We
					* allocated a buffer large enough for full-duplex
					* behaviour with all allowed sizes of SSL records,
					* hence we set the last argument to 1 (which means
					* "split the buffer into separate input and output
					* areas").
					*/
					bear::br_ssl_engine_set_buffer(&socket.sc.eng, iobuf, BR_SSL_BUFSIZE_BIDI * sizeof(char), 1);
					bear::br_ssl_server_reset(&socket.sc);													// Reset the server context, for a new handshake.
					bear::br_sslio_init(&socket.ioc, &socket.sc.eng, TLSSocket::sock_read, &socket, TLSSocket::sock_write, &socket);	// Initialise the simplified I/O wrapper context.

					// Read bytes until we get the ready sign
					char request[6];
					while (!(bear::br_sslio_read(&socket.ioc, request, 6) == 6 && strcmp(request, "ready") == 0));

					socket.Send("ready", 6);

					// Sample HTTP response to send.
					//static const char* HTTP_RES = "ready";
					//bear::br_sslio_write_all(&ioc, HTTP_RES, strlen(HTTP_RES));
					//bear::br_sslio_flush(&ioc);

				client_drop:
					if (int err = bear::br_ssl_engine_last_error(&socket.sc.eng))
					{
						socket.Close();
						return false;
					}

					return socket.socket.is_open();
				}
				catch (const asio::error_code&)
				{
					return false;
				}
			}
		};
	}
}

#ifdef CPPU_USE_NAMESPACE
using namespace cppu;
#endif