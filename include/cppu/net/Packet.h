#pragma once

#include "../dtypes.h"

#include <vector>


namespace cppu
{
	namespace net
	{
		class Packet
		{
		private:
			mutable std::vector<char> data;
			mutable uint pointer;

			inline void SetSize(uint size) { reinterpret_cast<uint&>(data.front()) = size; }

		public:
			Packet()
				: data(sizeof(uint))
				, pointer(sizeof(uint))
			{
				reinterpret_cast<uint&>(data[0]) = 0U;
			}

			Packet(std::vector<char>&& data, uint pointer = 0)
				: data(std::move(data))
				, pointer(pointer + sizeof(uint))
			{}

			template<class T>
			inline const T& GetValue() const
			{
				T& v = reinterpret_cast<T&>(data[pointer]);
				pointer += sizeof(T);
				return v;
			}

			template<class T>
			inline void AddValue(const T& value)
			{
				SetSize(data.size() + sizeof(T));
				data.resize(GetSize());

				memcpy(&data[pointer], &value, sizeof(T));
				pointer += sizeof(T);
			}

			inline const void* GetData() const { return data.data(); }
			inline uint GetSize() const { return reinterpret_cast<uint&>(data.front()); }
			inline uint GetPacketSize() const { return reinterpret_cast<uint&>(data.front()) + sizeof(uint); }
			inline void ResetPointer() const { pointer = sizeof(uint); }
			inline void SetPointer(uint p) const { pointer = p + sizeof(uint); }
		};
	}
}

#ifdef CPPU_USE_NAMESPACE
using namespace cppu;
#endif