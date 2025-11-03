#pragma once

#include "Component.h"
#include <glm/glm.hpp>

class NodeRenderContext;

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
		virtual void preFrame(Node* rootNode, NodeRenderContext& context) = 0;
		virtual void postFrame(NodeRenderContext& context) { }
	};

	RenderContext* renderContext;
	virtual void render(NodeRenderContext& context) = 0;
	virtual bool shouldRender(int phase) { return phase == 0; }
	bool enabled = true;
};