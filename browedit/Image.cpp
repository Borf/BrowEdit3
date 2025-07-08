#include "Image.h"

Image::Image(const std::string& fileName)
{
	std::istream* is = util::FileIO::open(fileName);
	if (!is)
	{
		std::cerr << "Image: Could not open " << fileName << std::endl;
		return;
	}
	is->seekg(0, std::ios_base::end);
	std::size_t len = is->tellg();
	char* fileData = new char[len];
	is->seekg(0, std::ios_base::beg);
	is->read(fileData, len);
	delete is;
	int depth;
	unsigned char* tmpData = stbi_load_from_memory((stbi_uc*)fileData, (int)len, &width, &height, &depth, 3);
	if (!tmpData)
	{
		const char* err = stbi_failure_reason();
		std::cerr << "Error loading file " << fileName << std::endl;
		std::cerr << err << std::endl;
		return;
	}
	delete[] fileData;

	data = new unsigned char[width * height];

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			data[x + width * y] = tmpData[3 * (x + width * y) + 3];
			if (tmpData[3 * (x + width * y) + 0] > 253 &&
				tmpData[3 * (x + width * y) + 1] < 2 &&
				tmpData[3 * (x + width * y) + 2] > 253)
			{
				data[x + width * y] = 0;
				hasAlpha = true;
			}
			if (tmpData[3 * (x + width * y) + 3] != 255)
				hasAlpha = true;
		}
	}
	stbi_image_free(tmpData);
}

float Image::get(const glm::vec2& uv)
{
	if (!data || !hasAlpha)
		return 1;

	int x1 = glm::min(width - 1, (int)floor(uv.x * width));
	int y1 = glm::min(height - 1, (int)floor(uv.y * height));
	int x2 = glm::min(width - 1, (int)ceil(uv.x * width));
	int y2 = glm::min(height - 1, (int)ceil(uv.y * height));

#if 1
	float fracX = glm::fract(uv.x * width);
	float fracY = glm::fract(uv.y * height);

	if (std::isnan(fracX) || std::isnan(fracY))
		return true;
	float top = glm::mix(data[x1 + width * y1], data[x2 + width * y1], fracX);
	float bottom = glm::mix(data[x1 + width * y2], data[x2 + width * y2], fracX);

	return glm::mix(top, bottom, fracY) / 255.0f;
#else
	return ((data[x1 + width * y1] + data[x1 + width * y2] + data[x2 + width * y1] + data[x2 + width * y2]) / 4.0f) / 255.0f;
#endif
}
