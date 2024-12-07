#pragma once

#include <thread>
#include <vector>
#include <string>
#include <glm/glm.hpp>

class Map;
class BrowEdit;
class Gnd;
class Rsw;
class Node;
class RswModel;
class RswModelCollider;
class RswObject;
class RswLight;
namespace math { class Ray; }
class Lightmapper
{
public:
	// For faster lookup, since using GetComponent gets slow if there are many models
	struct light_model {
		Node* node;
		RswModel* rswModel;
		RswModelCollider* collider;
	};

	struct light_quad_node {
		std::vector<light_model> models;
		glm::vec2 range[2];
	};

	// For faster lookup, since using GetComponent gets slow if there are many lights
	struct light_data {
		Node* light;
		RswObject* rswObject;
		RswLight* rswLight;
	};

private:
	BrowEdit* browEdit;
	Gnd* gnd;
	Rsw* rsw;
	std::vector<std::vector<struct Lightmapper::light_quad_node>> quadtree;
	std::vector<struct Lightmapper::light_data> lights;
	std::vector<struct Lightmapper::light_model> models;
	glm::vec3 lightDirection;

	std::thread mainThread;
public:
	Map* map;
	bool buildDebugPoints = false;
	bool running = true;

	Lightmapper(Map* map, BrowEdit* browEdit);
	void begin();
private:
	void run();
	void onDone();

	bool collidesMap(const math::Ray& ray, int cx, int cy, float maxDistance);
	std::pair<glm::vec3, int> calculateLight(const glm::vec3& groundPos, const glm::vec3& normal, int cx, int cy);
	void calcPos(int direction, int tileId, int x, int y);

	void setProgressText(const std::string& text);
	void setProgress(float);

};

