#pragma once

#include <string>
#include <browedit/gl/Shader.h>
class Node;
class Rsw;
namespace gl { class FBO; }

class Map
{
public:
	
	


	std::string name;

	Node* rootNode = nullptr;
	Rsw* rsw = nullptr;
	Node* gndNode = nullptr;

	Map(const std::string& name);

};