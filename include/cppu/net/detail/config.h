#pragma once

#ifdef WIN32
#	ifndef _WIN32_WINNT
#		define _WIN32_WINNT 0x0501
#	endif
#endif

#define ASIO_STANDALONE
#define ASIO_HEADER_ONLY