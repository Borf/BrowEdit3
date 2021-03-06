#include <Windows.h>
#include "GndTextureActions.h"
#include <browedit/components/Gnd.h>
#include <browedit/components/GndRenderer.h>
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/BrowEdit.h>
#include <browedit/MapView.h>
#include <browedit/util/ResourceManager.h>
#include <browedit/gl/Texture.h>

GndTextureAddAction::GndTextureAddAction(const std::string& fileName) : fileName(fileName)
{
}

void GndTextureAddAction::perform(Map* map, BrowEdit* browEdit)
{
	auto gnd = map->rootNode->getComponent<Gnd>();
	auto gndRenderer = map->rootNode->getComponent<GndRenderer>();
	for (auto t : gnd->textures)
		if (t->file == fileName)
			return;
	gnd->textures.push_back(new Gnd::Texture(fileName, fileName));
	gndRenderer->textures.push_back(util::ResourceManager<gl::Texture>::load("data\\texture\\" + fileName));
	if (browEdit->activeMapView && browEdit->activeMapView->map == map)
	{
		browEdit->activeMapView->textureSelected = (int)gnd->textures.size() - 1;
	}
}

void GndTextureAddAction::undo(Map* map, BrowEdit* browEdit)
{
	auto gnd = map->rootNode->getComponent<Gnd>();
	auto gndRenderer = map->rootNode->getComponent<GndRenderer>();
	delete gnd->textures.back();
	gnd->textures.pop_back();

	util::ResourceManager<gl::Texture>::unload(gndRenderer->textures.back());
	gndRenderer->textures.pop_back();

	for(auto &mv : browEdit->mapViews)
	{
		if(mv.map == map && mv.textureSelected >= gnd->textures.size())
			mv.textureSelected = (int)gnd->textures.size() - 1;
	}

}

std::string GndTextureAddAction::str()
{
	return "Added Texture";
}

GndTextureChangeAction::GndTextureChangeAction(int index, const std::string& fileName) : index(index), newTexture(fileName)
{
}

void GndTextureChangeAction::perform(Map* map, BrowEdit* browEdit)
{
	auto gnd = map->rootNode->getComponent<Gnd>();
	auto gndRenderer = map->rootNode->getComponent<GndRenderer>();
	oldTexture = gnd->textures[index]->file;
	gnd->textures[index]->file = newTexture;

	util::ResourceManager<gl::Texture>::unload(gndRenderer->textures[index]);
	gndRenderer->textures[index] = util::ResourceManager<gl::Texture>::load("data\\texture\\" + newTexture);
}

void GndTextureChangeAction::undo(Map* map, BrowEdit* browEdit)
{
	auto gnd = map->rootNode->getComponent<Gnd>();
	auto gndRenderer = map->rootNode->getComponent<GndRenderer>();
	gnd->textures[index]->file = oldTexture;

	util::ResourceManager<gl::Texture>::unload(gndRenderer->textures[index]);
	gndRenderer->textures[index] = util::ResourceManager<gl::Texture>::load("data\\texture\\" + oldTexture);
}

std::string GndTextureChangeAction::str()
{
	return "Change texture to " + util::iso_8859_1_to_utf8(newTexture);
}


