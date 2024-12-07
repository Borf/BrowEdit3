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

	float wrap360(float value);

	std::string& tolowerInPlace(std::string& str);
	std::string tolower(std::string str);

	std::vector<std::string> split(std::string value, const std::string &seperator);
	std::string combine(const std::vector<std::string> &items, const std::string& seperator);
	std::string replace(std::string orig, const std::string& find, const std::string& replace);
	static inline std::string ltrim(std::string s) {
		s.erase(0, s.find_first_not_of(" \n\r\t"));
		return s;
	}

	// trim from end
	static inline std::string rtrim(std::string s) {
		s.erase(s.find_last_not_of(" \n\r\t") + 1);
		return s;
	}

	// trim from both ends
	static inline std::string trim(std::string s) {
		return ltrim(rtrim(s));
	}

	std::string SaveAsDialog(const std::string& fileName, const char* filter = "All\0*.*\0");
	std::string SelectFileDialog(std::string defaultFilename, const char* filter = "All\0*.*\0");
	std::string SelectPathDialog(std::string path);

	bool ColorEdit3(BrowEdit* browEdit, Map* map, Node* node, const char* label, glm::vec3* ptr, const std::string& action = "");
	bool ColorEdit4(BrowEdit* browEdit, Map* map, Node* node, const char* label, glm::vec4* ptr, const std::string& action = "");
	bool DragInt(BrowEdit* browEdit, Map* map, Node* node, const char* label, int* ptr, float v_speed = 1, int v_min = 0.0f, int v_max = 0.0f, const std::string& action = "", const std::function<void(int*, int)>& editAction = nullptr);
	bool DragFloat(BrowEdit* browEdit, Map* map, Node* node, const char* label, float* ptr, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const std::string& action = "");
	bool DragFloat2(BrowEdit* browEdit, Map* map, Node* node, const char* label, glm::vec2* ptr, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const std::string& action = "");
	bool DragFloat3(BrowEdit* browEdit, Map* map, Node* node, const char* label, glm::vec3* ptr, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const std::string& action = "", bool moveTogether = false);
	bool InputText(BrowEdit* browEdit, Map* map, Node* node, const char* label, std::string* ptr, ImGuiInputTextFlags flags = 0, const std::string& action = "");
	bool Checkbox(BrowEdit* browEdit, Map* map, Node* node, const char* label, bool* ptr, const std::string& action = "");

	template<class T>
	bool InputTextMulti(BrowEdit* browEdit, Map* map, const std::vector<T*>& data, const char* label, const std::function<std::string* (T*)>& getProp, const std::function<void(Node* node, std::string* ptr, std::string* startValue, const std::string& action)>& editAction);

	template<class T>
	bool InputTextMulti(BrowEdit* browEdit, Map* map, const std::vector<T*>& data, const char* label, const std::function<std::string* (T*)>& getProp);

	template<class T>
	bool DragIntMulti(BrowEdit* browEdit, Map* map, const std::vector<T*>& data, const char* label, const std::function<int* (T*)>& callback, int v_speed, int v_min, int v_max);

	template<class T>
	bool DragCharMulti(BrowEdit* browEdit, Map* map, const std::vector<T*>& data, const char* label, const std::function<char* (T*)>& callback, int v_speed, int v_min, int v_max);

	template<class T>
	bool ColorEdit3Multi(BrowEdit* browEdit, Map* map, const std::vector<T*>& data, const char* label, const std::function<glm::vec3* (T*)>& callback);

	template<class T>
	bool ColorEdit4Multi(BrowEdit* browEdit, Map* map, const std::vector<T*>& data, const char* label, const std::function<glm::vec4* (T*)>& callback);

	template<class T>
	bool DragFloatMulti(BrowEdit* browEdit, Map* map, const std::vector<T*>& data, const char* label, const std::function<float* (T*)>& callback, float v_speed, float v_min, float v_max);

	template<class T>
	bool DragFloat2Multi(BrowEdit* browEdit, Map* map, const std::vector<T*>& data, const char* label, const std::function<glm::vec2* (T*)>& callback, float v_speed, float v_min, float v_max, bool moveTogether = false);

	template<class T>
	bool DragFloat3Multi(BrowEdit* browEdit, Map* map, const std::vector<T*>& data, const char* label, const std::function<glm::vec3* (T*)>& callback, float v_speed, float v_min, float v_max, bool moveTogether = false);

	template<class T>
	bool CheckboxMulti(BrowEdit* browEdit, Map* map, const std::vector<T*>& data, const char* label, const std::function<bool* (T*)>& callback);

	template<class T>
	bool ComboBoxMulti(BrowEdit* browEdit, Map* map, const std::vector<T*>& data, const char* label, const char* items, const std::function<int* (T*)>& callback);

	template<class T>
	bool EditableGraphMulti(BrowEdit* browEdit, Map* map, const std::vector<T*>& data, const char* label, const std::function<std::vector<glm::vec2>* (T*)>& callback, std::function<float(const std::vector<glm::vec2>&, float)> interpolationStyle);


	float interpolateLagrange(const std::vector<glm::vec2>& f, float x);
	float interpolateSpline(const std::vector<glm::vec2>& f, float x);
	float interpolateLinear(const std::vector<glm::vec2>& f, float x);
	float interpolateSCurve(float x);
	bool EditableGraph(const char* label, std::vector<glm::vec2>* points, std::function<float(const std::vector<glm::vec2>&, float)> interpolationStyle, bool& activated);
	void Graph(const char* label, std::function<float(float)> func);


	//todo: move to math
	glm::quat RotationBetweenVectors(glm::vec3 start, glm::vec3 dest);
	glm::quat RotateTowards(glm::quat q1, const glm::quat &q2, float maxAngle);


	std::string callstack();
}

namespace glm
{
	void to_json(nlohmann::json& j, const glm::vec4& v);
	void from_json(const nlohmann::json& j, glm::vec4& v);
	void to_json(nlohmann::json& j, const glm::vec3& v);
	void from_json(const nlohmann::json& j, glm::vec3& v);
	void to_json(nlohmann::json& j, const glm::vec2& v);
	void from_json(const nlohmann::json& j, glm::vec2& v);

	void to_json(nlohmann::json& j, const glm::lowp_i8vec4& v);
	void from_json(const nlohmann::json& j, glm::lowp_i8vec4& v);

	void to_json(nlohmann::json& j, const glm::ivec4& v);
	void from_json(const nlohmann::json& j, glm::ivec4& v);
	void to_json(nlohmann::json& j, const glm::ivec2& v);
	void from_json(const nlohmann::json& j, glm::ivec2& v);

	void to_json(nlohmann::json& j, const glm::quat& v);
	void from_json(const nlohmann::json& j, glm::quat& v);

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