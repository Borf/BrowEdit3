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
namespace gl { class Texture; }

class Rsw : public Component, public ImguiProps
{
public:
	short version = 0x201;
	int buildNumber = 0;
	std::string iniFile;
	std::string gndFile;
	std::string gatFile;

	int lubVersion = 2;

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

	struct Water
	{
		float	height = 0;
		int		type = 0;
		float	amplitude = 1;
		float	waveSpeed = 2.0f;
		float	wavePitch = 50.0f;
		int		textureAnimSpeed = 3;
	};

	class WaterData
	{
	public:
		int splitWidth = 0;
		int splitHeight = 0;

		std::vector<std::vector<Water>> zones;

		Water* getFromGat(int x, int y, Gnd* gnd);
		Water* getFromGnd(int x, int y, Gnd* gnd);
		void resize(int width, int height);
	};

	struct
	{
		int			longitude;
		int			latitude;
		glm::vec3	diffuse;
		glm::vec3	ambient;
		float		intensity;
		float		lightmapAmbient = 0.5f; //EXTRA
	} light;


	struct
	{
		float nearPlane = 0.0f;
		float farPlane = 1.0f;
		float factor = 0.5f;
		glm::vec4 color;
	} fog;


	class KeyFrame
	{
	public:
		float time;
		KeyFrame(float time) : time(time) {}
		virtual void buildEditor();
	};
	template<class T>
	class KeyFrameData : public KeyFrame
	{
	public:
		T data;
		KeyFrameData(float time, const T& data) : KeyFrame(time), data(data) {}
		KeyFrameData(float time) : KeyFrame(time) {}
		virtual void buildEditor();
	};

	class Track
	{
	public:
		std::string name;
		std::vector<KeyFrame*> frames;
		KeyFrame* getBeforeFrame(float time);
		KeyFrame* getAfterFrame(float time);
	};

	class CameraTarget
	{
	public:
		enum class LookAt
		{
			Point, 
			Direction,
			FixedDirection,
		} lookAt;
		float turnSpeed;
		glm::quat angle;
		glm::vec3 point;

		CameraTarget(const glm::vec3& point, float speed) : lookAt(LookAt::Point), point(point), turnSpeed(speed), angle(-1,0,0,0) {}
		CameraTarget(const glm::quat& angle, float speed) : lookAt(LookAt::Direction), angle(angle), turnSpeed(speed), point(0) {}
		CameraTarget() : lookAt(LookAt::Point), point(glm::vec3(0)), turnSpeed(0.1f), angle(-1, 0, 0, 0) {}
	};

	class LightmapSettings
	{
	public:
		int quality = 1;
		bool shadows = true;
		bool heightSelectionOnly = false;
		bool additiveShadow = true;
		glm::ivec2 rangeX;
		glm::ivec2 rangeY;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(LightmapSettings, quality, shadows, heightSelectionOnly, rangeX, rangeY);
	} lightmapSettings;

	class CropSettings
	{
	public:
		bool heightSelectionOnly = true;
		int cropLeft = 0;
		int cropTop = 0;
		int cropRight = 0;
		int cropBottom = 0;
	} cropSettings;

	std::vector<Track> cinematicTracks;
	float cinematicLength;

	int			unknown[4];
	std::vector<glm::vec3> quadtreeFloats;
	QuadTreeNode* quadtree = nullptr;
	std::map<std::string, std::map<std::string, glm::vec4>> colorPresets;
	WaterData water;

	Rsw();
	~Rsw();

	void load(const std::string& fileName, Map* map, BrowEdit* browEdit, bool loadModels = true, bool loadGnd = true);
	void save(const std::string& fileName, BrowEdit* browEdit);
	void newMap(const std::string& fileName, int width, int height, Map* map, BrowEdit* browEdit);
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
	void save(std::ofstream& file, Rsw *rsw);
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
	float shadowStrength = 1.0f;
	bool gatCollision = true;
	int gatStraightType = 0;
	int gatType = -1;

	RswModel() : aabb(glm::vec3(), glm::vec3()) {}
	RswModel(const std::string &fileName) : aabb(glm::vec3(), glm::vec3()), animType(0), animSpeed(1), blockType(0), fileName(fileName) {}
	RswModel(RswModel* other);
	void load(std::istream* is, int version, unsigned char buildNumber, bool loadModel);
	void loadExtra(nlohmann::json data);
	void save(std::ofstream &file, int version, int buildNumber);
	nlohmann::json saveExtra();
	static void buildImGuiMulti(BrowEdit* browEdit, const std::vector<Node*>&);

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(RswModel, animType, animSpeed, blockType, fileName, shadowStrength, gatCollision, gatStraightType, gatType);
};

class RswLight : public Component
{
public:
	glm::vec3		color;
	float			range;
	// custom properties!!!!!!!!!
	bool diffuseLighting = true;
	enum class Type
	{
		Point,
		Spot,
		Sun
	} lightType = Type::Point;
	bool givesShadow = false;
	bool affectShadowMap = false;
	bool affectLightmap = true;
	bool enabled = true;
	bool shadowTerrain = true;

	bool sunMatchRswDirection = true;
	glm::vec3 direction = glm::normalize(glm::vec3(1, -1, 1));

	float spotlightWidth = .5f;

	enum class FalloffStyle {
		Exponential = 0,
		SplineTweak = 1,
		LagrangeTweak = 2,
		LinearTweak = 3,
		Magic = 4,
		S_Curve = 5,
	};
	FalloffStyle falloffStyle = FalloffStyle::S_Curve;

	float cutOff = 0.5f;
	float intensity = 1;
	float minShadowDistance = 0;
	std::vector<glm::vec2> falloff = { glm::vec2(0,1), glm::vec2(1,0) };
	// end custom properties
	float realRange();

	RswLight() {}
	void load(std::istream* is);
	void loadExtra(nlohmann::json data);
	void save(std::ofstream& file);
	nlohmann::json saveExtra();
	static void buildImGuiMulti(BrowEdit* browEdit, const std::vector<Node*>&);
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(RswLight, color, enabled, lightType, spotlightWidth, sunMatchRswDirection, direction, range, givesShadow, cutOff, cutOff, intensity, minShadowDistance, affectShadowMap, affectLightmap, falloff, falloffStyle, shadowTerrain, diffuseLighting);
};

class LubEffect : public Component
{
public:
	glm::vec3 dir1;
	glm::vec3 dir2;
	glm::vec3 gravity;
	glm::vec3 pos;
	glm::vec3 radius;
	glm::vec4 color;
	glm::vec2 rate;
	glm::vec2 size;
	glm::vec2 life;
	std::string texture;
	float speed;
	int srcmode;
	int destmode;
	int maxcount;
	int zenable;
	int billboard_off;
	glm::vec3 rotate_angle; // v3
	bool dirty;

	void load(const nlohmann::json& data);
	static void buildImGuiMulti(BrowEdit* browEdit, const std::vector<Node*>&);
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(LubEffect, dir1, dir2, gravity, pos, radius, color, rate, size, life, texture, speed, srcmode, destmode, maxcount, zenable, billboard_off, rotate_angle);
};

class RswEffect : public Component
{
public:
	int	id = 0;
	float loop = 0;
	float param1 = 0;
	float param2 = 0;
	float param3 = 0;
	float param4 = 0;

	RswEffect() {}
	void load(std::istream* is);
	void save(std::ofstream& file);
	static void buildImGuiMulti(BrowEdit* browEdit, const std::vector<Node*>&);
	static inline std::map<int, gl::Texture*> previews;
	static inline std::map<int, gl::Texture*> previewAnim;
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

	RswSound() {}
	RswSound(const std::string &fileName) : fileName(fileName) {}
	void play();
	void load(std::istream* is, int version);
	void save(std::ofstream& file, int version);
	static void buildImGuiMulti(BrowEdit* browEdit, const std::vector<Node*>&);
	
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(RswSound, fileName, vol, width, height, range, cycle);
};

class RswModelCollider : public Collider
{
	std::vector<std::vector<std::vector<glm::vec3>>> buffered_faces;
	RswModel* rswModel = nullptr;
	Rsm* rsm = nullptr;
	RsmRenderer* rsmRenderer = nullptr;
public:
	void begin();
	bool collidesTexture(const math::Ray& ray, float minDistance = 0.0f, float maxDistance = 9999999.0f);
	bool collidesTexture(Rsm::Mesh* mesh, const math::Ray& ray, const glm::mat4& matrix, float minDistance, float maxDistance);
	void calculateWorldFaces(Rsm::Mesh* mesh, const glm::mat4& matrix);
	void calculateWorldFaces();

	std::vector<glm::vec3> getVerticesWorldSpace(Rsm::Mesh* mesh = nullptr, const glm::mat4& matrix = glm::mat4(1.0f));
	std::vector<glm::vec3> getAllVerticesWorldSpace(Rsm::Mesh* mesh = nullptr, const glm::mat4& matrix = glm::mat4(1.0f));

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