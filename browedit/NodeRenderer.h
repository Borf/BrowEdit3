#pragma once

#include <glm/glm.hpp>
#include <map>
#include <vector>
#include <browedit/components/Renderer.h>
class Node;
class NodeRenderContext;

class NodeRenderer
{
public:
	static void begin();
	static void end();
	static void render(Node* rootNode, NodeRenderContext& context);
};

class NodeRenderContext
{
public:
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;

	std::map<Node*, std::map<Renderer::RenderContext*, std::vector<Renderer*>>> renderers;
	std::map<Node*, std::vector<Renderer::RenderContext*>> ordered;

};