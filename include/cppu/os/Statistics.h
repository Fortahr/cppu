#pragma once

// uint64 uint32 etc
#include "../dtypes.h"

// define if you want to use TotalCPUUsage (not application specific)
//#define STATISTICS_OVERALL_CPU

#ifdef WIN32
#include "windows.h"
#include "psapi.h"
	#ifdef STATISTICS_OVERALL_CPU
	#include "TCHAR.h"
	#include "pdh.h"
	#endif
#elif defined _linux
#include "sys/types.h"
#include "sys/sysinfo.h"
#elif defined __APPLE__
#include <mach/mach.h>
#include <mach/vm_statistics.h>
#include <mach/mach_types.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>
#endif

namespace cppu
{
	namespace os
	{
		class Statistics
		{
		private:
#ifdef WIN32
#ifdef STATISTICS_OVERALL_CPU
			static PDH_HQUERY& cpuQuery() { static PDH_HQUERY v; return v; }
			static PDH_HCOUNTER& cpuTotal() { static PDH_HCOUNTER v; return v; }
#endif
			static uint64& lastCPU() { static uint64 v; return v; }
			static uint64& lastSysCPU() { static uint64 v; return v; }
			static uint64& lastUserCPU() { static uint64 v; return v; }
			static int& numProcessors() { static int v; return v; }
			static HANDLE& self() { static HANDLE v; return v; }
#endif

			static bool& cpuChecksEnabled() { static bool v; return v; }

			static void Init()
			{
#ifdef WIN32
#ifdef OSSTATISTIC_OVERALL_CPU
				PdhOpenQuery(NULL, NULL, &cpuQuery);
				PdhAddCounter(cpuQuery, "\\Processor(_Total)\\% Processor Time", NULL, &cpuTotal);
				PdhCollectQueryData(cpuQuery);
#endif
				SYSTEM_INFO sysInfo;
				FILETIME ftime, fsys, fuser;

				GetSystemInfo(&sysInfo);
				numProcessors() = sysInfo.dwNumberOfProcessors;

				GetSystemTimeAsFileTime(&ftime);
				memcpy(&lastCPU(), &ftime, sizeof(FILETIME));

				self() = GetCurrentProcess();
				GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
				memcpy(&lastSysCPU(), &fsys, sizeof(FILETIME));
				memcpy(&lastUserCPU(), &fuser, sizeof(FILETIME));
#endif

				cpuChecksEnabled() = true;
			}

		public:
			static uint64 TotalVirtualMemory()
			{
#ifdef WIN32
				MEMORYSTATUSEX memInfo;
				memInfo.dwLength = sizeof(MEMORYSTATUSEX);
				GlobalMemoryStatusEx(&memInfo);
				return memInfo.ullTotalPageFile;
#else
				return 0;
#endif
			}

			static uint64 VirtualMemoryUsed()
			{
#ifdef WIN32
				MEMORYSTATUSEX memInfo;
				memInfo.dwLength = sizeof(MEMORYSTATUSEX);
				GlobalMemoryStatusEx(&memInfo);
				return memInfo.ullTotalPageFile - memInfo.ullAvailPageFile;
#else
				return 0;
#endif
			}

			static uint32 VirtualMemoryUsedByProcess()
			{
#ifdef WIN32
				PROCESS_MEMORY_COUNTERS_EX pmc;
				GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(PROCESS_MEMORY_COUNTERS_EX));
				return pmc.PrivateUsage;
#else
				return 0;
#endif
			}

			static uint64 TotalPhysicalMemory()
			{
#ifdef WIN32
				MEMORYSTATUSEX memInfo;
				memInfo.dwLength = sizeof(MEMORYSTATUSEX);
				GlobalMemoryStatusEx(&memInfo);
				return memInfo.ullTotalPhys;
#else
				return 0;
#endif
			}

			static uint64 PhysicalMemoryUsed()
			{
#ifdef WIN32
				MEMORYSTATUSEX memInfo;
				memInfo.dwLength = sizeof(MEMORYSTATUSEX);
				GlobalMemoryStatusEx(&memInfo);
				return memInfo.ullTotalPhys - memInfo.ullAvailPhys;
#else
				return 0;
#endif
			}

			static uint32 PhysicalMemoryUsedByProcess()
			{
#ifdef WIN32
				PROCESS_MEMORY_COUNTERS pmc;
				GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(PROCESS_MEMORY_COUNTERS));
				return pmc.WorkingSetSize;

#elif defined(__APPLE__) && defined(__MACH__)
				/* OSX ------------------------------------------------------ */
				struct mach_task_basic_info info;
				mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
				if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
					(task_info_t)&info, &infoCount) != KERN_SUCCESS)
					return (size_t)0L;		/* Can't access? */
				return (size_t)info.resident_size;

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
				/* Linux ---------------------------------------------------- */
				long rss = 0L;
				FILE* fp = NULL;
				if ((fp = fopen("/proc/self/statm", "r")) == NULL)
					return (size_t)0L;		/* Can't open? */
				if (fscanf(fp, "%*s%ld", &rss) != 1)
				{
					fclose(fp);
					return (size_t)0L;		/* Can't read? */
				}
				fclose(fp);
				return (size_t)rss * (size_t)sysconf(_SC_PAGESIZE);

#else
				/* AIX, BSD, Solaris, and Unknown OS ------------------------ */
				return (size_t)0L;			/* Unsupported. */
#endif
			}

			static double TotalCPUUsage()
			{
				if (cpuChecksEnabled())
				{
#ifdef WIN32
#ifdef OSSTATISTIC_OVERALL_CPU
					PDH_FMT_COUNTERVALUE counterVal;
					PdhCollectQueryData(cpuQuery);
					PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);
					return counterVal.doubleValue;
#else
					return 0;
#endif
#else
					return 0;
#endif
				}
				else
				{
					Init();
					return 0.0;
				}
			}

			static double CPUUsageByProcess()
			{
				if (cpuChecksEnabled())
				{
#ifdef WIN32
					FILETIME ftime, fsys, fuser;
					uint64 now, sys, user;
					double percent;


					GetSystemTimeAsFileTime(&ftime);
					memcpy(&now, &ftime, sizeof(FILETIME));


					GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
					memcpy(&sys, &fsys, sizeof(FILETIME));
					memcpy(&user, &fuser, sizeof(FILETIME));
					percent = double((sys - lastSysCPU()) + (user - lastUserCPU()));
					percent /= now - lastCPU();
					percent /= numProcessors();
					lastCPU() = now;
					lastUserCPU() = user;
					lastSysCPU() = sys;


					return percent * 100.0;
#else
					return 0;
#endif
				}
				else
				{
					Init();
					return 0.0;
				}

			}
		};
	}
}

#ifdef CPPU_USE_NAMESPACE
using namespace cppu;
#endif