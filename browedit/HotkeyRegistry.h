#pragma once

#include <string>
#include <vector>

enum class HotkeyAction
{
	Global_Save,
	Global_Load,
	Global_Exit,
	Global_Lightmap,

	Texture_RotateLeft,
	Texture_RotateRight,
	Texture_FlipHorizontal,
	Texture_FlipVertical,

	Gat_RaiseToHeight,
};

class HotkeyRegistry
{
public:
	inline static std::vector<std::string> actions;
	static void init();
};