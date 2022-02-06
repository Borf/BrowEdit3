#include "Lightmapper.h"
#include <browedit/BrowEdit.h>
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/GndRenderer.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/Rsw.h>
#include <browedit/math/Ray.h>

#include <glm/glm.hpp>
#include <iostream>

Lightmapper::Lightmapper(Map* map, BrowEdit* browEdit) : map(map), browEdit(browEdit)
{
}

void Lightmapper::begin()
{
	if (browEdit->windowData.progressWindowVisible)
		return;
	browEdit->windowData.progressWindowVisible = true;
	browEdit->windowData.progressWindowProgres = 0;
	browEdit->windowData.progressWindowText = "Calculating lightmaps";
	browEdit->windowData.progressWindowOnDone = std::bind(&Lightmapper::onDone, this);
	mainThread = std::thread(&Lightmapper::run, this);
	browEdit->windowData.progressWindowOnDone = [&]()
	{
		auto be = browEdit;
		be->lightmapper->mainThread.join();
		delete be->lightmapper;
		be->lightmapper = nullptr;
	};
}

void Lightmapper::setProgressText(const std::string& text)
{
	std::lock_guard<std::mutex> guard(browEdit->windowData.progressMutex);
	browEdit->windowData.progressWindowText = text;
}
void Lightmapper::setProgress(float progress)
{
	std::lock_guard<std::mutex> guard(browEdit->windowData.progressMutex);
	browEdit->windowData.progressWindowProgres = progress;
	if(progress == 1)
		browEdit->windowData.progressWindowVisible = false;
}

void Lightmapper::run()
{
	gnd = map->rootNode->getComponent<Gnd>();
	rsw = map->rootNode->getComponent<Rsw>();

	setProgressText("Making lightmaps unique");
	std::cout << "Before:\t" << gnd->tiles.size() << " tiles, " << gnd->lightmaps.size() << " lightmaps" << std::endl;
	gnd->makeLightmapsUnique();
	std::cout << "After:\t" << gnd->tiles.size() << " tiles, " << gnd->lightmaps.size() << " lightmaps" << std::endl;

	setProgressText("Gathering Lights");
	map->rootNode->traverse([&](Node* n) {
		if (n->getComponent<RswLight>())
			lights.push_back(n);
		});


	lightDirection[0] = -glm::cos(glm::radians((float)rsw->light.longitude)) * glm::sin(glm::radians((float)rsw->light.latitude));
	lightDirection[1] = glm::cos(glm::radians((float)rsw->light.latitude));
	lightDirection[2] = glm::sin(glm::radians((float)rsw->light.longitude)) * glm::sin(glm::radians((float)rsw->light.latitude));

	setProgressText("Calculating map quads");
	mapQuads = gnd->getMapQuads();

	setProgressText("Calculating lightmaps");
	std::vector<std::thread> threads;
	std::atomic<int> finishedX(0);
	std::mutex progressMutex;

	for (int t = 0; t < threadCount; t++)
	{
		threads.push_back(std::thread([&]()
			{
				for (int x; (x = finishedX++) < gnd->width;)
				{
					std::cout << "Row " << x << std::endl;

					progressMutex.lock();
					browEdit->windowData.progressWindowProgres = x / (float)gnd->width;
					progressMutex.unlock();

					for (int y = 0; y < gnd->height; y++)
						//for (int y = 0; y < 10; y++)
					{
						Gnd::Cube* cube = gnd->cubes[x][y];

						for (int i = 0; i < 3; i++)
							if (cube->tileIds[i] != -1)
								calcPos(i, cube->tileIds[i], x, y);
					}
				}
			}));
	}

	threads.push_back(std::thread([&]()
		{
			while (finishedX < gnd->width)
			{
				std::this_thread::sleep_for(std::chrono::seconds(2));
				progressMutex.lock();
				map->rootNode->getComponent<GndRenderer>()->gndShadowDirty = true;
				progressMutex.unlock();
			}
			std::cout << "Done!" << std::endl;
			progressMutex.lock();
			browEdit->windowData.progressWindowProgres = 1.0f;
			browEdit->windowData.progressWindowVisible = false;
			gnd->makeLightmapBorders();
			map->rootNode->getComponent<GndRenderer>()->gndShadowDirty = true;
			progressMutex.unlock();
		}));



	for (auto& t : threads)
		t.join();

	
}

bool Lightmapper::collidesMap(const math::Ray& ray)
{
	static std::vector<glm::vec3> quad;
	quad.resize(4);
	float t;
	for (std::size_t i = 0; i < mapQuads.size(); i += 4)
	{
		for (int ii = 0; ii < 4; ii++)
			quad[ii] = mapQuads[i + ii];
		if (ray.LineIntersectPolygon(quad, t))
			if (t > 0)
				return true;
	}
	return false;
};


int Lightmapper::calculateLight(const glm::vec3& groundPos, const glm::vec3& normal)
{
	int intensity = 0;

	if (rsw->light.lightmapAmbient > 0)
		intensity = (int)(rsw->light.lightmapAmbient * 255);

	//sunlight calculation
	if (rsw->light.lightmapIntensity > 0 && glm::dot(normal, lightDirection) > 0)
	{
		math::Ray ray(groundPos, glm::normalize(lightDirection));
		bool collides = false;
		//check objects
		map->rootNode->traverse([&collides, &ray](Node* n) {
			if (collides)
				return;
			auto rswModel = n->getComponent<RswModel>();
			if (rswModel)
			{
				auto collider = n->getComponent<RswModelCollider>();
				if (collider->getCollisions(ray).size() > 0)
					collides = true;
			}
			});
		//check floor
		//if (!collides && collidesMap(ray)) //TODO: this makes the lightmap not work
		//	collides = true;
		//check walls


		if (!collides)
		{
			intensity += (int)(rsw->light.lightmapIntensity * 255);
		}
	}

	for (auto light : lights)
	{
		auto rswObject = light->getComponent<RswObject>();
		auto rswLight = light->getComponent<RswLight>();

		glm::vec3 lightPosition(5 * gnd->width + rswObject->position.x, -rswObject->position.y, 5 * gnd->height - rswObject->position.z);

		float distance = glm::distance(lightPosition, groundPos);
		if (distance > rswLight->realRange())
			continue;

		float d = glm::max(distance - rswLight->range, 0.0f);
		float denom = d / rswLight->range + 1;
		float attenuation = rswLight->intensity / (denom * denom);
		if (rswLight->cutOff > 0)
			attenuation = glm::max(0.0f, (attenuation - rswLight->cutOff) / (1 - rswLight->cutOff));


		math::Ray ray(groundPos, glm::normalize(lightPosition - groundPos));
		bool collides = false;
		map->rootNode->traverse([&](Node* n) {
			if (collides)
				return;
			auto rswModel = n->getComponent<RswModel>();
			if (rswModel)
			{
				auto collider = n->getComponent<RswModelCollider>();
				if (collider->getCollisions(ray).size() > 0)
					collides = true;
			}
		});
		if (!collides)
		{
			intensity += (int)attenuation;
		}
	}
	return intensity;
};


void Lightmapper::calcPos(int direction, int tileId, int x, int y)
{
	float qualityStep = 1.0f / quality;
	int height = gnd->height;
	const float s = 10 / 6.0f;


	Gnd::Tile* tile = gnd->tiles[tileId];
	assert(tile && tile->lightmapIndex != -1);
	Gnd::Lightmap* lightmap = gnd->lightmaps[tile->lightmapIndex];

	Gnd::Cube* cube = gnd->cubes[x][y];

	for (int xx = 1; xx < 7; xx++)
	{
		for (int yy = 1; yy < 7; yy++)
		{
			int totalIntensity = 0;
			int count = 0;
			for (float xxx = 0; xxx < 1; xxx += qualityStep)
			{
				for (float yyy = 0; yyy < 1; yyy += qualityStep)
				{
					glm::vec3 groundPos;
					glm::vec3 normal;
					//todo: use proper height using barycentric coordinats
					if (direction == 0)
					{
						groundPos = glm::vec3(10 * x + s * ((xx + xxx) - 1), -(cube->h1 + cube->h2 + cube->h3 + cube->h4) / 4.0f, 10 * height + 10 - 10 * y - s * ((yy + yyy) - 1));
						normal = glm::vec3(0, 1, 0);
					}
					else if (direction == 1) //side
					{
						auto otherCube = gnd->cubes[x][y + 1];
						float h1 = glm::mix(cube->h3, cube->h4, ((xx + xxx) - 1) / 6.0f);
						float h2 = glm::mix(otherCube->h2, otherCube->h1, ((xx + xxx) - 1) / 6.0f);
						float h = glm::mix(h1, h2, ((yy + yyy) - 1) / 6.0f);

						groundPos = glm::vec3(10 * x + s * ((xx + xxx) - 1), -h, 10 * height - 10 * y);
						normal = glm::vec3(0, 0, 1);

						if (h1 < h2)
							normal = -normal;
					}
					else if (direction == 2) //front
					{
						auto otherCube = gnd->cubes[x + 1][y];
						float h1 = glm::mix(cube->h2, cube->h4, ((xx + xxx) - 1) / 6.0f);
						float h2 = glm::mix(otherCube->h1, otherCube->h3, ((xx + xxx) - 1) / 6.0f);
						float h = glm::mix(h1, h2, ((yy + yyy) - 1) / 6.0f);

						groundPos = glm::vec3(10 * x, -h, 10 * height + 10 - 10 * y + s * ((xx + xxx) - 1));
						normal = glm::vec3(-1, 0, 0);

						if (h1 < h2)
							normal = -normal;

					}

					totalIntensity += calculateLight(groundPos, normal);
					count++;
				}
			}

			int intensity = totalIntensity / count;
			if (intensity > 255)
				intensity = 255;

			lightmap->data[xx + 8 * yy] = intensity;
			lightmap->data[64 + 3 * (xx + 8 * yy) + 0] = 0;
			lightmap->data[64 + 3 * (xx + 8 * yy) + 1] = 0;
			lightmap->data[64 + 3 * (xx + 8 * yy) + 2] = 0;
		}
	}
};



void Lightmapper::onDone()
{
	std::cout << "Done!" << std::endl;
}