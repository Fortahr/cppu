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
			inline static ::asio::io_context context = {};

			inline static bool threadsRunning = false;
			inline static std::mutex threadsLock = {};
			inline static std::vector<std::thread> threads = {};
			inline static std::condition_variable threadsWait = {};
		}

		inline ::asio::io_context& GetContext()
		{
			return details::context;
		}

		inline void Stop()
		{
			details::threadsRunning = false;
			details::threadsWait.notify_all();
			details::context.stop();

			for (std::size_t i = 0; i < details::threads.size(); ++i)
				details::threads[i].join();
		}

		inline void Start(uint threads = 1)
		{
			Stop();
			
			details::threadsRunning = true;

			details::threads.resize(threads);
			for (std::size_t i = 0; i < details::threads.size(); ++i)
			{
				details::threads[i] = std::thread([=]()
				{
					while (details::threadsRunning)
					{
						{
							std::unique_lock<std::mutex> lk(details::threadsLock);
							details::threadsWait.wait(lk);
						}

						asio::error_code error;
						details::context.run(error);
					}
				});
			}
		}
	}
}
