#include "NodeRenderer.h"
#include "components/Renderer.h"
#include <map>
#include <vector>
#include <algorithm>
#include "Node.h"

void NodeRenderer::begin()
{

}


void NodeRenderer::end()
{

}


void NodeRenderer::render(Node* rootNode, NodeRenderContext& context)
{
	std::map<Renderer::RenderContext*, std::vector<Renderer*>>& renderers = context.renderers[rootNode];
	std::vector<Renderer::RenderContext*>& ordered = context.ordered[rootNode];
	if (rootNode->dirty)
	{
		renderers.clear();
		rootNode->traverse([&renderers](Node* n) {
			n->dirty = false;
			for (auto c : n->components)
			{
				auto cc = dynamic_cast<Renderer*>(c);
				if (cc)
					renderers[cc->renderContext].push_back(cc);
			}
			});

		ordered.clear();
		for (auto r : renderers)
			ordered.push_back(r.first);
		std::sort(ordered.begin(), ordered.end(), [](Renderer::RenderContext* a, Renderer::RenderContext* b) { return a->order < b->order; });
	}

	for (auto r : ordered)
	{
		r->preFrame(context.projectionMatrix, context.viewMatrix);
		for(int phase = 0; phase < r->phases; phase++)
			for (auto renderer : renderers[r])
				if(renderer->enabled && renderer->shouldRender(phase))
					renderer->render();
	}


}