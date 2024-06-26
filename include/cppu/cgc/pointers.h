#pragma once

#include <cstring>

#include "details/base_counter.h"
#include "details/garbage_cleaner.h"
#include "details/destructors.h"

#define GET_NAME(X) #X

// Container Garbage Collection
namespace cppu
{
	namespace cgc
	{
		template<class T>
		class weak_ptr;
		
		template<class T>
		class strong_ptr
		{
			friend class constructor;
			template<typename> friend class strong_ptr;
			template<typename> friend class weak_ptr;
			template<typename> friend class raw_ptr;
			template<typename T1, typename T2, typename> friend strong_ptr<T1> static_pointer_cast(const strong_ptr<T2>&) noexcept;
			template<typename T1, typename T2, typename> friend strong_ptr<T1> dynamic_pointer_cast(const strong_ptr<T2>&) noexcept;
			template<typename T1, typename T2, typename> friend const strong_ptr<T1>& const_pointer_cast(const strong_ptr<T2>&) noexcept;
			template<typename T1, typename T2, typename> friend const strong_ptr<T1>& reinterpret_pointer_cast(const strong_ptr<T2>&) noexcept;

		private:
			T* pointer;
			details::base_counter* refCounter;

			strong_ptr(T* pointer, details::base_counter* refCounter)
				: pointer(pointer)
				, refCounter(refCounter)
			{
				IncrementStrongReference();
			}

		public:
			typedef T element_type;

			// constructors
			strong_ptr() noexcept
				: pointer(nullptr)
				, refCounter(nullptr)
			{ }

			strong_ptr(std::nullptr_t) noexcept
				: pointer(nullptr)
				, refCounter(nullptr)
			{ }
						
			strong_ptr(const strong_ptr<T>& copy)
				: pointer(copy.pointer)
				, refCounter(copy.refCounter)
			{
				IncrementStrongReference();
			}

			strong_ptr(const weak_ptr<T>& copy)
				: pointer(copy.pointer)
				, refCounter(copy.refCounter)
			{
				IncrementStrongReference();
			}

			strong_ptr(strong_ptr<T>&& move) noexcept
				: pointer(std::move(move.pointer))
				, refCounter(std::move(move.refCounter))
			{
				move.pointer = nullptr;
				move.refCounter = nullptr;
			}

			// up cast constructor
			template <typename TD, typename = typename std::enable_if_t<std::is_base_of_v<T, TD> && std::is_assignable_v<T*&, TD*>>>
			strong_ptr(const strong_ptr<TD>& copy)
				: pointer(copy.pointer)
				, refCounter(copy.refCounter)
			{
				IncrementStrongReference();
			}

			// up cast move constructor
			template <typename TD, typename = typename std::enable_if_t<std::is_base_of_v<T, TD> && std::is_assignable_v<T*&, TD*>>>
			strong_ptr(strong_ptr<TD>&& move) noexcept
				: pointer(std::move(move.pointer))
				, refCounter(std::move(move.refCounter))
			{
				move.pointer = nullptr;
				move.refCounter = nullptr;
			}

			~strong_ptr()
			{
				DecrementStrongReference();
			}

			T* ptr() const noexcept
			{
				return pointer;
			}

			// copy (copy-and-swap)
			/*strong_ptr<T>& operator=(strong_ptr<T> copy)
			{
				std::swap(pointer, copy.pointer);
				std::swap(refCounter, copy.refCounter);
				this->IncrementStrongReference();
				return *this;
			}*/

			// copy (non-copy-and-swap)
			strong_ptr<T>& operator=(const strong_ptr<T>& copy)
			{
				if (copy.pointer != this->pointer)
				{
					this->DecrementStrongReference();

					pointer = copy.pointer;
					refCounter = copy.refCounter;
					this->IncrementStrongReference();
				}
				return *this;
			}

			// move
			strong_ptr<T>& operator=(strong_ptr<T>&& move) noexcept
			{
				std::swap(pointer, move.pointer);
				std::swap(refCounter, move.refCounter);
				return *this;
			}

			strong_ptr<T>& operator=(std::nullptr_t) noexcept
			{
				pointer = nullptr;
				refCounter = nullptr;
				return *this;
			}
			
			operator strong_ptr<void*>&()
			{
				return *reinterpret_cast<strong_ptr<void*>*>(this);
			}

			operator weak_ptr<T> ()
			{
				return weak_ptr<T>(*this);
			}

			template <typename TB, typename = typename std::enable_if<std::is_same<TB, T>::value || std::is_base_of<TB, T>::value>::type>
			inline bool operator==(const strong_ptr<TB>& rhs) const { return pointer == rhs.pointer; }
			template <typename TB, typename = typename std::enable_if<std::is_same<TB, T>::value || std::is_base_of<TB, T>::value>::type>
			inline bool operator!=(const strong_ptr<TB>& rhs) const { return pointer != rhs.pointer; }

			inline bool operator==(const T* rhs) const { return pointer == rhs; }
			inline bool operator!=(const T* rhs) const { return pointer != rhs; }

			template <typename TB, typename = typename std::enable_if<std::is_same<TB, T>::value || std::is_base_of<TB, T>::value>::type>
			inline bool operator==(const TB& rhs) const { return pointer == &rhs; }
			template <typename TB, typename = typename std::enable_if<std::is_same<TB, T>::value || std::is_base_of<TB, T>::value>::type>
			inline bool operator!=(const TB& rhs) const { return pointer != &rhs; }

			inline operator bool() const { return pointer != nullptr; }

			inline T* operator->() const { return pointer; }
			inline T& operator*() const { return *pointer; }

			/*weak_ptr<T> weak_ptr()
			{
				return weak_ptr(this);
			}*/

		private:
			inline void IncrementStrongReference()
			{
				if (refCounter != nullptr)
 					refCounter->strongReferences.fetch_add(1);
			}

			// shared pointer mechanics
			void DecrementStrongReference()
			{
				auto* pointer = this->pointer;
				auto* refCounter = this->refCounter;

				if (refCounter != nullptr)
				{
					// we'll check if the previous value was 2, if so it will decrement to 0 in this procedure then mark this one as garbage
					// the last strong value holder is merely a safety measure so weak ptrs won't destroy the ref counter before we do so in here.
					if (refCounter->strongReferences.fetch_sub(1) == 2)
					{
						// add it so the collection can clean it up and give out the free slot again
						refCounter->add_as_garbage((void*)pointer);

						// after this we've set the actual strong ref count so any weak ptr can delete the ref counter from this point
						refCounter->strongReferences.fetch_sub(1);

						// strong ref base_counter will be 0, now we only need to check the weak ref base_counter
						// and remove the tracking object if it's 1
						if (refCounter->weakReferences.load() == 0)
							delete refCounter;
					}
				}
			}
		};

		template<class T>
		class weak_ptr
		{
			template<typename> friend class strong_ptr;
			template<typename> friend class weak_ptr;
			template<typename> friend class raw_ptr;
			template<typename T1, typename T2, typename> friend weak_ptr<T1> static_pointer_cast(const weak_ptr<T2>&) noexcept;
			template<typename T1, typename T2, typename> friend weak_ptr<T1> dynamic_pointer_cast(const weak_ptr<T2>&) noexcept;
			template<typename T1, typename T2, typename> friend const weak_ptr<T1>& const_pointer_cast(const weak_ptr<T2>&) noexcept;
			template<typename T1, typename T2, typename> friend const weak_ptr<T1>& reinterpret_pointer_cast(const weak_ptr<T2>&) noexcept;
		private:
			T* pointer;
			details::base_counter* refCounter;

			weak_ptr(T* pointer, details::base_counter* refCounter)
				: pointer(pointer)
				, refCounter(refCounter)
			{
				IncrementWeakReference();
			}

		public:
			typedef T element_type;

			// constructors
			weak_ptr()
				: pointer(nullptr)
				, refCounter(nullptr)
			{ }

			weak_ptr(std::nullptr_t)
				: pointer(nullptr)
				, refCounter(nullptr)
			{ }

			weak_ptr(weak_ptr<T>* copy)
				: pointer(copy->pointer)
				, refCounter(copy->refCounter)
			{
				IncrementWeakReference();
			}

			weak_ptr(const weak_ptr<T>* copy)
				: pointer(copy->pointer)
				, refCounter(copy->refCounter)
			{
				IncrementWeakReference();
			}

			weak_ptr(const weak_ptr<T>& copy)
				: pointer(copy.pointer)
				, refCounter(copy.refCounter)
			{
				IncrementWeakReference();
			}

			/*weak_ptr(const strong_ptr<T>& copy)
				: pointer(copy.pointer)
				, refCounter(copy.refCounter)
			{
				IncrementWeakReference();
			}*/

			weak_ptr(const strong_ptr<std::remove_const_t<T>>& copy)
				: pointer(copy.pointer)
				, refCounter(copy.refCounter)
			{
				IncrementWeakReference();
			}

			// up cast constructor
			template <typename TD, typename = typename std::enable_if_t<std::is_base_of_v<T, TD>&& std::is_assignable_v<T*&, TD*>>>
			weak_ptr(const weak_ptr<TD>& copy)
				: pointer(copy.pointer)
				, refCounter(copy.refCounter)
			{
				IncrementWeakReference();
			}

			// up cast move constructor
			template <typename TD, typename = typename std::enable_if_t<std::is_base_of_v<T, TD>&& std::is_assignable_v<T*&, TD*>>>
			weak_ptr(weak_ptr<TD>&& move) noexcept
				: pointer(std::move(move.pointer))
				, refCounter(std::move(move.refCounter))
			{
				move.pointer = nullptr;
				move.refCounter = nullptr;
			}

			~weak_ptr()
			{
				DecrementWeakReference();
			}

			T* ptr() const
			{
				return pointer;
			}

			template <typename TB, typename = typename std::enable_if<std::is_same<TB, T>::value || std::is_base_of<TB, T>::value>::type>
			inline bool operator==(const weak_ptr<TB>& rhs) const { return pointer == rhs.pointer; }
			template <typename TB, typename = typename std::enable_if<std::is_same<TB, T>::value || std::is_base_of<TB, T>::value>::type>
			inline bool operator!=(const weak_ptr<TB>& rhs) const { return pointer != rhs.pointer; }

			template <typename TB, typename = typename std::enable_if<std::is_same<TB, T>::value || std::is_base_of<TB, T>::value>::type>
			inline bool operator==(const TB& rhs) const { return pointer == &rhs; }
			template <typename TB, typename = typename std::enable_if<std::is_same<TB, T>::value || std::is_base_of<TB, T>::value>::type>
			inline bool operator!=(const TB& rhs) const { return pointer != &rhs; }

			inline bool operator==(const T* rhs) const { return pointer == rhs; }
			inline bool operator!=(const T* rhs) const { return pointer != rhs; }

			inline operator bool() const { return pointer != nullptr; }

			inline T* operator->() const { return pointer; }
			inline T& operator*() const { return *pointer; }

		private:
			inline void IncrementWeakReference()
			{
				if (refCounter != nullptr)
					refCounter->weakReferences.fetch_add(1);
			}

			void DecrementWeakReference()
			{
				if (refCounter != nullptr)
				{
					// decrement by one and calculate the proper new value (fetc_add returns previous value)
					details::RefCount weakRef = refCounter->weakReferences.fetch_sub(1) - 1;

					// if both base_counters are 0 then delete the ref base_counter
					if (refCounter->strongReferences.load() + weakRef == 0)
						delete refCounter; // remove this tracking object
				}
			}
		};

		template<class T>
		class raw_ptr
		{
		private:
			T* pointer;

		public:
			typedef T element_type;

			raw_ptr()
				: pointer(nullptr)
			{}

			raw_ptr(std::nullptr_t)
				: pointer(nullptr)
			{}

			raw_ptr(T* ptr)
				: pointer(ptr)
			{}

			raw_ptr(const strong_ptr<T>& ptr)
				: pointer(ptr.pointer)
			{ }

			raw_ptr(const weak_ptr<T>& ptr)
				: pointer(ptr.pointer)
			{ }
			
			template <typename T2, typename = std::enable_if_t<std::is_base_of_v<T, T2>>>
			raw_ptr(const raw_ptr<T2>& ptr)
				: pointer(ptr.pointer)
			{ }

			template <typename T2, typename = std::enable_if_t<std::is_base_of_v<T, T2>>>
			raw_ptr(const strong_ptr<T2>&ptr)
				: pointer(ptr.pointer)
			{ }

			template <typename T2, typename = std::enable_if_t<std::is_base_of_v<T, T2>>>
			raw_ptr(const weak_ptr<T2>&ptr)
				: pointer(ptr.pointer)
			{ }

			inline T* operator->() { return pointer; }
			inline const T* operator->() const { return pointer; }

			inline operator T*() const { return pointer; }
			inline operator bool() const { return pointer != nullptr; }

			template <typename T2, typename = typename std::enable_if<std::is_base_of<T, T2>::value || std::is_base_of<T2, T>::value || std::is_same<T, T2>::value>::type>
			inline explicit operator T2* () const { return static_cast<T2*>(pointer); }

			//template <typename T2, typename = typename std::enable_if<std::is_base_of<T, typename T2::Type>::value>::type>
			//inline raw_ptr<T>& operator=(const T2& rhs) const { pointer == rhs.pointer; return *this; }
		};

		template <typename T, typename T2, typename = typename std::enable_if<std::is_base_of<T, T2>::value || std::is_base_of<T2, T>::value || std::is_same<T, T2>::value>::type>
		strong_ptr<T> static_pointer_cast(const strong_ptr<T2>& cast) noexcept
		{
			return strong_ptr<T>(static_cast<T*>(cast.pointer), cast.refCounter);
		}

		///
		/// \brief only down casting is supported, for up casting just use the constructor (it's implicit!) or the static cast
		template <typename T, typename T2, typename = typename std::enable_if<std::is_base_of<T2, T>::value>::type>
		strong_ptr<T> dynamic_pointer_cast(const strong_ptr<T2>& cast) noexcept
		{
			// dynamic_cast guarantees the types are interchangeble
			strong_ptr<T> ptr;
			if ((ptr.pointer = dynamic_cast<T*>(cast.pointer)))
			{
				ptr.refCounter = cast.refCounter;
				ptr.IncrementStrongReference();
			}

			return ptr;
		}

		///
		/// \brief remove the const attribute
		template <typename T>
		const strong_ptr<T>& const_pointer_cast(const strong_ptr<T>& cast) noexcept
		{
			return strong_ptr<T>(const_cast<std::remove_const_t<T>*>(cast.pointer), cast.refCounter);
		}

		///
		/// \brief reinterpret the pointers' bit as if it is another type, unsafe by nature so programmer should gurantee its safety
		template <typename T, typename T2>
		const strong_ptr<T>& reinterpret_pointer_cast(const strong_ptr<T2>& cast) noexcept
		{
			return *reinterpret_cast<const strong_ptr<T>*>(&cast);
		}
	}
}
