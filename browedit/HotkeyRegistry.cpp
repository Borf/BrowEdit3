#include "HotkeyRegistry.h"
#include <magic_enum.hpp>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <browedit/BrowEdit.h>
#include <browedit/Hotkey.h>
#include <GLFW/glfw3.h>

void HotkeyRegistry::init(const std::map<std::string, Hotkey> &config)
{
	defaultHotkeys = util::FileIO::getJson("data\\defaulthotkeys.json").get< std::map<std::string, Hotkey>>();

	for (auto& c : config)
	{
		auto action = magic_enum::enum_cast<HotkeyAction>(c.first);
		if (action.has_value())
		{
			bool found = false;
			for (auto& h : hotkeys)
			{
				if (h.action == action)
				{
					h.hotkey = c.second;
					found = true;
					break;
				}
			}
			if (!found)
				hotkeys.push_back({ action.value(), c.second, []() {return true; }, nullptr });
		}
	}

}

void HotkeyRegistry::runAction(HotkeyAction action)
{
	for (const auto& hotkey : hotkeys)
	{
		if (hotkey.action == action && hotkey.condition() && hotkey.callback)
			hotkey.callback();
	}
}

bool HotkeyRegistry::checkHotkeys()
{
	for (const auto& hotkey : hotkeys)
	{
		if (ImGui::IsKeyPressed(hotkey.hotkey.keyCode) && ImGui::GetIO().KeyMods == hotkey.hotkey.modifiers && hotkey.condition() && hotkey.callback != nullptr)
		{
			hotkey.callback();
			return true;
		}
	}
	return false;
}

void HotkeyRegistry::registerAction(HotkeyAction action, const std::function<void()>& callback, const std::function<bool()>& condition)
{
	for (auto& h : hotkeys)
	{
		if (h.action == action)
		{
			h.callback = callback;
			return;
		}
	}
	hotkeys.push_back({action, Hotkey(), condition, callback });
}

const HotkeyCombi& HotkeyRegistry::getHotkey(HotkeyAction action)
{
	for (const auto& h : hotkeys)
		if (h.action == action)
			return h;
	throw "Could not find action ";
}


void HotkeyRegistry::showHotkeyPopup(BrowEdit* browEdit)
{
	ImGui::SetNextWindowSize(ImVec2(400, 250));
	if (ImGui::BeginPopupModal("HotkeyPopup", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDecoration))
	{
		static std::string lastFilter = "";
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::SetWindowFocus("HotkeyPopup");
		bool runSelected = false;
		if (ImGui::InputText("##Filter", &browEdit->windowData.hotkeyPopupFilter, ImGuiInputTextFlags_EnterReturnsTrue))
			runSelected = true;
		if (lastFilter != browEdit->windowData.hotkeyPopupFilter)
			browEdit->windowData.hotkeyPopupSelectedIndex = 0;
		lastFilter = browEdit->windowData.hotkeyPopupFilter;
		ImGui::SetKeyboardFocusHere(-1);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		if (ImGui::BeginListBox("##Items"))
		{
			std::string filter = browEdit->windowData.hotkeyPopupFilter;
			std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);

			int itemCount = 0;

			for (auto& hotkeyEnum : magic_enum::enum_entries<HotkeyAction>())
			{
				const auto& combi = getHotkey(hotkeyEnum.first);
				if (combi.condition != nullptr && !combi.condition())
					continue;
				std::string str(hotkeyEnum.second);
				std::transform(str.begin(), str.end(), str.begin(), ::tolower);

				if (browEdit->windowData.hotkeyPopupFilter != "" && str.find(browEdit->windowData.hotkeyPopupFilter) == std::string::npos)
					continue;

				std::string itemText(hotkeyEnum.second);
				if (combi.hotkey.keyCode != 0)
					itemText += " (" + combi.hotkey.toString() + ")";
				ImGui::Selectable(itemText.c_str(), browEdit->windowData.hotkeyPopupSelectedIndex == itemCount);
				if(ImGui::IsItemClicked())
					browEdit->windowData.hotkeyPopupSelectedIndex = itemCount;
				if ((ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) || (runSelected && itemCount == browEdit->windowData.hotkeyPopupSelectedIndex))
				{
					runAction(hotkeyEnum.first);
					ImGui::CloseCurrentPopup();
				}
				itemCount++;
			}
			ImGui::EndListBox();

			if (ImGui::IsKeyPressed(GLFW_KEY_UP))
				browEdit->windowData.hotkeyPopupSelectedIndex = std::max(0, browEdit->windowData.hotkeyPopupSelectedIndex - 1);
			if (ImGui::IsKeyPressed(GLFW_KEY_DOWN))
				browEdit->windowData.hotkeyPopupSelectedIndex = std::min(itemCount, browEdit->windowData.hotkeyPopupSelectedIndex + 1);

		}
		ImGui::EndPopup();
	}
}