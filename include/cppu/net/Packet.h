#pragma once

#include <vector>


namespace cppu
{
	namespace net
	{
		class Packet
		{
		private:
			mutable std::vector<char> data;
			mutable uint32_t pointer;

			inline void SetSize(uint32_t size)
			{
				reinterpret_cast<uint32_t&>(data.front()) = size;
				data.resize(size);
			}

		public:
			Packet()
				: data(sizeof(uint32_t))
				, pointer(sizeof(uint32_t))
			{
				reinterpret_cast<uint32_t&>(data.front()) = 0U;
			}

			Packet(std::vector<char>&& data, uint32_t pointer = 0)
				: data(std::move(data))
				, pointer(pointer + sizeof(uint32_t))
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
				memcpy(data.data() + pointer, &value, sizeof(T));
				pointer += sizeof(T);
			}

			inline const void* GetData() const { return data.data(); }
			inline uint32_t GetSize() const { return reinterpret_cast<uint32_t&>(data.front()); }
			inline uint32_t GetPacketSize() const { return reinterpret_cast<uint32_t&>(data.front()) + sizeof(uint32_t); }
			inline void ResetPointer() const { pointer = sizeof(uint32_t); }
			inline void SetPointer(uint32_t p) const { pointer = p + sizeof(uint32_t); }
		};
	}
}
