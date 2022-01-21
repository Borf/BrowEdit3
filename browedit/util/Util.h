#pragma once

class BrowEdit;
class Node;
#include <glm/glm.hpp>
#include <string>

namespace util
{
	short swapShort(const short s);


	bool DragFloat3(BrowEdit* browEdit, Node* node, const char* label, glm::vec3 *ptr, float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const std::string& action = "");
}