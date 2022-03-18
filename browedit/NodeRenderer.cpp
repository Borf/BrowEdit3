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
	//TODO: link node to the cache in this context, so that if the node changes, the context's cache invalidates

	//TODO: cache renderers

	std::map<Renderer::RenderContext*, std::vector<Renderer*>> renderers;
	rootNode->traverse([&renderers](Node* n){
		for (auto c : n->components)
		{
			auto cc = dynamic_cast<Renderer*>(c);
			if (cc)
				renderers[cc->renderContext].push_back(cc);
		}
	});

	std::vector<Renderer::RenderContext*> ordered;
	for (auto r : renderers)
		ordered.push_back(r.first);
	std::sort(ordered.begin(), ordered.end(), [](Renderer::RenderContext* a, Renderer::RenderContext* b) { return a->order < b->order; });


	for (auto r : ordered)
	{
		r->preFrame(context.projectionMatrix, context.viewMatrix);
		for (auto renderer : renderers[r])
			renderer->render();
	}


}