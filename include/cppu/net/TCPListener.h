#pragma once

#include "../dtypes.h"
#include <deque>
#include <mutex>
#include <functional>

#include "./detail/config.h"
#include <asio/ip/tcp.hpp>
#include "TCPSocket.h"

namespace cppu
{
	namespace net
	{
		class TCPListener
		{
		private:
			asio::ip::tcp::acceptor acceptor;
			std::mutex lock;
			asio::ip::tcp::socket nextSocket;
			std::deque<asio::ip::tcp::socket> acceptedSockets;

			void AcceptNext()
			{
				acceptor.async_accept(nextSocket, std::bind(&TCPListener::AcceptHandler, this, std::placeholders::_1));
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
			TCPListener()
				: acceptor(net::GetContext())
				, nextSocket(net::GetContext())
			{

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

			bool Accept(TCPSocket& socket)
			{
				std::lock_guard<std::mutex> lk(lock);
				if (!acceptedSockets.empty())
				{
					asio::ip::tcp::socket& oldsocket = acceptedSockets.back();
					socket.socket = std::move(oldsocket);
					acceptedSockets.pop_back();
					return true;
				}
				else
				{
					asio::error_code error;
					acceptor.accept(socket.socket, error);
					return socket.socket.is_open();
				}

				return false;
			}
		};
	}
}

#ifdef CPPU_USE_NAMESPACE
using namespace cppu;
#endif