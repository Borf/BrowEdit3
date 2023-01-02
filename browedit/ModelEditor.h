#pragma once

#include <string>
#include <browedit/NodeRenderer.h>
#include <browedit/components/Rsm.h>
class Node;
class BrowEdit;
namespace gl { class FBO; class Texture;  }
class SimpleShader;

class ModelEditor
{
	class ModelView
	{
	public:
		Node* node;
		gl::FBO* fbo;
		Rsm::Mesh* selectedMesh = nullptr;
		glm::vec3 camera = glm::vec3(0,0,50);
	};

public:
	std::string id;
	std::vector<ModelView> models;
	NodeRenderContext nodeRenderContext;

	gl::Texture* gridTexture;
	SimpleShader* simpleShader;

//#ifdef _DEBUG
//	bool opened = true;
//#else
	bool opened = false;
//#endif

	void load(const std::string& fileName);
	void run(BrowEdit* browEdit);
};