#include <Windows.h>
#include "Lightmapper.h"
#include <browedit/BrowEdit.h>
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/Image.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/GndRenderer.h>
#include <browedit/components/Rsw.h>
#include <browedit/math/Ray.h>
#include <browedit/util/ResourceManager.h>

#include <glm/glm.hpp>
#include <iostream>
#include <imgui_internal.h>

extern std::mutex debugPointMutex;
extern std::vector<std::vector<glm::vec3>> debugPoints;
double startTime;


Lightmapper::Lightmapper(Map* map, BrowEdit* browEdit) : map(map), browEdit(browEdit)
{
	auto rsw = map->rootNode->getComponent<Rsw>();
	auto gnd = map->rootNode->getComponent<Gnd>();

	rsw->lightmapSettings.rangeX = glm::ivec2(0, gnd->width);
	rsw->lightmapSettings.rangeY = glm::ivec2(0, gnd->height);
}

void Lightmapper::begin()
{
	startTime = ImGui::GetTime();
	if (buildDebugPoints)
	{
		debugPoints.clear();
		debugPoints.resize(2);
	}
	if (browEdit->windowData.progressWindowVisible)
		return;

	gnd = map->rootNode->getComponent<Gnd>();
	rsw = map->rootNode->getComponent<Rsw>();

	setProgressText("Cleaning tiles");
	gnd->cleanTiles();
	setProgressText("Making lightmaps unique");
	std::cout << "Before:\t" << gnd->tiles.size() << " tiles, " << gnd->lightmaps.size() << " lightmaps" << std::endl;
	gnd->makeLightmapsUnique();
	std::cout << "After:\t" << gnd->tiles.size() << " tiles, " << gnd->lightmaps.size() << " lightmaps" << std::endl;
	map->rootNode->getComponent<GndRenderer>()->setChunksDirty();

	map->rootNode->getComponent<GndRenderer>()->gndShadowDirty = true;
	for (auto& cc : map->rootNode->getComponent<GndRenderer>()->chunks)
		for (auto c : cc)
			c->dirty = true;

	browEdit->windowData.progressWindowVisible = true;
	browEdit->windowData.progressWindowProgres = 0;
	browEdit->windowData.progressWindowText = "Calculating lightmaps";
	browEdit->windowData.progressCancel = [this]()
	{
		running = false;
	};
	mainThread = std::thread(&Lightmapper::run, this);
	browEdit->windowData.progressWindowOnDone = [&]()
	{
		onDone();
		auto be = browEdit;
		be->lightmapper->mainThread.join();
		delete be->lightmapper;
		be->lightmapper = nullptr;
		std::cout << "Lightmapper: took " + std::to_string(ImGui::GetTime() - startTime) + " seconds" << std::endl;
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
	setProgressText("Gathering Lights");
	lights.clear();
	models.clear();
	map->rootNode->traverse([&](Node* n) {
		if (n->getComponent<RswLight>())
		lights.push_back(n);
		if (RswModel* m = n->getComponent<RswModel>())
			if(m->shadowStrength > 0)
				models.push_back(n);
	});
	auto rsw = map->rootNode->getComponent<Rsw>();
	auto gnd = map->rootNode->getComponent<Gnd>();

	std::cout << "Lightmapper: Complexity " << rsw->lightmapSettings.quality << "*"<< rsw->lightmapSettings.quality<<"*" << gnd->width << "*" << gnd->height << "*" << lights.size() << "*" << models.size() << "=" << rsw->lightmapSettings.quality * rsw->lightmapSettings.quality * gnd->width * gnd->height * lights.size() * models.size() << std::endl;

	auto& settings = rsw->lightmapSettings;


	lightDirection[0] = -glm::cos(glm::radians((float)rsw->light.longitude)) * glm::sin(glm::radians((float)rsw->light.latitude));
	lightDirection[1] = glm::cos(glm::radians((float)rsw->light.latitude));
	lightDirection[2] = glm::sin(glm::radians((float)rsw->light.longitude)) * glm::sin(glm::radians((float)rsw->light.latitude));

	//setProgressText("Calculating map quads");
	//mapQuads = gnd->getMapQuads();

	/*if (buildDebugPoints)
	{
		debugPointMutex.lock();
		for (auto i = 0; i < mapQuads.size(); i += 4)
		{
			glm::vec3 v1 = mapQuads[i];
			glm::vec3 v2 = mapQuads[i + 1];
			glm::vec3 v3 = mapQuads[i + 2];
			glm::vec3 v4 = mapQuads[i + 3];

			for (float x = 0; x < 1; x += 0.05f)
			{
				for (float y = 0; y < 1; y += 0.05f)
				{
					glm::vec3 p1 = glm::mix(v1, v2, x);
					glm::vec3 p2 = glm::mix(v4, v3, x);
					debugPoints[0].push_back(glm::mix(p1, p2, y));
				}
			}

		}
		debugPointMutex.unlock();
	}*/


	setProgressText("Calculating lightmaps");
	std::vector<std::thread> threads;
	std::atomic<int> finishedX(settings.rangeX[0]);
	std::atomic<int> finishedThreadCount(0);
	std::mutex progressMutex;


	threads.push_back(std::thread([&]()
	{
			int timer = 0;
			while ((finishedX < settings.rangeX[1] || finishedThreadCount < browEdit->config.lightmapperThreadCount) && running)
			{
				std::this_thread::sleep_for(std::chrono::seconds(1));
				timer++;
				if (timer > browEdit->config.lightmapperRefreshTimer)
				{
					progressMutex.lock();
					map->rootNode->getComponent<GndRenderer>()->gndShadowDirty = true;
					progressMutex.unlock();
					timer = 0;
				}
			}
			std::cout << "Finished threads: " << finishedThreadCount << std::endl;
			std::cout << "Done with threads, unlocking main thread!" << std::endl;
			progressMutex.lock();
			browEdit->windowData.progressWindowProgres = 1.0f;
			browEdit->windowData.progressWindowVisible = false;
			progressMutex.unlock();
		}));
	for (int t = 0; t < browEdit->config.lightmapperThreadCount; t++)
	{
		threads.push_back(std::thread([&, t]()
			{
				for (int x; (x = finishedX++) < settings.rangeX[1] && running;)
				{
					std::cout << "Row " << x << std::endl;

					progressMutex.lock();
					browEdit->windowData.progressWindowProgres = x / (float)gnd->width;
					progressMutex.unlock();

					for (int y = settings.rangeY[0]; y < settings.rangeY[1]; y++)
					{
						if (settings.heightSelectionOnly && std::find(map->tileSelection.begin(), map->tileSelection.end(), glm::ivec2(x, y)) == map->tileSelection.end())
							continue;
						Gnd::Cube* cube = gnd->cubes[x][y];

						for (int i = 0; i < 3; i++)
							if (cube->tileIds[i] != -1)
								calcPos(i, cube->tileIds[i], x, y);
					}
				}
				std::cout << "Thread " << t << " finished" << std::endl;
				finishedThreadCount++;
			}));
	}





	for (auto& t : threads)
		t.join();

}

bool Lightmapper::collidesMap(const math::Ray& ray, float maxDistance)
{
	auto point = gnd->rayCast(ray, false, 0, 0, -1, -1, 0.05f);
	return point != glm::vec3(std::numeric_limits<float>().max()) && glm::distance(point, ray.origin) < maxDistance;
};


std::pair<glm::vec3, int> Lightmapper::calculateLight(const glm::vec3& groundPos, const glm::vec3& normal)
{
	int intensity = 0;
	glm::vec3 colorInc(0.0f);
	auto rsw = map->rootNode->getComponent<Rsw>();
	auto& settings = rsw->lightmapSettings;

	if (rsw->light.lightmapAmbient > 0)
		intensity = (int)(rsw->light.lightmapAmbient * 255);

	for (auto light : lights)
	{
		auto rswObject = light->getComponent<RswObject>();
		auto rswLight = light->getComponent<RswLight>();
		if (!rswLight->enabled)
			continue;
		glm::vec3 lightPosition(5 * gnd->width + rswObject->position.x, -rswObject->position.y, 5 * gnd->height - rswObject->position.z+10);
		glm::vec3 lightDirection2 = glm::normalize(lightPosition - groundPos);
		if (rswLight->lightType == RswLight::Type::Sun && !rswLight->sunMatchRswDirection)
			lightDirection2 = rswLight->direction; //TODO: should this be -direction?
		else if (rswLight->lightType == RswLight::Type::Sun && rswLight->sunMatchRswDirection)
			lightDirection2 = lightDirection;
		auto dotproduct = glm::dot(normal, lightDirection2);

		if (dotproduct <= 0)
			continue;

		float distance = glm::distance(lightPosition, groundPos);
		float attenuation = 0;
		if (rswLight->lightType != RswLight::Type::Sun)
		{
			if (rswLight->falloffStyle == RswLight::FalloffStyle::Magic)
			{
				if (distance > rswLight->realRange())
					continue;

				float d = glm::max(distance - rswLight->range, 0.0f);
				float denom = d / rswLight->range + 1;
				attenuation = rswLight->intensity / (denom * denom);
				if (rswLight->cutOff > 0)
					attenuation = glm::max(0.0f, (attenuation - rswLight->cutOff) / (1 - rswLight->cutOff));
			}
			else
			{
				if (distance > rswLight->range)
					continue;
				float d = distance / rswLight->range;
				if (rswLight->falloffStyle == RswLight::FalloffStyle::SplineTweak)
					attenuation = glm::clamp(util::interpolateSpline(rswLight->falloff, d), 0.0f, 1.0f) * 255.0f;
				else if (rswLight->falloffStyle == RswLight::FalloffStyle::LagrangeTweak)
					attenuation = glm::clamp(util::interpolateLagrange(rswLight->falloff, d), 0.0f, 1.0f) * 255.0f;
				else if (rswLight->falloffStyle == RswLight::FalloffStyle::LinearTweak)
					attenuation = glm::clamp(util::interpolateLinear(rswLight->falloff, d), 0.0f, 1.0f) * 255.0f;
				else if (rswLight->falloffStyle == RswLight::FalloffStyle::Exponential)
					attenuation = glm::clamp((1 - glm::pow(d, rswLight->cutOff)), 0.0f, 1.0f) * 255.0f;
			}
			if (rswLight->lightType == RswLight::Type::Spot)
			{
				float dp = glm::dot(lightDirection2, -rswLight->direction);
				if (dp < (1-rswLight->spotlightWidth))
					attenuation = 0;
				else
				{
					float fac = 1-((1-glm::abs(dp)) / rswLight->spotlightWidth);
					attenuation *= fac;
				}
			}
		}
		else if (rswLight->lightType == RswLight::Type::Sun)
		{
			attenuation = 255;
		}

		bool collides = false;
		float shadowStrength = 0.0f;
		if (settings.shadows)
		{
			math::Ray ray(groundPos, lightDirection2);
			if (rswLight->givesShadow && attenuation > 0)
			{
				for(auto& n : models) {
					if (collides && shadowStrength >= 1)
						continue;
					auto rswModel = n->getComponent<RswModel>();
					auto collider = n->getComponent<RswModelCollider>();
					if (collider->collidesTexture(ray, 0, distance- rswLight->minShadowDistance))
					{
						collides = true;
						shadowStrength += rswModel->shadowStrength;
					}
				}
			}
			if (!collides && shadowStrength < 1 && collidesMap(math::Ray(groundPos, lightDirection2), distance))
			{
				collides = true;
				shadowStrength = 1;
			}
		}
		if (shadowStrength > 1)
			shadowStrength = 1;
		if (shadowStrength <= 1)
		{
			if (rswLight->diffuseLighting)
				attenuation *= dotproduct;

			if (rswLight->affectShadowMap)
				intensity += (int)((1-shadowStrength) * attenuation * rswLight->intensity);
			if (rswLight->affectLightmap)
				colorInc += (1-shadowStrength) * (attenuation / 255.0f) * rswLight->color * rswLight->intensity;
		}
	}
	return std::pair<glm::vec3, int>(colorInc, intensity);
};


void TriangleBarycentricCoords(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c, const glm::vec2& p, float& out_u, float& out_v, float& out_w)
{
	glm::vec2 v0 = b - a;
	glm::vec2 v1 = c - a;
	glm::vec2 v2 = p - a;
	const float denom = v0.x * v1.y - v1.x * v0.y;
	out_v = (v2.x * v1.y - v1.x * v2.y) / denom;
	out_w = (v0.x * v2.y - v2.x * v0.y) / denom;
	out_u = 1.0f - out_v - out_w;
}

bool TriangleContainsPoint(const glm::vec2 &a, const glm::vec2& b, const glm::vec2& c, const glm::vec2& p)
{
	bool b1 = ((p.x - b.x) * (a.y - b.y) - (p.y - b.y) * (a.x - b.x)) <= 0.0f;
	bool b2 = ((p.x - c.x) * (b.y - c.y) - (p.y - c.y) * (b.x - c.x)) <= 0.0f;
	bool b3 = ((p.x - a.x) * (c.y - a.y) - (p.y - a.y) * (c.x - a.x)) <= 0.0f;
	return ((b1 == b2) && (b2 == b3));
}


void Lightmapper::calcPos(int direction, int tileId, int x, int y)
{
	auto rsw = map->rootNode->getComponent<Rsw>();
	auto& settings = rsw->lightmapSettings;

	float qualityStep = 1.0f / settings.quality;
	int height = gnd->height;
	const float sx = 10.0f / (gnd->lightmapWidth - 2);
	const float sy = 10.0f / (gnd->lightmapHeight - 2);


	Gnd::Tile* tile = gnd->tiles[tileId];
	assert(tile && tile->lightmapIndex != -1);
	Gnd::Lightmap* lightmap = gnd->lightmaps[tile->lightmapIndex];

	Gnd::Cube* cube = gnd->cubes[x][y];

	for (int xx = 1; xx < gnd->lightmapWidth - 1; xx++)
	{
		for (int yy = 1; yy < gnd->lightmapHeight - 1; yy++)
		{
			glm::vec3 totalColor(0.0f);
			int totalIntensity = 0;
			int count = 0;
			for (float xxx = 0; xxx < 1; xxx += qualityStep)
			{
				for (float yyy = 0; yyy < 1; yyy += qualityStep)
				{
					if (!running)
						return;
					glm::vec3 groundPos(0,0,0);
					glm::vec3 normal(0,1,0);

					if (direction == 0)
					{
						glm::vec2 v1((x + 0) * 10, 10 * height - (y + 1) * 10 + 10);//1 , -cube->heights[2]
						glm::vec2 v2((x + 0) * 10, 10 * height - (y + 0) * 10 + 10);//2 , -cube->heights[0]
						glm::vec2 v3((x + 1) * 10, 10 * height - (y + 0) * 10 + 10);//3 , -cube->heights[1]
						glm::vec2 v4((x + 1) * 10, 10 * height - (y + 1) * 10 + 10);//4 , -cube->heights[3]
						glm::vec2 p(10 * x + sx * ((xx + xxx) - 1), 10 * height + 10 - 10 * y - sy * ((yy + yyy) - 1));

						float h = (v1.y + v2.y + v3.y + v4.y) / 4.0f;

						if (TriangleContainsPoint(v1, v2, v3, p))
						{
							float u, v, w;
							TriangleBarycentricCoords(v1, v2, v3, p, u, v, w);
							h = u * -cube->heights[2] + v * -cube->heights[0] + w * -cube->heights[1];
							normal = u * cube->normals[2] + v * cube->normals[0] + w * cube->normals[1];
						}
						else if (TriangleContainsPoint(v3, v4, v1, p))
						{
							float u, v, w;
							TriangleBarycentricCoords(v3, v4, v1, p, u, v, w);
							h = u * -cube->heights[1] + v * -cube->heights[3] + w * -cube->heights[2];
							normal = u * cube->normals[1] + v * cube->normals[3] + w * cube->normals[2];
						}
						else
						{
							std::cout << "uhoh";
						}
						normal.y *= -1;
						normal = glm::normalize(normal);
						groundPos = glm::vec3(p.x, h, p.y);
					}
					else if (direction == 1 && y < gnd->height - 1) //side
					{
						auto otherCube = gnd->cubes[x][y + 1];
						float h1 = glm::mix(cube->h3, cube->h4, ((xx + xxx) - 1) / 6.0f);
						float h2 = glm::mix(otherCube->h1, otherCube->h2, ((xx + xxx) - 1) / 6.0f);
						float h = glm::mix(h1, h2, ((yy + yyy) - 1) / 6.0f);

						groundPos = glm::vec3(10 * x + sx * ((xx + xxx) - 1), -h, 10 * height - 10 * y);
						normal = glm::vec3(0, 0, 1);

						if (h1 < h2)
							normal = -normal;

					}
					else if (direction == 2 && x < gnd->width - 1) //front
					{
						auto otherCube = gnd->cubes[x + 1][y];
						float h1 = glm::mix(cube->h4, cube->h2, ((xx + xxx) - 1) / 6.0f);
						float h2 = glm::mix(otherCube->h3, otherCube->h1, ((xx + xxx) - 1) / 6.0f);
						float h = glm::mix(h1, h2, ((yy + yyy) - 1) / 6.0f);

						groundPos = glm::vec3(10 * x + 10, -h, 10 * height - 10 * y + sy * ((xx + xxx) - 1));
						normal = glm::vec3(-1, 0, 0);

						if (h1 < h2)
							normal = -normal;
					}
					else
						throw "wtf";
					if (buildDebugPoints)
					{
						debugPointMutex.lock();
						debugPoints[1].push_back(groundPos);
						debugPointMutex.unlock();
					}

					auto light = calculateLight(groundPos, normal);
					totalIntensity += glm::min(255, light.second);
					totalColor += glm::min(glm::vec3(1.0f, 1.0f, 1.0f), light.first);
					count++;
				}
			}

			int intensity = totalIntensity / count;
			if (intensity > 255)
				intensity = 255;
			if (intensity < 0)
				intensity = 0;

			glm::vec3 color = totalColor / (float)count;

			lightmap->data[xx + gnd->lightmapWidth * yy] = intensity;
			lightmap->data[gnd->lightmapOffset() + 3 * (xx + gnd->lightmapWidth * yy) + 0] = glm::min(255, (int)(color.r * 255));
			lightmap->data[gnd->lightmapOffset() + 3 * (xx + gnd->lightmapWidth * yy) + 1] = glm::min(255, (int)(color.g * 255));
			lightmap->data[gnd->lightmapOffset() + 3 * (xx + gnd->lightmapWidth * yy) + 2] = glm::min(255, (int)(color.b * 255));
		}
	}
};



void Lightmapper::onDone()
{
	std::cout << "Done!" << std::endl;
	gnd->makeLightmapBorders(browEdit);
	gnd->cleanLightmaps();
	gnd->cleanTiles();
	map->rootNode->getComponent<GndRenderer>()->gndShadowDirty = true;
	map->rootNode->getComponent<GndRenderer>()->setChunksDirty();
	util::ResourceManager<Image>::clear();
}