#pragma once

#include <string_view>
#include <iostream>

#include "./detail/config.h"
#include "./Net.h"
#include <asio/ip/address.hpp>
#include <asio/ip/host_name.hpp>
#include <asio/ip/udp.hpp>

namespace cppu
{
	namespace net
	{
		struct EndPoint;

		struct IPAddress
		{
			friend struct EndPoint;
		private:
			asio::ip::address address;

			IPAddress(const asio::ip::address& address)
				: address(address)
			{

			}

		public:
			IPAddress()
			{

			}

			IPAddress(std::string_view ip)
				: address(asio::ip::address::from_string(ip.data()))
			{
			}

			IPAddress(uint32 ip)
				: address(asio::ip::address_v4(ip))
			{
			}

			IPAddress(std::array<byte, 4> ip)
				: address(asio::ip::address_v4(ip))
			{
			}

			IPAddress(std::array<byte, 16> ip)
				: address(asio::ip::address_v6(ip))
			{
			}

			static IPAddress GetRemoteAddress(std::string_view domain)
			{
				asio::ip::udp::resolver resolver(net::GetContext());
				asio::ip::udp::resolver::query query(asio::ip::udp::v4(), domain.data(), "");
				asio::ip::udp::resolver::iterator resolvedBegin = resolver.resolve(query);
				asio::ip::udp::resolver::iterator resolvedEnd;

				asio::ip::udp::resolver::iterator picked = resolvedBegin;
				asio::ip::udp::resolver::iterator it = resolvedBegin;
				while (++it != resolvedEnd)
					picked = it;

				return IPAddress((*picked).endpoint().address());
			}

			static IPAddress GetLocalAddress()
			{
				asio::ip::udp::resolver resolver(net::GetContext());
				asio::ip::udp::resolver::query query(asio::ip::udp::v4(), asio::ip::host_name(), "");
				asio::ip::udp::resolver::iterator resolvedBegin = resolver.resolve(query);
				asio::ip::udp::resolver::iterator resolvedEnd;

				asio::ip::udp::resolver::iterator picked = resolvedBegin;
				asio::ip::udp::resolver::iterator it = resolvedBegin;
				while (++it != resolvedEnd)
					picked = it;
				
				return IPAddress((*picked).endpoint().address());
			}

			inline operator std::string()
			{
				return address.to_string();
			}
		};
	}
}

#ifdef CPPU_USE_NAMESPACE
using namespace cppu;
#endif