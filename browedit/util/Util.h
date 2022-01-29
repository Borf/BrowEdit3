#pragma once

class BrowEdit;
class Map;
class Node;
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <imgui.h>

namespace util
{
	short swapShort(const short s);
	std::string iso_8859_1_to_utf8(const std::string& str);
	std::string utf8_to_iso_8859_1(const std::string& str);


	std::vector<std::string> split(std::string value, const std::string &seperator);
	std::string combine(const std::vector<std::string> &items, const std::string& seperator);

	std::string SaveAsDialog(const std::string& fileName, const char* filter = "All\0*.*\0");

	bool ColorEdit3(BrowEdit* browEdit, Map* map, Node* node, const char* label, glm::vec3* ptr, const std::string& action = "");
	bool DragInt(BrowEdit* browEdit, Map* map, Node* node, const char* label, int* ptr, float v_speed = 1, int v_min = 0.0f, int v_max = 0.0f, const std::string& action = "");
	bool DragFloat(BrowEdit* browEdit, Map* map, Node* node, const char* label, float* ptr, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const std::string& action = "");
	bool DragFloat3(BrowEdit* browEdit, Map* map, Node* node, const char* label, glm::vec3* ptr, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const std::string& action = "");
	bool InputText(BrowEdit* browEdit, Map* map, Node* node, const char* label, std::string* ptr, ImGuiInputTextFlags flags = 0, const std::string& action = "");
}