#pragma once

#include "Component.h"
#include <string>
#include <map>
#include <vector>
#include <functional>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


class Rsm : public Component
{
public:
	class Mesh
	{
	public:
		class Face
		{
		public:
			short vertexIds[3];
			short texCoordIds[3];
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

		std::vector<std::string> textureFiles; //for > 0203
		std::vector<int> textures;
		std::vector<RotFrame> rotFrames;
		std::vector<PosFrame> posFrames;
		std::vector<ScaleFrame> scaleFrames;


		glm::vec3 bbmin;
		glm::vec3 bbmax;
		glm::vec3 bbrange;

		float maxRange;
		void calcMatrix1(int time);
		void calcMatrix2();
		bool matrixDirty = true;

		void setBoundingBox(glm::vec3& bbmin, glm::vec3& bbmax);
		void setBoundingBox2(glm::mat4& mat, glm::vec3& realbbmin, glm::vec3& realbbmax);
	};


	void updateMatrices();
public:
	Rsm(const std::string& fileName);
	~Rsm();
	void reload();

	std::string fileName;
	bool loaded;
	short version;
	Mesh* rootMesh;
	char unknown[16];
	char alpha;
	int animLen;
	std::vector<std::string> textures;



	int meshCount;
	glm::vec3 realbbmin;
	glm::vec3 realbbmax;
	glm::vec3 realbbrange;

	glm::vec3 bbmin;
	glm::vec3 bbmax;
	glm::vec3 bbrange;

	float maxRange;
	enum class ShadeType
	{
		SHADE_NO,
		SHADE_FLAT,
		SHADE_SMOOTH,
		SHADE_BLACK,
	} shadeType;

};