#pragma once
#include <Spore\BasicIncludes.h>
#include "AdvancedCreatureDataResource.h"

// We use this file for the user-side of the mod: the paint categories, clicking on parts, etc

const uint32_t ACP_CATEGORY_ID = id("ACP_ce_category_paintbrush");
const uint32_t ACP_PAINT_DEFAULT = id("VE_Old_Car_Matte_01");
enum PartMode {
	Single,
	Symmetric,
	Similar,
	Categorical // TODO: allow painting by rigblock type via Ctrl + Shift
};

const uint32_t cur_paint_blue = 0x958A6A35;
const uint32_t cur_paint_green = 0x958A6A36;
const uint32_t cur_paint_red = 0x958A6A37;


member_detour(PaletteCategoryUI_LayoutPagePanel__detour, Palettes::PaletteCategoryUI, void())
{
	void detoured();
};

virtual_detour(Editor_HandleMessage__detour, Editors::cEditor, App::IMessageListener, bool(uint32_t messageID, void* msg))
{
	bool detoured(uint32_t messageID, void* msg);
};

virtual_detour(Editor_OnMouseDown__detour, Editors::cEditor, App::IGameMode, bool(MouseButton, float, float, MouseState))
{
	bool detoured(MouseButton mouseButton, float mouseX, float mouseY, MouseState mouseState);
};

virtual_detour(Editor_OnMouseUp__detour, Editors::cEditor, App::IGameMode, bool(MouseButton, float, float, MouseState))
{
	bool detoured(MouseButton mouseButton, float mouseX, float mouseY, MouseState mouseState);
};

virtual_detour(Editor_OnKeyDown__detour, Editors::cEditor, App::IGameMode, bool(int, KeyModifiers))
{
	bool detoured(int virtualKey, KeyModifiers modifiers);
};

virtual_detour(Editor_OnKeyUp__detour, Editors::cEditor, App::IGameMode, bool(int, KeyModifiers))
{
	bool detoured(int virtualKey, KeyModifiers modifiers);
};

member_detour(SetCursor__detour, UTFWin::cCursorManager, bool(uint32_t))
{
	bool detoured(uint32_t id);
};

virtual_detour(Editor_Update__detour, Editors::cEditor, App::IGameMode, void(float, float))
{
	void detoured(float delta1, float delta2);
};
