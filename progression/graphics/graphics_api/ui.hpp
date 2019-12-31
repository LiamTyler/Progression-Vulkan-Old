#pragma once

#include "imgui/imgui.h"
#include "graphics/graphics_api/command_buffer.hpp"

namespace Progression
{
namespace Gfx
{
namespace UIOverlay
{

    bool Init();
    void Shutdown();
    
    void Draw( CommandBuffer& cmdBuf );
    bool CapturingMouse(); // true if cursor is ontop of UI element

    bool Header( const char* caption );
	bool CheckBox( const char* caption, bool* value );
	bool CheckBox( const char* caption, int* value );
	bool InputFloat( const char* caption, float* value, float step, uint32_t precision );
	bool SliderFloat( const char* caption, float* value, float min, float max );
	bool SliderInt( const char* caption, int* value, int min, int max );
	bool ComboBox( const char* caption, int* itemindex, const std::vector< std::string >& items );
	bool Button( const char* caption );
	void Text( const char* formatstr, ... );

    bool Visible();
    void SetVisible( bool b );
    bool Updated();

} // namespace UIOverlay
} // namespace Gfx
} // namespace Progression