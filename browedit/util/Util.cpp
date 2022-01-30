#include <Windows.h>
#include <direct.h>
#include <commdlg.h>
#include "Util.h"
#include <browedit/Map.h>
#include <browedit/BrowEdit.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <iostream>
#include <browedit/actions/ObjectChangeAction.h>

namespace util
{
	short swapShort(const short s)
	{
		return ((s&0xff)<<8) | ((s>>8)&0xff);
	}


	std::string iso_8859_1_to_utf8(const std::string& str)
	{
		std::string strOut;
		for (auto it = str.cbegin(); it != str.cend(); ++it)
		{
			uint8_t ch = *it;
			if (ch < 0x80) {
				strOut.push_back(ch);
			}
			else {
				strOut.push_back(0xc0 | ch >> 6);
				strOut.push_back(0x80 | (ch & 0x3f));
			}
		}
		return strOut;
	}

	std::string utf8_to_iso_8859_1(const std::string& str)
	{
		std::string strOut;
		for (auto it = str.cbegin(); it != str.cend(); ++it)
		{
			uint8_t ch = *it;
			if (ch < 0x80) {
				strOut.push_back(ch);
			}
			else {
				++it;
				uint8_t ch2 = *it;
				strOut.push_back((ch & ~0xc0) << 6 | (ch2 & ~0x80));
			}
		}
		return strOut;
	}



	std::vector<std::string> split(std::string value, const std::string &seperator)
	{
		std::vector<std::string> ret;
		while (value.find(seperator) != std::string::npos)
		{
			size_t index = value.find(seperator);
			ret.push_back(value.substr(0, index));
			value = value.substr(index + seperator.length());
		}
		ret.push_back(value);
		return ret;
	}

	std::string combine(const std::vector<std::string>& items, const std::string& seperator)
	{
		std::string ret = "";
		for (std::size_t i = 0; i < items.size(); i++)
		{
			ret += items[i];
			if (i + 1 < items.size())
				ret += seperator;
		}
		return ret;
	}
	bool ColorEdit3(BrowEdit* browEdit, Map* map, Node* node, const char* label, glm::vec3* ptr, const std::string& action)
	{
		static glm::vec3 startValue;
		bool ret = ImGui::ColorEdit3(label, glm::value_ptr(*ptr));

		if (ImGui::IsItemActivated())
			startValue = *ptr;
		if (ImGui::IsItemDeactivatedAfterEdit())
			map->doAction(new ObjectChangeAction(node, ptr, startValue, action == "" ? label : action), browEdit);
		return ret;
	}

	bool DragInt(BrowEdit* browEdit, Map* map, Node* node, const char* label, int* ptr, float v_speed, int v_min, int v_max, const std::string& action)
	{
		static int startValue;
		bool ret = ImGui::DragInt(label, ptr, v_speed, v_min, v_max);

		if (ImGui::IsItemActivated())
			startValue = *ptr;
		if (ImGui::IsItemDeactivatedAfterEdit())
			map->doAction(new ObjectChangeAction(node, ptr, startValue, action == "" ? label : action), browEdit);
		return ret;
	}
	bool DragFloat(BrowEdit* browEdit, Map* map, Node* node, const char* label, float* ptr, float v_speed, float v_min, float v_max, const std::string& action)
	{
		static float startValue;
		bool ret = ImGui::DragFloat(label, ptr, v_speed, v_min, v_max);

		if (ImGui::IsItemActivated())
			startValue = *ptr;
		if (ImGui::IsItemDeactivatedAfterEdit())
			map->doAction(new ObjectChangeAction(node, ptr, startValue, action == "" ? label : action), browEdit);
		return ret;
	}

	bool DragFloat3(BrowEdit* browEdit, Map* map, Node* node, const char* label, glm::vec3* ptr, float v_speed, float v_min, float v_max, const std::string& action)
	{
		static glm::vec3 startValue;
		bool ret = ImGui::DragFloat3(label, glm::value_ptr(*ptr), v_speed, v_min, v_max);

		if (ImGui::IsItemActivated())
			startValue = *ptr;
		if (ImGui::IsItemDeactivatedAfterEdit())
			map->doAction(new ObjectChangeAction(node, ptr, startValue, action == "" ? label : action), browEdit);
		return ret;
	}

	bool InputText(BrowEdit* browEdit, Map* map, Node* node, const char* label, std::string* ptr, ImGuiInputTextFlags flags, const std::string& action)
	{
		static std::string startValue;
		bool ret = ImGui::InputText(label, ptr, flags);

		if (ImGui::IsItemActivated())
			startValue = *ptr;
		if (ImGui::IsItemDeactivatedAfterEdit())
			map->doAction(new ObjectChangeAction(node, ptr, startValue, action == "" ? label : action), browEdit);
		return ret;
	}




	std::string SaveAsDialog(const std::string& fileNameStr, const char* filter)
	{
		CoInitializeEx(0, 0);

		char fileName[1024];
		strcpy_s(fileName, 1024, fileNameStr.c_str());
		
		char curdir[100];
		_getcwd(curdir, 100);
		HWND hWnd = nullptr;
		OPENFILENAME ofn;
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = hWnd;
		ofn.lpstrFile = fileName;
		ofn.nMaxFile = 1024;
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 2;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT;
		if (GetSaveFileName(&ofn))
		{
			std::string newFileName(fileName);
			if (newFileName.rfind(".") > newFileName.rfind("\\"))
				newFileName = newFileName.substr(0, newFileName.rfind("."));
			_chdir(curdir);
			return fileName;
		}
		_chdir(curdir);
		return "";
	}
}



namespace glm
{
	void to_json(nlohmann::json& j, const glm::vec3& v) {
		j = nlohmann::json{ v.x, v.y, v.z };
	}
	void from_json(const nlohmann::json& j, glm::vec3& v) {
		j[0].get_to(v.x);
		j[1].get_to(v.y);
		j[2].get_to(v.z);
	}
}