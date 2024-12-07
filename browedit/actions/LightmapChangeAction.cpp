#include "LightmapChangeAction.h"

#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/GndRenderer.h>

LightmapChangeAction::LightmapChangeAction()
{
}

LightmapChangeAction::~LightmapChangeAction()
{
    for (int i = 0; i < m_oldLightmaps.size(); i++)
        delete m_oldLightmaps[i];

    for (int i = 0; i < m_newLightmaps.size(); i++)
        delete m_newLightmaps[i];
}

void LightmapChangeAction::setPreviousData(std::vector<Gnd::Lightmap*> lightmaps, std::vector<Gnd::Tile*> tiles)
{
    for (int i = 0; i < lightmaps.size(); i++)
        m_oldLightmaps.push_back(new Gnd::Lightmap(*lightmaps[i]));
    for (int i = 0; i < tiles.size(); i++)
        m_oldLightmapIndex.push_back(tiles[i]->lightmapIndex);
}

void LightmapChangeAction::setCurrentData(std::vector<Gnd::Lightmap*> lightmaps, std::vector<Gnd::Tile*> tiles)
{
    for (int i = 0; i < lightmaps.size(); i++)
        m_newLightmaps.push_back(new Gnd::Lightmap(*lightmaps[i]));
    for (int i = 0; i < tiles.size(); i++)
        m_newLightmapIndex.push_back(tiles[i]->lightmapIndex);
}

void LightmapChangeAction::perform(Map* map, BrowEdit* browEdit)
{
    Gnd* gnd = map->rootNode->getComponent<Gnd>();

    for (int i = 0; i < gnd->lightmaps.size(); i++)
        delete gnd->lightmaps[i];

    gnd->lightmaps.clear();

    for (int i = 0; i < m_newLightmaps.size(); i++)
        gnd->lightmaps.push_back(new Gnd::Lightmap(*m_newLightmaps[i]));

    for (int i = 0; i < m_newLightmapIndex.size(); i++)
        gnd->tiles[i]->lightmapIndex = m_newLightmapIndex[i];

    map->rootNode->getComponent<GndRenderer>()->gndShadowDirty = true;
    map->rootNode->getComponent<GndRenderer>()->setChunksDirty();
}

void LightmapChangeAction::undo(Map* map, BrowEdit* browEdit)
{
    Gnd* gnd = map->rootNode->getComponent<Gnd>();

    for (int i = 0; i < gnd->lightmaps.size(); i++)
        delete gnd->lightmaps[i];

    gnd->lightmaps.clear();

    for (int i = 0; i < m_oldLightmaps.size(); i++)
        gnd->lightmaps.push_back(new Gnd::Lightmap(*m_oldLightmaps[i]));

    for (int i = 0; i < m_oldLightmapIndex.size(); i++)
        gnd->tiles[i]->lightmapIndex = m_oldLightmapIndex[i];

    map->rootNode->getComponent<GndRenderer>()->gndShadowDirty = true;
    map->rootNode->getComponent<GndRenderer>()->setChunksDirty();
}

std::string LightmapChangeAction::str()
{
    return "Changed Lightmap";
}
