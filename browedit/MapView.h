#pragma once

#include <browedit/gl/Shader.h>
#include <browedit/NodeRenderer.h>
#include <browedit/math/Ray.h>
#include <browedit/math/Plane.h>
#include <browedit/components/BillboardRenderer.h>
#include <browedit/components/Gnd.h>
#include <browedit/Gadget.h>
#include <glm/gtx/quaternion.hpp>
class BrowEdit;
class Map;
struct ImVec2;

namespace gl 
{ 
	class FBO; 
	class Texture;
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
	class CubeMesh : public Mesh
	{
	public:
		virtual void buildVertices(std::vector<VertexP3T2N3>& verts) override;
	};
	class SphereMesh : public Mesh
	{
	public:
		virtual void buildVertices(std::vector<VertexP3T2N3>& verts) override;
	};

	class SkyBoxMesh : public Mesh
	{
	public:
		virtual void buildVertices(std::vector<VertexP3T2N3>& verts) override;
	};
	class SkyDomeMesh : public Mesh
	{
	public:
		virtual void buildVertices(std::vector<VertexP3T2N3>& verts) override;
	};

public:
	Map* map = nullptr;
	int id;
	std::string viewName;

	gl::VBO<VertexP3T2>* gridVbo = nullptr;
	gl::VBO<VertexP3T2>* rotateGridVbo = nullptr;

	NodeRenderContext nodeRenderContext;
	Gadget gadget;
	Gadget gadgetHeight[9];
	BillboardRenderer::BillboardShader* billboardShader;
	SimpleShader* simpleShader = nullptr;
	static inline SphereMesh sphereMesh;
	static inline CubeMesh cubeMesh;
	static inline SkyBoxMesh skyBoxMesh;
	static inline SkyDomeMesh skyDomeMesh;
	static inline std::vector<gl::Texture*> skyTextures;
	static inline gl::Texture* skyTexture;
	bool skyboxCube = false;
	float skyboxHeight = 0;
	float skyboxRotation = 0;
	static inline gl::Texture* cubeTexture = nullptr;
	gl::Texture* whiteTexture;

	bool opened = true;
	gl::FBO* fbo;
	
	bool showViewOptions = false;
	bool ortho = false;
	bool lockedGizmo = false;
	glm::vec3 cameraCenter;
	glm::vec2 cameraRot = glm::vec2(45.0f, 0.0f);
	float cameraDistance = 500;
	bool hovered = false;

	bool cameraAnimating = false;
	glm::vec2 cameraTargetRot;
	glm::vec3 cameraTargetPos;


	bool snapToGrid = false;
	bool gridLocal = true;
	float gridSizeTranslate = 5;
	float gridSizeRotate = 45;
	float gridSizeScale = 0.5f;
	float gridOffsetTranslate = 0;
	float gridOffsetRotate = 0;
	float gridOffsetScale = 0;

	bool showAllLights = false;
	bool showLightSphere = false;

	float gadgetOpacity = 0.5f;
	float gadgetScale = 1.0f;
	float gadgetThickness = 1.0f;

	int quadTreeMaxLevel = 0;

	enum class PivotPoint {
		Local,
		GroupCenter
	} pivotPoint = PivotPoint::Local;

	void buildTextureGrid();
	gl::VBO<VertexP3T2>* textureGridVbo = nullptr;
	bool textureGridSmart = true;
	bool textureGridDirty = true;

	int textureSelected = 0;
	glm::vec2 textureEditUv1 = glm::vec2(0.0f, 0.0f);
	glm::vec2 textureEditUv2 = glm::vec2(1.0f, 1.0f);

	int textureBrushWidth = 4;
	int textureBrushHeight = 4;
	bool textureBrushFlipH = false;
	bool textureBrushFlipV = false;
	bool textureBrushFlipD = false;
	inline bool textureBrushFlipWeird() { return ((textureBrushFlipH ? 1 : 0) + (textureBrushFlipV ? 1 : 0) + (textureBrushFlipD ? 1 : 0)) % 2 == 0; }
	bool textureBrushKeepShadow = true;
	bool textureBrushKeepColor = true;
	bool textureBrushAutoFlipSize = true;
	bool textureBrushUpdateBySelection = true;
	int textureBrushMask() { return (textureBrushFlipD ? 4 : 0) | (textureBrushFlipH ? 2 : 0) | (textureBrushFlipV ? 1 : 0); }
	void textureBrushMask(int mask)
	{
		textureBrushFlipD = (mask & 4) != 0;
		textureBrushFlipH = (mask & 2) != 0;
		textureBrushFlipV = (mask & 1) != 0;
	}

	bool deleteTiles = false;
	int wallWidth = 4;
	float wallTop = 0;
	bool wallTopAuto = true;
	float wallBottom = 0;
	bool wallBottomAuto = true;
	int wallOffset = 0;
	bool wallAutoStraight = true;
	bool wallTopDropper = false;
	bool wallBottomDropper = false;


	//height edit mode
	bool mouseDown = false;
	
	bool cinematicPlay = false;
	glm::vec3 cinematicLastCameraPosition;
	glm::quat cinematicCameraDirection = glm::quat(glm::vec3(0,0,0));


	math::Ray mouseRay;
	math::Plane mouseDragPlane;
	glm::vec3 mouseDragStart;
	glm::vec2 mouseDragStart2D;
	std::vector<glm::ivec2> selectLasso;
	std::vector<glm::vec3> objectSelectLasso;


	MouseState mouseState;
	MouseState prevMouseState;

	MapView(Map* map, const std::string &viewName);
	MapView(const Map& other) = delete;
	MapView() = delete;

	void toolbar(BrowEdit* browEdit);

	void update(BrowEdit* browEdit, const ImVec2& size, float deltaTime);
	void render(BrowEdit* browEdit);
	void postRenderTextureMode(BrowEdit* browEdit);
	void postRenderHeightMode(BrowEdit* browEdit);
	void postRenderObjectMode(BrowEdit* browEdit);
	void postRenderGatMode(BrowEdit* browEdit);
	void postRenderWallMode(BrowEdit* browEdit);
	void postRenderColorMode(BrowEdit* browEdit);
	void postRenderShadowMode(BrowEdit* browEdit);
	void postRenderCinematicMode(BrowEdit* browEdit);

	void gatEdit_adjustToGround(BrowEdit* browEdit);
	
	bool drawCameraWidget();


	void rebuildObjectModeGrid();
	void rebuildObjectRotateModeGrid();

	//todo, move this to a struct for better organisation
	bool viewLightmapShadow = true;
	bool viewLightmapColor = true;
	bool viewColors = true;
	bool viewLighting = true;
	bool smoothColors = false;
	bool viewTextures = true;
	bool viewEmptyTiles = true;
	bool viewGat = false;
	bool viewGatGat = true;
	float gatOpacity = 0.5f;
	bool viewFog = false;

	bool viewModels = true;
	bool viewEffects = true;
	bool viewSounds = true;
	bool viewLights = true;
	bool viewWater = true;
	bool viewEffectIcons = true;

	void focusSelection();
	void drawLight(Node* n);
};