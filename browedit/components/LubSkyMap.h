#pragma once

#include "Component.h"
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>
#include <sol.hpp>

#include <glad/gl.h>
#include <glm/glm.hpp>

class BrowEdit;

class LubSkyMap : public Component
{
public:
	class CloudEffect
	{
	public:
		// Lub settings from mapskydata.lub
		int Num = 1000;
		float NumPerSquared = 1000;
		int CullDist = 400;
		glm::vec3 Color = glm::vec3(1.0f);
		float Size = 20.0f;
		float Size_Extra = 20.0f;
		float Expand_Rate = 0.05f;
		float Alpha_Inc_Time = 80.0f;
		float Alpha_Inc_Time_Extra = 200.0f;
		float Alpha_Inc_Speed = 1.0f;
		float Alpha_Dec_Time = 300.0f;
		float Alpha_Dec_Time_Extra = 200.0f;
		float Alpha_Dec_Speed = 0.5f;
		float Height = 40.0f;
		float Height_Extra = 10.0f;

		// Hidden lub settings (used mostly by star effect or old cloud effect
		int dirMode = 0;
		glm::vec4 forcedDir = glm::vec4(0.0f);
		float scaleX = 1.0f;
		float scaleY = 1.0f;
		float uvCycleSpeed = 0.0f;

		// Only used by Old Cloud type 11
		std::vector<const LubSkyMap::CloudEffect*> subClouds;

		// Only used by Old Cloud 3
		bool snapToGround = false;

		// Only modified by Old Clouds
		int blendSrc = GL_SRC_ALPHA;
		int blendDst = GL_ONE_MINUS_SRC_ALPHA;

		int oldCloudEffect = 0;
		bool useStarTextures = false;
		bool useFogTextures = false;

		bool isValid = true;

		CloudEffect();
		CloudEffect(LubSkyMap::CloudEffect* cloud);
	};
private:
	std::string createSkyMapTable(std::string mapIdentifier);
	size_t findMatchingTableEnd(const std::string& data, size_t openBrace);
public:
	LubSkyMap();
	~LubSkyMap();

	glm::vec3 BG_Color = glm::vec3(0.0f);
	bool Star_Effect = false;
	bool BG_Fog = true;

	std::vector<LubSkyMap::CloudEffect*> clouds;
	std::vector<int> oldClouds;

	bool isEnabled = false;

	const std::string mapskyFileName = "data\\luafiles514\\lua files\\mapskydata\\mapskydata.lub";

	bool preload(const std::string& mapName, sol::state& lua);
	bool load(const std::string& mapName);
	bool save(const std::string& mapName, BrowEdit* browEdit);

	const LubSkyMap::CloudEffect* getStarEffectTemplate();
	const LubSkyMap::CloudEffect* getTemplate(int id);
};