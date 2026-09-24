#include "Str.h"

#include <browedit/util/Util.h>
#include <browedit/util/FileIO.h>
#include <iostream>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>

Str::Str(const std::string& fileName)
{
	this->fileName = fileName;
	auto file = util::FileIO::open(fileName);

	if (!file)
	{
		std::cerr << "STR: Unable to open " << fileName << std::endl;
		return;
	}

	char header[4];
	file->read(header, 4);
	if (!(header[0] == 'S' && header[1] == 'T' && header[2] == 'R' && header[3] == 'M'))
	{
		std::cerr << "STR: Unknown STR header in file " << fileName << ", stopped loading" << std::endl;
		delete file;
		return;
	}

	file->read(reinterpret_cast<char*>(&version), sizeof(short));
	version = util::swapShort(version);

	// Unknown data
	file->seekg(2, std::ios::cur);

	file->read(reinterpret_cast<char*>(&fps), sizeof(int));
	file->read(reinterpret_cast<char*>(&maxKeyFrame), sizeof(int));

	int layerCount;
	file->read(reinterpret_cast<char*>(&layerCount), sizeof(int));
	layers.reserve(layerCount);

	// Unknown data
	file->seekg(16, std::ios::cur);

	for (int i = 0; i < layerCount; i++)
	{
		layers.push_back(std::make_unique<Layer>(this, file));
	}

	// Apparently, the client doesn't use the maxKeyFrame value but calculates it from latest frame time.
	//if (maxKeyFrame == 0x6d617246) {
	maxKeyFrame = 0;

	for (auto& layer : layers) {
		for (auto& frame : layer->frames) {
			maxKeyFrame = glm::max(maxKeyFrame, frame.time);
		}
	}
	//}

	delete file;
}

Str::~Str()
{
}

Str::Layer::Layer(Str* str, std::istream* file)
{
	this->str = str;

	int count;
	file->read(reinterpret_cast<char*>(&count), sizeof(int));
	textures.reserve(count);

	for (int i = 0; i < count; i++)
	{
		textures.push_back(util::FileIO::readString(file, 128));
	}

	file->read(reinterpret_cast<char*>(&count), sizeof(int));
	frames.resize(count);

	for (int i = 0; i < count; i++)
	{
		file->read(reinterpret_cast<char*>(&frames[i].time), sizeof(int));
		file->read(reinterpret_cast<char*>(&frames[i].type), sizeof(int));
		file->read(reinterpret_cast<char*>(glm::value_ptr(frames[i].offset)), sizeof(float) * 2);

		for (int j = 0; j < 8; j++)
			file->read(reinterpret_cast<char*>(&frames[i].uvs[j]), sizeof(float));

		for (int j = 0; j < 8; j++)
			file->read(reinterpret_cast<char*>(&frames[i].positions[j]), sizeof(float));

		file->read(reinterpret_cast<char*>(&frames[i].textureIndex), sizeof(float));
		file->read(reinterpret_cast<char*>(&frames[i].animationType), sizeof(int));
		file->read(reinterpret_cast<char*>(&frames[i].delay), sizeof(float));
		file->read(reinterpret_cast<char*>(&frames[i].angle), sizeof(float));
		file->read(reinterpret_cast<char*>(glm::value_ptr(frames[i].color)), sizeof(float) * 4);
		file->read(reinterpret_cast<char*>(&frames[i].blendSrc), sizeof(int));
		file->read(reinterpret_cast<char*>(&frames[i].blendDst), sizeof(int));
		file->read(reinterpret_cast<char*>(&frames[i].mtPresent), sizeof(int));

		if (frames[i].delay == INFINITY)
			frames[i].delay = 0;

		frames[i].angle *= (360.0f / 1024.0f);
		frames[i].color /= 255.0f;
	}

	// Simplify layer format by marking frames as interpolated and removing the extra duplicate frame.
	for (int i = count - 1; i >= 0; i--) {
		if (frames[i].type == 1 && i - 1 > 0 && frames[i - 1].type == 0 && frames[i - 1].time == frames[i].time) {
			frames[i - 1].isInterpolated = true;
		}
	}

	frames.erase(std::remove_if(frames.begin(), frames.end(), [](Frame frame) {
		return frame.type == 1;
	}), frames.end());
}

Str::Layer::~Layer()
{
	frames.clear();
}

