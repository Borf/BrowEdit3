#include "Gnd.h"
#include <browedit/util/Util.h>
#include <browedit/util/FileIO.h>
#include <iostream>

Gnd::Gnd(const std::string& fileName)
{
	auto file = util::FileIO::open("data\\" + fileName);
	if (!file)
	{
		width = 0;
		height = 0;
		std::cerr << "GND: Unable to open gnd file: " << fileName << ".gnd" << std::endl;
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
					file->read(reinterpret_cast<char*>(&cube->tileSide), sizeof(int));
					file->read(reinterpret_cast<char*>(&cube->tileFront), sizeof(int));
				}
				else
				{
					unsigned short up, side, front;
					file->read(reinterpret_cast<char*>(&up), sizeof(unsigned short));
					file->read(reinterpret_cast<char*>(&side), sizeof(unsigned short));
					file->read(reinterpret_cast<char*>(&front), sizeof(unsigned short));

					cube->tileUp = up;
					cube->tileSide = side;
					cube->tileFront = front;
				}

				if (cube->tileUp >= (int)tiles.size())
				{
					std::cout << "GND: Wrong value for tileup at " << x << ", " << y << "Found " << cube->tileUp << ", but only " << tiles.size() << " tiles found" << std::endl;
					cube->tileUp = -1;
				}
				if (cube->tileSide >= (int)tiles.size())
				{
					std::cout << "GND: Wrong value for tileside at " << x << ", " << y << std::endl;
					cube->tileSide = -1;
				}
				if (cube->tileFront >= (int)tiles.size())
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



glm::vec3 Gnd::rayCast(const math::Ray& ray)
{
	float f = 0;
	for (auto x = 0; x < cubes.size(); x++)
	{
		for (auto y = 0; y < cubes[x].size(); y++)
		{
			Gnd::Cube* cube = cubes[x][y];

			if (cube->tileUp != -1)
			{
				Gnd::Tile* tile = tiles[cube->tileUp];

				glm::vec3 v1(10 * x, -cube->h3, 10 * height - 10 * y);
				glm::vec3 v2(10 * x + 10, -cube->h4, 10 * height - 10 * y);
				glm::vec3 v3(10 * x, -cube->h1, 10 * height - 10 * y + 10);
				glm::vec3 v4(10 * x + 10, -cube->h2, 10 * height - 10 * y + 10);

				{
					std::vector<glm::vec3> v{ v3, v2, v1 };
					if (ray.LineIntersectPolygon(v, f))
						return ray.origin + f * ray.dir;
				}
				{
					std::vector<glm::vec3> v{ v4, v2, v3 };
					if (ray.LineIntersectPolygon(v, f))
						return ray.origin + f * ray.dir;
				}
			}
			if (cube->tileFront != -1 && x < width - 1)
			{
				Gnd::Tile* tile = tiles[cube->tileFront];
				assert(tile->lightmapIndex >= 0);

				glm::vec3 v1(10 * x + 10, -cube->h2, 10 * height - 10 * y + 10);
				glm::vec3 v2(10 * x + 10, -cube->h4, 10 * height - 10 * y);
				glm::vec3 v3(10 * x + 10, -cubes[x + 1][y]->h1, 10 * height - 10 * y + 10);
				glm::vec3 v4(10 * x + 10, -cubes[x + 1][y]->h3, 10 * height - 10 * y);

				{
					std::vector<glm::vec3> v{ v3, v2, v1 };
					if (ray.LineIntersectPolygon(v, f))
						return ray.origin + f * ray.dir;
				}
				{
					std::vector<glm::vec3> v{ v4, v2, v3 };
					if (ray.LineIntersectPolygon(v, f))
						return ray.origin + f * ray.dir;
				}
			}
			if (cube->tileSide != -1 && y < height - 1)
			{
				Gnd::Tile* tile = tiles[cube->tileSide];

				glm::vec3 v1(10 * x, -cube->h3, 10 * height - 10 * y);
				glm::vec3 v2(10 * x + 10, -cube->h4, 10 * height - 10 * y);
				glm::vec3 v4(10 * x + 10, -cubes[x][y + 1]->h2, 10 * height - 10 * y);
				glm::vec3 v3(10 * x, -cubes[x][y + 1]->h1, 10 * height - 10 * y);

				{
					std::vector<glm::vec3> v{ v3, v2, v1 };
					if (ray.LineIntersectPolygon(v, f))
						return ray.origin + f * ray.dir;
				}
				{
					std::vector<glm::vec3> v{ v4, v2, v3 };
					if (ray.LineIntersectPolygon(v, f))
						return ray.origin + f * ray.dir;
				}
			}
		}
	}
	return glm::vec3(0, 0, 0);
}









Gnd::Texture::Texture()
{
	texture = nullptr;
}

Gnd::Texture::~Texture()
{
	if (texture)
		delete[] texture; //TODO: resource manager
	texture = nullptr;
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



