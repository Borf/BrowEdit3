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
class Rsw;

namespace gl {
	class Texture;
}

class Gnd : public Component, public ImguiProps
{
public:
	Gnd(const std::string& fileName, Rsw* rsw);
	Gnd(int width, int height);
	~Gnd();
	void save(const std::string &fileName, Rsw* rsw);
	glm::vec3 rayCast(const math::Ray& ray, bool emptyTiles = false, int xMin = 0, int yMin = 0, int xMax = -1, int yMax = -1, float offset = 0.0f);
	glm::vec3 rayCastLightmap(const math::Ray& ray, int cx, int cy, int xMin = 0, int yMin = 0, int xMax = -1, int yMax = -1, float offset = 0.0f);
	void makeLightmapsUnique();
	void makeLightmapsClear();
	void makeLightmapBorders(BrowEdit* browEdit);
	void makeLightmapsSmooth(BrowEdit* browEdit);
	void makeTilesUnique();
	void cleanLightmaps();
	void makeLightmapsDiffRes(int rx, int ry);
	void removeZeroHeightWalls();
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
		friend void to_json(nlohmann::json& nlohmann_json_j, const Texture& nlohmann_json_t) {
			nlohmann_json_j["file"] = util::iso_8859_1_to_utf8(nlohmann_json_t.file);
			nlohmann_json_j["name"] = util::iso_8859_1_to_utf8(nlohmann_json_t.name);
		}
		friend void from_json(const nlohmann::json& nlohmann_json_j, Texture& nlohmann_json_t) { 
			nlohmann_json_t.file = util::utf8_to_iso_8859_1(nlohmann_json_j["file"].get<std::string>());
			nlohmann_json_t.name = util::utf8_to_iso_8859_1(nlohmann_json_j["name"].get<std::string>());
		}
		bool operator == (const Texture& other) const { return this->name == other.name && this->file == other.file; }
	};

	class Lightmap
	{
	protected:
		Lightmap() {
			gnd = nullptr;
			data = nullptr;
		}
		friend class CopyCube;

	public:
		class LightmapRow
		{
		public:
			class LightmapCell
			{
			public:
				Lightmap* lightmap;
				int x, y;

				LightmapCell& operator = (const LightmapCell& other)
				{
					lightmap->data[lightmap->gnd->lightmapWidth * y + x] = other.lightmap->data[other.lightmap->gnd->lightmapWidth * other.y + other.x];
					lightmap->data[lightmap->gnd->lightmapWidth * lightmap->gnd->lightmapHeight + 3 * (lightmap->gnd->lightmapWidth * y + x) + 0] = other.lightmap->data[other.lightmap->gnd->lightmapWidth * other.lightmap->gnd->lightmapHeight + 3 * (other.lightmap->gnd->lightmapWidth * other.y + other.x) + 0];
					lightmap->data[lightmap->gnd->lightmapWidth * lightmap->gnd->lightmapHeight + 3 * (lightmap->gnd->lightmapWidth * y + x) + 1] = other.lightmap->data[other.lightmap->gnd->lightmapWidth * other.lightmap->gnd->lightmapHeight + 3 * (other.lightmap->gnd->lightmapWidth * other.y + other.x) + 1];
					lightmap->data[lightmap->gnd->lightmapWidth * lightmap->gnd->lightmapHeight + 3 * (lightmap->gnd->lightmapWidth * y + x) + 2] = other.lightmap->data[other.lightmap->gnd->lightmapWidth * other.lightmap->gnd->lightmapHeight + 3 * (other.lightmap->gnd->lightmapWidth * other.y + other.x) + 2];
					return *this;
				}
			};
			Lightmap* lightmap;
			int x;
			LightmapCell operator [](int y)
			{
				return LightmapCell{ lightmap, x, y };
			}
		};

		Lightmap(Gnd* gnd) {
			this->gnd = gnd;
			this->data = new unsigned char[gnd->lightmapWidth * gnd->lightmapHeight * 4];
			memset(data, 255, gnd->lightmapWidth*gnd->lightmapHeight);
			memset(data + gnd->lightmapWidth * gnd->lightmapHeight, 0, gnd->lightmapWidth * gnd->lightmapHeight * 3);
		};
		Lightmap(const Lightmap& other)
		{
			gnd = other.gnd;
			this->data = new unsigned char[gnd->lightmapWidth * gnd->lightmapHeight * 4];
			if(other.data)
				memcpy(data, other.data, gnd->lightmapWidth * gnd->lightmapHeight * 4 * sizeof(unsigned char));
		}
		~Lightmap()
		{
			if(data)
				delete[] data;
			data = nullptr;
		}
		unsigned char* data;
		Gnd* gnd;
		void expandBorders();
		const unsigned int hash() const;
		bool operator == (const Lightmap& other) const;
		LightmapRow operator [] (int x) { return LightmapRow{ this, x }; }
		friend void to_json(nlohmann::json& nlohmann_json_j, const Lightmap& nlohmann_json_t) {
			for (int i = 0; i < nlohmann_json_t.gnd->lightmapWidth* nlohmann_json_t.gnd->lightmapHeight*4; i++) {
				nlohmann_json_j["data"].push_back(nlohmann_json_t.data[i]);
			}
		}
		friend void from_json(const nlohmann::json & nlohmann_json_j, Lightmap & nlohmann_json_t) { for (int i = 0; i < 256; i++) { nlohmann_json_t.data[i] = nlohmann_json_j["data"][i].get<int>(); } }
	};

	class Tile
	{
	public:
		Tile() : textureIndex(-1), lightmapIndex(-1), v1(0.0f), v2(0.0f), v3(0.0f), v4(0.0f), color(255,255,255,255) {};
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
		int lightmapIndex;
		glm::ivec4 color;
		const unsigned char hash() const;
		bool operator == (const Tile& other) const;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(Tile, textureIndex, lightmapIndex, color, texCoords);
	};

	class Cube
	{
	public:
		Cube()
		{
			h1 = h2 = h3 = h4 = 0;
			selected = false;
			tileUp = -1;
			tileFront = -1;
			tileSide = -1;
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
	float tileScale = 10.0f;
	int maxTexName = 80;
	int lightmapWidth;
	int lightmapHeight;
	int gridSizeCell;
	inline int lightmapOffset() { return lightmapWidth * lightmapHeight; }
	std::vector<Texture*> textures;
	std::vector<Lightmap*> lightmaps;
	std::vector<Tile*> tiles;
	std::vector<std::vector<Cube*> > cubes;


	Lightmap* getLightmapLeft(const glm::ivec3& pos, int& side);
	Lightmap* getLightmapRight(const glm::ivec3& pos, int& side);
	Lightmap* getLightmapTop(const glm::ivec3& pos, int& side);
	Lightmap* getLightmapBottom(const glm::ivec3& pos, int& side);

	static inline 	glm::ivec3 connectInfo[4][4] = {
		{	glm::ivec3(0,0, 0),			glm::ivec3(-1,0, 1),		glm::ivec3(0,-1, 2),		glm::ivec3(-1,-1, 3),	},
		{	glm::ivec3(0,0, 1),			glm::ivec3(1,0, 0),			glm::ivec3(0,-1, 3),		glm::ivec3(1,-1, 2),	},
		{	glm::ivec3(0,0, 2),			glm::ivec3(-1,0, 3),		glm::ivec3(0,1, 0),			glm::ivec3(-1,1, 1),	},
		{	glm::ivec3(0,0, 3),			glm::ivec3(1,0, 2),			glm::ivec3(0,1, 1),			glm::ivec3(1,1, 0),		}
	};


	virtual void buildImGui(BrowEdit* browEdit) override;

};