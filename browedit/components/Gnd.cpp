#include "Gnd.h"
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

Gnd::Gnd(const std::string& fileName)
{
	auto file = util::FileIO::open("data\\" + fileName);
	if (!file)
	{
		width = 0;
		height = 0;
		std::cerr << "GND: Unable to open gnd file: " << fileName<< std::endl;
		return;
	}
	std::cout<< "GND: Reading gnd file" << std::endl;
	char header[4];
	file->read(header, 4);
	if (header[0] == 'G' && header[1] == 'R' && header[2] == 'G' && header[3] == 'N')
	{
		file->read(reinterpret_cast<char*>(&version), sizeof(short));
		version = util::swapShort(version);
	}
	else
	{
		version = 0;
		std::cerr<< "GND: Invalid GND file" << std::endl;
		return;
	}

	int textureCount = 0;

	if (version > 0)
	{
		file->read(reinterpret_cast<char*>(&width), sizeof(int));
		file->read(reinterpret_cast<char*>(&height), sizeof(int));
		file->read(reinterpret_cast<char*>(&tileScale), sizeof(float));
		file->read(reinterpret_cast<char*>(&textureCount), sizeof(int));
		file->read(reinterpret_cast<char*>(&maxTexName), sizeof(int)); //80
	}
	else
	{
		file->seekg(6, std::ios_base::cur);//TODO: test this
		file->read(reinterpret_cast<char*>(&width), sizeof(int));
		file->read(reinterpret_cast<char*>(&height), sizeof(int));
		file->read(reinterpret_cast<char*>(&textureCount), sizeof(int));
	}

	textures.reserve(textureCount);
	for (int i = 0; i < textureCount; i++)
	{
		Texture* texture = new Texture();
		texture->file = util::FileIO::readString(file, 40);
		texture->name = util::FileIO::readString(file, 40);
		//texture->texture = new Texture("data/texture/" + texture->file); //TODO: cached loader
		textures.push_back(texture);
	}


	if (version > 0)
	{
		int lightmapCount;
		file->read(reinterpret_cast<char*>(&lightmapCount), sizeof(int));
		file->read(reinterpret_cast<char*>(&lightmapWidth), sizeof(int));
		file->read(reinterpret_cast<char*>(&lightmapHeight), sizeof(int));
		file->read(reinterpret_cast<char*>(&gridSizeCell), sizeof(int));

		//Fix lightmap format if it was invalid. by Henko
		if (lightmapWidth != 8 || lightmapHeight != 8 || gridSizeCell != 1)
		{
			std::cerr << "GND: Invalid Lightmap Format in " << fileName << ".gnd" << std::endl;
			lightmapWidth = 8;
			lightmapHeight = 8;
			gridSizeCell = 1;
		}

		lightmaps.reserve(lightmapCount);
		for (int i = 0; i < lightmapCount; i++)
		{
			Lightmap* lightmap = new Lightmap();
			file->read((char*)lightmap->data, 256);
			lightmaps.push_back(lightmap);
		}

		int tileCount;
		file->read(reinterpret_cast<char*>(&tileCount), sizeof(int));
		std::cout << "GND: Tilecount: " << tileCount << std::endl;
		tiles.reserve(tileCount);
		for (int i = 0; i < tileCount; i++)
		{
			Tile* tile = new Tile();

			file->read(reinterpret_cast<char*>(&tile->v1.x), sizeof(float));
			file->read(reinterpret_cast<char*>(&tile->v2.x), sizeof(float));
			file->read(reinterpret_cast<char*>(&tile->v3.x), sizeof(float));
			file->read(reinterpret_cast<char*>(&tile->v4.x), sizeof(float));
			file->read(reinterpret_cast<char*>(&tile->v1.y), sizeof(float));
			file->read(reinterpret_cast<char*>(&tile->v2.y), sizeof(float));
			file->read(reinterpret_cast<char*>(&tile->v3.y), sizeof(float));
			file->read(reinterpret_cast<char*>(&tile->v4.y), sizeof(float));

			file->read(reinterpret_cast<char*>(&tile->textureIndex), sizeof(short));
			file->read(reinterpret_cast<char*>(&tile->lightmapIndex), sizeof(unsigned short));


			if (tile->lightmapIndex < 0 || tile->lightmapIndex == (unsigned short)-1)
			{
				std::cout << "GND: Lightmapindex < 0" << std::endl;
				tile->lightmapIndex = 0;
			}

			if (tile->textureIndex < 0 || tile->textureIndex >= textureCount)
			{
				std::cout << "GND: TextureIndex < 0" << std::endl;
				tile->textureIndex = 0;
			}


			tile->color.b = (unsigned char)file->get();
			tile->color.g = (unsigned char)file->get();
			tile->color.r = (unsigned char)file->get();
			tile->color.a = (unsigned char)file->get();
			tiles.push_back(tile);
		}

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
				cube->calcNormal();

				

				if (version >= 0x0106)
				{
					file->read(reinterpret_cast<char*>(&cube->tileUp), sizeof(int));
					file->read(reinterpret_cast<char*>(&cube->tileFront), sizeof(int));
					file->read(reinterpret_cast<char*>(&cube->tileSide), sizeof(int));
				}
				else
				{
					unsigned short up, side, front;
					file->read(reinterpret_cast<char*>(&up), sizeof(unsigned short));
					file->read(reinterpret_cast<char*>(&front), sizeof(unsigned short));
					file->read(reinterpret_cast<char*>(&side), sizeof(unsigned short));

					cube->tileUp = up;
					cube->tileSide = side;
					cube->tileFront = front;
				}

				if (cube->tileUp >= (int)tiles.size() || cube->tileUp < -1)
				{
					std::cout << "GND: Wrong value for tileup at " << x << ", " << y << ", Found " << cube->tileUp << ", but only " << tiles.size() << " tiles found" << std::endl;
					cube->tileUp = -1;
				}
				if (cube->tileSide >= (int)tiles.size() || cube->tileSide < -1)
				{
					std::cout << "GND: Wrong value for tileside at " << x << ", " << y << std::endl;
					cube->tileSide = -1;
				}
				if (cube->tileFront >= (int)tiles.size() || cube->tileFront < -1)
				{
					std::cout << "GND: Wrong value for tilefront at " << x << ", " << y << std::endl;
					cube->tileFront = -1;
				}

				cubes[x][y] = cube;
			}
		}
	}
	else
	{
		//TODO: port code...too lazy for now
	}
	delete file;
	std::cout << "GND: Done reading gnd file" << std::endl;

	for (int x = 1; x < width - 1; x++)
		for (int y = 1; y < height - 1; y++)
			cubes[x][y]->calcNormals(this, x, y);
	std::cout << "GND: Done calculating normals" << std::endl;
}


Gnd::~Gnd()
{
	for (auto t : textures)
		delete t;
	for (auto l : lightmaps)
		delete l;
	for (auto t : tiles)
		delete t;
	for (auto r : cubes)
		for (auto c : r)
			delete c;
}

void Gnd::save(const std::string& fileName)
{
	std::ofstream file(fileName.c_str(), std::ios_base::binary | std::ios_base::out);
	if (!file.is_open())
	{
		std::cerr << "GND: Unable to open gnd file: " << fileName << std::endl;
		return;
	}
	std::cout << "GND: Reading gnd file" << std::endl;
	char header[4] = { 'G', 'R', 'G', 'N'};
	file.write(header, 4);
	version = util::swapShort(version);
	file.write(reinterpret_cast<char*>(&version), sizeof(short));
	version = util::swapShort(version);

	int textureCount = (int)textures.size();
	if (version > 0)
	{
		file.write(reinterpret_cast<char*>(&width), sizeof(int));
		file.write(reinterpret_cast<char*>(&height), sizeof(int));
		file.write(reinterpret_cast<char*>(&tileScale), sizeof(float));
		file.write(reinterpret_cast<char*>(&textureCount), sizeof(int));
		file.write(reinterpret_cast<char*>(&maxTexName), sizeof(int)); //80
	}
	else
	{
		file.put(0); file.put(0); file.put(0); file.put(0); file.put(0); file.put(0);
		file.write(reinterpret_cast<char*>(&width), sizeof(int));
		file.write(reinterpret_cast<char*>(&height), sizeof(int));
		file.write(reinterpret_cast<char*>(&textureCount), sizeof(int));
	}

	for (auto texture : textures)
	{
		util::FileIO::writeString(file, texture->file, 40);
		util::FileIO::writeString(file, texture->name, 40);
	}

	if (version > 0)
	{
		int lightmapCount = (int)lightmaps.size();
		file.write(reinterpret_cast<char*>(&lightmapCount), sizeof(int));
		file.write(reinterpret_cast<char*>(&lightmapWidth), sizeof(int));
		file.write(reinterpret_cast<char*>(&lightmapHeight), sizeof(int));
		file.write(reinterpret_cast<char*>(&gridSizeCell), sizeof(int));

		for (auto lightmap : lightmaps)
			file.write((char*)lightmap->data, 256);

		int tileCount = (int)tiles.size();
		file.write(reinterpret_cast<char*>(&tileCount), sizeof(int));
		for (auto tile : tiles)
		{
			file.write(reinterpret_cast<char*>(&tile->v1.x), sizeof(float));
			file.write(reinterpret_cast<char*>(&tile->v2.x), sizeof(float));
			file.write(reinterpret_cast<char*>(&tile->v3.x), sizeof(float));
			file.write(reinterpret_cast<char*>(&tile->v4.x), sizeof(float));
			file.write(reinterpret_cast<char*>(&tile->v1.y), sizeof(float));
			file.write(reinterpret_cast<char*>(&tile->v2.y), sizeof(float));
			file.write(reinterpret_cast<char*>(&tile->v3.y), sizeof(float));
			file.write(reinterpret_cast<char*>(&tile->v4.y), sizeof(float));

			file.write(reinterpret_cast<char*>(&tile->textureIndex), sizeof(short));
			file.write(reinterpret_cast<char*>(&tile->lightmapIndex), sizeof(unsigned short));


			if (tile->lightmapIndex < 0 || tile->lightmapIndex == (unsigned short)-1)
			{
				std::cout << "GND: Lightmapindex < 0" << std::endl;
				tile->lightmapIndex = 0;
			}

			file.put(tile->color.b);
			file.put(tile->color.g);
			file.put(tile->color.r);
			file.put(tile->color.a);
		}

		for (int y = 0; y < height; y++)
		{
			for (int x = 0; x < width; x++)
			{
				Cube* cube = cubes[x][y];
				file.write(reinterpret_cast<char*>(&cube->h1), sizeof(float));
				file.write(reinterpret_cast<char*>(&cube->h2), sizeof(float));
				file.write(reinterpret_cast<char*>(&cube->h3), sizeof(float));
				file.write(reinterpret_cast<char*>(&cube->h4), sizeof(float));

				if (version >= 0x0106)
				{
					file.write(reinterpret_cast<char*>(&cube->tileUp), sizeof(int));
					file.write(reinterpret_cast<char*>(&cube->tileFront), sizeof(int));
					file.write(reinterpret_cast<char*>(&cube->tileSide), sizeof(int));
				}
				else
				{
					unsigned short up, side, front;
					up = cube->tileUp;
					front = cube->tileFront;
					side = cube->tileSide;


					file.write(reinterpret_cast<char*>(&up), sizeof(unsigned short));
					file.write(reinterpret_cast<char*>(&front), sizeof(unsigned short));
					file.write(reinterpret_cast<char*>(&side), sizeof(unsigned short));

				}
			}
		}
	}
	else
	{
		//TODO: port code...too lazy for now
	}
	std::cout << "GND: Done saving GND" << std::endl;
}



glm::vec3 Gnd::rayCast(const math::Ray& ray, bool emptyTiles, int xMin, int yMin, int xMax, int yMax, float rayOffset)
{
	if (cubes.size() == 0)
		return glm::vec3(0,0,0);

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
			math::AABB box(glm::vec3(10*xx, -999999, 10*height - 10*(yy+chunkSize)), glm::vec3(10*(xx + chunkSize), 999999, 10*height - (10 * yy)));
			if (!box.hasRayCollision(ray, -999999, 9999999))
				continue;
			for (int x = xx; x < glm::min(width, xx + chunkSize); x++)
			{
				for (int y = yy; y < glm::min(height, yy + chunkSize); y++)
				{
					Gnd::Cube* cube = cubes[x][y];

					if (cube->tileUp != -1 || emptyTiles)
					{
						glm::vec3 v1(10 * x, -cube->h3, 10 * height - 10 * y);
						glm::vec3 v2(10 * x + 10, -cube->h4, 10 * height - 10 * y);
						glm::vec3 v3(10 * x, -cube->h1, 10 * height - 10 * y + 10);
						glm::vec3 v4(10 * x + 10, -cube->h2, 10 * height - 10 * y + 10);

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
					if (cube->tileSide != -1 && x < width - 1)
					{
						glm::vec3 v1(10 * x + 10, -cube->h2, 10 * height - 10 * y + 10);
						glm::vec3 v2(10 * x + 10, -cube->h4, 10 * height - 10 * y);
						glm::vec3 v3(10 * x + 10, -cubes[x + 1][y]->h1, 10 * height - 10 * y + 10);
						glm::vec3 v4(10 * x + 10, -cubes[x + 1][y]->h3, 10 * height - 10 * y);

						{
							std::vector<glm::vec3> v{ v4, v2, v1 };
							if (ray.LineIntersectPolygon(v, f))
								if (f >= rayOffset)
									collisions.push_back(ray.origin + f * ray.dir);
						}
						{
							std::vector<glm::vec3> v{ v4, v1, v3 };
							if (ray.LineIntersectPolygon(v, f))
								if (f >= rayOffset)
									collisions.push_back(ray.origin + f * ray.dir);
						}
					}
					if (cube->tileFront != -1 && y < height - 1)
					{
						glm::vec3 v1(10 * x, -cube->h3, 10 * height - 10 * y);
						glm::vec3 v2(10 * x + 10, -cube->h4, 10 * height - 10 * y);
						glm::vec3 v4(10 * x + 10, -cubes[x][y + 1]->h2, 10 * height - 10 * y);
						glm::vec3 v3(10 * x, -cubes[x][y + 1]->h1, 10 * height - 10 * y);

						{
							std::vector<glm::vec3> v{ v4, v2, v1 };
							if (ray.LineIntersectPolygon(v, f))
								if (f >= rayOffset)
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
	}
	if(collisions.size() == 0)
		return glm::vec3(0, 0, 0);

	std::sort(collisions.begin(), collisions.end(), [&ray](const glm::vec3& a, const glm::vec3& b) {
		return glm::distance(a, ray.origin) < glm::distance(b, ray.origin);
		});
	return collisions[0];
}


void Gnd::makeLightmapsUnique()
{
	makeTilesUnique();
	std::set<int> taken;
	for (Tile* t : tiles)
	{
		if (t->lightmapIndex != -1 && taken.find(t->lightmapIndex) == taken.end())
			taken.insert(t->lightmapIndex);
		else
		{
			Lightmap* l;
			if(t->lightmapIndex == -1)
				l = new Lightmap();
			else
				l = new Lightmap(*lightmaps[t->lightmapIndex]);
			t->lightmapIndex = (unsigned short)lightmaps.size();
			lightmaps.push_back(l);
		}
	}
	node->getComponent<GndRenderer>()->setChunksDirty();
	node->getComponent<GndRenderer>()->gndShadowDirty = true;

}

void Gnd::makeLightmapsClear()
{
	lightmaps.clear();
	Lightmap* l = new Lightmap();
	lightmaps.push_back(l);

	for (Tile* t : tiles)
		t->lightmapIndex = 0;
	node->getComponent<GndRenderer>()->setChunksDirty();
	node->getComponent<GndRenderer>()->gndShadowDirty = true;

}
void Gnd::makeLightmapBorders()
{
	makeLightmapsUnique();
	std::cout<< "Fixing borders" << std::endl;
	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			Gnd::Cube* cube = cubes[x][y];
			int tileId = cube->tileUp;
			//fix topside
			if (tileId != -1)
			{
				Gnd::Tile* tile = tiles[tileId];
				assert(tile && tile->lightmapIndex != -1);
				Gnd::Lightmap* lightmap = lightmaps[tile->lightmapIndex];

				for (int i = 0; i < 8; i++)
				{
					lightmap->data[i + 8 * 0] = getLightmapBrightness(x, y - 1, i, 6);
					lightmap->data[i + 8 * 7] = getLightmapBrightness(x, y + 1, i, 1);
					lightmap->data[0 + 8 * i] = getLightmapBrightness(x - 1, y, 6, i);
					lightmap->data[7 + 8 * i] = getLightmapBrightness(x + 1, y, 1, i);

					for (int c = 0; c < 3; c++)
					{
						lightmap->data[64 + 3 * (i + 8 * 0) + c] = getLightmapColor(x, y - 1, i, 6)[c];
						lightmap->data[64 + 3 * (i + 8 * 7) + c] = getLightmapColor(x, y + 1, i, 1)[c];
						lightmap->data[64 + 3 * (0 + 8 * i) + c] = getLightmapColor(x - 1, y, 6, i)[c];
						lightmap->data[64 + 3 * (7 + 8 * i) + c] = getLightmapColor(x + 1, y, 1, i)[c];
					}

				}
			}
			tileId = cube->tileSide;
			if (tileId != -1 && x > 0 && x < width-1 && y < height-1)
			{
				Gnd::Tile* tile = tiles[tileId];
				assert(tile && tile->lightmapIndex != -1);
				Gnd::Lightmap* lightmap = lightmaps[tile->lightmapIndex];

				auto otherCube = cubes[x - 1][y];
				if (otherCube && otherCube->tileSide != -1)
				{
					auto otherTile = tiles[otherCube->tileSide];
					auto otherLightmap = lightmaps[otherTile->lightmapIndex];

					for (int i = 0; i < 8; i++)
						lightmap->data[0 + 8 * i] = otherLightmap->data[6 + 8 * i];
					for (int i = 0; i < 8; i++)
						for (int c = 0; c < 3; c++)
							lightmap->data[64 + 3 * (0 + 8 * i) + c] = otherLightmap->data[64+3*(6 + 8 * i)+c];
				}
				else
					for (int i = 0; i < 8; i++)
						lightmap->data[0 + 8 * i] = lightmap->data[1 + 8 * i];

				otherCube = cubes[x + 1][y];
				if (otherCube && otherCube->tileSide != -1)
				{
					auto otherTile = tiles[otherCube->tileSide];
					auto otherLightmap = lightmaps[otherTile->lightmapIndex];

					for (int i = 0; i < 8; i++)
						lightmap->data[7 + 8 * i] = otherLightmap->data[1 + 8 * i];
					for (int i = 0; i < 8; i++)
						for (int c = 0; c < 3; c++)
							lightmap->data[64+3*(7 + 8 * i)+c] = otherLightmap->data[64+3*(1 + 8 * i)+c];
				}
				else
					for (int i = 0; i < 8; i++)
						lightmap->data[7 + 8 * i] = lightmap->data[6 + 8 * i];

				//top and bottom
				otherCube = cubes[x][y + 1];
				if (otherCube && otherCube->tileUp != -1)
				{
					auto otherTile = tiles[otherCube->tileUp];
					auto otherLightmap = lightmaps[otherTile->lightmapIndex];

					for (int i = 0; i < 8; i++)
						lightmap->data[i + 8 * 7] = otherLightmap->data[i + 8 * 1]; //bottom
					for (int i = 0; i < 8; i++)
						for (int c = 0; c < 3; c++)
							lightmap->data[64+3*(i + 8 * 7)+c] = otherLightmap->data[64+3*(i + 8 * 1)+c]; //bottom
				}
				else
					for (int i = 0; i < 8; i++)
						lightmap->data[i + 8 * 7] = lightmap->data[i + 8 * 6];


				otherCube = cubes[x][y];
				if (otherCube && otherCube->tileUp != -1)
				{
					auto otherTile = tiles[otherCube->tileUp];
					auto otherLightmap = lightmaps[otherTile->lightmapIndex];

					for (int i = 0; i < 8; i++)
						lightmap->data[i + 8 * 0] = otherLightmap->data[i + 8 * 1]; //bottom
					for (int i = 0; i < 8; i++)
						for (int c = 0; c < 3; c++)
							lightmap->data[64+3*(i + 8 * 0)+c] = otherLightmap->data[64+3*(i + 8 * 1)+c]; //bottom
				}
				else
					for (int i = 0; i < 8; i++)
						lightmap->data[i + 8 * 0] = lightmap->data[i + 8 * 1];
			}

			tileId = cube->tileFront;
			if (tileId != -1 && y > 0 && y < height - 1 && x < width - 1)
			{
				Gnd::Tile* tile = tiles[tileId];
				assert(tile && tile->lightmapIndex != -1);
				Gnd::Lightmap* lightmap = lightmaps[tile->lightmapIndex];

				if (y < width)
				{
					auto otherCube = cubes[x][y + 1];
					if (otherCube->tileFront != -1)
					{
						auto otherTile = tiles[otherCube->tileFront];
						auto otherLightmap = lightmaps[otherTile->lightmapIndex];

						for (int i = 0; i < 8; i++)
							lightmap->data[0 + 8 * i] = otherLightmap->data[6 + 8 * i];
						for (int i = 0; i < 8; i++)
							for (int c = 0; c < 3; c++)
								lightmap->data[64+3*(0 + 8 * i)+c] = otherLightmap->data[64+3*(6 + 8 * i)+c];
					}
					else
						for (int i = 0; i < 8; i++)
							lightmap->data[0 + 8 * i] = lightmap->data[1 + 8 * i];
				}

				if (y > 0)
				{
					auto otherCube = cubes[x][y - 1];
					if (otherCube && otherCube->tileFront != -1)
					{
						auto otherTile = tiles[otherCube->tileFront];
						auto otherLightmap = lightmaps[otherTile->lightmapIndex];

						for (int i = 0; i < 8; i++)
							lightmap->data[7 + 8 * i] = otherLightmap->data[1 + 8 * i];
						for (int i = 0; i < 8; i++)
							for (int c = 0; c < 3; c++)
								lightmap->data[64+3*(7 + 8 * i)+c] = otherLightmap->data[64+3*(1 + 8 * i)+c];
					}
					else
						for (int i = 0; i < 8; i++)
							lightmap->data[7 + 8 * i] = lightmap->data[6 + 8 * i];
				}

				if (x < width)
				{
					//top and bottom
					auto otherCube = cubes[x + 1][y];
					if (otherCube->tileUp != -1)
					{
						auto otherTile = tiles[otherCube->tileUp];
						auto otherLightmap = lightmaps[otherTile->lightmapIndex];

						for (int i = 0; i < 8; i++)
							lightmap->data[i + 8 * 7] = otherLightmap->data[i + 8 * 1]; //bottom
						for (int i = 0; i < 8; i++)
							for (int c = 0; c < 3; c++)
								lightmap->data[64+3*(i + 8 * 7)+c] = otherLightmap->data[64+3*(i + 8 * 1)+c]; //bottom
					}
					else
						for (int i = 0; i < 8; i++)
							lightmap->data[i + 8 * 7] = lightmap->data[i + 8 * 6];
				}

				auto otherCube = cubes[x][y];
				if (otherCube && otherCube->tileUp != -1)
				{
					auto otherTile = tiles[otherCube->tileUp];
					auto otherLightmap = lightmaps[otherTile->lightmapIndex];

					for (int i = 0; i < 8; i++)
						lightmap->data[i + 8 * 0] = otherLightmap->data[i + 8 * 1]; //bottom
					for (int i = 0; i < 8; i++)
						for (int c = 0; c < 3; c++)
							lightmap->data[64+3*(i + 8 * 0)+c] = otherLightmap->data[64+3*(i + 8 * 1)+c]; //bottom
				}
				else
					for (int i = 0; i < 8; i++)
						lightmap->data[i + 8 * 0] = lightmap->data[i + 8 * 1];
			}

		}
	}

	node->getComponent<GndRenderer>()->setChunksDirty();
	node->getComponent<GndRenderer>()->gndShadowDirty = true;

}


void Gnd::makeLightmapBorders(int x, int y)
{
	if (x < 0 || x >= width || y < 0 || y >= height)
		return;
	Gnd::Cube* cube = cubes[x][y];
	int tileId = cube->tileUp;
	if (tileId == -1)
		return;
	Gnd::Tile* tile = tiles[tileId];
	assert(tile && tile->lightmapIndex != -1);
	Gnd::Lightmap* lightmap = lightmaps[tile->lightmapIndex];

	for (int i = 0; i < 8; i++)
	{
		lightmap->data[i + 8 * 0] = getLightmapBrightness(x, y - 1, i, 6);
		lightmap->data[i + 8 * 7] = getLightmapBrightness(x, y + 1, i, 1);
		lightmap->data[0 + 8 * i] = getLightmapBrightness(x - 1, y, 6, i);
		lightmap->data[7 + 8 * i] = getLightmapBrightness(x + 1, y, 1, i);

		for (int c = 0; c < 3; c++)
		{
			lightmap->data[64 + 3 * (i + 8 * 0) + c] = getLightmapColor(x, y - 1, i, 6)[c];
			lightmap->data[64 + 3 * (i + 8 * 7) + c] = getLightmapColor(x, y + 1, i, 1)[c];
			lightmap->data[64 + 3 * (0 + 8 * i) + c] = getLightmapColor(x - 1, y, 6, i)[c];
			lightmap->data[64 + 3 * (7 + 8 * i) + c] = getLightmapColor(x + 1, y, 1, i)[c];

		}
	}
	node->getComponent<GndRenderer>()->setChunksDirty();
	node->getComponent<GndRenderer>()->gndShadowDirty = true;

}


int Gnd::getLightmapBrightness(int x, int y, int lightmapX, int lightmapY)
{
	if (x < 0 || y < 0 || x >= width || y >= height)
		return 0;

	Gnd::Cube* cube = cubes[x][y];
	int tileId = cube->tileUp;
	if (tileId == -1)
		return 0;
	Gnd::Tile* tile = tiles[tileId];
	assert(tile && tile->lightmapIndex != -1);
	Gnd::Lightmap* lightmap = lightmaps[tile->lightmapIndex];

	return lightmap->data[lightmapX + 8 * lightmapY];

}

glm::ivec3 Gnd::getLightmapColor(int x, int y, int lightmapX, int lightmapY)
{
	if (x < 0 || y < 0 || x >= width || y >= height)
		return glm::ivec3(0, 0, 0);

	Gnd::Cube* cube = cubes[x][y];
	int tileId = cube->tileUp;
	if (tileId == -1)
		return glm::ivec3(0, 0, 0);
	Gnd::Tile* tile = tiles[tileId];
	assert(tile && tile->lightmapIndex != -1);
	Gnd::Lightmap* lightmap = lightmaps[tile->lightmapIndex];

	return glm::ivec3(
		lightmap->data[64 + (lightmapX + 8 * lightmapY) * 3 + 0],
		lightmap->data[64 + (lightmapX + 8 * lightmapY) * 3 + 1],
		lightmap->data[64 + (lightmapX + 8 * lightmapY) * 3 + 2]);


}

void Gnd::cleanLightmaps()
{
	std::cout<< "Lightmap cleanup, starting with " << lightmaps.size() << " lightmaps" <<std::endl;
	std::map<unsigned char, std::vector<std::size_t>> lookup;

	for (int i = 0; i < (int)lightmaps.size(); i++)
	{
		unsigned char hash = lightmaps[i]->hash();
		bool found = false;
		if (lookup.find(hash) != lookup.end())
		{
			for (const auto ii : lookup[hash])
			{
				if ((*lightmaps[i]) == (*lightmaps[ii]))
				{// if it is found
					assert(i > ii);
					//change all tiles with lightmap i to ii
					for (auto tile : tiles)
						if (tile->lightmapIndex == i)
							tile->lightmapIndex = (unsigned short)ii;
						else if (tile->lightmapIndex > i)
							tile->lightmapIndex--;
					//remove lightmap i
					delete lightmaps[i];
					lightmaps.erase(lightmaps.begin() + i);
					i--;
					found = true;
					break;
				}
			}
		}
		if (!found)
		{
			lookup[hash].push_back(i);
		}

	}

	std::cout<< "Lightmap cleanup, ending with " << lightmaps.size() << " lightmaps" << std::endl;
	node->getComponent<GndRenderer>()->setChunksDirty();
	node->getComponent<GndRenderer>()->gndShadowDirty = true;

}


void Gnd::makeLightmapsSmooth()
{
	makeTilesUnique();
	makeLightmapsUnique();
	makeLightmapBorders();
	std::cout << "Smoothing..." << std::endl;
	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			Gnd::Cube* cube = cubes[x][y];
			int tileId = cube->tileUp;
			if (tileId == -1)
				continue;
			Gnd::Tile* tile = tiles[tileId];
			assert(tile && tile->lightmapIndex != -1);
			Gnd::Lightmap* lightmap = lightmaps[tile->lightmapIndex];

			char newData[64];

			for (int xx = 1; xx < 7; xx++)
			{
				for (int yy = 1; yy < 7; yy++)
				{
					int total = 0;
					for (int xxx = xx - 1; xxx <= xx + 1; xxx++)
						for (int yyy = yy - 1; yyy <= yy + 1; yyy++)
							total += lightmap->data[xxx + 8 * yyy];
					newData[xx + 8 * yy] = total / 9;
				}
			}
			memcpy(lightmap->data, newData, 64 * sizeof(char));
		}
	}
	makeLightmapBorders();
	node->getComponent<GndRenderer>()->setChunksDirty();
	node->getComponent<GndRenderer>()->gndShadowDirty = true;
}

void Gnd::cleanTiles()
{
	std::cout << "Tiles cleanup, starting with " << tiles.size() << " tiles" << std::endl;
	std::set<int> used;
	for (int y = 0; y < height; y++)
		for (int x = 0; x < width; x++)
		{
			used.insert(cubes[x][y]->tileUp);
			used.insert(cubes[x][y]->tileFront);
			used.insert(cubes[x][y]->tileSide);
		}


	std::list<std::size_t> toRemove;
	for (std::size_t i = 0; i < tiles.size(); i++)
		if (used.find((int)i) == used.end())
			toRemove.push_back(i);
	toRemove.reverse();

	for (std::size_t i : toRemove)
	{
		tiles.erase(tiles.begin() + i);
		for (int y = 0; y < height; y++)
			for (int x = 0; x < width; x++)
				for(int ii = 0; ii < 3; ii++)
					if (cubes[x][y]->tileIds[ii] > i)
						cubes[x][y]->tileIds[ii]--;
	}
	std::cout<< "Tiles cleanup, ending with " << tiles.size() << " tiles" << std::endl;
}

void Gnd::flattenTiles(Map* map, BrowEdit* browEdit, const std::vector<glm::ivec2>& tiles)
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

void Gnd::smoothTiles(Map* map, BrowEdit* browEdit, const std::vector<glm::ivec2>& tiles, int axis)
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

void Gnd::addRandomHeight(Map* map, BrowEdit* browEdit, const std::vector<glm::ivec2>& tiles, float min, float max)
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

void Gnd::connectHigh(Map* map, BrowEdit* browEdit, const std::vector<glm::ivec2>& tiles)
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

void Gnd::connectLow(Map* map, BrowEdit* browEdit, const std::vector<glm::ivec2>& tiles)
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


void Gnd::perlinNoise(const std::vector<glm::ivec2>& tiles)
{
	CubeHeightChangeAction* action = new CubeHeightChangeAction(this, tiles);
	auto gndRenderer = node->getComponent<GndRenderer>();

	FastNoiseLite noise;
	noise.SetNoiseType(FastNoiseLite::NoiseType::NoiseType_Perlin);
	noise.SetFractalType(FastNoiseLite::FractalType::FractalType_FBm);
	noise.SetFractalOctaves(5);
	noise.SetSeed(0);

	for (auto tile : tiles)
	{
		for (int i = 0; i < 4; i++)
		{
			float x = (float)(tile.x + i % 2);
			float y = (float)(tile.y + i / 2);
			cubes[tile.x][tile.y]->heights[i] -= 1000 * noise.GetNoise(x,y);
		}
		gndRenderer->setChunkDirty(tile.x, tile.y);
	}
	action->setNewHeights(this, tiles);
//	map->doAction(action, browEdit);
}

void Gnd::makeTilesUnique()
{
	std::set<int> taken;
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			for (int c = 0; c < 3; c++)
			{
				if (cubes[x][y]->tileIds[c] == -1)
					continue;
				if (taken.find(cubes[x][y]->tileIds[c]) == taken.end())
					taken.insert(cubes[x][y]->tileIds[c]);
				else
				{
					Tile* t = new Tile(*tiles[cubes[x][y]->tileIds[c]]);
					cubes[x][y]->tileIds[c] = (int)tiles.size();
					tiles.push_back(t);
				}
			}
		}
	}
}








Gnd::Texture::Texture()
{
	//texture = nullptr;
}

Gnd::Texture::~Texture()
{
	//if (texture)
	//	delete[] texture; //TODO: resource manager
	//texture = nullptr;
}

void Gnd::Cube::calcNormal()
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
	for (int i = 0; i < 4; i++)
		normals[i] = normal;
}


void Gnd::Cube::calcNormals(Gnd* gnd, int x, int y)
{
	for (int i = 0; i < 4; i++)
	{
		normals[i] = glm::vec3(0, 0, 0);
		for (int ii = 0; ii < 4; ii++)
		{
			int xx = (ii % 2) * ((i % 2 == 0) ? -1 : 1);
			int yy = (ii / 2) * (i < 2 ? -1 : 1);
			if (x + xx >= 0 && x + xx < gnd->width && y + yy >= 0 && y + yy < gnd->height)
				normals[i] += gnd->cubes[x + xx][y + yy]->normal;
		}
		normals[i] = glm::normalize(normals[i]);
	}
}

bool Gnd::Cube::sameHeight(const Cube& other) const
{
	for (int i = 0; i < 4; i++)
		if (glm::abs(heights[i] - heights[0]) > 0.01f || glm::abs(other.heights[i] - heights[0]) > 0.01f)
			return false;
	return true;
}



std::vector<glm::vec3> Gnd::getMapQuads()
{
	std::vector<glm::vec3> quads;

	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			auto cube = cubes[x][y];
			if (cube->tileUp != -1)
			{
				quads.push_back(glm::vec3((x + 0) * 10, -cube->heights[2], 10 * height - (y + 1) * 10 + 10));//1
				quads.push_back(glm::vec3((x + 0) * 10, -cube->heights[0], 10 * height - (y + 0) * 10 + 10));//2
				quads.push_back(glm::vec3((x + 1) * 10, -cube->heights[1], 10 * height - (y + 0) * 10 + 10));//3
				quads.push_back(glm::vec3((x + 1) * 10, -cube->heights[3], 10 * height - (y + 1) * 10 + 10));//4
			}
			if (cube->tileSide != -1 && y < height-1)
			{
				quads.push_back(glm::vec3(10 * x, -cube->h3, 10 * height - 10 * y));//1
				quads.push_back(glm::vec3(10 * x + 10, -cube->h4, 10 * height - 10 * y));//2
				quads.push_back(glm::vec3(10 * x + 10, -cubes[x][y + 1]->h2, 10 * height - 10 * y));//3
				quads.push_back(glm::vec3(10 * x, -cubes[x][y + 1]->h1, 10 * height - 10 * y));//4
			}
			if (cube->tileFront != -1 && x < width - 1)
			{
				quads.push_back(glm::vec3(10 * x + 10, -cube->h2, 10 * height - 10 * y + 10));//1
				quads.push_back(glm::vec3(10 * x + 10, -cube->h4, 10 * height - 10 * y));//2
				quads.push_back(glm::vec3(10 * x + 10, -cubes[x + 1][y]->h3, 10 * height - 10 * y));//3
				quads.push_back(glm::vec3(10 * x + 10, -cubes[x + 1][y]->h1, 10 * height - 10 * y + 10));//4
			}
		}
	}
	return quads;
}

const unsigned char Gnd::Lightmap::hash() const //actually a crc, but will work
{
#define POLY 0x82f63b78
	unsigned char crc = ~0;
	for (int i = 0; i < 256; i++) {
		crc ^= data[i];
		for (int k = 0; k < 8; k++)
			crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
	}
	return ~crc;
}


const unsigned char Gnd::Tile::hash() const
{
#define POLY 0x82f63b78
	unsigned char crc = ~0;
	for (int i = 0; i < sizeof(Tile); i++) {
		crc ^= ((char*)this)[i];
		for (int k = 0; k < 8; k++)
			crc = crc & 1 ? (crc >> 1) ^ POLY : crc >> 1;
	}
	return ~crc;
}

bool Gnd::Lightmap::operator==(const Lightmap& other) const
{
	return memcmp(data, other.data, 256) == 0;
}


bool Gnd::Tile::operator==(const Gnd::Tile& o) const
{
	return	v1 == o.v1 &&
		v2 == o.v2 &&
		v3 == o.v3 &&
		v4 == o.v4 &&
		textureIndex == o.textureIndex &&
		lightmapIndex == o.lightmapIndex &&
		color == o.color;
}


glm::vec3 Gnd::getPos(int x, int y, int index)
{
	auto cube = cubes[x][y];
	if (index == 0)
		return glm::vec3((x + 0) * 10, -cube->heights[0], 10 * height - (y + 0) * 10 + 10);//2
	if (index == 1)
		return glm::vec3((x + 1) * 10, -cube->heights[1], 10 * height - (y + 0) * 10 + 10);//3
	if(index == 2)
		return glm::vec3((x + 0) * 10, -cube->heights[2], 10 * height - (y + 1) * 10 + 10);//1
	if(index == 3)
		return glm::vec3((x + 1) * 10, -cube->heights[3], 10 * height - (y + 1) * 10 + 10);//4

	return glm::vec3(0, 0,0);
}