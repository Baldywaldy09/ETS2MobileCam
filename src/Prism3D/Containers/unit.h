/*-----------------------------------------*/
// Project	: ETS2MobileCam
// File		: Prism3D/Containers/unit.h
/*-----------------------------------------*/

#pragma once
#include <cstdint>
#include <atomic>
namespace prism { class unit_descriptor_t; }

// last updated 1.57

#pragma pack(push, 1)
namespace prism
{
	// 1.55+
	class unit_t // Size: 0x0010
	{
	public:
		std::atomic<uint32_t> m_unk; //0x0008 (0x04)
		uint32_t m_unk2; //0x000C (0x04)

		virtual uint64_t destructor(bool unk);
		virtual uint64_t destroy();
		virtual prism::unit_t* _clone();
		virtual void Function3();
		virtual void Function4();
		virtual unit_descriptor_t* get_unit_descriptor();
		virtual void Function6();
		virtual void set_attribute();
		virtual void pre_save();
		virtual void post_save();
		virtual void pre_load();
		virtual void post_load(); // 0x58

		template<typename T>
		T* as() { return reinterpret_cast<T*>(this); }

		template<typename T>
		const T* as() const { return reinterpret_cast<const T*>(this); }
	};
	static_assert(sizeof(unit_t) == 0x10);
}
#pragma pack(pop)
