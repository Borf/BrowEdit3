#pragma once

#include "Component.h"
#include <glm/glm.hpp>

class Renderer : public Component
{
public:
	virtual ~Renderer() {}

	class RenderContext
	{
	public:
		int order = 0;
		int phases = 1;
		virtual void preFrame(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix) = 0;
	};

	RenderContext* renderContext;
	virtual void render() = 0;
	virtual bool shouldRender(int phase) { return true; }
	bool enabled = true;
};