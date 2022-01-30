#pragma once

#include <browedit/gl/Shader.h>
#include <browedit/NodeRenderer.h>
#include <browedit/math/Ray.h>
#include <browedit/math/Plane.h>
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
public:
	Map* map = nullptr;
	int id;
	std::string viewName;

	NodeRenderContext nodeRenderContext;
	Gadget gadget;

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


	math::Ray mouseRay;

	math::Plane mouseDragPlane;
	glm::vec3 mouseDragStart;
	glm::vec2 mouseDragStart2D;


	MouseState mouseState;
	MouseState prevMouseState;

	MapView(Map* map, const std::string &viewName);
	MapView(const Map& other) = delete;
	MapView() = delete;


	void update(BrowEdit* browEdit, const ImVec2& size);
	void updateObjectMode(BrowEdit* browEdit);
	void render(BrowEdit* browEdit);
	void postRenderObjectMode(BrowEdit* browEdit);


	bool viewLightmapShadow = true;
	bool viewLightmapColor = true;
	bool viewColors = true;
	bool viewLighting = true;
	bool smoothColors = true;
};