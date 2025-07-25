#include "Util.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/norm.hpp>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <imgui_internal.h>
#include <iostream>
#include <ShlObj_core.h>

#include <browedit/Map.h>
#include <browedit/BrowEdit.h>
#include <browedit/components/Rsw.h>
#include <browedit/components/RsmRenderer.h>
#include <browedit/actions/ObjectChangeAction.h>
#include <browedit/actions/GroupAction.h>
#include <browedit/gl/Texture.h>

#ifdef WIN32
	#include <Windows.h>
	#include <direct.h>
	#include <commdlg.h>

	#include <DbgHelp.h>
	#pragma comment(lib,"dbghelp.lib")

	#define GLFW_EXPOSE_NATIVE_WIN32
	#define GLFW_NATIVE_INCLUDE_NONE
#endif
#include <GLFW/glfw3native.h>

// The hacks are continuing
extern BrowEdit* browEdit;

namespace util
{
	std::pair<const char*, double> getSizeUnit(std::size_t x)
	{
		if (x == 0) return std::make_pair(util::sizeUnits[0], 0.f);

		uint8_t unit = 0;

		while((x / 1024) > 1024)
		{
			if((unit + 1) == util::sizeUnits.size()) break;

			++unit;
			x /= 1024;
		}

		double size = static_cast<double>(x);
		if ((x / 1024.f) > 0 && (unit + 1) < util::sizeUnits.size())
		{
			++unit;
			size /= 1024.f;
		}

		return std::make_pair(util::sizeUnits[unit], size);
	}

	const char* toSizeUnit(std::size_t x)
	{

		return getSizeUnit(x).first;
	}

	double toUnitSize(std::size_t x)
	{
		return getSizeUnit(x).second;
	}

	short swapShort(const short s)
	{
		return ((s&0xff)<<8) | ((s>>8)&0xff);
	}

	float wrap360(float value)
	{
		while (value > 180)
			value -= 360;
		while (value < -180)
			value += 360;
		return value;
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

	std::string& tolowerInPlace(std::string& str)
	{
		std::transform(str.begin(), str.end(), str.begin(), ::tolower);
		return str;
	}

	std::string tolower(std::string str)
	{
		std::transform(str.begin(), str.end(), str.begin(), ::tolower);
		return str;
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

	std::string replace(std::string orig, const std::string& find, const std::string& replace)
	{
		size_t index = 0;
		while (true)
		{
			index = orig.find(find, index);
			if (index == std::string::npos)
				break;
			orig.replace(index, find.length(), replace);
			index += replace.length();
		}
		return orig;
	}



	bool ColorEdit3(BrowEdit* browEdit, Map* map, Node* node, const char* label, glm::vec3* ptr, const std::string& action)
	{
		static glm::vec3 startValue;
		bool ret = ImGui::ColorEdit3(label, glm::value_ptr(*ptr));

		if (ImGui::IsItemActivated())
			startValue = *ptr;
		if (ImGui::IsItemDeactivatedAfterEdit())
			map->doAction(new ObjectChangeAction(node, ptr, startValue, action == "" ? label : action), browEdit);
		ImGui::PushID(label);
		if (ImGui::BeginPopupContextItem("CopyPaste"))
		{
			try {
				if (ImGui::MenuItem("Copy"))
				{
					json clipboard;
					to_json(clipboard, *ptr);
					ImGui::SetClipboardText(clipboard.dump(1).c_str());
				}
				if (ImGui::MenuItem("Paste"))
				{
					auto cb = ImGui::GetClipboardText();
					if (cb)
					{
						startValue = *ptr;
						from_json(json::parse(std::string(cb)), *ptr);
						map->doAction(new ObjectChangeAction(node, ptr, startValue, action == "" ? label : action), browEdit);
						ret = true;
					}
				}
			}
			catch (...) {}
			ImGui::EndPopup();
		}
		ImGui::PopID();
		return ret;
	}
	bool ColorEdit4(BrowEdit* browEdit, Map* map, Node* node, const char* label, glm::vec4* ptr, const std::string& action)
	{
		static glm::vec4 startValue;
		bool ret = ImGui::ColorEdit4(label, glm::value_ptr(*ptr));

		if (ImGui::IsItemActivated())
			startValue = *ptr;
		if (ImGui::IsItemDeactivatedAfterEdit())
			map->doAction(new ObjectChangeAction(node, ptr, startValue, action == "" ? label : action), browEdit);
		ImGui::PushID(label);
		if (ImGui::BeginPopupContextItem("CopyPaste"))
		{
			try {
				if (ImGui::MenuItem("Copy"))
				{
					json clipboard;
					to_json(clipboard, *ptr);
					ImGui::SetClipboardText(clipboard.dump(1).c_str());
				}
				if (ImGui::MenuItem("Paste"))
				{
					auto cb = ImGui::GetClipboardText();
					if (cb)
					{
						startValue = *ptr;
						from_json(json::parse(std::string(cb)), *ptr);
						map->doAction(new ObjectChangeAction(node, ptr, startValue, action == "" ? label : action), browEdit);
						ret = true;
					}
				}
			}
			catch (...) {}
			ImGui::EndPopup();
		}
		ImGui::PopID();
		return ret;
	}

	bool DragInt(BrowEdit* browEdit, Map* map, Node* node, const char* label, int* ptr, float v_speed, int v_min, int v_max, const std::string& action, const std::function<void(int*, int)>& editAction)
	{
		static int startValue;
		bool ret = ImGui::DragInt(label, ptr, v_speed, v_min, v_max);

		if (ImGui::IsItemActivated())
			startValue = *ptr;
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			if (editAction != nullptr)
				editAction(ptr, startValue);
			else
				map->doAction(new ObjectChangeAction(node, ptr, startValue, action == "" ? label : action), browEdit);
		}
		ImGui::PushID(label);
		if (ImGui::BeginPopupContextItem("CopyPaste"))
		{
			try {
				if (ImGui::MenuItem("Copy"))
				{
					ImGui::SetClipboardText(std::to_string(*ptr).c_str());
				}
				if (ImGui::MenuItem("Paste"))
				{
					auto cb = ImGui::GetClipboardText();
					if (cb)
					{
						startValue = *ptr;
						*ptr = std::stoi(cb);
						if (editAction != nullptr)
							editAction(ptr, startValue);
						else
							map->doAction(new ObjectChangeAction(node, ptr, startValue, action == "" ? label : action), browEdit);
						ret = true;
					}
				}
			}
			catch (...) {}
			ImGui::EndPopup();
		}
		ImGui::PopID();
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
		ImGui::PushID(label);
		if (ImGui::BeginPopupContextItem("CopyPaste"))
		{
			try {
				if (ImGui::MenuItem("Copy"))
				{
					ImGui::SetClipboardText(std::to_string(*ptr).c_str());
				}
				if (ImGui::MenuItem("Paste"))
				{
					auto cb = ImGui::GetClipboardText();
					if (cb)
					{
						startValue = *ptr;
						*ptr = std::stof(cb);
						map->doAction(new ObjectChangeAction(node, ptr, startValue, action == "" ? label : action), browEdit);
						ret = true;
					}
				}
			}
			catch (...) {}
			ImGui::EndPopup();
		}
		ImGui::PopID();		
		return ret;
	}

	bool DragFloat2(BrowEdit* browEdit, Map* map, Node* node, const char* label, glm::vec2* ptr, float v_speed, float v_min, float v_max, const std::string& action)
	{
		static glm::vec2 startValue;
		bool ret = ImGui::DragFloat2(label, glm::value_ptr(*ptr), v_speed, v_min, v_max);

		if (ImGui::IsItemActivated())
			startValue = *ptr;
		if (ImGui::IsItemDeactivatedAfterEdit())
			map->doAction(new ObjectChangeAction(node, ptr, startValue, action == "" ? label : action), browEdit);
		ImGui::PushID(label);
		if (ImGui::BeginPopupContextItem("CopyPaste"))
		{
			try {
				if (ImGui::MenuItem("Copy"))
				{
					json clipboard;
					to_json(clipboard, *ptr);
					ImGui::SetClipboardText(clipboard.dump(1).c_str());
				}
				if (ImGui::MenuItem("Paste"))
				{
					auto cb = ImGui::GetClipboardText();
					if (cb)
					{
						startValue = *ptr;
						from_json(json::parse(std::string(cb)), *ptr);
						map->doAction(new ObjectChangeAction(node, ptr, startValue, action == "" ? label : action), browEdit);
						ret = true;
					}
				}
			}
			catch (...) {}
			ImGui::EndPopup();
		}
		ImGui::PopID();
		return ret;
	}

	bool DragFloat3(BrowEdit* browEdit, Map* map, Node* node, const char* label, glm::vec3* ptr, float v_speed, float v_min, float v_max, const std::string& action, bool moveTogether)
	{
		static glm::vec3 startValue;
		bool ret = ImGui::DragFloat3(label, glm::value_ptr(*ptr), v_speed, v_min, v_max);
		if (ret && moveTogether)
		{
			if ((*ptr)[1] == (*ptr)[2])
				(*ptr)[2] = (*ptr)[1] = (*ptr)[0];
			else if ((*ptr)[0] == (*ptr)[1])
				(*ptr)[0] = (*ptr)[1] = (*ptr)[2];
			else if ((*ptr)[0] == (*ptr)[2])
				(*ptr)[2] = (*ptr)[0] = (*ptr)[1];
		}


		if (ImGui::IsItemActivated())
			startValue = *ptr;
		if (ImGui::IsItemDeactivatedAfterEdit())
			map->doAction(new ObjectChangeAction(node, ptr, startValue, action == "" ? label : action), browEdit);
		ImGui::PushID(label);
		if(ImGui::BeginPopupContextItem("CopyPaste"))
		{
			try {
				if (ImGui::MenuItem("Copy"))
				{
					json clipboard;
					to_json(clipboard, *ptr);
					ImGui::SetClipboardText(clipboard.dump(1).c_str());
				}
				if (ImGui::MenuItem("Paste"))
				{
					auto cb = ImGui::GetClipboardText();
					if (cb)
					{
						startValue = *ptr;
						from_json(json::parse(std::string(cb)), *ptr);
						map->doAction(new ObjectChangeAction(node, ptr, startValue, action == "" ? label : action), browEdit);
						ret = true;
					}
				}
			}
			catch (...) {}
			ImGui::EndPopup();
		}
		ImGui::PopID();

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
		ImGui::PushID(label);
		if (ImGui::BeginPopupContextItem("CopyPaste"))
		{
			try {
				if (ImGui::MenuItem("Copy"))
				{
					ImGui::SetClipboardText(ptr->c_str());
				}
				if (ImGui::MenuItem("Paste"))
				{
					auto cb = ImGui::GetClipboardText();
					if (cb)
					{
						startValue = *ptr;
						*ptr = cb;
						map->doAction(new ObjectChangeAction(node, ptr, startValue, action == "" ? label : action), browEdit);
						ret = true;
					}
				}
			}
			catch (...) {}
			ImGui::EndPopup();
		}
		ImGui::PopID();

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
		ImGui::PushID(label);
		if (ImGui::BeginPopupContextItem("CopyPaste"))
		{
			try {
				if (ImGui::MenuItem("Copy"))
				{
					ImGui::SetClipboardText(*ptr ? "true":"false");
				}
				if (ImGui::MenuItem("Paste"))
				{
					auto cb = ImGui::GetClipboardText();
					if (cb)
					{
						startValue = *ptr;
						if (strcmp(cb, "true") == 0)
							*ptr = true;
						else
							*ptr = false;
						map->doAction(new ObjectChangeAction(node, ptr, startValue, action == "" ? label : action), browEdit);
						ret = true;
					}
				}
			}
			catch (...) {}
			ImGui::EndPopup();
		}
		ImGui::PopID();
		return ret;
	}
	

	template<class T>
	bool InputTextMulti(BrowEdit* browEdit, Map* map, const std::vector<T*>& data, const char* label, const std::function<std::string* (T*)>& getProp, const std::function<void(Node* node, std::string* ptr, std::string* startValue, const std::string& action)>& editAction)
	{
		static std::vector<std::string> startValues;
		bool differentValues = !std::all_of(data.begin(), data.end(), [&](T* o) { return *getProp(o) == *getProp(data.front()); });
		std::string f = *getProp(data.front());
		if (differentValues)
			f = "multiple";
		bool ret = ImGui::InputText(label, &f);
		if (ret)
			for (auto o : data)
				*getProp(o) = f;
		if (ImGui::IsItemActivated())
		{
			startValues.clear();
			for (auto o : data)
				startValues.push_back(*getProp(o));
		}
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			auto ga = new GroupAction();
			for (auto i = 0; i < data.size(); i++)
				editAction(data[i]->node, getProp(data[i]), &startValues[i], label);
			map->doAction(ga, browEdit);
		}
		if (!differentValues)
		{
			ImGui::PushID(label);
			if (ImGui::BeginPopupContextItem("CopyPaste"))
			{
				try {
					if (ImGui::MenuItem("Copy"))
					{
						ImGui::SetClipboardText(getProp(data[0])->c_str());
					}
					if (ImGui::MenuItem("Paste"))
					{
						auto cb = ImGui::GetClipboardText();
						if (cb)
						{
							startValues.clear();
							for (auto o : data)
							{
								startValues.push_back(*getProp(o));
								*getProp(o) = cb;
							}
							auto ga = new GroupAction();
							for (auto i = 0; i < data.size(); i++)
								editAction(data[i]->node, getProp(data[i]), &startValues[i], label);
							ret = true;
						}
					}
				}
				catch (...) {}
				ImGui::EndPopup();
			}
			ImGui::PopID();
		}
		return ret;
	}
	template bool InputTextMulti<RswObject>(BrowEdit* browEdit, Map* map, const std::vector<RswObject*>& data, const char* label, const std::function<std::string* (RswObject*)>& getProp, const std::function<void(Node* node, std::string* ptr, std::string* startValue, const std::string& action)>& editAction);
	template bool InputTextMulti<RswLight>(BrowEdit* browEdit, Map* map, const std::vector<RswLight*>& data, const char* label, const std::function<std::string* (RswLight*)>& getProp, const std::function<void(Node* node, std::string* ptr, std::string* startValue, const std::string& action)>& editAction);
	template bool InputTextMulti<RswEffect>(BrowEdit* browEdit, Map* map, const std::vector<RswEffect*>& data, const char* label, const std::function<std::string* (RswEffect*)>& getProp, const std::function<void(Node* node, std::string* ptr, std::string* startValue, const std::string& action)>& editAction);
	template bool InputTextMulti<LubEffect>(BrowEdit* browEdit, Map* map, const std::vector<LubEffect*>& data, const char* label, const std::function<std::string* (LubEffect*)>& getProp, const std::function<void(Node* node, std::string* ptr, std::string* startValue, const std::string& action)>& editAction);
	template bool InputTextMulti<RswSound>(BrowEdit* browEdit, Map* map, const std::vector<RswSound*>& data, const char* label, const std::function<std::string* (RswSound*)>& getProp, const std::function<void(Node* node, std::string* ptr, std::string* startValue, const std::string& action)>& editAction);
	template bool InputTextMulti<RswModel>(BrowEdit* browEdit, Map* map, const std::vector<RswModel*>& data, const char* label, const std::function<std::string* (RswModel*)>& getProp, const std::function<void(Node* node, std::string* ptr, std::string* startValue, const std::string& action)>& editAction);

	template<class T>
	bool InputTextMulti(BrowEdit* browEdit, Map* map, const std::vector<T*>& data, const char* label, const std::function<std::string* (T*)>& getProp)
	{
		static std::vector<std::string> startValues;
		bool differentValues = !std::all_of(data.begin(), data.end(), [&](T* o) { return *getProp(o) == *getProp(data.front()); });
		std::string f = *getProp(data.front());
		if (differentValues)
			f = "multiple";
		bool ret = ImGui::InputText(label, &f);
		if (ret)
			for (auto o : data)
				*getProp(o) = f;
		if (ImGui::IsItemActivated())
		{
			startValues.clear();
			for (auto o : data)
				startValues.push_back(*getProp(o));
		}
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			auto ga = new GroupAction();
			for (auto i = 0; i < data.size(); i++)
				ga->addAction(new ObjectChangeAction(data[i]->node, getProp(data[i]), startValues[i], label));
			map->doAction(ga, browEdit);
		}
		if (!differentValues)
		{
			ImGui::PushID(label);
			if (ImGui::BeginPopupContextItem("CopyPaste"))
			{
				try {
					if (ImGui::MenuItem("Copy"))
					{
						ImGui::SetClipboardText(getProp(data[0])->c_str());
					}
					if (ImGui::MenuItem("Paste"))
					{
						auto cb = ImGui::GetClipboardText();
						if (cb)
						{
							startValues.clear();
							for (auto o : data)
							{
								startValues.push_back(*getProp(o));
								*getProp(o) = cb;
							}
							auto ga = new GroupAction();
							for (auto i = 0; i < data.size(); i++)
								ga->addAction(new ObjectChangeAction(data[i]->node, getProp(data[i]), startValues[i], label));
							ret = true;
						}
					}
				}
				catch (...) {}
				ImGui::EndPopup();
			}
			ImGui::PopID();
		}
		return ret;
	}
	template bool InputTextMulti<RswObject>(BrowEdit* browEdit, Map* map, const std::vector<RswObject*>& data, const char* label, const std::function<std::string* (RswObject*)>& getProp);
	template bool InputTextMulti<RswLight>(BrowEdit* browEdit, Map* map, const std::vector<RswLight*>& data, const char* label, const std::function<std::string* (RswLight*)>& getProp);
	template bool InputTextMulti<RswEffect>(BrowEdit* browEdit, Map* map, const std::vector<RswEffect*>& data, const char* label, const std::function<std::string* (RswEffect*)>& getProp);
	template bool InputTextMulti<LubEffect>(BrowEdit* browEdit, Map* map, const std::vector<LubEffect*>& data, const char* label, const std::function<std::string* (LubEffect*)>& getProp);
	template bool InputTextMulti<RswSound>(BrowEdit* browEdit, Map* map, const std::vector<RswSound*>& data, const char* label, const std::function<std::string* (RswSound*)>& getProp);
	template bool InputTextMulti<RswModel>(BrowEdit* browEdit, Map* map, const std::vector<RswModel*>& data, const char* label, const std::function<std::string* (RswModel*)>& getProp);

	const ImGuiDataTypeInfo* ImGui::DataTypeGetInfo(ImGuiDataType data_type);
	bool DragScalarNMultiLabel(const char* label, ImGuiDataType data_type, void* p_data, int components, float v_speed, const void* p_min, const void* p_max, const std::vector<const char*> &formats, ImGuiSliderFlags flags)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		bool value_changed = false;
		ImGui::BeginGroup();
		ImGui::PushID(label);
		ImGui::PushMultiItemsWidths(components, ImGui::CalcItemWidth());
		size_t type_size = ImGui::DataTypeGetInfo(data_type)->Size;
		for (int i = 0; i < components; i++)
		{
			ImGui::PushID(i);
			if (i > 0)
				ImGui::SameLine(0, g.Style.ItemInnerSpacing.x);
			value_changed |= ImGui::DragScalar("", data_type, p_data, v_speed, p_min, p_max, formats[i], flags);
			ImGui::PopID();
			ImGui::PopItemWidth();
			p_data = (void*)((char*)p_data + type_size);
		}
		ImGui::PopID();

		const char* label_end = ImGui::FindRenderedTextEnd(label);
		if (label != label_end)
		{
			ImGui::SameLine(0, g.Style.ItemInnerSpacing.x);
			ImGui::TextEx(label, label_end);
		}

		ImGui::EndGroup();
		return value_changed;
	}

	template<class T>
	bool DragFloatMulti(BrowEdit* browEdit, Map* map, const std::vector<T*> &data, const char* label, const std::function<float* (T*)>& getProp, float v_speed, float v_min, float v_max)
	{
		static std::vector<float> startValues;
		bool differentValues = !std::all_of(data.begin(), data.end(), [&](T* o) { return *getProp(o) == *getProp(data.front()); });
		float* f = getProp(data.front());
		bool ret = ImGui::DragFloat(label, f, v_speed, v_min, v_max, differentValues ? "multiple" : nullptr);
		if(ret)
			for (auto o : data)
				*getProp(o) = *f;
		if (ImGui::IsItemActivated())
		{
			startValues.clear();
			for (auto o : data)
				startValues.push_back(*getProp(o));
		}
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			auto ga = new GroupAction();
			for(auto i = 0; i < data.size(); i++)
				ga->addAction(new ObjectChangeAction(data[i]->node, getProp(data[i]), startValues[i], label));
			map->doAction(ga, browEdit);
		}
		if (!differentValues)
		{
			ImGui::PushID(label);
			if (ImGui::BeginPopupContextItem("CopyPaste"))
			{
				try {
					if (ImGui::MenuItem("Copy"))
					{
						ImGui::SetClipboardText(std::to_string(*getProp(data[0])).c_str());
					}
					if (ImGui::MenuItem("Paste"))
					{
						auto cb = ImGui::GetClipboardText();
						if (cb)
						{
							startValues.clear();
							for (auto o : data)
							{
								startValues.push_back(*getProp(o));
								*getProp(o) = std::stof(cb);
							}
							auto ga = new GroupAction();
							for (auto i = 0; i < data.size(); i++)
								ga->addAction(new ObjectChangeAction(data[i]->node, getProp(data[i]), startValues[i], label));
							ret = true;
						}
					}
				}
				catch (...) {}
				ImGui::EndPopup();
			}
			ImGui::PopID();
		}
		return ret;
	}
	template bool DragFloatMulti<RswObject>(BrowEdit* browEdit, Map* map, const std::vector<RswObject*>& data, const char* label, const std::function<float* (RswObject*)>& getProp, float v_speed, float v_min, float v_max);
	template bool DragFloatMulti<RswLight>(BrowEdit* browEdit, Map* map, const std::vector<RswLight*>& data, const char* label, const std::function<float* (RswLight*)>& getProp, float v_speed, float v_min, float v_max);
	template bool DragFloatMulti<RswEffect>(BrowEdit* browEdit, Map* map, const std::vector<RswEffect*>& data, const char* label, const std::function<float* (RswEffect*)>& getProp, float v_speed, float v_min, float v_max);
	template bool DragFloatMulti<LubEffect>(BrowEdit* browEdit, Map* map, const std::vector<LubEffect*>& data, const char* label, const std::function<float* (LubEffect*)>& getProp, float v_speed, float v_min, float v_max);
	template bool DragFloatMulti<RswSound>(BrowEdit* browEdit, Map* map, const std::vector<RswSound*>& data, const char* label, const std::function<float* (RswSound*)>& getProp, float v_speed, float v_min, float v_max);
	template bool DragFloatMulti<RswModel>(BrowEdit* browEdit, Map* map, const std::vector<RswModel*>& data, const char* label, const std::function<float* (RswModel*)>& getProp, float v_speed, float v_min, float v_max);


	template<class T>
	bool DragIntMulti(BrowEdit* browEdit, Map* map, const std::vector<T*>& data, const char* label, const std::function<int* (T*)>& getProp, int v_speed, int v_min, int v_max)
	{
		static std::vector<int> startValues;
		bool differentValues = !std::all_of(data.begin(), data.end(), [&](T* o) { return *getProp(o) == *getProp(data.front()); });
		int* f = getProp(data.front());
		bool ret = ImGui::DragInt(label, f, (float)v_speed, v_min, v_max, differentValues ? "multiple" : nullptr);
		if (ret)
			for (auto o : data)
				*getProp(o) = *f;
		if (ImGui::IsItemActivated())
		{
			startValues.clear();
			for (auto o : data)
				startValues.push_back(*getProp(o));
		}
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			auto ga = new GroupAction();
			for (auto i = 0; i < data.size(); i++)
				ga->addAction(new ObjectChangeAction(data[i]->node, getProp(data[i]), startValues[i], label));
			map->doAction(ga, browEdit);
		}
		if (!differentValues)
		{
			ImGui::PushID(label);
			if (ImGui::BeginPopupContextItem("CopyPaste"))
			{
				try {
					if (ImGui::MenuItem("Copy"))
					{
						ImGui::SetClipboardText(std::to_string(*getProp(data[0])).c_str());
					}
					if (ImGui::MenuItem("Paste"))
					{
						auto cb = ImGui::GetClipboardText();
						if (cb)
						{
							startValues.clear();
							for (auto o : data)
							{
								startValues.push_back(*getProp(o));
								*getProp(o) = std::stoi(cb);
							}
							auto ga = new GroupAction();
							for (auto i = 0; i < data.size(); i++)
								ga->addAction(new ObjectChangeAction(data[i]->node, getProp(data[i]), startValues[i], label));
							ret = true;
						}
					}
				}
				catch (...) {}
				ImGui::EndPopup();
			}
			ImGui::PopID();
		}
		return ret;
	}
	template bool DragIntMulti<RswObject>(BrowEdit* browEdit, Map* map, const std::vector<RswObject*>& data, const char* label, const std::function<int* (RswObject*)>& getProp, int v_speed, int v_min, int v_max);
	template bool DragIntMulti<RswLight>(BrowEdit* browEdit, Map* map, const std::vector<RswLight*>& data, const char* label, const std::function<int* (RswLight*)>& getProp, int v_speed, int v_min, int v_max);
	template bool DragIntMulti<RswEffect>(BrowEdit* browEdit, Map* map, const std::vector<RswEffect*>& data, const char* label, const std::function<int* (RswEffect*)>& getProp, int v_speed, int v_min, int v_max);
	template bool DragIntMulti<LubEffect>(BrowEdit* browEdit, Map* map, const std::vector<LubEffect*>& data, const char* label, const std::function<int* (LubEffect*)>& getProp, int v_speed, int v_min, int v_max);
	template bool DragIntMulti<RswSound>(BrowEdit* browEdit, Map* map, const std::vector<RswSound*>& data, const char* label, const std::function<int* (RswSound*)>& getProp, int v_speed, int v_min, int v_max);
	template bool DragIntMulti<RswModel>(BrowEdit* browEdit, Map* map, const std::vector<RswModel*>& data, const char* label, const std::function<int* (RswModel*)>& getProp, int v_speed, int v_min, int v_max);


	template<class T>
	bool DragCharMulti(BrowEdit* browEdit, Map* map, const std::vector<T*>& data, const char* label, const std::function<char* (T*)>& getProp, int v_speed, int v_min, int v_max)
	{
		static std::vector<char> startValues;
		bool differentValues = !std::all_of(data.begin(), data.end(), [&](T* o) { return *getProp(o) == *getProp(data.front()); });
		char* f = getProp(data.front());
		bool ret = ImGui::DragScalar(label, ImGuiDataType_U8, f, (float)v_speed, &v_min, &v_max, differentValues ? "multiple" : nullptr);
		if (ret)
			for (auto o : data)
				*getProp(o) = *f;
		if (ImGui::IsItemActivated())
		{
			startValues.clear();
			for (auto o : data)
				startValues.push_back(*getProp(o));
		}
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			auto ga = new GroupAction();
			for (auto i = 0; i < data.size(); i++)
				ga->addAction(new ObjectChangeAction(data[i]->node, getProp(data[i]), startValues[i], label));
			map->doAction(ga, browEdit);
		}
		if (!differentValues)
		{
			ImGui::PushID(label);
			if (ImGui::BeginPopupContextItem("CopyPaste"))
			{
				try {
					if (ImGui::MenuItem("Copy"))
					{
						ImGui::SetClipboardText(std::to_string(*getProp(data[0])).c_str());
					}
					if (ImGui::MenuItem("Paste"))
					{
						auto cb = ImGui::GetClipboardText();
						if (cb)
						{
							startValues.clear();
							for (auto o : data)
							{
								startValues.push_back(*getProp(o));
								*getProp(o) = std::stoi(cb);
							}
							auto ga = new GroupAction();
							for (auto i = 0; i < data.size(); i++)
								ga->addAction(new ObjectChangeAction(data[i]->node, getProp(data[i]), startValues[i], label));
							ret = true;
						}
					}
				}
				catch (...) {}
				ImGui::EndPopup();
			}
			ImGui::PopID();
		}
		return ret;
	}
	template bool DragCharMulti<RswObject>(BrowEdit* browEdit, Map* map, const std::vector<RswObject*>& data, const char* label, const std::function<char* (RswObject*)>& getProp, int v_speed, int v_min, int v_max);
	template bool DragCharMulti<RswLight>(BrowEdit* browEdit, Map* map, const std::vector<RswLight*>& data, const char* label, const std::function<char* (RswLight*)>& getProp, int v_speed, int v_min, int v_max);
	template bool DragCharMulti<RswEffect>(BrowEdit* browEdit, Map* map, const std::vector<RswEffect*>& data, const char* label, const std::function<char* (RswEffect*)>& getProp, int v_speed, int v_min, int v_max);
	template bool DragCharMulti<LubEffect>(BrowEdit* browEdit, Map* map, const std::vector<LubEffect*>& data, const char* label, const std::function<char* (LubEffect*)>& getProp, int v_speed, int v_min, int v_max);
	template bool DragCharMulti<RswSound>(BrowEdit* browEdit, Map* map, const std::vector<RswSound*>& data, const char* label, const std::function<char* (RswSound*)>& getProp, int v_speed, int v_min, int v_max);
	template bool DragCharMulti<RswModel>(BrowEdit* browEdit, Map* map, const std::vector<RswModel*>& data, const char* label, const std::function<char* (RswModel*)>& getProp, int v_speed, int v_min, int v_max);

	template<class T>
	bool DragFloat3Multi(BrowEdit* browEdit, Map* map, const std::vector<T*>& data, const char* label, const std::function<glm::vec3* (T*)>& getProp, float v_speed, float v_min, float v_max, bool moveTogether)
	{
		static std::vector<glm::vec3> startValues;
		glm::vec3* f = getProp(data.front());
		std::vector<const char*> formats;
		bool differentValues = false;
		for (int c = 0; c < 3; c++)
		{
			bool diff = !std::all_of(data.begin(), data.end(), [&](T* o) { return (*getProp(o))[c] == (*getProp(data.front()))[c]; });
			differentValues = differentValues || diff;
			formats.push_back(diff ? "multiple" : nullptr);
		}
		bool ret = DragScalarNMultiLabel(label, ImGuiDataType_Float, glm::value_ptr(*f), 3, v_speed, &v_min, &v_max, formats, 0);
		if (ret && moveTogether)
		{
			if ((*f)[1] == (*f)[2])
				(*f)[2] = (*f)[1] = (*f)[0];
			else if ((*f)[0] == (*f)[1])
				(*f)[0] = (*f)[1] = (*f)[2];
			else if ((*f)[0] == (*f)[2])
				(*f)[2] = (*f)[0] = (*f)[1];
		}
		if (ret)
		{
			for (auto o : data)
			{
				if (f->x != startValues[0].x)
					getProp(o)->x = f->x;
				if (f->y != startValues[0].y)
					getProp(o)->y = f->y;
				if (f->z != startValues[0].z)
					getProp(o)->z = f->z;
			}
		}
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			auto ga = new GroupAction();
			for (auto i = 0; i < data.size(); i++)
				ga->addAction(new ObjectChangeAction(data[i]->node, getProp(data[i]), startValues[i], label));
			map->doAction(ga, browEdit);
		}
		if (ImGui::IsItemActivated())
		{
			startValues.clear();
			for (auto o : data)
				startValues.push_back(*getProp(o));
		}
		if (!differentValues)
		{
			ImGui::PushID(label);
			if (ImGui::BeginPopupContextItem("CopyPaste"))
			{
				try {
					if (ImGui::MenuItem("Copy"))
					{
						json clipboard;
						to_json(clipboard, *getProp(data[0]));
						ImGui::SetClipboardText(clipboard.dump(1).c_str());
					}
					if (ImGui::MenuItem("Paste"))
					{
						auto cb = ImGui::GetClipboardText();
						if (cb)
						{
							startValues.clear();
							for (auto o : data)
							{
								startValues.push_back(*getProp(o));
								from_json(json::parse(std::string(cb)), *getProp(o));
							}
							auto ga = new GroupAction();
							for (auto i = 0; i < data.size(); i++)
								ga->addAction(new ObjectChangeAction(data[i]->node, getProp(data[i]), startValues[i], label));
							ret = true;
						}
					}
				}
				catch (...) {}
				ImGui::EndPopup();
			}
			ImGui::PopID();
		}

		return ret;
	}
	template bool DragFloat3Multi<RswObject>(BrowEdit* browEdit, Map* map, const std::vector<RswObject*>& data, const char* label, const std::function<glm::vec3* (RswObject*)>& getProp, float v_speed, float v_min, float v_max, bool moveTogether);
	template bool DragFloat3Multi<RswModel>(BrowEdit* browEdit, Map* map, const std::vector<RswModel*>& data, const char* label, const std::function<glm::vec3* (RswModel*)>& getProp, float v_speed, float v_min, float v_max, bool moveTogether);
	template bool DragFloat3Multi<RswEffect>(BrowEdit* browEdit, Map* map, const std::vector<RswEffect*>& data, const char* label, const std::function<glm::vec3* (RswEffect*)>& getProp, float v_speed, float v_min, float v_max, bool moveTogether);
	template bool DragFloat3Multi<LubEffect>(BrowEdit* browEdit, Map* map, const std::vector<LubEffect*>& data, const char* label, const std::function<glm::vec3* (LubEffect*)>& getProp, float v_speed, float v_min, float v_max, bool moveTogether);
	template bool DragFloat3Multi<RswSound>(BrowEdit* browEdit, Map* map, const std::vector<RswSound*>& data, const char* label, const std::function<glm::vec3* (RswSound*)>& getProp, float v_speed, float v_min, float v_max, bool moveTogether);
	template bool DragFloat3Multi<RswLight>(BrowEdit* browEdit, Map* map, const std::vector<RswLight*>& data, const char* label, const std::function<glm::vec3* (RswLight*)>& getProp, float v_speed, float v_min, float v_max, bool moveTogether);

	template<class T>
	bool DragFloat2Multi(BrowEdit* browEdit, Map* map, const std::vector<T*>& data, const char* label, const std::function<glm::vec2* (T*)>& getProp, float v_speed, float v_min, float v_max, bool moveTogether)
	{
		static std::vector<glm::vec2> startValues;
		glm::vec2* f = getProp(data.front());
		std::vector<const char*> formats;
		bool differentValues = false;
		for (int c = 0; c < 2; c++)
		{
			bool diff = !std::all_of(data.begin(), data.end(), [&](T* o) { return (*getProp(o))[c] == (*getProp(data.front()))[c]; });
			differentValues = differentValues || diff;
			formats.push_back(diff ? "multiple" : nullptr);
		}
		bool ret = DragScalarNMultiLabel(label, ImGuiDataType_Float, glm::value_ptr(*f), 2, v_speed, &v_min, &v_max, formats, 0);
		if (ret && moveTogether)
		{
			//TODO: not used
		}
		if (ImGui::IsItemActivated())
		{
			startValues.clear();
			for (auto o : data)
				startValues.push_back(*getProp(o));
		}
		if (ret)
		{
			for (auto o : data)
			{
				if (f->x != startValues[0].x)
					getProp(o)->x = f->x;
				if (f->y != startValues[0].y)
					getProp(o)->y = f->y;
			}
		}
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			auto ga = new GroupAction();
			for (auto i = 0; i < data.size(); i++)
				ga->addAction(new ObjectChangeAction(data[i]->node, getProp(data[i]), startValues[i], label));
			map->doAction(ga, browEdit);
		}
		if (!differentValues)
		{
			ImGui::PushID(label);
			if (ImGui::BeginPopupContextItem("CopyPaste"))
			{
				try {
					if (ImGui::MenuItem("Copy"))
					{
						json clipboard;
						to_json(clipboard, *getProp(data[0]));
						ImGui::SetClipboardText(clipboard.dump(1).c_str());
					}
					if (ImGui::MenuItem("Paste"))
					{
						auto cb = ImGui::GetClipboardText();
						if (cb)
						{
							startValues.clear();
							for (auto o : data)
							{
								startValues.push_back(*getProp(o));
								from_json(json::parse(std::string(cb)), *getProp(o));
							}
							auto ga = new GroupAction();
							for (auto i = 0; i < data.size(); i++)
								ga->addAction(new ObjectChangeAction(data[i]->node, getProp(data[i]), startValues[i], label));
							ret = true;
						}
					}
				}
				catch (...) {}
				ImGui::EndPopup();
			}
			ImGui::PopID();
		}

		return ret;
	}
	template bool DragFloat2Multi<RswObject>(BrowEdit* browEdit, Map* map, const std::vector<RswObject*>& data, const char* label, const std::function<glm::vec2* (RswObject*)>& getProp, float v_speed, float v_min, float v_max, bool moveTogether);
	template bool DragFloat2Multi<RswModel>(BrowEdit* browEdit, Map* map, const std::vector<RswModel*>& data, const char* label, const std::function<glm::vec2* (RswModel*)>& getProp, float v_speed, float v_min, float v_max, bool moveTogether);
	template bool DragFloat2Multi<RswEffect>(BrowEdit* browEdit, Map* map, const std::vector<RswEffect*>& data, const char* label, const std::function<glm::vec2* (RswEffect*)>& getProp, float v_speed, float v_min, float v_max, bool moveTogether);
	template bool DragFloat2Multi<LubEffect>(BrowEdit* browEdit, Map* map, const std::vector<LubEffect*>& data, const char* label, const std::function<glm::vec2* (LubEffect*)>& getProp, float v_speed, float v_min, float v_max, bool moveTogether);
	template bool DragFloat2Multi<RswSound>(BrowEdit* browEdit, Map* map, const std::vector<RswSound*>& data, const char* label, const std::function<glm::vec2* (RswSound*)>& getProp, float v_speed, float v_min, float v_max, bool moveTogether);
	template bool DragFloat2Multi<RswLight>(BrowEdit* browEdit, Map* map, const std::vector<RswLight*>& data, const char* label, const std::function<glm::vec2* (RswLight*)>& getProp, float v_speed, float v_min, float v_max, bool moveTogether);

	template<class T>
	bool ColorEdit3Multi(BrowEdit* browEdit, Map* map, const std::vector<T*>& data, const char* label, const std::function<glm::vec3* (T*)>& getProp)
	{
		static std::vector<glm::vec3> startValues;
		glm::vec3* f = getProp(data.front());
		std::vector<const char*> formats;
		bool differentValues = !std::all_of(data.begin(), data.end(), [&](T* o) { return (*getProp(o)) == (*getProp(data.front())); });
		bool ret = ImGui::ColorEdit3(label,glm::value_ptr(*f));
		if (ImGui::IsItemActivated())
		{
			startValues.clear();
			for (auto o : data)
				startValues.push_back(*getProp(o));
		}
		if (ret)
		{
			for (auto o : data)
				*getProp(o) = *f;
		}
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			auto ga = new GroupAction();
			for (auto i = 0; i < data.size(); i++)
				ga->addAction(new ObjectChangeAction(data[i]->node, getProp(data[i]), startValues[i], label));
			map->doAction(ga, browEdit);
		}
		if (!differentValues)
		{
			ImGui::PushID(label);
			if (ImGui::BeginPopupContextItem("CopyPaste"))
			{
				try {
					if (ImGui::MenuItem("Copy"))
					{
						json clipboard;
						to_json(clipboard, *getProp(data[0]));
						ImGui::SetClipboardText(clipboard.dump(1).c_str());
					}
					if (ImGui::MenuItem("Paste"))
					{
						auto cb = ImGui::GetClipboardText();
						if (cb)
						{
							startValues.clear();
							for (auto o : data)
							{
								startValues.push_back(*getProp(o));
								from_json(json::parse(std::string(cb)), *getProp(o));
							}
							auto ga = new GroupAction();
							for (auto i = 0; i < data.size(); i++)
								ga->addAction(new ObjectChangeAction(data[i]->node, getProp(data[i]), startValues[i], label));
							ret = true;
						}
					}
				}
				catch (...) {}
				ImGui::EndPopup();
			}
			ImGui::PopID();
		}
		return ret;
	}
	template bool ColorEdit3Multi<RswObject>(BrowEdit* browEdit, Map* map, const std::vector<RswObject*>& data, const char* label, const std::function<glm::vec3* (RswObject*)>& getProp);
	template bool ColorEdit3Multi<RswModel>(BrowEdit* browEdit, Map* map, const std::vector<RswModel*>& data, const char* label, const std::function<glm::vec3* (RswModel*)>& getProp);
	template bool ColorEdit3Multi<RswEffect>(BrowEdit* browEdit, Map* map, const std::vector<RswEffect*>& data, const char* label, const std::function<glm::vec3* (RswEffect*)>& getProp);
	template bool ColorEdit3Multi<LubEffect>(BrowEdit* browEdit, Map* map, const std::vector<LubEffect*>& data, const char* label, const std::function<glm::vec3* (LubEffect*)>& getProp);
	template bool ColorEdit3Multi<RswSound>(BrowEdit* browEdit, Map* map, const std::vector<RswSound*>& data, const char* label, const std::function<glm::vec3* (RswSound*)>& getProp);
	template bool ColorEdit3Multi<RswLight>(BrowEdit* browEdit, Map* map, const std::vector<RswLight*>& data, const char* label, const std::function<glm::vec3* (RswLight*)>& getProp);



	template<class T>
	bool ColorEdit4Multi(BrowEdit* browEdit, Map* map, const std::vector<T*>& data, const char* label, const std::function<glm::vec4* (T*)>& getProp)
	{
		static std::vector<glm::vec4> startValues;
		glm::vec4* f = getProp(data.front());
		std::vector<const char*> formats;
		bool differentValues = !std::all_of(data.begin(), data.end(), [&](T* o) { return (*getProp(o)) == (*getProp(data.front())); });
		bool ret = ImGui::ColorEdit4(label, glm::value_ptr(*f));
		if (ImGui::IsItemActivated())
		{
			startValues.clear();
			for (auto o : data)
				startValues.push_back(*getProp(o));
		}
		if (ret)
		{
			for (auto o : data)
				*getProp(o) = *f;
		}
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			auto ga = new GroupAction();
			for (auto i = 0; i < data.size(); i++)
				ga->addAction(new ObjectChangeAction(data[i]->node, getProp(data[i]), startValues[i], label));
			map->doAction(ga, browEdit);
		}
		if (!differentValues)
		{
			ImGui::PushID(label);
			if (ImGui::BeginPopupContextItem("CopyPaste"))
			{
				try {
					if (ImGui::MenuItem("Copy"))
					{
						json clipboard;
						to_json(clipboard, *getProp(data[0]));
						ImGui::SetClipboardText(clipboard.dump(1).c_str());
					}
					if (ImGui::MenuItem("Paste"))
					{
						auto cb = ImGui::GetClipboardText();
						if (cb)
						{
							startValues.clear();
							for (auto o : data)
							{
								startValues.push_back(*getProp(o));
								from_json(json::parse(std::string(cb)), *getProp(o));
							}
							auto ga = new GroupAction();
							for (auto i = 0; i < data.size(); i++)
								ga->addAction(new ObjectChangeAction(data[i]->node, getProp(data[i]), startValues[i], label));
							ret = true;
						}
					}
				}
				catch (...) {}
				ImGui::EndPopup();
			}
			ImGui::PopID();
		}
		return ret;
	}
	template bool ColorEdit4Multi<RswObject>(BrowEdit* browEdit, Map* map, const std::vector<RswObject*>& data, const char* label, const std::function<glm::vec4* (RswObject*)>& getProp);
	template bool ColorEdit4Multi<RswModel>(BrowEdit* browEdit, Map* map, const std::vector<RswModel*>& data, const char* label, const std::function<glm::vec4* (RswModel*)>& getProp);
	template bool ColorEdit4Multi<RswEffect>(BrowEdit* browEdit, Map* map, const std::vector<RswEffect*>& data, const char* label, const std::function<glm::vec4* (RswEffect*)>& getProp);
	template bool ColorEdit4Multi<LubEffect>(BrowEdit* browEdit, Map* map, const std::vector<LubEffect*>& data, const char* label, const std::function<glm::vec4* (LubEffect*)>& getProp);
	template bool ColorEdit4Multi<RswSound>(BrowEdit* browEdit, Map* map, const std::vector<RswSound*>& data, const char* label, const std::function<glm::vec4* (RswSound*)>& getProp);
	template bool ColorEdit4Multi<RswLight>(BrowEdit* browEdit, Map* map, const std::vector<RswLight*>& data, const char* label, const std::function<glm::vec4* (RswLight*)>& getProp);

	template<class T>
	bool CheckboxMulti(BrowEdit* browEdit, Map* map, const std::vector<T*>& data, const char* label, const std::function<bool* (T*)>& getProp)
	{
		static std::vector<bool> startValues;
		bool differentValues = !std::all_of(data.begin(), data.end(), [&](T* o) { return *getProp(o) == *getProp(data.front()); });
		bool* f = getProp(data.front());

		ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, differentValues);
		bool ret = ImGui::Checkbox(label, f);
		ImGui::PopItemFlag();
		if (ret)
			for (auto o : data)
				*getProp(o) = *f;
		if (ImGui::IsItemActivated())
		{
			startValues.clear();
			for (auto o : data)
				startValues.push_back(*getProp(o));
		}
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			auto ga = new GroupAction();
			for (auto i = 0; i < data.size(); i++)
				ga->addAction(new ObjectChangeAction<bool>(data[i]->node, getProp(data[i]), startValues[i], label));
			map->doAction(ga, browEdit);
		}
		if (!differentValues)
		{
			ImGui::PushID(label);
			if (ImGui::BeginPopupContextItem("CopyPaste"))
			{
				try {
					if (ImGui::MenuItem("Copy"))
					{
						ImGui::SetClipboardText(*getProp(data[0]) ? "true" : "false");
					}
					if (ImGui::MenuItem("Paste"))
					{
						auto cb = ImGui::GetClipboardText();
						if (cb)
						{
							startValues.clear();
							for (auto o : data)
							{
								startValues.push_back(*getProp(o));
								*getProp(o) = strcmp(cb, "true") == 0 ? true : false;
							}
							auto ga = new GroupAction();
							for (auto i = 0; i < data.size(); i++)
								ga->addAction(new ObjectChangeAction<bool>(data[i]->node, getProp(data[i]), startValues[i], label));
							ret = true;
						}
					}
				}
				catch (...) {}
				ImGui::EndPopup();
			}
			ImGui::PopID();
		}
		return ret;
	}
	template bool CheckboxMulti<RswObject>(BrowEdit* browEdit, Map* map, const std::vector<RswObject*>& data, const char* label, const std::function<bool* (RswObject*)>& getProp);
	template bool CheckboxMulti<RswLight>(BrowEdit* browEdit, Map* map, const std::vector<RswLight*>& data, const char* label, const std::function<bool* (RswLight*)>& getProp);
	template bool CheckboxMulti<RswEffect>(BrowEdit* browEdit, Map* map, const std::vector<RswEffect*>& data, const char* label, const std::function<bool* (RswEffect*)>& getProp);
	template bool CheckboxMulti<LubEffect>(BrowEdit* browEdit, Map* map, const std::vector<LubEffect*>& data, const char* label, const std::function<bool* (LubEffect*)>& getProp);
	template bool CheckboxMulti<RswSound>(BrowEdit* browEdit, Map* map, const std::vector<RswSound*>& data, const char* label, const std::function<bool* (RswSound*)>& getProp);
	template bool CheckboxMulti<RswModel>(BrowEdit* browEdit, Map* map, const std::vector<RswModel*>& data, const char* label, const std::function<bool* (RswModel*)>& getProp);



	template<class T>
	bool ComboBoxMulti(BrowEdit* browEdit, Map* map, const std::vector<T*>& data, const char* label, const char* items, const std::function<int* (T*)>& getProp)
	{
		static std::vector<bool> startValues;
		bool differentValues = !std::all_of(data.begin(), data.end(), [&](T* o) { return *getProp(o) == *getProp(data.front()); });
		int f = *getProp(data.front());
		char* itemsEdited = (char*)items;
		if (differentValues)
		{
			f = 0;
			const char* p = items;
			while (*p)
				p += strlen(p) + 1;
			auto len = (p - items)+1;
			itemsEdited = new char[len + 9];
			memcpy(itemsEdited, "Multiple\0", 9);
			memcpy(itemsEdited+9, items, len);
		}
		bool ret = ImGui::Combo(label, &f, (const char*)itemsEdited);
		if (differentValues)
			delete[] itemsEdited;
		if (ret)
			for (auto o : data)
				*getProp(o) = differentValues ? f-1 : f;
		if (ImGui::IsItemActivated())
		{
			startValues.clear();
			for (auto o : data)
				startValues.push_back(*getProp(o));
		}
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			auto ga = new GroupAction();
			for (auto i = 0; i < data.size(); i++)
				ga->addAction(new ObjectChangeAction<int>(data[i]->node, getProp(data[i]), startValues[i], label));
			map->doAction(ga, browEdit);
		}
		if (!differentValues)
		{
			ImGui::PushID(label);
			if (ImGui::BeginPopupContextItem("CopyPaste"))
			{
				try {
					if (ImGui::MenuItem("Copy"))
					{
						ImGui::SetClipboardText(std::to_string(*getProp(data[0])).c_str());
					}
					if (ImGui::MenuItem("Paste"))
					{
						auto cb = ImGui::GetClipboardText();
						if (cb)
						{
							startValues.clear();
							for (auto o : data)
							{
								startValues.push_back(*getProp(o));
								*getProp(o) = std::stoi(cb);
							}
							auto ga = new GroupAction();
							for (auto i = 0; i < data.size(); i++)
								ga->addAction(new ObjectChangeAction<int>(data[i]->node, getProp(data[i]), startValues[i], label));
							ret = true;
						}
					}
				}
				catch (...) {}
				ImGui::EndPopup();
			}
			ImGui::PopID();
		}
		return ret;
	}
	template bool ComboBoxMulti<RswObject>(BrowEdit* browEdit, Map* map, const std::vector<RswObject*>& data, const char* label, const char* items, const std::function<int* (RswObject*)>& getProp);
	template bool ComboBoxMulti<RswLight>(BrowEdit* browEdit, Map* map, const std::vector<RswLight*>& data, const char* label, const char* items, const std::function<int* (RswLight*)>& getProp);
	template bool ComboBoxMulti<RswEffect>(BrowEdit* browEdit, Map* map, const std::vector<RswEffect*>& data, const char* label, const char* items, const std::function<int* (RswEffect*)>& getProp);
	template bool ComboBoxMulti<LubEffect>(BrowEdit* browEdit, Map* map, const std::vector<LubEffect*>& data, const char* label, const char* items, const std::function<int* (LubEffect*)>& getProp);
	template bool ComboBoxMulti<RswSound>(BrowEdit* browEdit, Map* map, const std::vector<RswSound*>& data, const char* label, const char* items, const std::function<int* (RswSound*)>& getProp);
	template bool ComboBoxMulti<RswModel>(BrowEdit* browEdit, Map* map, const std::vector<RswModel*>& data, const char* label, const char* items, const std::function<int* (RswModel*)>& getProp);


	float interpolateSpline(const std::vector<glm::vec2>& data, float p)
	{
		auto n = data.size();
		std::vector<float> h, F,s;
		std::vector<std::vector<float>> m;
		h.resize(n, 0);
		F.resize(n, 0);
		s.resize(n, 0);
		m.resize(n);
		for (auto i = 0; i < n; i++)
			m[i].resize(n);

		for (auto i = n - 1; i > 0; i--)
		{
			F[i] = (data[i].y - data[i - 1].y) / (data[i].x - data[i - 1].x);
			h[i - 1] = data[i].x - data[i - 1].x;
		}

		//*********** formation of h, s , f matrix **************//
		for (auto i = 1; i < n - 1; i++)
		{
			m[i][i] = 2 * (h[i - 1] + h[i]);
			if (i != 1)
			{
				m[i][i - 1] = h[i - 1];
				m[i - 1][i] = h[i - 1];
			}
			m[i][n - 1] = 6 * (F[i + 1] - F[i]);
		}

		//***********  forward elimination **************//

		for (auto i = 1; i < n - 2; i++)
		{
			float temp = (m[i + 1][i] / m[i][i]);
			for (auto j = 1; j <= n - 1; j++)
				m[i + 1][j] -= temp * m[i][j];
		}
		float sum;
		//*********** back ward substitution *********//
		for (auto i = n - 2; i > 0; i--)
		{
			sum = 0;
			for (auto j = i; j <= n - 2; j++)
				sum += m[i][j] * s[j];
			s[i] = (m[i][n - 1] - sum) / m[i][i];
		}
		for (auto i = 0; i < n - 1; i++)
			if (data[i].x <= p && p <= data[i + 1].x)
			{
				auto a = (s[i + 1] - s[i]) / (6 * h[i]);
				auto b = s[i] / 2;
				auto c = (data[i + 1].y - data[i].y) / h[i] - (2 * h[i] * s[i] + s[i + 1] * h[i]) / 6;
				auto d = data[i].y;
				sum = a * glm::pow((p - data[i].x), 3.0f) + b * glm::pow((p - data[i].x), 2.0f) + c * (p - data[i].x) + d;
			}
		return sum;
	}

	float interpolateSpline(const std::vector<glm::vec3>& data, float p)
	{
		auto n = data.size();
		std::vector<float> h, F, s;
		std::vector<std::vector<float>> m;
		h.resize(n, 0);
		F.resize(n, 0);
		s.resize(n, 0);
		m.resize(n);
		for (auto i = 0; i < n; i++)
			m[i].resize(n);

		for (auto i = n - 1; i > 0; i--)
		{
			F[i] = (data[i].y - data[i - 1].y) / (data[i].x - data[i - 1].x);
			h[i - 1] = data[i].x - data[i - 1].x;
		}

		//*********** formation of h, s , f matrix **************//
		for (auto i = 1; i < n - 1; i++)
		{
			m[i][i] = 2 * (h[i - 1] + h[i]);
			if (i != 1)
			{
				m[i][i - 1] = h[i - 1];
				m[i - 1][i] = h[i - 1];
			}
			m[i][n - 1] = 6 * (F[i + 1] - F[i]);
		}

		//***********  forward elimination **************//

		for (auto i = 1; i < n - 2; i++)
		{
			float temp = (m[i + 1][i] / m[i][i]);
			for (auto j = 1; j <= n - 1; j++)
				m[i + 1][j] -= temp * m[i][j];
		}
		float sum;
		//*********** back ward substitution *********//
		for (auto i = n - 2; i > 0; i--)
		{
			sum = 0;
			for (auto j = i; j <= n - 2; j++)
				sum += m[i][j] * s[j];
			s[i] = (m[i][n - 1] - sum) / m[i][i];
		}
		for (auto i = 0; i < n - 1; i++)
			if (data[i].x <= p && p <= data[i + 1].x)
			{
				auto a = (s[i + 1] - s[i]) / (6 * h[i]);
				auto b = s[i] / 2;
				auto c = (data[i + 1].y - data[i].y) / h[i] - (2 * h[i] * s[i] + s[i + 1] * h[i]) / 6;
				auto d = data[i].y;
				sum = a * glm::pow((p - data[i].x), 3.0f) + b * glm::pow((p - data[i].x), 2.0f) + c * (p - data[i].x) + d;
			}
		return sum;
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
	float interpolateSCurve(float x) {
		// Alright, this isn't an S Curve formula anymore, but your typical light attenuation formula... but with the shape of an S Curve.
		x = 2 * x - 1;
		const float a = 1.5f;
		const float b = 1.5f;

		if (x <= -1)
			return 2;
		if (x >= 1)
			return 0;
		if (x <= 0)
			return -1.0f / (1.0f + a * -x + b * x * x) * (1.0f + x) + 2;

		return 1.0f / (1.0f + a * x + b * x * x) * (1.0f - x);
	}

	bool EditableGraph(const char* label, std::vector<glm::vec2>* points, std::function<float(const std::vector<glm::vec2>&, float)> interpolationStyle, bool& activated)
	{
		bool changed = false;
		auto window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;
		const ImGuiStyle& style = ImGui::GetStyle();
		const ImGuiIO& IO = ImGui::GetIO();
		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		
		int hovered = ImGui::IsItemActive() || ImGui::IsItemHovered(); // IsItemDragged() ?

		if(label != nullptr)
			ImGui::Text(label);

//		ImGui::Dummy(ImVec2(0, 3));
		// prepare canvas
		const float avail = ImGui::GetContentRegionAvailWidth();
		const float dim = ImMin(avail, 300.0f);
		ImVec2 Canvas(dim, dim);

		ImRect bb(window->DC.CursorPos, window->DC.CursorPos + Canvas);
		ImGui::ItemSize(bb);
		if (!ImGui::ItemAdd(bb, NULL))
			return false;


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
		static bool c = false;
		static bool dragged = false;
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
			if (ImGui::IsItemActivated())
				activated = true;
			if (ImGui::IsItemClicked(1))
			{
				it = points->erase(it); ImGui::PopID();
				changed = true;
				continue;
			}
			if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0))
			{
				if (i > 0 && i < points->size()-1)
					v.x += ImGui::GetIO().MouseDelta.x / Canvas.x;
				v.y -= ImGui::GetIO().MouseDelta.y / Canvas.y;
				v = glm::clamp(v, 0.0f, 1.0f);
				dragged = true;
			}
			else if (ImGui::IsMouseReleased(0) && dragged)
			{
				dragged = false;
				changed = true;
			}
			ImGui::PopID();
			it++;
			i++;
		}
		if(changed)
			std::sort(points->begin(), points->end(), [](const glm::vec2& a, const glm::vec2& b) { return std::signbit(a.x - b.x); });

		ImGui::SetCursorScreenPos(bb.Min);
		if (ImGui::InvisibleButton("new", bb.GetSize()))
		{
			ImVec2 pos = (IO.MousePos - bb.Min) / Canvas;
			pos.y = 1.0f - pos.y;
			points->push_back(glm::vec2(pos.x, pos.y));
			std::sort(points->begin(), points->end(), [](const glm::vec2& a, const glm::vec2& b){ return std::signbit(a.x - b.x);});
			changed = true;
		}
		ImGui::SetCursorScreenPos(cursorPos);
		return changed;
	}

	template<class T>
	bool EditableGraphMulti(BrowEdit* browEdit, Map* map, const std::vector<T*>& data, const char* label, const std::function<std::vector<glm::vec2>* (T*)>& getProp, std::function<float(const std::vector<glm::vec2>&, float)> interpolationStyle)
	{
		static std::vector<std::vector<glm::vec2>> startValues;
		bool differentValues = !std::all_of(data.begin(), data.end(), [&](T* o) { return *getProp(o) == *getProp(data.front()); });
		std::vector<glm::vec2>* f = getProp(data.front());
		if (differentValues)
		{
			ImGui::TextColored(ImVec4(1,0,0,1), "MULTIPLE VALUES");
			ImGui::SameLine();
		}
		ImGui::Text(label);
		bool ret = false;
		if (!differentValues)
		{
			ImGui::PushID(label);
			if (ImGui::BeginPopupContextItem("CopyPaste"))
			{
				try {
					if (ImGui::MenuItem("Copy"))
					{
						json values;
						to_json(values, *f);
						ImGui::SetClipboardText(values.dump(1).c_str());
					}
					if (ImGui::MenuItem("Paste"))
					{
						auto cb = ImGui::GetClipboardText();
						if (cb)
						{
							startValues.clear();
							for (auto o : data)
							{
								startValues.push_back(*getProp(o));
								from_json(json::parse(std::string(cb)), *getProp(o));
							}
							auto ga = new GroupAction();
							for (auto i = 0; i < data.size(); i++)
								ga->addAction(new ObjectChangeAction(data[i]->node, getProp(data[i]), startValues[i], label));
							ret = true;
						}
					}
				}
				catch (...) {}
				ImGui::EndPopup();
			}
			ImGui::PopID();
		}

		bool activated = false;
		ret = ret || EditableGraph(nullptr, f, interpolationStyle, activated);
		if (ret)
			for (auto o : data)
				*getProp(o) = *f;
		if (ImGui::IsItemActivated() || activated)
		{
			startValues.clear();
			for (auto o : data)
				startValues.push_back(*getProp(o));
		}
		if (ret)
		{
			auto ga = new GroupAction();
			for (auto i = 0; i < data.size(); i++)
			{
				if (data[i]->node == nullptr)
					throw "Error, data has no node";
				if (startValues.size() <= i)
					throw "Error, start values are not set properly...";
				ga->addAction(new ObjectChangeAction(data[i]->node, getProp(data[i]), startValues[i], label));
			}
			map->doAction(ga, browEdit);
		}
		return ret;
	}

	template bool EditableGraphMulti<RswLight>(BrowEdit* browEdit, Map* map, const std::vector<RswLight*>& data, const char* label, const std::function<std::vector<glm::vec2>* (RswLight*)>& callback, std::function<float(const std::vector<glm::vec2>&, float)> interpolationStyle);



	void Graph(const char* label, std::function<float(float)> func)
	{
		auto window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return;
		const ImGuiStyle& style = ImGui::GetStyle();
		const ImGuiIO& IO = ImGui::GetIO();
		ImDrawList* DrawList = ImGui::GetWindowDrawList();

		int hovered = ImGui::IsItemActive() || ImGui::IsItemHovered(); // IsItemDragged() ?

		ImGui::Text(label);
		//ImGui::Dummy(ImVec2(0, 3));

		// prepare canvas
		const float avail = ImGui::GetContentRegionAvailWidth();
		const float dim = ImMin(avail, 300.0f);
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

	std::string SelectFileDialog(std::string defaultFilename, const char* filter)
	{
		CoInitializeEx(0, 0);
		char curdir[100];
		_getcwd(curdir, 100);

		char buf[256];
		ZeroMemory(&buf, sizeof(buf));
		strcpy_s(buf, 256, defaultFilename.c_str());

		OPENFILENAME ofn;
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = glfwGetWin32Window(browEdit->window);
		ofn.lpstrFile = buf;
		ofn.nMaxFile = 1024;
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 2;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT;
		if (GetOpenFileName(&ofn))
		{
			_chdir(curdir);
			return buf;
		}
		_chdir(curdir);
		return "";
	}

	std::string SaveAsDialog(const std::string& fileNameStr, const char* filter)
	{
		CoInitializeEx(0, 0);

		char fileName[1024];
		strcpy_s(fileName, 1024, fileNameStr.c_str());
		
		char curdir[100];
		_getcwd(curdir, 100);

		OPENFILENAME ofn;
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = glfwGetWin32Window(browEdit->window);
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

	int CALLBACK BrowseCallBackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
	{
		switch (uMsg)
		{
		case BFFM_INITIALIZED:
			::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
			break;
		}
		return 0;
	}
	std::string SelectPathDialog(std::string path, std::string title)
	{
		CoInitializeEx(0, 0);
		CHAR szDir[MAX_PATH];
		BROWSEINFO bInfo;
		bInfo.hwndOwner = glfwGetWin32Window(browEdit->window);
		bInfo.pidlRoot = NULL;
		bInfo.pszDisplayName = szDir;
		bInfo.lpszTitle = title.c_str();
		bInfo.ulFlags = 0;
		bInfo.lpfn = BrowseCallBackProc;
		if (path.find(":") == std::string::npos)
			path = std::filesystem::current_path().string() + "\\" + path; //TODO: fix this

		bInfo.lParam = (LPARAM)path.c_str();
		bInfo.iImage = -1;

		LPITEMIDLIST lpItem = SHBrowseForFolder(&bInfo);
		if (lpItem != NULL)
		{
			SHGetPathFromIDList(lpItem, szDir);
			std::string retpath = szDir;
			if (retpath[retpath.size() - 1] != '\\')
				retpath += "\\";
			return retpath;
		}
		return "";
	}

	glm::quat RotationBetweenVectors(glm::vec3 start, glm::vec3 dest) {
		start = glm::normalize(start);
		dest = glm::normalize(dest);

		float cosTheta = dot(start, dest);
		glm::vec3 rotationAxis;

		if (cosTheta < -1 + 0.001f) {
			// special case when vectors in opposite directions:
			// there is no "ideal" rotation axis
			// So guess one; any will do as long as it's perpendicular to start
			rotationAxis = glm::cross(glm::vec3(0.0f, 0.0f, 1.0f), start);
			if (glm::length2(rotationAxis) < 0.01) // bad luck, they were parallel, try again!
				rotationAxis = glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), start);

			rotationAxis = normalize(rotationAxis);
			return glm::angleAxis(glm::radians(180.0f), rotationAxis);
		}

		rotationAxis = glm::cross(start, dest);

		float s = sqrt((1 + cosTheta) * 2);
		float invs = 1 / s;

		return glm::quat(
			s * 0.5f,
			rotationAxis.x * invs,
			rotationAxis.y * invs,
			rotationAxis.z * invs
		);

	}

	glm::quat RotateTowards(glm::quat q1, const glm::quat &q2, float maxAngle) {

		if (maxAngle < 0.001f) {
			// No rotation allowed. Prevent dividing by 0 later.
			return q1;
		}

		float cosTheta = dot(q1, q2);

		// q1 and q2 are already equal.
		// Force q2 just to be sure
		if (cosTheta > 0.9999f) {
			return q2;
		}

		// Avoid taking the long path around the sphere
		if (cosTheta < 0) {
			q1 = q1 * -1.0f;
			cosTheta *= -1.0f;
		}

		float angle = acos(cosTheta);

		// If there is only a 2&deg; difference, and we are allowed 5&deg;,
		// then we arrived.
		if (angle < maxAngle) {
			return q2;
		}

		float fT = maxAngle / angle;
		angle = maxAngle;

		glm::quat res = (sin((1.0f - fT) * angle) * q1 + sin(fT * angle) * q2) / sin(angle);
		res = glm::normalize(res);
		return res;

	}

	std::string callstack()
	{
#ifdef WIN32
//#ifdef _DEBUG
#if 1
		if (IsDebuggerPresent())
			return "";
		std::string ret;
		unsigned int   i;
		void* stack[100];
		unsigned short frames;
		SYMBOL_INFO* symbol;
		IMAGEHLP_LINE line;
		HANDLE         process;

		process = GetCurrentProcess();

		SymInitialize(process, NULL, TRUE);

		frames = CaptureStackBackTrace(0, 100, stack, NULL);
		symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 128 * sizeof(char), 1);
		symbol->MaxNameLen = 127;
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

		ZeroMemory(&line, sizeof(IMAGEHLP_LINE));
		line.SizeOfStruct = sizeof(IMAGEHLP_LINE);
		DWORD dwDisplacement;

		for (i = 0; i < frames; i++)
		{
			SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
			SymGetLineFromAddr(process, (DWORD64)(stack[i]), &dwDisplacement, &line);
			char buf[2048];
			sprintf_s(buf, 2048, "%i: (%s:%i)\t%s\n", frames - i - 1, line.FileName, line.LineNumber, symbol->Name);

			ret += buf;
		}
		SymCleanup(process);
		free(symbol);

		return ret;
#else //no debug
		return "";
#endif
#elif !defined(BLIB_ANDROID) && !defined(BLIB_NX)
		std::string ret;
		void* callstack[128];
		int frames = backtrace(callstack, 128);
		char** strs = backtrace_symbols(callstack, frames);
		for (int i = 0; i < frames; i++)
		{
			ret += std::string(strs[i]) + "\n";
		}
		free(strs);
		return ret;
#endif
		return "";
	}

}



namespace glm
{
	void to_json(nlohmann::json& j, const glm::vec4& v) {
		j = nlohmann::json{ v.x, v.y, v.z, v.w };
	}
	void from_json(const nlohmann::json& j, glm::vec4& v) {
		j[0].get_to(v.x);
		j[1].get_to(v.y);
		j[2].get_to(v.z);
		j[3].get_to(v.w);
	}
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
	void to_json(nlohmann::json& j, const glm::ivec4& v) {
		j = nlohmann::json{ v.x, v.y, v.z, v.w };
	}
	void from_json(const nlohmann::json& j, glm::ivec4& v) {
		j[0].get_to(v.x);
		j[1].get_to(v.y);
		j[2].get_to(v.z);
		j[3].get_to(v.w);
	}

	void to_json(nlohmann::json& j, const glm::ivec2& v) {
		j = nlohmann::json{ v.x, v.y };
	}
	void from_json(const nlohmann::json& j, glm::ivec2& v) {
		j[0].get_to(v.x);
		j[1].get_to(v.y);
	}
	void to_json(nlohmann::json& j, const glm::lowp_i8vec4& v) {
		j = nlohmann::json{ v.x, v.y, v.z, v.w };
	}
	void from_json(const nlohmann::json& j, glm::lowp_i8vec4& v) {
		j[0].get_to(v.x);
		j[1].get_to(v.y);
		j[2].get_to(v.z);
		j[3].get_to(v.w);
	}
	void to_json(nlohmann::json& j, const glm::quat& v) {
		j = nlohmann::json{ v.x, v.y, v.z, v.w };
	}
	void from_json(const nlohmann::json& j, glm::quat& v) {
		j[0].get_to(v.x);
		j[1].get_to(v.y);
		j[2].get_to(v.z);
		j[3].get_to(v.w);
	}
}



void to_json(nlohmann::json& j, const ImVec4& v) {
	j = nlohmann::json{ v.x, v.y, v.z, v.w };
}
void from_json(const nlohmann::json& j, ImVec4& v) {
	j[0].get_to(v.x);
	j[1].get_to(v.y);
	j[2].get_to(v.z);
	j[3].get_to(v.w);
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
		if(!j.is_null())
			v.clear();
		int i = 0;
		for (const auto& jj : j)
		{
			glm::vec2 vv;
			glm::from_json(jj, vv);
			v.push_back(vv);
		}
	}
}