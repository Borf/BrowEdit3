#include "Gat.h"
#include <browedit/util/Util.h>
#include <browedit/util/FileIO.h>
#include <browedit/math/AABB.h>
#include <browedit/Node.h>
#include <browedit/Map.h>
#include <browedit/components/GndRenderer.h>
#include <browedit/actions/CubeHeightChangeAction.h>
#include <browedit/actions/CubeTileChangeAction.h>
#include <iostream>
#include <fstream>
#include <FastNoiseLite.h>
#include <glm/gtc/type_ptr.hpp>

Gat::Gat(const std::string& fileName)
{
	auto file = util::FileIO::open(fileName);
	if (!file)
	{
		width = 0;
		height = 0;
		std::cerr << "GAT: Unable to open gat file: " << fileName<< std::endl;
		return;
	}
	std::cout<< "GAT: Reading gat file" << std::endl;
	char header[4];
	file->read(header, 4);
	if (!(header[0] == 'G' && header[1] == 'R' && header[2] == 'A' && header[3] == 'T'))
	{
		version = 0;
		std::cerr << "GAT: Invalid GAT file, attempting to load" << std::endl;
		return;
	}

	file->read(reinterpret_cast<char*>(&version), sizeof(short));
	file->read(reinterpret_cast<char*>(&width), sizeof(int));
	file->read(reinterpret_cast<char*>(&height), sizeof(int));

	cubes.resize(width, std::vector<Cube*>(height, NULL));
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			Cube* cube = new Cube();
			file->read(reinterpret_cast<char*>(&cube->h1), sizeof(float));
			file->read(reinterpret_cast<char*>(&cube->h2), sizeof(float));
			file->read(reinterpret_cast<char*>(&cube->h3), sizeof(float));
			file->read(reinterpret_cast<char*>(&cube->h4), sizeof(float));
			file->read(reinterpret_cast<char*>(&cube->gatType), sizeof(int));
			cube->calcNormal();
			cubes[x][y] = cube;
		}
	}
	delete file;
	std::cout << "GAT: Done reading gat file" << std::endl;
}

Gat::Gat(int width, int height)
{
	this->width = width;
	this->height = height;
	cubes.resize(width, std::vector<Cube*>(height, NULL));
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			Cube* cube = new Cube();
			for (int i = 0; i < 4; i++)
				cube->heights[i] = 0;
			cube->gatType = 0;
			cube->calcNormal();
			cubes[x][y] = cube;
		}
	}
}

Gat::~Gat()
{
	for (auto r : cubes)
		for (auto c : r)
			delete c;
}

void Gat::save(const std::string& fileName)
{
	std::ofstream file(fileName.c_str(), std::ios_base::binary | std::ios_base::out);
	if (!file.is_open())
	{
		std::cerr << "GAT: Unable to open gat file: " << fileName << std::endl;
		return;
	}
	std::cout << "GAT: writing gat file" << std::endl;
	char header[4] = { 'G', 'R', 'A', 'T'};
	file.write(header, 4);
	file.write(reinterpret_cast<char*>(&version), sizeof(short));
	file.write(reinterpret_cast<char*>(&width), sizeof(int));
	file.write(reinterpret_cast<char*>(&height), sizeof(int));
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			Cube* cube = cubes[x][y];
			file.write(reinterpret_cast<char*>(&cube->h1), sizeof(float));
			file.write(reinterpret_cast<char*>(&cube->h2), sizeof(float));
			file.write(reinterpret_cast<char*>(&cube->h3), sizeof(float));
			file.write(reinterpret_cast<char*>(&cube->h4), sizeof(float));
			file.write(reinterpret_cast<char*>(&cube->gatType), sizeof(int));
		}
	}
	std::cout << "GAT: Done saving gat" << std::endl;
}

void Gat::Cube::calcNormal()
{
	/*
		1----2
		|\   |
		| \  |
		|  \ |
		|   \|
		3----4
	*/
	glm::vec3 v1(10, -h1, 0);
	glm::vec3 v2(0, -h2, 0);
	glm::vec3 v3(10, -h3, 10);
	glm::vec3 v4(0, -h4, 10);

	glm::vec3 normal1 = glm::normalize(glm::cross(v4 - v3, v1 - v3));
	glm::vec3 normal2 = glm::normalize(glm::cross(v1 - v2, v4 - v2));
	normal = glm::normalize(normal1 + normal2);
}


glm::vec3 Gat::rayCast(const math::Ray& ray, int xMin, int yMin, int xMax, int yMax, float rayOffset)
{
	if (cubes.size() == 0)
		return glm::vec3(std::numeric_limits<float>().max());

	if (xMax == -1)
		xMax = (int)cubes.size();
	if (yMax == -1)
		yMax = (int)cubes[0].size();

	xMin = glm::max(0, xMin);
	yMin = glm::max(0, yMin);
	xMax = glm::min(xMax, (int)cubes.size());
	yMax = glm::min(yMax, (int)cubes[0].size());

	const int chunkSize = 10;


	std::vector<glm::vec3> collisions;
	float f = 0;
	for (auto xx = xMin; xx < xMax; xx+= chunkSize)
	{
		for (auto yy = yMin; yy < yMax; yy+= chunkSize)
		{
			math::AABB box(glm::vec3(5*(xx-1), -999999, 5*height - 5*(yy+chunkSize+1)), glm::vec3(5*(xx + chunkSize+1), 999999, 5*height - (5 * (yy-1))));
			if (!box.hasRayCollision(ray, -999999, 9999999))
				continue;
			for (int x = xx; x < glm::min(width, xx + chunkSize); x++)
			{
				for (int y = yy; y < glm::min(height, yy + chunkSize); y++)
				{
					auto cube = cubes[x][y];

					glm::vec3 v1(5 * x, -cube->h3, 5 * height - 5 * y + 5);
					glm::vec3 v2(5 * x + 5, -cube->h4, 5 * height - 5 * y + 5);
					glm::vec3 v3(5 * x, -cube->h1, 5 * height - 5 * y + 10);
					glm::vec3 v4(5 * x + 5, -cube->h2, 5 * height - 5 * y + 10);

					{
						std::vector<glm::vec3> v{ v4, v2, v1 };
						if (ray.LineIntersectPolygon(v, f))
							if(f >= rayOffset)
								collisions.push_back(ray.origin + f * ray.dir);
					}
					{
						std::vector<glm::vec3> v{ v4, v1, v3 };
						if (ray.LineIntersectPolygon(v, f))
							if (f >= rayOffset)
								collisions.push_back(ray.origin + f * ray.dir);
					}

				}
			}

		}
	}
	if(collisions.size() == 0)
		return glm::vec3(std::numeric_limits<float>().max());

	std::sort(collisions.begin(), collisions.end(), [&ray](const glm::vec3& a, const glm::vec3& b) {
		return glm::distance(a, ray.origin) < glm::distance(b, ray.origin);
		});
	return collisions[0];
}


/*void Gat::flattenTiles(Map* map, BrowEdit* browEdit, const std::vector<glm::ivec2>& tiles)
{
	CubeHeightChangeAction* action = new CubeHeightChangeAction(this, tiles);
	float avg = 0;
	for (auto& t : map->tileSelection)
		for (int i = 0; i < 4; i++)
			avg += cubes[t.x][t.y]->heights[i];
	avg /= map->tileSelection.size() * 4;
	for (auto& t : map->tileSelection)
		for (int i = 0; i < 4; i++)
			cubes[t.x][t.y]->heights[i] = avg;
	action->setNewHeights(this, tiles);
	map->doAction(action, browEdit);
	node->getComponent<GndRenderer>()->setChunksDirty();
}

void Gat::smoothTiles(Map* map, BrowEdit* browEdit, const std::vector<glm::ivec2>& tiles, int axis)
{
	CubeHeightChangeAction* action = new CubeHeightChangeAction(this, tiles);
	glm::ivec2 offsets[] = {glm::ivec2(0, 0), glm::ivec2(1, 0) ,glm::ivec2(0, 1), glm::ivec2(1, 1)};


	auto gndRenderer = node->getComponent<GndRenderer>();
	std::vector<std::vector<std::pair<float,int>>> heights;
	heights.resize(width + 1, std::vector<std::pair<float, int>>());
	for (int x = 0; x <= width; x++)
		heights[x].resize(height + 1);
	for (int x = 0; x < width; x++)
		for (int y = 0; y < height; y++)
			for (auto i = 0; i < 4; i++)
			{
				if (std::find(tiles.begin(), tiles.end(), glm::ivec2(x, y)) == tiles.end())
					continue;

				heights[x + offsets[i].x][y + offsets[i].y].first += cubes[x][y]->heights[i];
				heights[x + offsets[i].x][y + offsets[i].y].second++;
			}


	for (auto t : tiles)
	{
		for(int i = 0; i < 4; i++)
		{
			cubes[t.x][t.y]->heights[i] = 0;
			int count = 0;
			for (int x = -1; x <= 1; x++)
			{
				for (int y = -1; y <= 1; y++)
				{
					if ((axis & 1) == 0 && x != 0)
						continue;
					if ((axis & 2) == 0 && y != 0)
						continue;
					if (t.x + x + offsets[i].x >= 0 && t.x + x +offsets[i].x <= width && t.y + y + offsets[i].y >= 0 && t.y + y + offsets[i].y <= height)
					{
						//if (std::find(tiles.begin(), tiles.end(), glm::ivec2(t.x + x + offsets[i].x, t.y + y + offsets[i].y)) == tiles.end())
						//	continue;

						if (heights[t.x + x + offsets[i].x][t.y + y + offsets[i].y].second > 0)
						{
							cubes[t.x][t.y]->heights[i] += heights[t.x + x + offsets[i].x][t.y + y + offsets[i].y].first / heights[t.x + x + offsets[i].x][t.y + y + offsets[i].y].second;
							count++;
						}
					}
				}
			}
			if(count > 0)
				cubes[t.x][t.y]->heights[i] /= count;
		}
	}
	node->getComponent<GndRenderer>()->setChunksDirty();
	action->setNewHeights(this, tiles);
	map->doAction(action, browEdit);
	node->getComponent<GndRenderer>()->setChunksDirty();
}

void Gat::addRandomHeight(Map* map, BrowEdit* browEdit, const std::vector<glm::ivec2>& tiles, float min, float max)
{
	CubeHeightChangeAction* action = new CubeHeightChangeAction(this, tiles);
	auto gndRenderer = node->getComponent<GndRenderer>();
	for (auto tile : tiles)
	{
		for (int i = 0; i < 4; i++)
			cubes[tile.x][tile.y]->heights[i] -= min + (rand() / (RAND_MAX / (max-min)));
		gndRenderer->setChunkDirty(tile.x, tile.y);
	}
	action->setNewHeights(this, tiles);
	map->doAction(action, browEdit);
}

void Gat::connectHigh(Map* map, BrowEdit* browEdit, const std::vector<glm::ivec2>& tiles)
{
	CubeHeightChangeAction* action = new CubeHeightChangeAction(this, tiles);
	auto gndRenderer = node->getComponent<GndRenderer>();
	for (auto t : tiles)
		for (int i = 0; i < 4; i++)
		{
			float h = 9999999;
			for (int ii = 0; ii < 4; ii++)
				if(inMap(glm::ivec2(t.x + connectInfo[i][ii].x, t.y + connectInfo[i][ii].y)))
					h = glm::min(h, cubes[t.x + connectInfo[i][ii].x][t.y + connectInfo[i][ii].y]->heights[connectInfo[i][ii].z]);
			cubes[t.x][t.y]->heights[connectInfo[i][0].z] = h;
			gndRenderer->setChunkDirty(t.x, t.y);
		}
	action->setNewHeights(this, tiles);
	map->doAction(action, browEdit);
}

void Gat::connectLow(Map* map, BrowEdit* browEdit, const std::vector<glm::ivec2>& tiles)
{
	CubeHeightChangeAction* action = new CubeHeightChangeAction(this, tiles);
	auto gndRenderer = node->getComponent<GndRenderer>();
	for (auto t : tiles)
		for (int i = 0; i < 4; i++)
		{
			float h = -9999999;
			for (int ii = 0; ii < 4; ii++)
				if (inMap(glm::ivec2(t.x + connectInfo[i][ii].x, t.y + connectInfo[i][ii].y)))
					h = glm::max(h, cubes[t.x + connectInfo[i][ii].x][t.y + connectInfo[i][ii].y]->heights[connectInfo[i][ii].z]);
			cubes[t.x][t.y]->heights[connectInfo[i][0].z] = h;
			gndRenderer->setChunkDirty(t.x, t.y);
		}
	action->setNewHeights(this, tiles);
	map->doAction(action, browEdit);
}


*/



bool Gat::Cube::sameHeight(const Cube& other) const
{
	for (int i = 0; i < 4; i++)
		if (glm::abs(heights[i] - heights[0]) > 0.01f || glm::abs(other.heights[i] - heights[0]) > 0.01f)
			return false;
	return true;
}



std::vector<glm::vec3> Gat::getMapQuads()
{
	std::vector<glm::vec3> quads;

	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			auto cube = cubes[x][y];
			quads.push_back(glm::vec3((x + 0) * 5, -cube->heights[2], 5 * height - (y + 1) * 5 + 5));//1
			quads.push_back(glm::vec3((x + 0) * 5, -cube->heights[0], 5 * height - (y + 0) * 5 + 5));//2
			quads.push_back(glm::vec3((x + 1) * 5, -cube->heights[1], 5 * height - (y + 0) * 5 + 5));//3
			quads.push_back(glm::vec3((x + 1) * 5, -cube->heights[3], 5 * height - (y + 1) * 5 + 5));//4
		}
	}
	return quads;
}


glm::vec3 Gat::getPos(int x, int y, int index, float fac)
{
	auto cube = cubes[x][y];
	if (index == 0)//5 * x										5 * gat->height - 5 * y + 10
		return glm::vec3((x * 5) + fac,			-cube->heights[0], 5 * height - 5 * y + 10 - fac);//2
	if (index == 1)
		return glm::vec3((5 * x + 5) - fac,		-cube->heights[1], 5 * height - 5 * y + 10 - fac);//3
	if(index == 2)
		return glm::vec3((x * 5) + fac,			-cube->heights[2], 5 * height - 5 * y + 5 + fac);//1
	if(index == 3)
		return glm::vec3((5 * x + 5) - fac,		-cube->heights[3], 5 * height - 5 * y + 5 + fac);//4

	return glm::vec3(0, 0,0);
}