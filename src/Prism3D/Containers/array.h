/*-----------------------------------------*/
// Project	: ETS2MobileCam
// File		: Prism3D/Containers/array.h
/*-----------------------------------------*/

#pragma once
#include <cstdint>
#include <stdexcept>
#include <cstring>
#include <type_traits>
#include <iterator>

// last updated 1.60

// T can be:
//   - a pointer type (array_dyn_t<foo_t*>) -> items is a list of 8-byte pointers
//   - a value type   (array_dyn_t<foo_t>)  -> items is a list of inline instances

#pragma pack(push, 1)
namespace prism
{
	class array_t
	{
	public:
		void* items;       // 0x0008
		uint64_t size;     // 0x0010
		uint64_t capacity; // 0x0018
		void* allocator;   // 0x0020

		virtual void destructor() {}
		virtual uint64_t clear() { return 0; }
		virtual uint64_t reallocate() { return 0; }
		virtual bool allocate(uint64_t new_size) { return false; }
	};

	template<typename T = void*>
	class array_dyn_t : public array_t // Size: 0x0028
	{
	public:
		T& operator[](uint64_t index)
		{
			if (index >= size)
				throw std::out_of_range("index");

			return reinterpret_cast<T*>(items)[index];
		}
		const T& operator[](uint64_t index) const
		{
			if (index >= size)
				throw std::out_of_range("index");

			return reinterpret_cast<const T*>(items)[index];
		}


		// Copies item in, growing storage first if needed.
		void push_back(const T& item)
		{
			if (size == capacity)
				allocate(size + 1);

			reinterpret_cast<T*>(items)[size] = item;
			++size;
		}


		// Grows storage if needed, then hands you a pointer to the new
		// (uninitialized) slot so you can fill it in yourself.
		template<typename Constructor>
		void push_back_construct(Constructor&& construct)
		{
			if (size == capacity)
				allocate(size + 1);

			construct(&reinterpret_cast<T*>(items)[size]);
			++size;
		}

		void remove_at(uint64_t index)
		{
			if (index >= size)
				throw std::out_of_range("index");

			T* arr = reinterpret_cast<T*>(items);
			if (index + 1 < size)
				std::memmove(&arr[index], &arr[index + 1], (size - index - 1) * sizeof(T));
			--size;
		}

		struct iterator {
			using iterator_category = std::forward_iterator_tag;
			using value_type = T;
			using difference_type = std::ptrdiff_t;
			using pointer = T*;
			using reference = T&;

			T* ptr;

			T& operator*() const { return *ptr; }
			auto operator->() const
			{
				if constexpr (std::is_pointer_v<T>)
					return *ptr; // T already a pointer, one deref gives the real struct pointer
				else
					return ptr; // T is a value/member, ptr already points at it
			}

			iterator& operator++() { ++ptr; return *this; }

			bool operator!=(const iterator& other) const { return ptr != other.ptr; }
			bool operator==(const iterator& other) const { return ptr == other.ptr; }
		};

		iterator begin() { return { reinterpret_cast<T*>(items) }; }
		iterator end() { return { reinterpret_cast<T*>(items) + size }; }
	};
	static_assert(sizeof(array_dyn_t<>) == 0x28);
};
#pragma pack(pop)
