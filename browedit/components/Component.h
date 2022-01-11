#pragma once

class Node;

class Component
{
public:
	Node* node = nullptr;
	virtual ~Component()
	{
	}
};