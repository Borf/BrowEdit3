#pragma once

#include "Component.h"
#include <browedit/util/Util.h>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <browedit/math/Ray.h>
#include <json.hpp>

class Map;
class BrowEdit;

namespace gl {
	class Texture;
}

class Gnd : public Component
{
public:
	Gnd(const std::string& fileName);
	~Gnd();
	void save(const std::string &fileName);
	glm::vec3 rayCast(const math::Ray& ray, bool emptyTiles = false, int xMin = 0, int yMin = 0, int xMax = -1, int yMax = -1, float offset = 0.0f);
	void makeLightmapsUnique();
	void makeLightmapsClear();
	void makeLightmapBorders();
	void makeLightmapBorders(int x, int y);
	int getLightmapBrightness(int x, int y, int lightmapX, int lightmapY);
	glm::ivec3 getLightmapColor(int x, int y, int lightmapX, int lightmapY);
	void makeLightmapsSmooth();
	void makeTilesUnique();
	void cleanLightmaps();
	void cleanTiles();
	void recalculateNormals();

	void flattenTiles(Map* map, BrowEdit* browEdit, const std::vector<glm::ivec2>& tiles);
	void smoothTiles(Map* map, BrowEdit* browEdit, const std::vector<glm::ivec2>& tiles, int axis);
	void addRandomHeight(Map* map, BrowEdit* browEdit, const std::vector<glm::ivec2> &tiles, float min, float max);
	void connectHigh(Map* map, BrowEdit* browEdit, const std::vector<glm::ivec2>& tiles);
	void connectLow(Map* map, BrowEdit* browEdit, const std::vector<glm::ivec2>& tiles);

	void perlinNoise(const std::vector<glm::ivec2>& tiles);

	glm::vec3 getPos(int cubeX, int cubeY, int index);
	inline bool inMap(const glm::ivec2& t) { return t.x >= 0 && t.x < width&& t.y >= 0 && t.y < height; }

	std::vector<glm::vec3> getMapQuads();

	class Texture
	{
	public:
		std::string name;
		std::string file;

//		Texture* texture;
		Texture();
		Texture(const std::string& name, const std::string& file) : name(name), file(file) {}
		~Texture();
	};

	class Lightmap
	{
	public:
		Lightmap() { 
			memset(data, 255, 64);
			memset(data + 64, 0, 256 - 64);
		};
		Lightmap(const Lightmap& other)
		{
			memcpy(data, other.data, 256 * sizeof(unsigned char));
		}
		unsigned char data[256];
		const unsigned char hash() const;
		bool operator == (const Lightmap& other) const;
	};

	class Tile
	{
	public:
		Tile() : textureIndex(-1), lightmapIndex(-1) {};
		Tile(const Tile& o) {
			v1 = o.v1;
			v2 = o.v2;
			v3 = o.v3;
			v4 = o.v4;
			textureIndex = o.textureIndex;
			lightmapIndex = o.lightmapIndex;
			color = o.color;
		}
		union
		{
			struct
			{
				glm::vec2 v1, v2, v4, v3; //cheating the order here to get them 'in order' for a quad
			};
			glm::vec2 texCoords[4];
		};
		short textureIndex;
		unsigned short lightmapIndex;
		glm::ivec4 color;
		const unsigned char hash() const;
		bool operator == (const Tile& other) const;
	};

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
			for (int i = 0; i < 3; i++)
				this->tileIds[i] = other->tileIds[i];
			for (int i = 0; i < 4; i++)
				this->normals[i] = other->normals[i];
			this->normal = other->normal;
		}
		union
		{
			struct
			{
				float h1, h2, h3, h4;
			};
			float heights[4];
		};
		union
		{
			struct
			{
				int tileUp, tileFront, tileSide;
			};
			int tileIds[3];
		};
		bool selected;

		glm::vec3 normal;
		glm::vec3 normals[4];

		void calcNormal();
		void calcNormals(Gnd* gnd, int x, int y);
		bool sameHeight(const Cube& other) const;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Cube, h1, h2, h3, h4, tileUp, tileFront, tileSide, normal, normals);
	};


	short version;
	int width;
	int height;
	float tileScale;
	int maxTexName;
	int lightmapWidth;
	int lightmapHeight;
	int gridSizeCell;
	std::vector<Texture*> textures;
	std::vector<Lightmap*> lightmaps;
	std::vector<Tile*> tiles;
	std::vector<std::vector<Cube*> > cubes;


	static inline 	glm::ivec3 connectInfo[4][4] = {
		{	glm::ivec3(0,0, 0),			glm::ivec3(-1,0, 1),		glm::ivec3(0,-1, 2),		glm::ivec3(-1,-1, 3),	},
		{	glm::ivec3(0,0, 1),			glm::ivec3(1,0, 0),			glm::ivec3(0,-1, 3),		glm::ivec3(1,-1, 2),	},
		{	glm::ivec3(0,0, 2),			glm::ivec3(-1,0, 3),		glm::ivec3(0,1, 0),			glm::ivec3(-1,1, 1),	},
		{	glm::ivec3(0,0, 3),			glm::ivec3(1,0, 2),			glm::ivec3(0,1, 1),			glm::ivec3(1,1, 0),		}
	};

};