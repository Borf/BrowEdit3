#include "LightmapNewAction.h"

#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/components/Gnd.h>

LightmapNewAction::LightmapNewAction(Gnd::Lightmap* lightmap) : lightmap(lightmap)
{
}

LightmapNewAction::~LightmapNewAction()
{
    //TODO: delete Lightmap, only if it is not used in the map anymore
}

void LightmapNewAction::perform(Map* map, BrowEdit* browEdit)
{
    map->rootNode->getComponent<Gnd>()->lightmaps.push_back(lightmap);
}

void LightmapNewAction::undo(Map* map, BrowEdit* browEdit)
{
    map->rootNode->getComponent<Gnd>()->lightmaps.pop_back();
}

std::string LightmapNewAction::str()
{
    return "Added Lightmap";
}
