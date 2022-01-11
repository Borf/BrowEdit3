#pragma once

#include <glm/glm.hpp>

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
};