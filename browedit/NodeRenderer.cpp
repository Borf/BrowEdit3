#include "NodeRenderer.h"
#include "components/Renderer.h"
#include <map>
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
		auto r = n->getComponent<Renderer>();
		if(r)
			renderers[r->renderContext].push_back(r); //TODO: multiple renderers
	});

	for (auto r : renderers)
	{
		r.first->preFrame(context.projectionMatrix, context.viewMatrix);
		for (auto renderer : r.second)
		{
			renderer->render();
		}
	}


}