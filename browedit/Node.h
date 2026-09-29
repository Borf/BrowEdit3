#pragma once

#include <json.hpp>
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>
#include <typeindex>
#include <glm/glm.hpp>

namespace math { class Ray; }
class Component;
class Map;

class Node
{
public:
	bool dirty = true;
	std::vector<Component*> components;
	std::vector<Node*> children;
	Node* parent = nullptr;
	Node* root = this;
	std::string name;

	Node(const std::string& name = "", Node* parent = nullptr);
	~Node();

private:
	// Magical cache to speed up searches to O(1) instead of O(n)
	mutable std::unordered_map<std::type_index, std::vector<Component*>> lookupCache;
public:
	void addComponent(Component* component);
	bool exists(Component* component);
	void setParent(Node* newParent);
	void removeChild(Node* child);

	void makeNameUnique(Node* rootNode);

	void onRename(Map* map);
	void addComponentsFromJson(const nlohmann::json& data);
	
	template<class T>
	T* getComponent()
	{
		const auto& vec = getComponents<T>();
		return vec.empty() ? nullptr : vec.front();
	}

	template<class T>
	const std::vector<T*>& getComponents() {
		std::type_index typeKey = std::type_index(typeid(T));

		// If already exists, skip search
		auto it = lookupCache.find(typeKey);
		if (it != lookupCache.end()) {
			return reinterpret_cast<const std::vector<T*>&>(it->second);
		}

		// If not found, cache the result
		std::vector<Component*>& cachedVec = lookupCache[typeKey];
		for (auto* c : components) {
			if (auto* casted = dynamic_cast<T*>(c)) {
				cachedVec.push_back(c);
			}
		}

		return reinterpret_cast<const std::vector<T*>&>(cachedVec);
	}

	template<class T>
	void addComponents(const std::vector<T*> &lst)
	{
		for (auto c : lst)
			addComponent(c);
	}

	template<class T>
	std::vector<T*> removeComponent()
	{
		std::vector<T*> ret;
		for(auto it = components.begin(); it != components.end(); )
		{
			T* cc = dynamic_cast<T*>(*it);
			if (cc)
			{
				ret.push_back(cc);
				it = components.erase(it);
			}
			else
				it++;
		}
		lookupCache.clear();
		return ret;
	}

	void removeComponent(Component* component)
	{
		for (auto it = components.begin(); it != components.end(); )
		{
			if (*it == component)
				it = components.erase(it);
			else
				it++;
		}
		lookupCache.clear();
	}

	void traverse(const std::function<void(Node*)>& callBack);

	//TODO: move this somewhere else?
	std::vector<std::pair<Node*, std::vector<glm::vec3>>> getCollisions(const math::Ray& ray);
};


void to_json(nlohmann::json& j, const Node& n);
void from_json(const nlohmann::json& j, Node& p);