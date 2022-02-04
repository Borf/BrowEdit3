#include "ModelChangeAction.h"
#include <browedit/util/Util.h>
#include <browedit/util/ResourceManager.h>
#include <browedit/components/Rsw.h>
#include <browedit/components/Rsm.h>
#include <browedit/components/RsmRenderer.h>
#include <browedit/Node.h>

ModelChangeAction::ModelChangeAction(Node* node, const std::string& newFileName) : node(node), newFileName(newFileName)
{
	auto rswModel = node->getComponent<RswModel>();
	oldFileName = "data\\model\\" + util::utf8_to_iso_8859_1(rswModel->fileName);

}

void ModelChangeAction::perform(Map* map, BrowEdit* browEdit)
{
	auto rswModel = node->getComponent<RswModel>();
	rswModel->fileName = util::iso_8859_1_to_utf8(newFileName);
	auto removed = node->removeComponent<Rsm>();
	for (auto r : removed)
		util::ResourceManager<Rsm>::unload(r);
	node->addComponent(util::ResourceManager<Rsm>::load(newFileName));
	node->getComponent<RsmRenderer>()->begin();
	node->getComponent<RswModelCollider>()->begin();
}

void ModelChangeAction::undo(Map* map, BrowEdit* browEdit)
{
	auto rswModel = node->getComponent<RswModel>();
	rswModel->fileName = util::iso_8859_1_to_utf8(oldFileName);
	auto removed = node->removeComponent<Rsm>();
	for (auto r : removed)
		util::ResourceManager<Rsm>::unload(r);
	node->addComponent(util::ResourceManager<Rsm>::load(oldFileName));
	node->getComponent<RsmRenderer>()->begin();
	node->getComponent<RswModelCollider>()->begin();
}

std::string ModelChangeAction::str()
{
	return "Changing model for " + node->name;
}
