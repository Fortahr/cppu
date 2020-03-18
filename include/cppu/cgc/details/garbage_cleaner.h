#pragma once

#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "icontainer.h"
#include "destructors.h"

#include <iostream>

namespace cppu
{
	namespace cgc
	{
		void gc_start();
		void gc_stop();

		namespace details
		{
			class garbage_cleaner
			{
				friend void cgc::gc_start();
				friend void cgc::gc_stop();
			private:
				static std::queue<icontainer*>& dirtyContainers() { static std::queue<icontainer*> v; return v; }

				static std::thread& thread() { static std::thread v; return v; }
				static std::mutex& lock() { static std::mutex v; return v; }
				static std::condition_variable& wait() { static std::condition_variable v; return v; }
				static bool& running() { static bool v = false; return v; }
				
				static void thread_function()
				{
					icontainer* container;

					while (running())
					{
						{
							std::unique_lock<std::mutex> lk(lock());
							wait().wait(lk);

							if (dirtyContainers().empty())
								continue;

							container = dirtyContainers().front();
							dirtyContainers().pop();
						}

						container->clean_garbage();
					}
				}

				static void enable()
				{
					if (!running())
					{
						running() = true;
						thread() = std::thread(&garbage_cleaner::thread_function);
					}
				}

				static void disable()
				{
					running() = false;
					wait().notify_one();
					thread().join();
				}

			public:
				static bool add_to_clean(icontainer* container)
				{
					dirtyContainers().push(container);
					wait().notify_one();
					return true;
				}
			};
		}
		
		inline void gc_start()
		{
			details::garbage_cleaner::enable();
		}

		inline void gc_stop()
		{
			details::garbage_cleaner::disable();
		}
	}
}