#include "TileNewAction.h"

#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/components/Gnd.h>

TileNewAction::TileNewAction(Gnd::Tile* tile) : tile(tile)
{
}

TileNewAction::~TileNewAction()
{
    //TODO: delete tile, only if it is not used in the map anymore
}

void TileNewAction::perform(Map* map, BrowEdit* browEdit)
{
    map->rootNode->getComponent<Gnd>()->tiles.push_back(tile);
}

void TileNewAction::undo(Map* map, BrowEdit* browEdit)
{
    map->rootNode->getComponent<Gnd>()->tiles.pop_back();
}

std::string TileNewAction::str()
{
    return "Added tile";
}
