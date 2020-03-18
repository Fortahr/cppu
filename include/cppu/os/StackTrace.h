#pragma once

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include <algorithm>

#include "../dtypes.h"


#ifdef _WIN32
#include <windows.h>
#pragma comment(lib, "dbghelp.lib")

// ignoring warning C4091 (typedef meant for C), it happens inside DbgHelp.h and isn't really interesting to us
#pragma warning(push)
#pragma warning(disable: 4091)
#include <DbgHelp.h>
#pragma warning(pop)

typedef USHORT(WINAPI *CaptureStackBackTraceType)(
	__in ULONG,
	__in ULONG,
	__out PVOID*,
	__out_opt PULONG
	);

#elif defined _linux or defined __APPLE__
#include <stdlib.h>
#include <execinfo.h>
#include <cxxabi.h>
#endif

#undef min

namespace cppu
{
	namespace os
	{
		class StackTrace
		{
		public:
			/// \brief Get backtrace/callstack and stream it into given stream
			/// \param stream stream which will be filled with backtrace/callstack info
			/// \param maxFrames max amount of functions you want to see traced (Windows Server 2003 and Windows XP:  The sum of the FramesToSkip and FramesToCapture parameters must be less than 63)
			static void GetBackTrace(std::ostream& stream, int ignoreLastFrames = 1, int maxFrames = 20)
			{
				stream << "stack trace:" << std::endl;

#ifdef _WIN32
				CaptureStackBackTraceType func = (CaptureStackBackTraceType)(GetProcAddress(LoadLibrary("kernel32.dll"), "RtlCaptureStackBackTrace"));

				if (func == nullptr)
					return;

				SymSetOptions(SYMOPT_LOAD_LINES);
				DWORD dwDisplacement;
				IMAGEHLP_LINE64 line;
				line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

				std::vector<void*> callersStack;
				callersStack.resize(std::min(62, maxFrames));

				HANDLE process = GetCurrentProcess();
				SymInitialize(process, nullptr, true);

				ushort callerLength = (func)(ignoreLastFrames, std::min(62, maxFrames + ignoreLastFrames), &callersStack[0], nullptr);

				SYMBOL_INFO* symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
				symbol->MaxNameLen = 255;
				symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

				for (uint i = 0; i < callerLength; ++i)
				{
					SymFromAddr(process, (DWORD64)(callersStack[i]), 0, symbol);
					SymGetLineFromAddr64(process, (DWORD64)callersStack[i], &dwDisplacement, &line);
					stream << "\t" << i << " : 0x" << callersStack[i] << " " << symbol->Name << "() line: " << line.LineNumber << " - (" << symbol->Address << ")" << std::endl;
				}

				free(symbol);
#elif defined _linux or defined __APPLE__ // TODO: this is untested, test it on linux and osx

				// storage array for stack trace address data
				void* callerStack[maxFrames + 1];

				// retrieve current stack addresses
				int callerLength = backtrace(callerStack, sizeof(callerStack) / sizeof(void*));

				if (callerLength == 0)
				{
					stream << "\t<empty, possibly corrupt>" << std::endl;
					return;
				}

				// resolve addresses into strings containing "filename(function+address)",
				// this array must be free()-ed
				char** symbolList = backtrace_symbols(callerStack, callerLength);

				// allocate string which will be filled with the demangled function name
				size_t functionNamesize = 256;
				char* functionName = (char*)malloc(functionNamesize);

				// iterate over the returned symbol lines. skip the first, it is the
				// address of this function.
				for (int i = 1; i < callerLength; i++)
				{
					char *beginName = 0, *beginOffset = 0, *endOffset = 0;

					// find parentheses and +address offset surrounding the mangled name:
					// ./module(function+0x15c) [0x8048a6d]
					for (char *p = symbolList[i]; *p; ++p)
					{
						if (*p == '(')
							beginName = p;
						else if (*p == '+')
							beginOffset = p;
						else if (*p == ')' && beginOffset) {
							endOffset = p;
							break;
						}
					}

					if (beginName && beginOffset && endOffset && beginName < beginOffset)
					{
						*beginName++ = '\0';
						*beginOffset++ = '\0';
						*endOffset = '\0';

						// mangled name is now in [beginName, beginOffset) and caller
						// offset in [beginOffset, endOffset). now apply
						// __cxa_demangle():

						int status;
						char* ret = abi::__cxa_demangle(beginName, functionName, &functionNamesize, &status);

						if (status == 0)
							stream << "\t" << symbolList[i] << " : " << ret << "+" << beginOffset << std::endl;
						else // demangling failed. Output function name as a C function with
							stream << "  " << symbolList[i] << " : " << beginName << "()" << beginOffset << std::endl;
					}
					else // couldn't parse the line? print the whole line.
						stream << "\t" << symbolList[i] << std::endl;
				}

				free(functionName);
				free(symbolList);
#endif
			}

			/// \brief Get backtrace/callstack and place it in a string
			/// \param output string which will be filled with backtrace/callstack info
			/// \param maxFrames max amount of functions you want to see traced (Windows Server 2003 and Windows XP: The sum of the FramesToSkip and FramesToCapture parameters must be less than 63)
			static void GetBackTrace(std::string& output, int maxFrames = 20)
			{
				std::stringstream stream;
				GetBackTrace(stream, maxFrames);

				output = stream.str();
			}
		};
	}
}

#ifdef CPPU_USE_NAMESPACE
using namespace cppu;
#endif