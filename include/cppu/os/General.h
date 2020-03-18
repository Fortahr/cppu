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
		HANDLE appInstance;
		FILE* stream;
#endif

		bool IsInstanceRunning(char* sAppName)
		{
#ifdef WIN32
			HWND     hWndMe;
			int      attempt;

			for (attempt = 0; attempt < 2; attempt++)
			{
				// Create or open a named semaphore.
				appInstance = CreateSemaphore(NULL, 0, 1, sAppName);
				// Close handle and return NULL if existing semaphore was opened.
				if ((appInstance != NULL) &&
					(GetLastError() == ERROR_ALREADY_EXISTS))
				{  // Someone has this semaphore open...
					CloseHandle(appInstance);
					appInstance = NULL;
					hWndMe = FindWindow(sAppName, NULL);
					if (hWndMe && IsWindow(hWndMe))
					{  // I found the guy, try to wake him up
						if (SetForegroundWindow(hWndMe))
						{  // Windows says we woke the other guy up
							return true;
						}
					}
					Sleep(100); // Maybe the semaphore will go away like the window did...
				}
				else
				{  // If new semaphore was created, return FALSE.
					return false;
				}
			}
			// We never got the semaphore, so we must 
			// behave as if a previous instance exists
			return true;

#else
			std::string command = "ps -Ac | grep '" + std::string(sAppName) + "' > /dev/null";
			return system("ps -Ac | grep 'AProcessName' > /dev/null") == 0;
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