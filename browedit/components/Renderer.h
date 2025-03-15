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
		int phase = 0;
		virtual void preFrame(Node* rootNode, const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix) = 0;
		virtual void postFrame() { }
	};

	RenderContext* renderContext;
	virtual void render() = 0;
	virtual bool shouldRender(int phase) { return phase == 0; }
	bool enabled = true;
};