/*-----------------------------------------*/
// Project	: ETS2MobileCam
// File		: Prism3D/Actors/camera_manager.h
/*-----------------------------------------*/

#pragma once
#include <cstdint>
#include "bmem.h"
#include "scs_logging.h"
using namespace scs_logging;

namespace prism { class camera_manager_u; }

// last updated 1.58

namespace prism::actors::camera_manager
{
	inline prism::camera_manager_u** pointer{}; //  prism::plain_ptr_t<prism::camera_manager_u>*

	inline prism::camera_manager_u* get()
	{
		if (pointer)
			return *pointer;

		uintptr_t pointer_instruction = bmem::patternScan("48 8B 05 ?? ?? ?? ?? 41 FF CE");
		if (!bmem::isAddressValid(pointer_instruction))
		{
			scs_log(1, "Failed to find the camera_manager pointer instruction!");
			pointer = nullptr;
			return nullptr;
		}
		pointer = reinterpret_cast<prism::camera_manager_u**>(bmem::relativeToAbsolute(pointer_instruction, 3, 7));

		return *pointer;
	}
}
