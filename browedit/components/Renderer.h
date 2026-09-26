#pragma once

#include "Component.h"
#include <glm/glm.hpp>

class NodeRenderContext;

// Drawing order priority for the renderers
// For example, Billboards is last so it will be drawn last, on top of everything else.
enum class RendererDrawPriority {
	Default,
	Gnd,
	Rsm,
	SkyMap,
	Gat,
	Water,
	Lub,
	Str,
	Billboard,
};

class Renderer : public Component
{
public:
	virtual ~Renderer() {}

	class RenderContext
	{
	public:
		RendererDrawPriority order = RendererDrawPriority::Default;
		int phases = 1;
		int phase = 0;
		virtual void preFrame(Node* rootNode, NodeRenderContext& context, std::vector<Renderer*>& renderers) = 0;
		virtual void postFrame(NodeRenderContext& context) { }
	};

	RenderContext* renderContext;
	virtual void render(NodeRenderContext& context) = 0;
	virtual bool shouldRender(int phase) { return phase == 0; }
	bool enabled = true;
};