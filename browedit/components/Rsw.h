#pragma once

#include "Component.h"
#include "Collider.h"
#include "ImguiProps.h"
#include "Rsm.h"
#include <string>
#include <glm/glm.hpp>
#include <browedit/util/Util.h>
#include <browedit/util/Tree.h>
#include <browedit/math/AABB.h>
#include <json.hpp>

class RsmRenderer;
class Gnd;
class Map;
class BrowEdit;

class Rsw : public Component, public ImguiProps
{
public:
	short version;
	unsigned char buildNumber = 0;
	std::string iniFile;
	std::string gndFile;
	std::string gatFile;

	class QuadTreeNode : public util::Tree<4, QuadTreeNode>
	{
	public:
		QuadTreeNode(std::vector<glm::vec3>::const_iterator &it, int level = 0);
		QuadTreeNode(float x, float y, float width, float height, int level);
		~QuadTreeNode();
		math::AABB bbox;
		glm::vec3 range[2];

		void draw(int);
	};

	struct
	{
		float	height;
		int		type;
		float	amplitude;
		float	waveSpeed;
		float	wavePitch;
		int		textureAnimSpeed;
	} water;

	struct
	{
		int			longitude;
		int			latitude;
		glm::vec3	diffuse;
		glm::vec3	ambient;
		float		intensity;
		float		lightmapAmbient = 0.5f; //EXTRA
		float		lightmapIntensity = 0.5f; // EXTRA
	} light;

	int			unknown[4];
	std::vector<glm::vec3> quadtreeFloats;
	QuadTreeNode* quadtree = nullptr;


	Rsw();

	void load(const std::string& fileName, Map* map, BrowEdit* browEdit, bool loadModels = true, bool loadGnd = true);
	void save(const std::string& fileName);
	void buildImGui(BrowEdit* browEdit) override;
	void recalculateQuadtree(QuadTreeNode* node = nullptr);
};



class RswObject : public Component
{
public:
	glm::vec3 position = glm::vec3(0,0,0);
	glm::vec3 rotation = glm::vec3(0,0,0);
	glm::vec3 scale = glm::vec3(1,1,1);

	RswObject() {}
	RswObject(RswObject* other);
	void load(std::istream* is, int version, unsigned char buildNumber, bool loadModel);
	void save(std::ofstream& file, int version);
	static void buildImGuiMulti(BrowEdit* browEdit, const std::vector<Node*>&);
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(RswObject, position, rotation, scale);
};

class RswModel : public Component
{
public:
	math::AABB aabb;
	int animType;
	float animSpeed;
	int blockType;
	std::string fileName; //UTF8
	std::string objectName; //not sure what this is, UTF8

	//custom properties
	bool givesShadow = true;

	RswModel() : aabb(glm::vec3(), glm::vec3()) {}
	RswModel(const std::string &fileName) : aabb(glm::vec3(), glm::vec3()), animType(0), animSpeed(1), blockType(0), fileName(fileName) {}
	RswModel(RswModel* other);
	void load(std::istream* is, int version, unsigned char buildNumber, bool loadModel);
	void loadExtra(nlohmann::json data);
	void save(std::ofstream &file, int version);
	nlohmann::json saveExtra();
	static void buildImGuiMulti(BrowEdit* browEdit, const std::vector<Node*>&);

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(RswModel, animType, animSpeed, blockType, fileName, givesShadow);
};

class RswLight : public Component
{
public:
	float todo[10];
	glm::vec3		color;
	float			range;
	// custom properties!!!!!!!!!
	enum class Type
	{
		Point,
		Spot
	} type = Type::Point;
	bool givesShadow = true;
	bool affectShadowMap = true;
	bool affectLightmap = true;

	int falloffStyle = 0;
	enum FalloffStyle {
		Exponential = 0,
		SplineTweak = 1,
		LagrangeTweak = 2,
		LinearTweak = 3,
		Magic = 4,
	};

	float cutOff = 0.5f;
	float intensity = 20;
	std::vector<glm::vec2> falloff = { glm::vec2(0,1), glm::vec2(1,0) };
	// end custom properties
	float realRange();

	RswLight() {}
	void load(std::istream* is);
	void loadExtra(nlohmann::json data);
	void save(std::ofstream& file);
	nlohmann::json saveExtra();
	static void buildImGuiMulti(BrowEdit* browEdit, const std::vector<Node*>&);
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(RswLight, color, range, givesShadow, cutOff, cutOff, intensity, affectShadowMap, affectLightmap, falloff, falloffStyle);
};

class LubEffect : public Component
{
public:
	glm::vec3 dir1;
	glm::vec3 dir2;
	glm::vec2 gravity;
	glm::vec3 pos;
	glm::vec3 radius;
	glm::vec3 color;
	glm::vec2 rate;
	glm::vec2 size;
	glm::vec2 life;
	std::string texture;
	float speed;
	int srcmode;
	int destmode;
	int maxcount;

	void load(const nlohmann::json& data);
	static void buildImGuiMulti(BrowEdit* browEdit, const std::vector<Node*>&);
};

class RswEffect : public Component
{
public:
	int	id;
	float loop;
	float param1;
	float param2;
	float param3;
	float param4;

	RswEffect() {}
	void load(std::istream* is);
	void save(std::ofstream& file);
	static void buildImGuiMulti(BrowEdit* browEdit, const std::vector<Node*>&);

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(RswEffect, id, loop, param1, param2, param3, param4);
};

class RswSound : public Component
{
public:
	std::string fileName;
	//float repeatDelay;
	float	vol = 0.7f;
	long	width = 10;
	long	height = 10;
	float	range = 100.0f;
	float	cycle = 4.0f;

	uint8_t unknown6[8];
	float unknown7 = -45.0f;
	float unknown8 = 0.0f;


	RswSound() {}
	RswSound(const std::string &fileName) : fileName(fileName) {}
	void play();
	void load(std::istream* is, int version);
	void save(std::ofstream& file, int version);
	static void buildImGuiMulti(BrowEdit* browEdit, const std::vector<Node*>&);
	
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(RswSound, fileName, vol, width, height, range, cycle, unknown6, unknown7, unknown8);
};

class RswModelCollider : public Collider
{
	RswModel* rswModel = nullptr;
	Rsm* rsm = nullptr;
	RsmRenderer* rsmRenderer = nullptr;
public:
	void begin();
	bool collidesTexture(const math::Ray& ray);
	bool collidesTexture(Rsm::Mesh* mesh, const math::Ray& ray, const glm::mat4& matrix);

	std::vector<glm::vec3> getVerticesWorldSpace(Rsm::Mesh* mesh = nullptr, const glm::mat4& matrix = glm::mat4(1.0f));

	std::vector<glm::vec3> getCollisions(const math::Ray& ray);
	std::vector<glm::vec3> getCollisions(Rsm::Mesh* mesh, const math::Ray& ray, const glm::mat4 &matrix);
};


class CubeCollider : public Collider
{
	math::AABB aabb;
	RswObject* rswObject = nullptr;
	Gnd* gnd = nullptr;
public:
	void begin();
	CubeCollider(int size);
	std::vector<glm::vec3> getCollisions(const math::Ray& ray);
	std::vector<glm::vec3> getCollisions(Rsm::Mesh* mesh, const math::Ray& ray, const glm::mat4& matrix);
};