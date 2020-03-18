#pragma once

#include "../dtypes.h"
#include <functional>

#include "./detail/config.h"
#include <asio/ip/udp.hpp>

#include "./Net.h"
#include "./ErrorCode.h"
#include "./EndPoint.h"

namespace cppu
{
	namespace net
	{
		class UDPSocket
		{
		private:
			asio::ip::udp::socket socket;

		private:

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
			UDPSocket()
				: socket(net::GetContext())
			{

			}

			ErrorCode Bind(const EndPoint& localEndPoint)
			{
				asio::ip::udp::endpoint udpEndPoint(localEndPoint.endPoint.address(), localEndPoint.endPoint.port());

				asio::error_code error;
				socket.open(udpEndPoint.protocol(), error);
				if (error)
					goto cancel_bind;

				socket.bind(udpEndPoint, error);
				if (error)
					goto cancel_bind;

				return static_cast<ErrorCode>(error.value());

			cancel_bind:
				socket.close();
				return static_cast<ErrorCode>(error.value());
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

			ErrorCode Send(const EndPoint& remoteEndPoint, const void* data, std::size_t size)
			{
				asio::error_code error;
				socket.send_to(asio::buffer(data, size), asio::ip::udp::endpoint(remoteEndPoint.endPoint.address(), remoteEndPoint.endPoint.port()), 0, error);
				return static_cast<ErrorCode>(error.value());
			}

			void SendAsync(const EndPoint& remoteEndPoint, const void* data, std::size_t size)
			{
				socket.async_send_to(asio::buffer(data, size), asio::ip::udp::endpoint(remoteEndPoint.endPoint.address(), remoteEndPoint.endPoint.port()),
					std::bind(&UDPSocket::SendHandler, this, std::placeholders::_1));

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
					std::bind(&UDPSocket::ReceiveHandler, this, std::placeholders::_1));

				details::threadsWait().notify_one();
			}

			std::size_t Receive(void* data, std::size_t size, EndPoint& remoteEndPoint)
			{
				asio::error_code error;
				return socket.receive_from(asio::buffer(data, size), reinterpret_cast<asio::ip::udp::endpoint&>(remoteEndPoint.endPoint), 0, error);
			}

			void ReceiveAsync(void* data, std::size_t size, EndPoint& remoteEndPoint)
			{
				socket.async_receive_from(asio::buffer(data, size), reinterpret_cast<asio::ip::udp::endpoint&>(remoteEndPoint.endPoint),
					std::bind(&UDPSocket::ReceiveHandler, this, std::placeholders::_1));

				details::threadsWait().notify_one();
			}
		};
	}
}

#ifdef CPPU_USE_NAMESPACE
using namespace cppu;
#endif