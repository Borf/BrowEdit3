#pragma once

#include "Component.h"
#include "Collider.h"
#include "Rsm.h"
#include "ImguiProps.h"
#include <string>
#include <glm/glm.hpp>
#include <browedit/util/Util.h>
#include <browedit/util/Tree.h>
#include <browedit/math/AABB.h>
#include <json.hpp>

class RsmRenderer;
class Gnd;

class Rsw : public Component, public ImguiProps
{
public:
	short version;
	std::string iniFile;
	std::string gndFile;
	std::string gatFile;

	class QuadTreeNode : public util::Tree<4, QuadTreeNode>
	{
	public:
		QuadTreeNode(std::vector<glm::vec3>::const_iterator &it, int level = 0);
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
		float	phase;
		float	surfaceCurve;
		int		animSpeed;
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

	void load(const std::string& fileName, bool loadModels = true, bool loadGnd = true);
	void save(const std::string& fileName);


	void buildImGui(BrowEdit* browEdit) override;
};



class RswObject : public Component, public ImguiProps
{
public:
	glm::vec3 position = glm::vec3(0,0,0);
	glm::vec3 rotation = glm::vec3(0,0,0);
	glm::vec3 scale = glm::vec3(1,1,1);

	RswObject() {}
	RswObject(RswObject* other);
	void load(std::istream* is, int version, bool loadModel);
	void save(std::ofstream& file, int version);
	void buildImGui(BrowEdit* browEdit) override;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(RswObject, position, rotation, scale);
};

class RswModel : public Component, public ImguiProps
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
	void load(std::istream* is, int version, bool loadModel);
	void loadExtra(nlohmann::json data);
	void save(std::ofstream &file, int version);
	nlohmann::json saveExtra();
	void buildImGui(BrowEdit* browEdit) override;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(RswModel, animType, animSpeed, blockType, fileName, givesShadow);
};


class RswLight : public Component, public ImguiProps
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
	float cutOff = 0;
	float intensity = 20;
	float realRange();
	
	// end custom properties

	RswLight() {}
	void load(std::istream* is);
	void loadExtra(nlohmann::json data);
	void save(std::ofstream& file);
	nlohmann::json saveExtra();
	void buildImGui(BrowEdit* browEdit) override;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(RswLight, color, range, givesShadow, cutOff, cutOff, intensity, affectShadowMap, affectLightmap);
};

class RswEffect : public Component, public ImguiProps
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
	void buildImGui(BrowEdit* browEdit) override;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(RswEffect, id, loop, param1, param2, param3, param4);
};

class RswSound : public Component, public ImguiProps
{
public:
	std::string fileName;
	//float repeatDelay;
	float vol;
	long	width;
	long	height;
	float	range;
	float	cycle;

	char unknown6[8];
	float unknown7;
	float unknown8;


	RswSound() {}
	void load(std::istream* is, int version);
	void save(std::ofstream& file, int version);
	void buildImGui(BrowEdit* browEdit) override;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(RswSound, fileName, vol, width, height, range, cycle, unknown6, unknown7, unknown8);
};

class RswModelCollider : public Collider
{
	RswModel* rswModel = nullptr;
	Rsm* rsm = nullptr;
	RsmRenderer* rsmRenderer = nullptr;
public:
	void begin();
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