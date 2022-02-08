#define IMGUI_DEFINE_MATH_OPERATORS
#include <Windows.h>
#include <direct.h>
#include <commdlg.h>
#include "Util.h"
#include <browedit/Map.h>
#include <browedit/BrowEdit.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <imgui_internal.h>
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
				if (it == str.cend())
					break;
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

	bool Checkbox(BrowEdit* browEdit, Map* map, Node* node, const char* label, bool* ptr, const std::string& action)
	{
		static bool startValue;
		bool ret = ImGui::Checkbox(label, ptr);

		if (ImGui::IsItemActivated())
			startValue = *ptr;
		if (ImGui::IsItemDeactivatedAfterEdit())
			map->doAction(new ObjectChangeAction(node, ptr, startValue, action == "" ? label : action), browEdit);
		return ret;
	}
	
	float interpolateLagrange(const std::vector<glm::vec2> &f, float x)
	{
		float result = 0; // Initialize result
		for (int i = 0; i < f.size(); i++)
		{
			float term = f[i].y;
			for (int j = 0; j < f.size(); j++)
			{
				if (j != i)
					term = term * (x - f[j].x) / (f[i].x - f[j].x);
			}
			result += term;
		}
		return result;
	}
	
	float interpolateLinear(const std::vector<glm::vec2>& f, float x) {
		glm::vec2 before(-9999,-9999), after(9999,9999);
		for (const auto& p : f)
		{
			if (p.x <= x && x - p.x < x - before.x)
				before = p;
			if (p.x >= x && p.x - x < after.x - x)
				after = p;
		}
		float diff = (x - before.x) / (after.x - before.x);
		return before.y + diff * (after.y - before.y);
	}

	void EditableGraph(const char* label, std::vector<glm::vec2>* points, std::function<float(const std::vector<glm::vec2>&, float)> interpolationStyle)
	{
		auto window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return;
		const ImGuiStyle& style = ImGui::GetStyle();
		const ImGuiIO& IO = ImGui::GetIO();
		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		
		int hovered = ImGui::IsItemActive() || ImGui::IsItemHovered(); // IsItemDragged() ?

		ImGui::Dummy(ImVec2(0, 3));

		// prepare canvas
		const float avail = ImGui::GetContentRegionAvailWidth();
		const float dim = avail;
		ImVec2 Canvas(dim, dim);

		ImRect bb(window->DC.CursorPos, window->DC.CursorPos + Canvas);
		ImGui::ItemSize(bb);
		if (!ImGui::ItemAdd(bb, NULL))
			return;

		const ImGuiID id = window->GetID(label);
		//hovered |= 0 != ImGui::IsItemHovered(ImRect(bb.Min, bb.Min + ImVec2(avail, dim)), id);

		ImGui::RenderFrame(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg, 1), true, style.FrameRounding);
		
		
		float inc = 1.0f / (bb.Max.x - bb.Min.x);
		float prev = glm::clamp(interpolationStyle(*points, 0), 0.0f, 1.0f);
		for (float f = inc; f < 1; f += inc)
		{
			float curr = glm::clamp(interpolationStyle(*points, f), 0.0f, 1.0f);
			window->DrawList->AddLine(
				ImVec2(bb.Min.x + (f - inc) * bb.GetWidth(), bb.Max.y - prev * bb.GetHeight()), 
				ImVec2(bb.Min.x + f * bb.GetWidth(), bb.Max.y - curr * bb.GetHeight()), ImGui::GetColorU32(ImGuiCol_Text, 1), 2.0f);
			prev = curr;
		}
		ImVec2 cursorPos = ImGui::GetCursorScreenPos();
		int i = 0;
		for (auto it = points->begin(); it != points->end(); )
		{
			auto& v = *it;
			ImVec2 pos = ImVec2(bb.Min.x + v.x * bb.GetWidth(), bb.Max.y - v.y * bb.GetHeight());
			window->DrawList->AddCircle(pos, 5, ImGui::GetColorU32(ImGuiCol_Text, 1), 0, 2.0f);
			ImGui::PushID(i);
			ImGui::SetCursorScreenPos(pos - ImVec2(5, 5));
			ImGui::InvisibleButton("button", ImVec2(2 * 5, 2 * 5));
			if (ImGui::IsItemActive() || ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("(%4.3f, %4.3f)", v.x, v.y);
			}
			if (ImGui::IsItemClicked(1))
			{
				it = points->erase(it); ImGui::PopID();
				continue;
			}
			if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0))
			{
				if (i > 0 && i < points->size()-1)
					v.x += ImGui::GetIO().MouseDelta.x / Canvas.x;
				v.y -= ImGui::GetIO().MouseDelta.y / Canvas.y;
				v = glm::clamp(v, 0.0f, 1.0f);
			}
			ImGui::PopID();
			it++;
			i++;
		}
		ImGui::SetCursorScreenPos(bb.Min);
		if (ImGui::InvisibleButton("new", bb.GetSize()))
		{
			ImVec2 pos = (IO.MousePos - bb.Min) / Canvas;
			pos.y = 1.0f - pos.y;
			points->push_back(glm::vec2(pos.x, pos.y));
			std::sort(points->begin(), points->end(), [](const glm::vec2& a, const glm::vec2& b)
				{
					return std::signbit(a.x - b.x);
				});

		}
		ImGui::SetCursorScreenPos(cursorPos);
	}

	void Graph(const char* label, std::function<float(float)> func)
	{
		auto window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return;
		const ImGuiStyle& style = ImGui::GetStyle();
		const ImGuiIO& IO = ImGui::GetIO();
		ImDrawList* DrawList = ImGui::GetWindowDrawList();

		int hovered = ImGui::IsItemActive() || ImGui::IsItemHovered(); // IsItemDragged() ?

		ImGui::Dummy(ImVec2(0, 3));

		// prepare canvas
		const float avail = ImGui::GetContentRegionAvailWidth();
		const float dim = avail;
		ImVec2 Canvas(dim, dim);

		ImRect bb(window->DC.CursorPos, window->DC.CursorPos + Canvas);
		ImGui::ItemSize(bb);
		if (!ImGui::ItemAdd(bb, NULL))
			return;

		const ImGuiID id = window->GetID(label);
		//hovered |= 0 != ImGui::IsItemHovered(ImRect(bb.Min, bb.Min + ImVec2(avail, dim)), id);

		ImGui::RenderFrame(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg, 1), true, style.FrameRounding);


		float inc = 1.0f / (bb.Max.x - bb.Min.x);
		float prev = glm::clamp(func(0), 0.0f, 1.0f);
		for (float f = inc; f < 1; f += inc)
		{
			float curr = glm::clamp(func(f), 0.0f, 1.0f);
			window->DrawList->AddLine(
				ImVec2(bb.Min.x + (f - inc) * bb.GetWidth(), bb.Max.y - prev * bb.GetHeight()),
				ImVec2(bb.Min.x + f * bb.GetWidth(), bb.Max.y - curr * bb.GetHeight()), ImGui::GetColorU32(ImGuiCol_Text, 1), 2.0f);
			prev = curr;
		}
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
	void to_json(nlohmann::json& j, const glm::vec2& v) {
		j = nlohmann::json{ v.x, v.y };
	}
	void from_json(const nlohmann::json& j, glm::vec2& v) {
		j[0].get_to(v.x);
		j[1].get_to(v.y);
	}
}


void to_json(nlohmann::json& j, const ImVec4& v) {
	j = nlohmann::json{ v.x, v.y, v.z, v.w };
}
void from_json(const nlohmann::json& j, ImVec4& v) {
	j[0].get_to(v.x);
	j[1].get_to(v.y);
	j[2].get_to(v.z);
	j[2].get_to(v.w);
}
void to_json(nlohmann::json& j, const ImVec2& v) {
	j = nlohmann::json{ v.x, v.y };
}
void from_json(const nlohmann::json& j, ImVec2& v) {
	j[0].get_to(v.x);
	j[1].get_to(v.y);
}


namespace std
{
	void to_json(nlohmann::json& j, const std::vector<glm::vec2>& v)
	{
		int i = 0;
		for (const auto& vv : v)
			glm::to_json(j[i++], vv);
	}
	void from_json(const nlohmann::json& j, std::vector<glm::vec2>& v)
	{
		int i = 0;
		for (const auto& jj : j)
		{
			glm::vec2 vv;
			glm::from_json(jj, vv);
			v.push_back(vv);
		}
	}
}