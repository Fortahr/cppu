#pragma once

#ifdef WIN32
#	ifndef WINVER
#		define WINVER  0x0601
#	endif

#	ifndef _WIN32_WINNT
#		define _WIN32_WINNT 0x0601
#	endif
#endif

#define ASIO_STANDALONE
#define ASIO_HEADER_ONLY