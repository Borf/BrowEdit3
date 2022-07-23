#include "Hotkey.h"
#include <browedit/util/glfw_keycodes_to_string.h>
#include <imgui.h>

std::string Hotkey::toString() const
{
	std::string text;
	if ((modifiers & ImGuiKeyModFlags_Ctrl) != 0)
		text += "Ctrl+";
	if ((modifiers & ImGuiKeyModFlags_Shift) != 0)
		text += "Shift+";
	if ((modifiers & ImGuiKeyModFlags_Alt) != 0)
		text += "Alt+";
	text += util::KeyCodeToStringSwitch((util::KeyCode)keyCode);

	if (keyCode == 0)
		text = "-";
	return text;
}
