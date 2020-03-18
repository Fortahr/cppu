#pragma once

#include "../dtypes.h"
#include <functional>

#include "./detail/config.h"
#include <asio/ip/tcp.hpp>

#include "./Net.h"
#include "./ErrorCode.h"
#include "./EndPoint.h"

namespace cppu
{
	namespace net
	{
		class TCPListener;

		class TCPSocket
		{
			friend class TCPListener;
		private:
			asio::ip::tcp::socket socket;

			void ConnectHandler(const asio::error_code& error, const std::function<void(TCPSocket*, ErrorCode)>& callback)
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

			static void IgnoreCallback(TCPSocket*, ErrorCode)
			{}

		public:
			TCPSocket()
				: socket(net::GetContext())
			{

			}

			TCPSocket(TCPSocket&& move)
				: socket(std::move(move.socket))
			{
			}

			TCPSocket& operator=(TCPSocket&& move)
			{
				std::swap(this->socket, move.socket);
				return *this;
			}

			EndPoint GetLocalEndPoint()
			{
				return EndPoint(socket.local_endpoint().address(), socket.local_endpoint().port());
			}

			EndPoint GetRemoteEndPoint()
			{
				return EndPoint(socket.remote_endpoint().address(), socket.remote_endpoint().port());
			}

			ErrorCode Connect(const EndPoint& remoteEndPoint)
			{
				asio::error_code error;
				asio::ip::tcp::endpoint tcpEndPoint(remoteEndPoint.endPoint.address(), remoteEndPoint.endPoint.port());

				socket.open(tcpEndPoint.protocol(), error);
				if (error)
					goto cancel_connect;

				socket.connect(tcpEndPoint, error);
				if (error)
					goto cancel_connect;

				return static_cast<ErrorCode>(error.value());

			cancel_connect:
				socket.close();
				return static_cast<ErrorCode>(error.value());
			}

			void ConnectAsync(const EndPoint& remoteEndPoint, std::function<void(TCPSocket*, ErrorCode)> callback = IgnoreCallback)
			{
				socket.async_connect(asio::ip::tcp::endpoint(remoteEndPoint.endPoint.address(), remoteEndPoint.endPoint.port()),
					std::bind(&TCPSocket::ConnectHandler, this, std::placeholders::_1, callback));

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
					asio::error_code error;
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
				asio::error_code error;
				socket.send(asio::buffer(data, size), 0, error);
				return static_cast<ErrorCode>(error.value());
			}

			void SendAsync(const void* data, std::size_t size)
			{
				socket.async_send(asio::buffer(data, size),
					std::bind(&TCPSocket::SendHandler, this, std::placeholders::_1));
				
				details::threadsWait().notify_one();
			}

			std::size_t Receive(void* data, std::size_t size)
			{
				asio::error_code error;
				return socket.receive(asio::buffer(data, size), 0, error);
			}

			void ReceiveAsync(void* data, std::size_t size)
			{
				socket.async_receive(asio::buffer(data, size),
					std::bind(&TCPSocket::ReceiveHandler, this, std::placeholders::_1));

				details::threadsWait().notify_one();
			}
		};
	}
}

#ifdef CPPU_USE_NAMESPACE
using namespace cppu;
#endif