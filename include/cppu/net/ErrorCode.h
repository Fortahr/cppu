#pragma once

namespace cppu
{
	namespace net
	{
		typedef unsigned short ErrorCode;

		namespace Error
		{
			enum : unsigned short
			{
				NONE = 0U,
				UNKNOWN_ERROR = 1U,

				SOCKET_INVALID = 10U,
				SOCKET_ALREADY_CONNECTED = 11U,

				// Listener
				LISTENER_ALREADY_LISTENING = 1000U,

				// WSA
				CONNECTION_ABORTED = 10053U,
				CONNECTION_RESET = 10054U,
				CONNECTION_TIMED_OUT = 10060U,
				CONNECTION_REFUSED = 10061U,
				SERVER_DOWN = 10064U,
				SERVER_UNREACHABLE = 10065U,

				// TLS
				TLS_VERIFICATION_SUCCESS = 2000U,
				TLS_CERTIFICATE_NOT_TRUSTED,
				TLS_CERTIFICATE_INVALID,

				// Authentication
				AUTHENTICATION_INVALID = 3000U
			};
		}
	}
}

#ifdef CPPU_USE_NAMESPACE
using namespace cppu;
#endif