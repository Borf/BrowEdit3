#include <browedit/BrowEdit.h>
#include <magic_enum.hpp>
#include <browedit/HotkeyRegistry.h>
#include <misc/cpp/imgui_stdlib.h>

std::map<std::string, Hotkey> hotkeys;

void BrowEdit::showHotkeyEditorWindow()
{
	if (hotkeys.size() == 0)
		hotkeys = config.hotkeys;

	ImGui::Begin("Hotkey Editor");

	std::string filter = "";
	ImGui::InputText("Filter", &filter);
	std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);

	std::string lastCat = "";
	for (auto& hotkeyEnum : magic_enum::enum_entries<HotkeyAction>())
	{
		std::string strHotkey = std::string(hotkeyEnum.second);
		if (filter != "")
		{
			std::string lower = strHotkey;
			std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
			if(lower.find(filter) == std::string::npos)
				continue;
		}

		std::string cat = "";
		auto title = strHotkey;
		if (strHotkey.find("_") != std::string::npos)
		{
			cat = strHotkey.substr(0, strHotkey.find("_"));
			title = strHotkey.substr(strHotkey.find("_") + 1);
		}

		if (cat != lastCat)
			ImGui::Text(cat.c_str());
		lastCat = cat;

		Hotkey hotkey;
		if (hotkeys.find(strHotkey) != hotkeys.end())
			hotkey = hotkeys[strHotkey];
		if (hotkeyInputBox(title.c_str(), hotkey))
			hotkeys[strHotkey] = hotkey;
	}

	if (ImGui::Button("Ok"))
	{
		windowData.hotkeyEditWindowVisible = false;
	}
	ImGui::SameLine();
	if (ImGui::Button("Cancel"))
	{
		windowData.hotkeyEditWindowVisible = false;
	}

	ImGui::End();
}