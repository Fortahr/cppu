#pragma once

#include "../dtypes.h"
#include <vector>
#include <thread>
#include <condition_variable>

#include "./detail/config.h"
#include <asio/io_context.hpp>

namespace cppu
{
	namespace net
	{
		namespace details
		{
			static ::asio::io_context& context() { static ::asio::io_context v; return v; }

			static bool& threadsRunning() { static bool v; return v; }
			static std::mutex& threadsLock() { static std::mutex v; return v; }
			static std::vector<std::thread>& threads() { static std::vector<std::thread> v; return v; }
			static std::condition_variable& threadsWait() { static std::condition_variable v; return v; }
		}

		inline ::asio::io_context& GetContext()
		{
			return details::context();
		}

		inline void Stop()
		{
			details::threadsRunning() = false;
			details::threadsWait().notify_all();
			details::context().stop();

			for (std::size_t i = 0; i < details::threads().size(); ++i)
				details::threads()[i].join();
		}

		inline void Start(uint threads = 1)
		{
			Stop();
			
			details::threadsRunning() = true;

			details::threads().resize(threads);
			for (std::size_t i = 0; i < details::threads().size(); ++i)
			{
				details::threads()[i] = std::thread([=]()
				{
					while (details::threadsRunning())
					{
						{
							std::unique_lock<std::mutex> lk(details::threadsLock());
							details::threadsWait().wait(lk);
						}

						asio::error_code error;
						details::context().run(error);
					}
				});
			}
		}
	}
}

#ifdef CPPU_USE_NAMESPACE
using namespace cppu;
#endif