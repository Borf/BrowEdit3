#pragma once

#include <string>
#include <browedit/NodeRenderer.h>

class Node;
class BrowEdit;
namespace gl { class FBO; }

class ModelEditor
{
	class ModelView
	{
	public:
		Node* node;
		gl::FBO* fbo;
	};

public:
	std::string id;
	std::vector<ModelView> models;
	NodeRenderContext nodeRenderContext;
#ifdef _DEBUG
	bool opened = true;
#else
	bool opened = false;
#endif

	void load(const std::string& fileName);
	void run(BrowEdit* browEdit);
};