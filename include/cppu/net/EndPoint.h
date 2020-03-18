#pragma once


#include "../dtypes.h"

#include "./detail/config.h"
#include <asio/ip/detail/endpoint.hpp>

#include "IPAddress.h"



namespace cppu
{
	namespace net
	{
		class UDPSocket;
		class TCPSocket;
		class TCPListener;
		class TLSSocket;
		class TLSListener;

		struct EndPoint
		{
			friend class UDPSocket;
			friend class TCPSocket;
			friend class TCPListener;
			friend class TLSSocket;
			friend class TLSListener;

		private:
			asio::ip::detail::endpoint endPoint;

			EndPoint(const asio::ip::address& address, uint16 port)
				: endPoint(address, port)
			{
			}

			EndPoint(const asio::ip::detail::endpoint& endPoint)
				: endPoint(endPoint)
			{
			}


		public:
			EndPoint()
			{
			}

			EndPoint(const IPAddress& address, uint16 port)
				: endPoint(address.address, port)
			{
			}

			explicit EndPoint(uint16 port)
				: endPoint(asio::ip::address(), port)
			{
			}

			IPAddress GetAddress()
			{
				return IPAddress(endPoint.address());
			}
			
			void SetAddress(IPAddress address)
			{
				endPoint.address(address.address);
			}

			uint16 GetPort()
			{
				return endPoint.port();
			}

			void SetPort(uint16 port)
			{
				endPoint.port(port);
			}
		};
	}
}

#ifdef CPPU_USE_NAMESPACE
using namespace cppu;
#endif