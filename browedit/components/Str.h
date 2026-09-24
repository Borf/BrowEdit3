#pragma once

#include "Component.h"
#include <string>
#include <map>
#include <vector>
#include <functional>
#include <memory>

#include <glm/glm.hpp>

class Str : public Component
{
public:
	class Layer
	{
	public:
		class Frame
		{
		public:
			int time;
			int type;
			glm::vec2 offset;
			float uvs[8];
			float positions[8];
			float textureIndex;
			enum class AnimationType
			{
				ANIM_STOP,
				ANIM_INTERPOLATION,
				ANIM_ONCE,
				ANIM_LOOP,
				ANIM_REVERSE_LOOP,
				ANIM_BI_LOOP,
			} animationType;
			float delay;
			float angle;
			glm::vec4 color;
			int blendSrc;
			int blendDst;
			int mtPresent;
			bool isInterpolated;
			virtual ~Frame() {};
		};

		Layer(Str* str, std::istream* strFile);
		~Layer();

		Str* str;
		std::vector<Frame> frames;
		std::vector<std::string> textures;
	};
public:
	Str(const std::string& fileName);
	~Str();

	std::string fileName;
	short version;

	int fps;
	int maxKeyFrame;
	std::vector<std::unique_ptr<Str::Layer>> layers;
};