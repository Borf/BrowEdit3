#pragma once

#include "Component.h"
#include <browedit/util/Util.h>
#include <browedit/components/ImguiProps.h>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <browedit/math/Ray.h>
#include <json.hpp>

class Map;
class BrowEdit;

class Gat : public Component, public ImguiProps
{
public:
	Gat(const std::string& fileName);
	Gat(int width, int height);
	~Gat();
	void save(const std::string &fileName);
	glm::vec3 rayCast(const math::Ray& ray, int xMin = 0, int yMin = 0, int xMax = -1, int yMax = -1, float offset = 0.0f);
	void buildImGui(BrowEdit* browEdit);

	void flattenTiles(Map* map, BrowEdit* browEdit, const std::vector<glm::ivec2>& tiles);
	void smoothTiles(Map* map, BrowEdit* browEdit, const std::vector<glm::ivec2>& tiles, int axis);
	void addRandomHeight(Map* map, BrowEdit* browEdit, const std::vector<glm::ivec2> &tiles, float min, float max);
	void connectHigh(Map* map, BrowEdit* browEdit, const std::vector<glm::ivec2>& tiles);
	void connectLow(Map* map, BrowEdit* browEdit, const std::vector<glm::ivec2>& tiles);

	glm::vec3 getPos(int cubeX, int cubeY, int index, float fac = 0);
	inline bool inMap(const glm::ivec2& t) { return t.x >= 0 && t.x < width&& t.y >= 0 && t.y < height; }

	std::vector<glm::vec3> getMapQuads();

	class Cube
	{
	public:
		Cube()
		{
			selected = false;
		}
		Cube(Cube* other)
		{
			for (int i = 0; i < 4; i++)
				this->heights[i] = other->heights[i];
			this->normal = other->normal;
		}
		union
		{
			struct
			{
				float h1, h2, h3, h4;
			};
			float heights[4] = {0,0,0,0};
		};
		int gatType;
		bool selected;
		glm::vec3 normal;

		bool sameHeight(const Cube& other) const;
		void calcNormal();

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Cube, h1, h2, h3, h4, gatType);
	};


	short version;
	int width;
	int height;
	std::vector<std::vector<Cube*> > cubes;

	static inline 	glm::ivec3 connectInfo[4][4] = {
		{	glm::ivec3(0,0, 0),			glm::ivec3(-1,0, 1),		glm::ivec3(0,-1, 2),		glm::ivec3(-1,-1, 3),	},
		{	glm::ivec3(0,0, 1),			glm::ivec3(1,0, 0),			glm::ivec3(0,-1, 3),		glm::ivec3(1,-1, 2),	},
		{	glm::ivec3(0,0, 2),			glm::ivec3(-1,0, 3),		glm::ivec3(0,1, 0),			glm::ivec3(-1,1, 1),	},
		{	glm::ivec3(0,0, 3),			glm::ivec3(1,0, 2),			glm::ivec3(0,1, 1),			glm::ivec3(1,1, 0),		}
	};

};