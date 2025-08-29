#pragma once

#include "Component.h"
#include <string>
#include <map>
#include <vector>
#include <functional>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#ifdef _DEBUG
#define VERSIONLIMIT(min, max, type, name) \
private:\
	type _##name##;\
public:\
	inline type & getProp_##name##()\
	{\
		if (version < min || version > max)\
			throw "Invalid version use!";\
		return _##name##;\
	}\
	inline void putProp_##name##(type value)\
	{\
		if (version < min|| version > max)\
			throw "Invalid version use!";\
		_##name## = value;\
	}\
public:\
	__declspec(property(get = getProp_##name##, put = putProp_##name##)) type name;
#else
#define VERSIONLIMIT(min, max, type, name) type name;
#endif
/*
#ifdef _DEBUG
#define VERSIONLIMITMESH(min, max, type, name) \
private:\
	type _##name##;\
public:\
	inline type & getProp_##name##()\
	{\
		if (model-version < min || model-version > max)\
			throw "Invalid version use!";\
		return _##name##;\
	}\
	inline void putProp_##name##(type value)\
	{\
		if (model-version < min|| model-version > max)\
			throw "Invalid version use!";\
		_##name## = value;\
	}\
public:\
	__declspec(property(get = getProp_##name##, put = putProp_##name##)) type name;
#else
#define VERSIONLIMITMESH(min, max, type, name) type name;
#endif
*/

class Rsm : public Component
{
public:
	class Mesh
	{
	public:
		class Face
		{
		public:
			unsigned short vertexIds[3];
			unsigned short texCoordIds[3];
			glm::vec3					normal;
			glm::vec3					vertexNormals[3];
			short texId;
			int twoSided;
			glm::ivec3 smoothGroups = glm::ivec3(-1);
			short padding;
		};
		class Frame
		{
		public:
			int time;
			virtual ~Frame() {};
		};
		class RotFrame : public Frame
		{
		public:
			glm::quat					quaternion;
		};
		class ScaleFrame : public Frame
		{
		public:
			glm::vec3					scale;
			float						data; //???????
		};
		class PosFrame : public Frame
		{
		public:
			glm::vec3					position;
			float						data; //???????
		};
		class TexFrame : public Frame
		{
		public:
			float						data;
		};

		Mesh(Rsm* model, std::istream* rsmFile);
		Mesh(Rsm* model);
		~Mesh();

		void save(std::ostream* pFile);
		Rsm* model;
		int index;
		std::string name;
		std::string parentName;

		glm::mat4 offset;
		glm::vec3 pos;

		std::vector<glm::vec3> vertices;
		std::vector<glm::vec2> texCoords;
		std::vector<Face> faces;
		std::vector<Mesh*> children;

		glm::mat4 matrix1;
		glm::mat4 matrix2;

		Mesh* parent;

		bool reverseCullFace;
		bool reverseCullFaceSub;
		// Decomposition values for the offset matrix
		glm::vec3 offsetPosition;
		glm::vec3 offsetScale;
		glm::vec3 offsetRotation;

		void fetchChildren(std::map<std::string, Mesh* > meshes);

		void foreach(const std::function<void(Mesh*)>& callback)
		{
			callback(this);
			for (auto child : children)
				child->foreach(callback);
		}

		glm::vec3 pos_;
		float rotangle;
		glm::vec3 rotaxis;
		glm::vec3 scale;

		//std::vector<std::string> textureFiles; //for > 0203
		std::vector<int> textures;
		std::vector<RotFrame> rotFrames;
		std::vector<PosFrame> posFrames;
		std::vector<ScaleFrame> scaleFrames;
		std::map<int, std::map<int, std::vector<TexFrame>>> textureFrames;


		glm::vec3 bbmin;
		glm::vec3 bbmax;
		glm::vec3 bbrange;

		float maxRange;
		void calcMatrix1(int time);
		void calcMatrix2();
		bool isAnimated = false;

		void setBoundingBox(glm::vec3& bbmin, glm::vec3& bbmax);
		void setBoundingBox2(glm::mat4& mat, glm::vec3& realbbmin, glm::vec3& realbbmax);
		void setBoundingBox3(glm::mat4& mat, glm::vec3& drawnbbmin, glm::vec3& drawnbbmmax);
		bool getTextureAnimation(int textureId, int time, glm::vec2& texTranslate, glm::vec3& texScale, float& texRot);
	};


	void updateMatrices();
	void setAnimated(Rsm::Mesh* mesh, bool isAnimated = false);
public:
	Rsm(const std::string& fileName);
	~Rsm();
	void reload();
	
	void save(const std::string& filename);

	std::string fileName;
	bool loaded;
	short version;
	Mesh* rootMesh;



	VERSIONLIMIT(0x0104, 0xFFFF, unsigned char, alpha);
	int animLen;
	std::vector<std::string> textures;

	VERSIONLIMIT(0x0202, 0xFFFF, float, fps);
	char unknown[16]; //TODO: make this versionlimit too (< 0x0202)




	int meshCount;
	glm::vec3 realbbmin;
	glm::vec3 realbbmax;
	glm::vec3 realbbrange;

	glm::vec3 bbmin;
	glm::vec3 bbmax;
	glm::vec3 bbrange;
	float maxRange;

	glm::vec3 drawnbbmin;
	glm::vec3 drawnbbmax;
	glm::vec3 drawnbbrange;

	enum class ShadeType
	{
		SHADE_NO,
		SHADE_FLAT,
		SHADE_SMOOTH,
		SHADE_BLACK,
	} shadeType;

};