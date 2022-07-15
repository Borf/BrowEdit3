#pragma once

#include <json.hpp>

class Hotkey
{
public:
	unsigned char modifiers = 0;
	int keyCode = 0;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(Hotkey, modifiers, keyCode);
};