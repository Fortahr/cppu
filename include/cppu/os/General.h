#pragma once

#ifdef WIN32
#include <windows.h>
#include <iostream>
#endif


namespace cppu
{
	namespace os
	{
#ifdef WIN32
		FILE* stream;
#endif

		bool IsInstanceRunning(char* applicationName)
		{
#ifdef WIN32
			// try to create or open a named semaphore.
			HANDLE appInstance = CreateSemaphore(nullptr, 0, 1, applicationName);
			if (appInstance != nullptr && GetLastError() == ERROR_ALREADY_EXISTS)
			{ 
				// semaphore is open
				CloseHandle(appInstance);
				appInstance = nullptr;
				
				return FindWindow(applicationName, nullptr) != nullptr;
			}
			else
				return false;

#else
			return system("ps -Ac | grep '" + std::string(applicationName) + "' > /dev/null") == 0;
#endif
		}

		bool ShowConsoleWindow()
		{
			bool debugConsoleEnabled = false;

#ifdef WIN32
			if (debugConsoleEnabled = AllocConsole() == TRUE)
			{
				freopen_s(&stream, "CONIN$", "r+t", stdin);
				freopen_s(&stream, "CONOUT$", "w+t", stdout);
				freopen_s(&stream, "CONOUT$", "w+t", stderr);

				// fixes output problems (when there are any)
				std::cout.flush();
				std::cout.clear();
			}
#endif

			return debugConsoleEnabled;
		}

		void CloseConsoleWindow()
		{
#ifdef WIN32
			FreeConsole();
#endif
		}
	}
}

#ifdef CPPU_USE_NAMESPACE
using namespace cppu;
#endif