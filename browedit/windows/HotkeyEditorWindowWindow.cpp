#include <browedit/BrowEdit.h>
#include <magic_enum.hpp>
#include <browedit/HotkeyRegistry.h>
#include <misc/cpp/imgui_stdlib.h>
#include <fstream>



void BrowEdit::showHotkeyEditorWindow()
{
	ImGui::Begin("Hotkey Editor");

	static std::string filter = "";
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

		title += "\nDefault: " + HotkeyRegistry::defaultHotkeys[strHotkey].toString();
		Hotkey hotkey;
		if (windowData.hotkeys.find(strHotkey) != windowData.hotkeys.end())
			hotkey = windowData.hotkeys[strHotkey];
		if (hotkeyInputBox(title.c_str(), hotkey))
			windowData.hotkeys[strHotkey] = hotkey;
		ImGui::SameLine();
		if (ImGui::Button(("Clear##" + strHotkey).c_str()))
		{
			windowData.hotkeys[strHotkey].keyCode = 0;
			windowData.hotkeys[strHotkey].modifiers = 0;
		}
	}

	if (ImGui::Button("Ok"))
	{
		config.hotkeys = windowData.hotkeys;
		HotkeyRegistry::init(config.hotkeys);
		config.save();
		windowData.hotkeyEditWindowVisible = false;
	}
	ImGui::SameLine();
	if (ImGui::Button("Cancel"))
	{
		windowData.hotkeyEditWindowVisible = false;
	}

	if (ImGui::Button("Export for defaults"))
	{
		HotkeyRegistry::defaultHotkeys = windowData.hotkeys;
		json j = HotkeyRegistry::defaultHotkeys;
		std::ofstream out("data\\defaulthotkeys.json");
		out << j;
	}
	ImGui::SameLine();
	if (ImGui::Button("Load defaults"))
		windowData.hotkeys = HotkeyRegistry::defaultHotkeys;

	ImGui::End();
}