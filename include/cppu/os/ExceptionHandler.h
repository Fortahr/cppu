#pragma once

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include <algorithm>
#include <exception>

#include "../dtypes.h"
#include "./StackTrace.h"

#ifdef _WIN32
#include <new.h>
#elif defined _linux or defined __APPLE__
// StackTrace.h got most
#endif

#undef min

namespace cppu
{
	namespace os
	{
		class ExceptionHandler
		{
		private:
			static int& maxFrames() { static int v = 64; return v; }

		public:
			/// \brief Registers the ExceptionHandler's exception and error handler function(s). Calls RegisterUnhandledExceptionHandlers() implicitly for this thread
			/// \param maxFrames max amount of functions you want to see traced (Windows Server 2003 and Windows XP:  The sum of the FramesToSkip and FramesToCapture parameters must be less than 63)
			static void RegisterExceptionHandlers(int maxFrames = 20)
			{
				ExceptionHandler::maxFrames() = maxFrames;

#ifdef _WIN32
				_set_purecall_handler(PureCallExceptionHandler);
				_set_invalid_parameter_handler(InvalidParameterHandler);
				_set_new_handler(OutOfMemoryHandler);
				_set_new_mode(1);

				signal(SIGABRT, SIGABRTSignalHandler);
				signal(SIGINT, SIGINTSignalHandler);
				signal(SIGTERM, SIGTERMSignalHandler);
#endif

				RegisterUnhandledExceptionHandlers();
			}

			/// \brief (Thread Specific) Registers the ExceptionHandler's termination function(s) with std::set_terminate and on Windows with SetUnhandledExceptionFilter.
			static void RegisterUnhandledExceptionHandlers()
			{
#ifdef _WIN32
				//AddVectoredExceptionHandler(1, (LPTOP_LEVEL_EXCEPTION_FILTER)WinUnHandledExceptionHandler);
				//SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)WinUnHandledExceptionHandler);

				signal(SIGFPE, SIGFPESignalHandler);
				signal(SIGILL, SIGILLSignalHandler);
				signal(SIGSEGV, SIGSEGVSignalHandler);
#endif

				std::set_new_handler(UnexpectedExceptionHandler);
				std::set_terminate(UnHandledExceptionHandler);
			}

		private:
			// TODO (Ylon): make more exception handlers to fit different needs, like a callback type which people can use to create windows with
			/// \brief Default exception handler, requests the backtrace/callstack and std::cout them
			static void UnHandledExceptionHandler()
			{
				std::stringstream stream;
				std::cout << "Unexpected Exception" << std::endl;
				StackTrace::GetBackTrace(stream, 2, ExceptionHandler::maxFrames());
				std::cout << stream.str() << std::endl;
				std::cin.get();
			}

			static void UnexpectedExceptionHandler()
			{
				std::stringstream stream;
				std::cout << "Unhandled Exception" << std::endl;
				StackTrace::GetBackTrace(stream, 2, ExceptionHandler::maxFrames());
				std::cout << stream.str() << std::endl;
				std::cin.get();
			}

			static void PureCallExceptionHandler()
			{
				std::stringstream stream;
				std::cout << "Pure virtual function has been called" << std::endl;
				//StackTrace::GetBackTrace(stream, 2, ExceptionHandler::maxFrames());
				//std::cout << stream.str() << std::endl;
				std::cin.get();
			}

			static void InvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, uint line, uintptr_t pReserved)
			{
				std::stringstream stream;
				stream << "Invalid parameter detected in function " << function << " File: " << file << " Line: " << line << std::endl;
				stream << "Expression: " << expression << std::endl;
				StackTrace::GetBackTrace(stream, 2, ExceptionHandler::maxFrames());
				std::cout << stream.str() << std::endl;
				std::cin.get();
			}

			static int OutOfMemoryHandler(size_t size)
			{
				std::stringstream stream;
				std::cout << "Out of memory while creating object on heap (" << size << "B)" << std::endl;
				//StackTrace::GetBackTrace(stream, 2, ExceptionHandler::maxFrames()); // crashes
				//std::cout << stream.str() << std::endl;
				std::cin.get();

				return 0;
			}

			static void SIGABRTSignalHandler(int e)
			{
				std::stringstream stream;
				std::cout << "SIGABRT" << std::endl;
				StackTrace::GetBackTrace(stream, 2, ExceptionHandler::maxFrames());
				std::cout << stream.str() << std::endl;
				std::cin.get();
			}

			static void SIGFPESignalHandler(int e)
			{
				std::stringstream stream;
				std::cout << "SIGFPE" << std::endl;
				StackTrace::GetBackTrace(stream, 2, ExceptionHandler::maxFrames());
				std::cout << stream.str() << std::endl;
				std::cin.get();
			}

			static void SIGILLSignalHandler(int e)
			{
				std::stringstream stream;
				std::cout << "SIGILL" << std::endl;
				StackTrace::GetBackTrace(stream, 2, ExceptionHandler::maxFrames());
				std::cout << stream.str() << std::endl;
				std::cin.get();
			}

			static void SIGINTSignalHandler(int e)
			{
				std::stringstream stream;
				std::cout << "SIGINT" << std::endl;
				StackTrace::GetBackTrace(stream, 2, ExceptionHandler::maxFrames());
				std::cout << stream.str() << std::endl;
				std::cin.get();
			}

			static void SIGSEGVSignalHandler(int e)
			{
				std::stringstream stream;
				std::cout << "SIGSEGV" << std::endl;
				StackTrace::GetBackTrace(stream, 2, ExceptionHandler::maxFrames());
				std::cout << stream.str() << std::endl;
				std::cin.get();
			}

			static void SIGTERMSignalHandler(int e)
			{
				std::stringstream stream;
				std::cout << "SIGTERM" << std::endl;
				StackTrace::GetBackTrace(stream, 2, ExceptionHandler::maxFrames());
				std::cout << stream.str() << std::endl;
				std::cin.get();
			}

#ifdef _WIN32
			// untested, till now std::set_terminate gets the info
			/// \brief Default exception handler for windows, requests the backtrace/callstack and std::cout them
			/// ref: http://www.drdobbs.com/tools/postmortem-debugging/185300443
			static int32 WinUnHandledExceptionHandler(struct _EXCEPTION_POINTERS* exInfo)
			{
				std::stringstream stream;
				std::cout << "Unhandled Exception (Windows)" << std::endl;
				//StackTrace::GetBackTrace(stream, 2, ExceptionHandler::maxFrames());
				//std::cout << stream.str() << std::endl;
				//std::cin.get();

				return EXCEPTION_CONTINUE_SEARCH;
				//return EXCEPTION_EXECUTE_HANDLER;
			}
#endif
		};
	}
}

#ifdef CPPU_USE_NAMESPACE
using namespace cppu;
#endif