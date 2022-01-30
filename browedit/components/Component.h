#pragma once
#include <json.hpp>

class Node;

class Component
{
public:
	Node* node = nullptr;
	virtual ~Component()
	{
	}
};
void to_json(nlohmann::json& j, const Component& c);