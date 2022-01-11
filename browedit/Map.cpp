#include <browedit/Map.h>
#include <browedit/Node.h>

#include <browedit/components/Rsw.h>



Map::Map(const std::string& name) : name(name)
{
	rootNode = new Node(name);
	auto rsw = new Rsw();
	rootNode->addComponent(rsw);	
	rsw->load(name);
}

