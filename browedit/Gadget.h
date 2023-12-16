#pragma once

#include <glm/glm.hpp>
#include <browedit/gl/Shader.h>
#include <browedit/gl/Vertex.h>
#include <browedit/gl/VBO.h>
#include "Mesh.h"
#include <vector>

class SimpleShader;

class Gadget
{
private:
	class ArrowMesh : public Mesh
	{
	public:
		virtual void buildVertices(std::vector<VertexP3T2N3>& verts) override;
	};
	class ScaleMesh : public Mesh
	{
	public:
		virtual void buildVertices(std::vector<VertexP3T2N3>& verts) override;
	};
	class ScaleCubeMesh : public Mesh
	{
	public:
		virtual void buildVertices(std::vector<VertexP3T2N3>& verts) override;
	};
	class RotateMesh : public Mesh
	{
	public:
		virtual void buildVertices(std::vector<VertexP3T2N3>& verts) override;
	};

	static inline SimpleShader* shader;
	static inline RotateMesh rotateMesh;
	static inline ScaleMesh scaleMesh;
	static inline ScaleCubeMesh scaleCubeMesh;

	static inline glm::mat4 viewMatrix;
	static inline glm::mat4 projectionMatrix;
	static inline glm::vec3 lightDirection;
public:
	static inline ArrowMesh arrowMesh;

	static void init();
	static void setMatrices(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix, const glm::vec3& lightDirection = glm::vec3(1, 1, 1));
	enum class Mode
	{
		Translate,
		Rotate,
		Scale,
		TranslateY,
	} mode = Mode::Translate;
	enum Axis
	{
		X = 1,
		Y = 2,
		Z = 4,
		XYZ = X | Y | Z
	};
	static void setMode(Mode mode);
	static void setThickness(float newThickness);

	bool disabled = false;
	bool axisClicked = false;
	bool axisReleased = false;
	bool axisDragged = false;
	int selectedAxis = 0;
	int hoveredAxis = 0;
	bool isMouseJustPressed = true;
	bool axisHovered;
	inline static float opacity = 1.0f;
	inline static float scale = 1.0f;
	inline static float thickness = 2;

	int selectedAxisIndex();

	void draw(const math::Ray& mouseRay, glm::mat4 modelMatrix, float angleX = 0);
};