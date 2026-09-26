#pragma once

#include "Action.h"
#include <browedit/components/Rsw.h>
#include <browedit/components/LubSkyMap.h>
#include <browedit/components/SkyMapRenderer.h>
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <vector>

class SkyMapOldCloudListChangedAction : public Action
{
	std::vector<int> oldValues;
	std::vector<int> newValues;
public:
	SkyMapOldCloudListChangedAction(const std::vector<int>& oldValues, const std::vector<int>& newValues)
	{
		this->oldValues = oldValues;
		this->newValues = newValues;
	}

	virtual void perform(Map* map, BrowEdit* browEdit)
	{
		auto skyMapRenderer = map->rootNode->getComponent<SkyMapRenderer>();
		auto lubSkyMap = map->rootNode->getComponent<LubSkyMap>();

		if (skyMapRenderer)
			skyMapRenderer->setDirty();

		if (lubSkyMap) {
			lubSkyMap->oldClouds.clear();

			for (auto value : newValues)
				lubSkyMap->oldClouds.push_back(value);
		}
	}
	virtual void undo(Map* map, BrowEdit* browEdit)
	{
		auto skyMapRenderer = map->rootNode->getComponent<SkyMapRenderer>();
		auto lubSkyMap = map->rootNode->getComponent<LubSkyMap>();

		if (skyMapRenderer)
			skyMapRenderer->setDirty();

		if (lubSkyMap) {
			lubSkyMap->oldClouds.clear();

			for (auto value : oldValues)
				lubSkyMap->oldClouds.push_back(value);
		}
	}
	virtual std::string str()
	{
		return "Modified old clouds";
	};
};

class AddCustomCloudAction : public Action
{
	LubSkyMap::CloudEffect* newValue;
public:
	AddCustomCloudAction(LubSkyMap::CloudEffect* newValue)
	{
		this->newValue = newValue;
	}

	~AddCustomCloudAction() {
		delete newValue;
	}

	virtual void perform(Map* map, BrowEdit* browEdit)
	{
		auto skyMapRenderer = map->rootNode->getComponent<SkyMapRenderer>();
		auto lubSkyMap = map->rootNode->getComponent<LubSkyMap>();

		if (skyMapRenderer)
			skyMapRenderer->setDirty();

		if (lubSkyMap) {
			lubSkyMap->clouds.push_back(newValue);
		}
	}
	virtual void undo(Map* map, BrowEdit* browEdit)
	{
		auto skyMapRenderer = map->rootNode->getComponent<SkyMapRenderer>();
		auto lubSkyMap = map->rootNode->getComponent<LubSkyMap>();

		if (skyMapRenderer)
			skyMapRenderer->setDirty();

		if (lubSkyMap) {
			lubSkyMap->clouds.pop_back();
		}
	}
	virtual std::string str()
	{
		return "Added custom clouds";
	};
};

class SkyMapCustomCloudListChangedAction : public Action
{
	std::vector<LubSkyMap::CloudEffect*> oldValues;
	std::vector<LubSkyMap::CloudEffect*> newValues;
public:
	SkyMapCustomCloudListChangedAction(const std::vector<LubSkyMap::CloudEffect*>& oldValues, const std::vector<LubSkyMap::CloudEffect*>& newValues)
	{
		this->oldValues = oldValues;
		this->newValues = newValues;
	}

	virtual void perform(Map* map, BrowEdit* browEdit)
	{
		auto skyMapRenderer = map->rootNode->getComponent<SkyMapRenderer>();
		auto lubSkyMap = map->rootNode->getComponent<LubSkyMap>();

		if (skyMapRenderer)
			skyMapRenderer->setDirty();

		if (lubSkyMap) {
			lubSkyMap->clouds.clear();

			for (auto& value : newValues)
				lubSkyMap->clouds.push_back(value);
		}
	}
	virtual void undo(Map* map, BrowEdit* browEdit)
	{
		auto skyMapRenderer = map->rootNode->getComponent<SkyMapRenderer>();
		auto lubSkyMap = map->rootNode->getComponent<LubSkyMap>();

		if (skyMapRenderer)
			skyMapRenderer->setDirty();

		if (lubSkyMap) {
			lubSkyMap->clouds.clear();

			for (auto& value : oldValues)
				lubSkyMap->clouds.push_back(value);
		}
	}
	virtual std::string str()
	{
		return "Modified old clouds";
	};
};