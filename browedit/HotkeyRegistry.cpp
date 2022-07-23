#include "HotkeyRegistry.h"
#include <magic_enum.hpp>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <browedit/BrowEdit.h>
#include <browedit/Hotkey.h>

void HotkeyRegistry::init(const std::map<std::string, Hotkey> &config)
{
	//auto hotkeys = magic_enum::enum_entries<HotkeyAction>();
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

	if (ImGui::BeginPopupModal("HotkeyPopup"))
	{
		ImGui::SetWindowFocus("HotkeyPopup");
		ImGui::SetItemDefaultFocus();
		ImGui::InputText("Filter", &browEdit->windowData.hotkeyPopupFilter);
		if (ImGui::BeginListBox("Items"))
		{
			for (auto& hotkeyEnum : magic_enum::enum_entries<HotkeyAction>())
			{
				const auto& combi = getHotkey(hotkeyEnum.first);
				if (combi.condition != nullptr && !combi.condition())
					continue;
				std::string str(hotkeyEnum.second);
				if (browEdit->windowData.hotkeyPopupFilter != "" && str.find(browEdit->windowData.hotkeyPopupFilter) == std::string::npos)
					continue;
				if (ImGui::Selectable((str + " " + combi.hotkey.toString()).c_str()))
				{
					runAction(hotkeyEnum.first);
				}
			}
			ImGui::EndListBox();
		}
		ImGui::EndPopup();
	}
}