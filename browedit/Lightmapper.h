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
namespace math { class Ray; }
class Lightmapper
{
	BrowEdit* browEdit;
	Gnd* gnd;
	Rsw* rsw;
	std::vector<Node*> lights;
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

	bool collidesMap(const math::Ray& ray, float maxDistance);
	std::pair<glm::vec3, int> calculateLight(const glm::vec3& groundPos, const glm::vec3& normal);
	void calcPos(int direction, int tileId, int x, int y);

	void setProgressText(const std::string& text);
	void setProgress(float);

};

