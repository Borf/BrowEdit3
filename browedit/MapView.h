#pragma once

#include <browedit/gl/Shader.h>
#include <browedit/NodeRenderer.h>
#include <browedit/math/Ray.h>
#include <browedit/math/Plane.h>
#include <browedit/components/BillboardRenderer.h>
#include <browedit/Gadget.h>
class BrowEdit;
class Map;
struct ImVec2;

namespace gl 
{ 
	class FBO; 
}

struct MouseState
{
	glm::vec2 position;
	int buttons;
	bool isPressed(int button)
	{
		return (buttons & (1 << button)) != 0;
	}
};


class MapView
{
	class SphereMesh : public Mesh
	{
	public:
		std::vector<glm::vec3> buildVertices();
	};
public:
	Map* map = nullptr;
	int id;
	std::string viewName;

	gl::VBO<VertexP3T2>* gridVbo = nullptr;

	NodeRenderContext nodeRenderContext;
	Gadget gadget;
	Gadget gadgetHeight[9];
	BillboardRenderer::BillboardShader* billboardShader;
	SimpleShader* simpleShader = nullptr;
	static inline SphereMesh sphereMesh;
	gl::Texture* whiteTexture;

	bool opened = true;
	gl::FBO* fbo;

	bool ortho = false;
	glm::vec2 cameraCenter;
	float cameraRotX = 45.0f;
	float cameraRotY = 0.0f;
	float cameraDistance = 500;
	bool hovered = false;

	bool snapToGrid = false;
	float gridSize = 5;
	bool gridLocal = true;

	bool showAllLights = false;
	bool showLightSphere = false;

	float gadgetOpacity = 0.5f;
	float gadgetScale = 1.0f;

	int quadTreeMaxLevel = 0;

	enum class PivotPoint {
		Local,
		GroupCenter
	} pivotPoint = PivotPoint::Local;


	//height edit mode
	bool mouseDown = false;



	math::Ray mouseRay;
	math::Plane mouseDragPlane;
	glm::vec3 mouseDragStart;
	glm::vec2 mouseDragStart2D;
	std::vector<glm::ivec2> selectLasso;


	MouseState mouseState;
	MouseState prevMouseState;

	MapView(Map* map, const std::string &viewName);
	MapView(const Map& other) = delete;
	MapView() = delete;

	void toolbar(BrowEdit* browEdit);

	void update(BrowEdit* browEdit, const ImVec2& size);
	void render(BrowEdit* browEdit);
	void postRenderObjectMode(BrowEdit* browEdit);
	void postRenderHeightMode(BrowEdit* browEdit);


	void rebuildObjectModeGrid();

	bool viewLightmapShadow = true;
	bool viewLightmapColor = true;
	bool viewColors = true;
	bool viewLighting = true;
	bool smoothColors = false;
	bool viewTextures = true;
	bool viewEmptyTiles = true;


	void focusSelection();
	void drawLight(Node* n);
};