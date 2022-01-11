#pragma once

#include <vector>
#include <string>
#include <functional>

class Component;
class Transform;

class Node
{
public:
	std::vector<Component*> components;
	std::vector<Node*> children;
	Transform* transform = nullptr;
	Node* parent = nullptr;
	std::string name;

	Node(const std::string& name = "", Node* parent = nullptr);
	void addComponent(Component* component);
	
	template<class T>
	T* getComponent()
	{
		for (auto c : components)
		{
			T* cc = dynamic_cast<T*>(c);
			if (cc)
				return cc;
		}
		return nullptr;
	}
	void traverse(const std::function<void(Node*)>& callBack);
};