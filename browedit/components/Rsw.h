#pragma once

#include "Component.h"
#include "ImguiProps.h"
#include <string>
#include <glm/glm.hpp>
#include <browedit/util/tree.h>
#include <browedit/math/AABB.h>

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
		QuadTreeNode(std::vector<glm::vec3>::const_iterator it, int level = 0);
		~QuadTreeNode();
		math::AABB bbox;
		glm::vec3 range[2];
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

	void load(const std::string& fileName);
	void save(const std::string& fileName);

	void buildImGui();
};



class RswObject : public Component, public ImguiProps
{
public:
	glm::vec3 position; //TODO: put in transform
	glm::vec3 rotation;
	glm::vec3 scale;

	void load(std::istream* is, int version);
	void buildImGui();
};

class RswModel : public Component, public ImguiProps
{
public:
	math::AABB aabb;
	int animType;
	float animSpeed;
	int blockType;
	std::string fileName;

	RswModel() : aabb(glm::vec3(), glm::vec3()) {}
	void load(std::istream* is, int version);
	void buildImGui();
};


class RswLight : public Component, public ImguiProps
{
public:
	float todo[10];
	glm::vec3		color;
	float			todo2;

	RswLight() {}
	void load(std::istream* is);
	void buildImGui();
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
	void buildImGui();
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
	void buildImGui();
};