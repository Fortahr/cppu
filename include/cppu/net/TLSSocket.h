#pragma once

#include "../dtypes.h"
#include "./detail/config.h"

#include <asio/ip/tcp.hpp>
#include <functional>

#include "./Net.h"
#include "./ErrorCode.h"
#include "./EndPoint.h"

#include "./TLSCertificate.h"

namespace bear
{
	using namespace bear;
	#include <bearssl.h>
}

namespace cppu
{
	namespace net
	{
		class TLSListener;

		class TLSSocket
		{
			friend class TLSListener;
		private:

			asio::ip::tcp::socket socket;

			bear::br_sslio_context ioc;
			bear::br_x509_minimal_context xc;
			bool isServer;
			union
			{
				bear::br_ssl_server_context sc;
				bear::br_ssl_client_context cc;
			};

			std::vector<unsigned char> buffer;
			std::string expectedHostName;
			
			void ConnectHandler(const asio::error_code& error, const std::function<void(cppu::net::TLSSocket*, cppu::net::ErrorCode)>& callback)
			{
				callback(this, static_cast<ErrorCode>(error.value()));
			}

			void SendHandler(const asio::error_code& error)
			{
				if (!error)
				{

				}
			}

			void ReceiveHandler(const asio::error_code& error)
			{
				if (!error)
				{

				}
			}
			
			static void IgnoreCallback(cppu::net::TLSSocket* socket, cppu::net::ErrorCode code)
			{}

			static int sock_read(void* ctx, unsigned char* buf, std::size_t len)
			{
				cppu::net::TLSSocket* socket = static_cast<cppu::net::TLSSocket*>(ctx);
				asio::error_code error;
				return socket->socket.receive(asio::buffer(buf, len), 0, error);
			}

			static int sock_write(void* ctx, const unsigned char* buf, std::size_t len)
			{
				cppu::net::TLSSocket* socket = static_cast<cppu::net::TLSSocket*>(ctx);
				asio::error_code error;
				socket->socket.send(asio::buffer(buf, len), 0, error);
				return static_cast<cppu::net::ErrorCode>(error.value()) == cppu::net::Error::NONE ? len : 0;
			}

		public:
			TLSSocket(bool isServer = false)
				: socket(cppu::net::GetContext())
				, buffer(BR_SSL_BUFSIZE_BIDI)
				, isServer(isServer)
			{

			}

			TLSSocket(TLSSocket&& move)
				: socket(std::move(move.socket))
				, ioc(std::move(move.ioc))
				, xc(std::move(move.xc))
				, buffer(BR_SSL_BUFSIZE_BIDI)
				, isServer(move.isServer)
			{
				if (isServer)
					sc = std::move(move.sc);
				else
					cc = std::move(move.cc);
			}

			TLSSocket& operator=(cppu::net::TLSSocket&& move)
			{
				std::swap(this->socket, move.socket);
				ioc = std::move(move.ioc);
				xc = std::move(move.xc);
				sc = std::move(move.sc);
				return *this;
			}

			~TLSSocket()
			{
				Close();
			}

			ErrorCode Connect(const EndPoint& remoteEndPoint)
			{
				asio::error_code error;
				asio::ip::tcp::endpoint tcpEndPoint(remoteEndPoint.endPoint.address(), remoteEndPoint.endPoint.port());
				auto& chain = cppu::net::TLSCertificate::GetCerticiatesChain();

				socket.open(tcpEndPoint.protocol(), error);
				if (!error)
				{

					socket.connect(tcpEndPoint, error);
					if (!error)
					{
						bear::br_ssl_client_init_full(&cc, &xc, chain.data(), chain.size());
						bear::br_ssl_engine_set_buffer(&cc.eng, buffer.data(), buffer.size(), 1);
						bear::br_ssl_client_reset(&cc, expectedHostName.data(), 0);
						bear::br_sslio_init(&ioc, &cc.eng, sock_read, this, sock_write, this);

						switch (sc.eng.err)
						{
						case 0:
							Send("ready", 6);

							if (!sc.eng.err)
								break; // only skip the default part when there's no error
						default:
							Close();
							return sc.eng.err;
						}

						return static_cast<cppu::net::ErrorCode>(error.value());
					}
				}

				return static_cast<cppu::net::ErrorCode>(error.value());
			}

			void ConnectAsync(const EndPoint& remoteEndPoint, std::function<void(TLSSocket*, ErrorCode)> callback = IgnoreCallback)
			{
				socket.async_connect(asio::ip::tcp::endpoint(remoteEndPoint.endPoint.address(), remoteEndPoint.endPoint.port()),
					std::bind(&TLSSocket::ConnectHandler, this, std::placeholders::_1, callback));

				details::threadsWait().notify_one();
			}

			bool IsOpen() const
			{
				return socket.is_open();
			}

			ErrorCode Close()
			{
				if (socket.is_open())
				{
					bear::br_sslio_write_all(&ioc, "close", 5);
					bear::br_sslio_close(&ioc);

					asio::error_code error;
					socket.shutdown(asio::socket_base::shutdown_both, error);
					socket.close(error);
					return static_cast<ErrorCode>(error.value());
				}

				return Error::NONE;
			}

			std::size_t DataAvailable() const
			{
				asio::error_code error;
				return socket.available(error);
			}

			ErrorCode SetBlocking(bool enabled)
			{
				asio::error_code error;
				socket.non_blocking(!enabled, error);
				return static_cast<ErrorCode>(error.value());
			}

			bool GetBlocking() const
			{
				return !socket.non_blocking();
			}
			
			ErrorCode Send(const void* data, std::size_t size)
			{
				if (bear::br_sslio_write_all(&ioc, data, size) != 0)
					return Error::UNKNOWN_ERROR;
				return bear::br_sslio_flush(&ioc);
			}

			ErrorCode Send(std::string_view data)
			{
				return Send(static_cast<const void*>(data.data()), data.size());
			}

			void SendAsync(const void* data, std::size_t size)
			{
				socket.async_send(asio::buffer(data, size),
					std::bind(&TLSSocket::SendHandler, this, std::placeholders::_1));
				
				details::threadsWait().notify_one();
			}
			
			std::size_t Receive(void* data, std::size_t size)
			{
				return bear::br_sslio_read(&ioc, data, size);
			}

			std::size_t Receive(std::string data)
			{
				return Receive(static_cast<void*>(&data[0]), data.size());
			}


			void ReceiveAsync(void* data, std::size_t size)
			{
				socket.async_receive(asio::buffer(data, size),
					std::bind(&TLSSocket::ReceiveHandler, this, std::placeholders::_1));

				details::threadsWait().notify_one();
			}

			EndPoint GetLocalEndPoint()
			{
				return EndPoint(socket.local_endpoint().address(), socket.local_endpoint().port());
			}

			EndPoint GetRemoteEndPoint()
			{
				return EndPoint(socket.remote_endpoint().address(), socket.remote_endpoint().port());
			}

			void SetExpectedHostName(std::string_view name)
			{
				expectedHostName = name;
			}
		};
	}
}

#ifdef CPPU_USE_NAMESPACE
using namespace cppu;
#endif