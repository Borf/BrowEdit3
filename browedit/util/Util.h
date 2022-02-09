#pragma once

class BrowEdit;
class Map;
class Node;
#include <json.hpp>
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
	bool Checkbox(BrowEdit* browEdit, Map* map, Node* node, const char* label, bool* ptr, const std::string& action = "");


	float interpolateLagrange(const std::vector<glm::vec2>& f, float x);
	float interpolateSpline(const std::vector<glm::vec2>& f, float x);
	float interpolateLinear(const std::vector<glm::vec2>& f, float x);
	void EditableGraph(const char* label, std::vector<glm::vec2>* points, std::function<float(const std::vector<glm::vec2>&, float)> interpolationStyle);
	void Graph(const char* label, std::function<float(float)> func);
}

namespace glm
{
	void to_json(nlohmann::json& j, const glm::vec3& v);
	void from_json(const nlohmann::json& j, glm::vec3& v);
	void to_json(nlohmann::json& j, const glm::vec2& v);
	void from_json(const nlohmann::json& j, glm::vec2& v);
}

void to_json(nlohmann::json& j, const ImVec4& v);
void from_json(const nlohmann::json& j, ImVec4& v);
void to_json(nlohmann::json& j, const ImVec2& v);
void from_json(const nlohmann::json& j, ImVec2& v);

namespace std
{
	void to_json(nlohmann::json& j, const std::vector<glm::vec2>& v);
	void from_json(const nlohmann::json& j, std::vector<glm::vec2>& v);
}