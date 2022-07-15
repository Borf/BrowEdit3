#include "HotkeyRegistry.h"
#include <magic_enum.hpp>
#include <iostream>

void HotkeyRegistry::init()
{
	auto hotkeys = magic_enum::enum_entries<HotkeyAction>();
	for (auto& hotkey : hotkeys)
	{
		//std::cout << hotkey.first << " " << hotkey.second << std::endl;
	}

}