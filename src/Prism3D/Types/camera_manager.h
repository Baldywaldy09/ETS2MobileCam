/*-----------------------------------------*/
// Project	: ETS2MobileCam
// File		: Prism3D/Types/camera_manager.h
/*-----------------------------------------*/

#pragma once
#include <cstdint>

#include "Prism3D/Containers/unit.h"
#include "Prism3D/Containers/array.h"
#include "Prism3D/Containers/math.h"

#include <Prism3D/Actors/camera_manager.h>

// last updated 1.58

#pragma pack(push, 1)
namespace prism
{
	class core_camera_u : public unit_t // Size: 0x00A0
	{
	public:
		char		pad_0010[12];			//0x0010 (0x0c)
		float		m_mouse_sensitivity;	//0x001C (0x04)
		float 		m_fov;					//0x0020 (0x04)
		float 		m_near_plane;			//0x0024 (0x04)
		float 		m_far_plane;			//0x0028 (0x04)
		char		pad_002C[12];			//0x002C (0x0c)
		float 		m_horizontal_fov;		//0x0038 (0x04)
		float 		m_vertical_fov;			//0x003C (0x04)
		placement_t m_placement;			//0x0040 (0x1c)
		matrix4x4_t m_projection_matrix;	//0x005C (0x40)
	};
	static_assert(sizeof(core_camera_u) == 0xA0);

	class camera_manager_u : public unit_t // Size: 0x0088
	{
	public:
		uint32_t 					m_current_camera;	//0x0010 (0x04)
		uint32_t 					m_target_camera;	//0x0014 (0x04) 14 = none
		char						pad_0018[16];		//0x0018 (0x10)
		bool 						unk1; 				//0x0028 (0x01) false = cant look or zoom
		bool 						unk2; 				//0x0029 (0x01) false = cant look
		char 						pad_002A[6];		//0x002A (0x06)
		array_dyn_t<core_camera_u*> m_cameras; 			//0x0030 (0x28) // array_dyn_t<plain_ptr_t<core_camera_u>>
		int8_t						N0001AD12; 			//0x0058 (0x01) if 0 cant change camera
		char						pad_0059[39];		//0x0059 (0x27)
		prism::core_camera_u*   	m_core_camera;		//0x0080 (0x08) // plain_ptr_t<core_camera_u>

		static prism::camera_manager_u* get() {
			return prism::actors::camera_manager::get();
		}
	};
	static_assert(sizeof(camera_manager_u) == 0x88);
}
#pragma pack(pop)
