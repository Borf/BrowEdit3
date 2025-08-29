#include <Windows.h>
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/actions/Action.h>
#include <browedit/actions/ObjectChangeAction.h>
#include <browedit/actions/GroupAction.h>
#include <browedit/actions/SelectAction.h>
#include <browedit/actions/DeleteObjectAction.h>
#include <browedit/actions/TileSelectAction.h>
#include <browedit/actions/TileNewAction.h>
#include <browedit/actions/CubeTileChangeAction.h>
#include <browedit/actions/TilePropertyChangeAction.h>
#include <browedit/components/Rsw.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/GndRenderer.h>
#include <browedit/components/BillboardRenderer.h>
#include <browedit/BrowEdit.h>
#include <browedit/util/ResourceManager.h>
#include <filesystem>
#include <fstream>

#include <json.hpp>
using json = nlohmann::json;

#include <stb/stb_image_write.h>
#include <stb/stb_image.h>
#include <set>
#include <iostream>
#include <thread>
#include <mutex>


Map::Map(const std::string& name, BrowEdit* browEdit) : name(name)
{
	rootNode = new Node(name);
	auto rsw = new Rsw();
	rootNode->addComponent(rsw);	
	rsw->load(name, this, browEdit);
	changed = false;
}

Map::Map(const std::string& name, int width, int height, BrowEdit* browEdit) : name(name)
{
	rootNode = new Node(name);
	auto rsw = new Rsw();
	rootNode->addComponent(rsw);
	rsw->newMap(name, width, height, this, browEdit);
	changed = true;
}


Map::~Map()
{
	for (auto u : undoStack)
		delete u;
	for (auto u : redoStack)
		delete u;
	delete rootNode;
}


std::vector<glm::ivec2> Map::getSelectionAroundTiles()
{
	auto gnd = rootNode->getComponent<Gnd>();
	std::vector<glm::ivec2> tilesAround;
	auto isTileSelected = [&](int x, int y) { return std::find(tileSelection.begin(), tileSelection.end(), glm::ivec2(x, y)) != tileSelection.end(); };

	for (auto& t : tileSelection)
		for (int xx = -1; xx <= 1; xx++)
			for (int yy = -1; yy <= 1; yy++)
				if (gnd->inMap(t + glm::ivec2(xx, yy)) &&
					std::find(tilesAround.begin(), tilesAround.end(), glm::ivec2(t.x + xx, t.y + yy)) == tilesAround.end() &&
					!isTileSelected(t.x + xx, t.y + yy))
					tilesAround.push_back(t + glm::ivec2(xx, yy));
	return tilesAround;
}

Node* Map::findAndBuildNode(const std::string& path, Node* root)
{
	if (!root)
		root = rootNode;
	if (path == "")
		return root;
	std::string firstPart = path;
	std::string secondPart = "";
	if (firstPart.find("\\") != std::string::npos)
	{
		firstPart = firstPart.substr(0, firstPart.find("\\"));
		secondPart = path.substr(path.find("\\") + 1);
	}
	for (auto c : root->children)
		if (c->name == firstPart)
			return findAndBuildNode(secondPart, c);
	Node* node = new Node(firstPart, root);
	return findAndBuildNode(secondPart, node);
}


void Map::doAction(Action* action, BrowEdit* browEdit)
{
	if (tempGroupAction)
	{
		tempGroupAction->addAction(action);
		return;
	}

	auto ga = dynamic_cast<GroupAction*>(action);
	if (ga && ga->isEmpty())
		return;

	if (std::find(undoStack.begin(), undoStack.end(), action) != undoStack.end())
	{
		std::cerr << "Double undo!" << std::endl;
		throw "Oops";
		return;
	}

	changed = true;
	action->perform(this, browEdit);
	undoStack.push_back(action);

	for (auto a : redoStack)
		delete a;
	redoStack.clear();
}

void Map::redo(BrowEdit* browEdit)
{
	if (redoStack.size() > 0)
	{
		redoStack.front()->perform(this, browEdit);
		undoStack.push_back(redoStack.front());
		redoStack.erase(redoStack.begin());
	}
}
void Map::beginGroupAction(const std::string &title)
{
	assert(!tempGroupAction);
	tempGroupAction = new GroupAction(title);
}
void Map::endGroupAction(BrowEdit* browEdit)
{
	assert(tempGroupAction);
	auto action = tempGroupAction;
	tempGroupAction = nullptr;
	doAction(action, browEdit);
}
void Map::undo(BrowEdit* browEdit)
{
	if (undoStack.size() > 0)
	{
		undoStack.back()->undo(this, browEdit);
		redoStack.insert(redoStack.begin(), undoStack.back());
		undoStack.pop_back();
	}
}



glm::vec3 Map::getSelectionCenter()
{
	int count = 0;
	glm::vec3 center(0.0f);
	for (auto n : selectedNodes)
		if (n->getComponent<RswObject>())
		{
			center += n->getComponent<RswObject>()->position;
			count++;
		}
	center /= count;
	return center;
}


void Map::invertScale(int axis, BrowEdit* browEdit)
{
	auto ga = new GroupAction();
	for (auto n : selectedNodes)
	{
		auto rswObject = n->getComponent<RswObject>();
		auto rsmRenderer = n->getComponent<RsmRenderer>();
		if (rswObject)
		{
			float orig = rswObject->scale[axis];
			rswObject->scale[axis] = -rswObject->scale[axis];
			ga->addAction(new ObjectChangeAction(n, &rswObject->scale[axis], orig, "Invert Scale"));
		}
		if (rsmRenderer)
			rsmRenderer->setDirty();
	}
	doAction(ga, browEdit);
}
void Map::nudgeSelection(int axis, int sign, BrowEdit* browEdit)
{
	float distance = browEdit->useGridForNudge ? browEdit->activeMapView->gridSizeTranslate : browEdit->nudgeDistance;
	auto ga = new GroupAction();
	for (auto n : selectedNodes)
	{
		auto rswObject = n->getComponent<RswObject>();
		auto rsmRenderer = n->getComponent<RsmRenderer>();
		if (rswObject)
		{
			float orig = rswObject->position[axis];
			rswObject->position[axis] += sign * distance;
			ga->addAction(new ObjectChangeAction(n, &rswObject->position[axis], orig, "Nudge"));
		}
		if (rsmRenderer)
			rsmRenderer->setDirty();
	}
	doAction(ga, browEdit);
}

void Map::rotateSelection(int axis, int sign, BrowEdit* browEdit)
{
	float distance = browEdit->useGridForNudge ? browEdit->activeMapView->gridSizeRotate : browEdit->rotateDistance;
	auto ga = new GroupAction();
	glm::vec3 groupCenter = getSelectionCenter();

	for (auto n : selectedNodes)
	{
		auto rswObject = n->getComponent<RswObject>();
		auto rsmRenderer = n->getComponent<RsmRenderer>();
		if (rswObject)
		{
			float orig = rswObject->rotation[axis];
			rswObject->rotation[axis] += sign * distance;
			ga->addAction(new ObjectChangeAction(n, &rswObject->rotation[axis], orig, "Rotate"));
			if (browEdit->activeMapView->pivotPoint == MapView::PivotPoint::GroupCenter)
			{
				auto originalPos = rswObject->position;
				float originalAngle = atan2(rswObject->position.z - groupCenter.z, rswObject->position.x - groupCenter.x);
				float dist = glm::length(glm::vec2(rswObject->position.z - groupCenter.z, rswObject->position.x - groupCenter.x));
				originalAngle -= glm::radians(sign * browEdit->rotateDistance);

				rswObject->position.x = groupCenter.x + dist * glm::cos(originalAngle);
				rswObject->position.z = groupCenter.z + dist * glm::sin(originalAngle);
				ga->addAction(new ObjectChangeAction(n, &rswObject->position, originalPos, "Translate"));
			}
		}
		if (rsmRenderer)
			rsmRenderer->setDirty();
	}
	doAction(ga, browEdit);

}

void Map::randomRotateSelection(int axis, BrowEdit* browEdit)
{
	auto ga = new GroupAction();
	glm::vec3 groupCenter = getSelectionCenter();

	for (auto n : selectedNodes)
	{
		auto rswObject = n->getComponent<RswObject>();
		auto rsmRenderer = n->getComponent<RsmRenderer>();
		if (rswObject)
		{
			float orig = rswObject->rotation[axis];
			rswObject->rotation[axis] = 360 * (rand() / (float)RAND_MAX);
			ga->addAction(new ObjectChangeAction(n, &rswObject->rotation[axis], orig, "Rotate"));
		}
		if (rsmRenderer)
			rsmRenderer->setDirty();
	}
	doAction(ga, browEdit);

}

void Map::flipSelection(int axis, BrowEdit* browEdit)
{
	glm::vec3 center = getSelectionCenter();
	auto ga = new GroupAction();
	for (auto n : selectedNodes)
	{
		auto rswObject = n->getComponent<RswObject>();
		auto rsmRenderer = n->getComponent<RsmRenderer>();
		if (rswObject)
		{
			float orig = rswObject->position[axis];
			rswObject->position[axis] = center[axis] - (rswObject->position[axis] - center[axis]);
			ga->addAction(new ObjectChangeAction(n, &rswObject->position[axis], orig, "Moving"));
		}
		if (rsmRenderer)
			rsmRenderer->setDirty();
	}
	doAction(ga, browEdit);
}

void Map::setSelectedItemsToFloorHeight(BrowEdit* browEdit)
{
	auto gnd = rootNode->getComponent<Gnd>();
	auto ga = new GroupAction();
	for (auto n : selectedNodes)
	{
		auto rswObject = n->getComponent<RswObject>();
		if (rswObject)
		{
			glm::vec3 position(5 * gnd->width + rswObject->position.x, -rswObject->position.y, 5 * gnd->height - rswObject->position.z + 10);

			glm::vec3 pos = gnd->rayCast(math::Ray(position + glm::vec3(0, 1000, 0), glm::vec3(0, -1, 0)));
			if (pos != glm::vec3(std::numeric_limits<float>().max()))
			{
				float orig = rswObject->position.y;
				rswObject->position.y = -pos.y;
				ga->addAction(new ObjectChangeAction(n, &rswObject->position.y, orig, "Moving"));
				if (n->getComponent<RsmRenderer>())
					n->getComponent<RsmRenderer>()->setDirty();
			}
		}
	}
	doAction(ga, browEdit);

}


void Map::selectNear(float nearDistance, BrowEdit* browEdit)
{
	auto selection = selectedNodes;
	auto ga = new GroupAction();
	rootNode->traverse([&](Node* n) {
		auto rswObject = n->getComponent<RswObject>();
		if (!rswObject)
			return;
		auto distance = 999999999.0f;
		for (auto nn : selection)
			if (n != nn && glm::distance(nn->getComponent<RswObject>()->position, rswObject->position) < distance)
				distance = glm::distance(nn->getComponent<RswObject>()->position, rswObject->position);
		if (distance < nearDistance)
		{
			auto sa = new SelectAction(this, n, true, false);
			sa->perform(this, browEdit);
			ga->addAction(sa);
		}
		});
	doAction(ga, browEdit);
}


void Map::selectSameModels(BrowEdit* browEdit)
{
	std::set<std::string> files;
	for (auto n : selectedNodes)
	{
		auto rswModel = n->getComponent<RswModel>();
		if (rswModel)
			files.insert(rswModel->fileName);
	}
	auto ga = new GroupAction();
	rootNode->traverse([&](Node* n) {
		if (std::find(selectedNodes.begin(), selectedNodes.end(), n) != selectedNodes.end())
			return;
		auto rswModel = n->getComponent<RswModel>();
		if (rswModel && std::find(files.begin(), files.end(), rswModel->fileName) != files.end())
		{
			auto sa = new SelectAction(this, n, true, false);
			sa->perform(this, browEdit);
			ga->addAction(sa);
		}
		});
	doAction(ga, browEdit);
}

template<class T>
void Map::selectAll(BrowEdit* browEdit)
{
	std::vector<Node*> newNodes;
	rootNode->traverse([&](Node* n) {
		if (std::find(selectedNodes.begin(), selectedNodes.end(), n) != selectedNodes.end())
			return;
		auto rswObject = n->getComponent<T>();
		if (rswObject)
		{
			newNodes.push_back(n);
		}
		});
	auto sa = new SelectAction(this, newNodes, true, false);
	doAction(sa, browEdit);
}
template void Map::selectAll<RswObject>(BrowEdit* browEdit);
template void Map::selectAll<RswModel>(BrowEdit* browEdit);
template void Map::selectAll<RswSound>(BrowEdit* browEdit);
template void Map::selectAll<RswEffect>(BrowEdit* browEdit);
template void Map::selectAll<RswLight>(BrowEdit* browEdit);

void Map::selectInvert(BrowEdit* browEdit)
{
	auto ga = new GroupAction();
	rootNode->traverse([&](Node* n) {
		bool selected = std::find(selectedNodes.begin(), selectedNodes.end(), n) != selectedNodes.end();

		auto rswObject = n->getComponent<RswObject>();
		if (rswObject)
		{
			auto sa = new SelectAction(this, n, true, selected);
			sa->perform(this, browEdit);
			ga->addAction(sa);
		}
		});
	doAction(ga, browEdit);
}


void Map::deleteSelection(BrowEdit* browEdit)
{
	if (selectedNodes.size() > 0)
	{
		auto ga = new GroupAction();
		for (auto n : selectedNodes)
			ga->addAction(new DeleteObjectAction(n));
		doAction(ga, browEdit);
	}
}

void Map::copySelection()
{
	json clipboard;
	for (auto n : selectedNodes)
		clipboard.push_back(*n);
	ImGui::SetClipboardText(clipboard.dump(1).c_str());
}


void Map::pasteSelection(BrowEdit* browEdit)
{
	try
	{
		auto c = ImGui::GetClipboardText();
		if (c == nullptr)
			return;
		std::string cb = c;
		if (cb == "")
			return;
		json clipboard = json::parse(cb);
		if (clipboard.size() > 0)
		{
			for (auto n : browEdit->newNodes) //TODO: should I do this?
				delete n.first;
			browEdit->newNodes.clear();

			for (auto n : clipboard)
			{
				auto newNode = new Node(n["name"].get<std::string>());
				newNode->addComponentsFromJson(n["components"]);
				glm::vec3 pos(0.0f);
				if (newNode->getComponent<RswObject>())
					pos = newNode->getComponent<RswObject>()->position;
				browEdit->newNodes.push_back(std::pair<Node*, glm::vec3>(newNode, pos));
			}
			glm::vec3 center(0, 0, 0);
			for (auto& n : browEdit->newNodes)
				center += n.second;
			center /= browEdit->newNodes.size();
			
			browEdit->newNodesCenter = center;
			for (auto& n : browEdit->newNodes)
				n.second = n.second - center;
			browEdit->newNodePlacement = BrowEdit::Ground;
		}
	}
	catch (...) {
		std::cerr << "!!!!!!!!!!! Some sort of error occurred???" << std::endl;
	};
}


void Map::optimizeGndTiles(BrowEdit* browEdit)
{
	rootNode->getComponent<Gnd>()->cleanTiles();
}


void Map::exportShadowMap(BrowEdit* browEdit, bool exportWalls, bool exportBorders)
{
	int tileSize = 8;
	int tilePadding = 0;
	if (!exportBorders)
	{
		tileSize = 6;
		tilePadding = 1;
	}
	int wallMultiplier = 2;
	if (!exportWalls)
		wallMultiplier = 1;

	auto shadowmapcpy = [&](unsigned char* src, char* dst, int dstx, int dsty, int w)
	{
		for (int x = 0; x < tileSize; x++)
			for (int y = 0; y < tileSize; y++)
				dst[(dstx+x)+w*(dsty+y)] = src[(x+tilePadding) + 8 * (y+tilePadding)];
	};
	auto gnd = rootNode->getComponent<Gnd>();
	char* img = new char[gnd->width * tileSize* wallMultiplier * gnd->height * tileSize * wallMultiplier];
	memset(img, 0, gnd->width * tileSize * wallMultiplier * gnd->height * tileSize * wallMultiplier);
	for (auto x = 0; x < gnd->width; x++)
	{
		for (auto y = 0; y < gnd->height; y++)
		{
			auto cube = gnd->cubes[x][y];
			int xx = wallMultiplier * tileSize * x;
			int yy = wallMultiplier * tileSize * y;

			if (cube->tileUp > -1 && gnd->tiles[cube->tileUp]->lightmapIndex > -1)
				shadowmapcpy(gnd->lightmaps[gnd->tiles[cube->tileUp]->lightmapIndex]->data, img, xx, yy, wallMultiplier*tileSize*gnd->width);
			if (exportWalls)
			{
				if (cube->tileSide > -1 && gnd->tiles[cube->tileSide]->lightmapIndex > -1)
					shadowmapcpy(gnd->lightmaps[gnd->tiles[cube->tileSide]->lightmapIndex]->data, img, xx, yy + tileSize, wallMultiplier * tileSize * gnd->width);
				if (cube->tileFront > -1 && gnd->tiles[cube->tileFront]->lightmapIndex > -1)
					shadowmapcpy(gnd->lightmaps[gnd->tiles[cube->tileFront]->lightmapIndex]->data, img, xx + tileSize, yy, wallMultiplier * tileSize * gnd->width);
			}
		}
	}
	stbi_write_png((browEdit->config.ropath + name + ".shadowmap.png").c_str(), gnd->width* wallMultiplier * tileSize, gnd->height* wallMultiplier * tileSize, 1, img, gnd->width * wallMultiplier * tileSize);
	delete[] img;
}
void Map::importShadowMap(BrowEdit* browEdit, bool exportWalls, bool exportBorders)
{
	int tileSize = 8;
	int tilePadding = 0;
	if (!exportBorders)
	{
		tileSize = 6;
		tilePadding = 1;
	}
	int wallMultiplier = 2;
	if (!exportWalls)
		wallMultiplier = 1;

	auto shadowmapcpy = [&](unsigned char* dst, unsigned char* src, int srcx, int srcy, int w)
	{
		for (int x = 0; x < tileSize; x++)
			for (int y = 0; y < tileSize; y++)
				dst[(x + tilePadding) + 8 * (y + tilePadding)] = src[(srcx + x) + w * (srcy + y)];
	};
	auto gnd = rootNode->getComponent<Gnd>();
	int w, h,c;
	unsigned char* img = stbi_load((browEdit->config.ropath + name + ".shadowmap.png").c_str(), &w, &h, &c, 1);
	if (w != gnd->width * wallMultiplier * tileSize ||
		h != gnd->height * wallMultiplier * tileSize)
	{
		std::cerr << "Image is wrong size" << std::endl;
		return;
	}

	for (auto x = 0; x < gnd->width; x++)
	{
		for (auto y = 0; y < gnd->height; y++)
		{
			auto cube = gnd->cubes[x][y];
			int xx = wallMultiplier * tileSize * x;
			int yy = wallMultiplier * tileSize * y;

			if (cube->tileUp > -1 && gnd->tiles[cube->tileUp]->lightmapIndex > -1)
				shadowmapcpy(gnd->lightmaps[gnd->tiles[cube->tileUp]->lightmapIndex]->data, img, xx, yy, wallMultiplier * tileSize * gnd->width);
			if (exportWalls)
			{
				if (cube->tileSide > -1 && gnd->tiles[cube->tileSide]->lightmapIndex > -1)
					shadowmapcpy(gnd->lightmaps[gnd->tiles[cube->tileSide]->lightmapIndex]->data, img, xx, yy + tileSize, wallMultiplier * tileSize * gnd->width);
				if (cube->tileFront > -1 && gnd->tiles[cube->tileFront]->lightmapIndex > -1)
					shadowmapcpy(gnd->lightmaps[gnd->tiles[cube->tileFront]->lightmapIndex]->data, img, xx + tileSize, yy, wallMultiplier * tileSize * gnd->width);
			}
		}
	}
	rootNode->getComponent<GndRenderer>()->gndShadowDirty = true;
	stbi_image_free(img);
}

void Map::exportLightMap(BrowEdit* browEdit, bool exportWalls, bool exportBorders)
{
	int tileSize = 8;
	int tilePadding = 0;
	if (!exportBorders)
	{
		tileSize = 6;
		tilePadding = 1;
	}
	int wallMultiplier = 2;
	if (!exportWalls)
		wallMultiplier = 1;

	auto lightmapcpy = [&](unsigned char* src, char* dst, int dstx, int dsty, int w)
	{
		for (int x = 0; x < tileSize; x++)
			for (int y = 0; y < tileSize; y++)
				for (int c = 0; c < 3; c++)
					dst[((dstx + x) + w * (dsty + y))*3+c] = src[64+ ((x+tilePadding) + 8 * (y+tilePadding))*3+c];
	};
	auto gnd = rootNode->getComponent<Gnd>();
	char* img = new char[(gnd->width * tileSize * wallMultiplier * gnd->height * tileSize * wallMultiplier)*3];
	memset(img, 0, gnd->width * tileSize * wallMultiplier * gnd->height * tileSize * wallMultiplier *3);
	for (auto x = 0; x < gnd->width; x++)
	{
		for (auto y = 0; y < gnd->height; y++)
		{
			auto cube = gnd->cubes[x][y];
			int xx = tileSize * wallMultiplier * x;
			int yy = tileSize * wallMultiplier * y;

			if (cube->tileUp > -1 && gnd->tiles[cube->tileUp]->lightmapIndex > -1)
				lightmapcpy(gnd->lightmaps[gnd->tiles[cube->tileUp]->lightmapIndex]->data, img, xx, yy, tileSize * wallMultiplier * gnd->width);
			if (exportWalls)
			{
				if (cube->tileSide > -1 && gnd->tiles[cube->tileSide]->lightmapIndex > -1)
					lightmapcpy(gnd->lightmaps[gnd->tiles[cube->tileSide]->lightmapIndex]->data, img, xx, yy + tileSize, tileSize * wallMultiplier * gnd->width);
				if (cube->tileFront > -1 && gnd->tiles[cube->tileFront]->lightmapIndex > -1)
					lightmapcpy(gnd->lightmaps[gnd->tiles[cube->tileFront]->lightmapIndex]->data, img, xx + tileSize, yy, tileSize * wallMultiplier * gnd->width);
			}
		}
	}
	stbi_write_png((browEdit->config.ropath + name + ".colormap.png").c_str(), gnd->width * tileSize * wallMultiplier, gnd->height * tileSize * wallMultiplier, 3, img, gnd->width * tileSize * wallMultiplier *3);
	delete[] img;
}
void Map::importLightMap(BrowEdit* browEdit, bool exportWalls, bool exportBorders)
{
	int tileSize = 8;
	int tilePadding = 0;
	if (!exportBorders)
	{
		tileSize = 6;
		tilePadding = 1;
	}
	int wallMultiplier = 2;
	if (!exportWalls)
		wallMultiplier = 1;

	auto lightmapcpy = [&](unsigned char* dst, unsigned char* src, int srcx, int srcy, int w)
	{
		for (int x = 0; x < tileSize; x++)
			for (int y = 0; y < tileSize; y++)
				for (int c = 0; c < 3; c++)
					dst[64 + ((x + tilePadding) + 8 * (y + tilePadding)) * 3 + c] = src[((srcx + x) + w * (srcy + y)) * 3 + c];
	};
	auto gnd = rootNode->getComponent<Gnd>();
	gnd->makeLightmapsUnique();
	int w, h, c;
	unsigned char* img = stbi_load((browEdit->config.ropath + name + ".colormap.png").c_str(), &w, &h, &c, 3);
	if (w != gnd->width * wallMultiplier*tileSize ||
		h != gnd->height * wallMultiplier*tileSize)
	{
		std::cerr << "Image is wrong size" << std::endl;
		return;
	}
	for (auto x = 0; x < gnd->width; x++)
	{
		for (auto y = 0; y < gnd->height; y++)
		{
			auto cube = gnd->cubes[x][y];
			int xx = tileSize * wallMultiplier * x;
			int yy = tileSize * wallMultiplier * y;

			if (cube->tileUp > -1)
			{
				if (gnd->tiles[cube->tileUp]->lightmapIndex == -1)
				{
					gnd->tiles[cube->tileUp]->lightmapIndex = (int)gnd->lightmaps.size();
					gnd->lightmaps.push_back(new Gnd::Lightmap(gnd));
				}
				lightmapcpy(gnd->lightmaps[gnd->tiles[cube->tileUp]->lightmapIndex]->data, img, xx, yy, tileSize * wallMultiplier * gnd->width);
			}
			if (exportWalls)
			{
				if (cube->tileSide > -1)
				{
					if (gnd->tiles[cube->tileSide]->lightmapIndex == -1)
					{
						gnd->tiles[cube->tileSide]->lightmapIndex = (int)gnd->lightmaps.size();
						gnd->lightmaps.push_back(new Gnd::Lightmap(gnd));
					}
					lightmapcpy(gnd->lightmaps[gnd->tiles[cube->tileSide]->lightmapIndex]->data, img, xx, yy + tileSize, tileSize * wallMultiplier * gnd->width);
				}
				if (cube->tileFront > -1)
				{
					if (gnd->tiles[cube->tileFront]->lightmapIndex == -1)
					{
						gnd->tiles[cube->tileFront]->lightmapIndex = (int)gnd->lightmaps.size();
						gnd->lightmaps.push_back(new Gnd::Lightmap(gnd));
					}
					lightmapcpy(gnd->lightmaps[gnd->tiles[cube->tileFront]->lightmapIndex]->data, img, xx + tileSize, yy, tileSize * wallMultiplier * gnd->width);
				}
			}
		}
	}
	gnd->makeLightmapsUnique();
	rootNode->getComponent<GndRenderer>()->gndShadowDirty = true;

	stbi_image_free(img);
}

void Map::exportTileColors(BrowEdit* browEdit, bool exportWalls)
{
	int wallMultiplier = 2;
	if (!exportWalls)
		wallMultiplier = 1;

	auto lightmapcpy = [&](const glm::ivec4& src, char* dst, int dstx, int dsty, int w)
	{
		for (int c = 0; c < 3; c++)
			dst[((dstx) + w * (dsty)) * 3 + c] = src[c];
	};
	auto gnd = rootNode->getComponent<Gnd>();
	char* img = new char[(gnd->width * wallMultiplier * gnd->height * wallMultiplier) * 3];
	memset(img, 0, gnd->width * wallMultiplier * gnd->height * wallMultiplier * 3);
	for (auto x = 0; x < gnd->width; x++)
	{
		for (auto y = 0; y < gnd->height; y++)
		{
			auto cube = gnd->cubes[x][y];
			int xx = wallMultiplier * x;
			int yy = wallMultiplier * y;

			if (cube->tileUp > -1)
				lightmapcpy(gnd->tiles[cube->tileUp]->color, img, xx, yy, wallMultiplier * gnd->width);
			if (exportWalls)
			{
				if (cube->tileSide > -1)
					lightmapcpy(gnd->tiles[cube->tileSide]->color, img, xx, yy + 1, wallMultiplier * gnd->width);
				if (cube->tileFront > -1)
					lightmapcpy(gnd->tiles[cube->tileFront]->color, img, xx + 1, yy, wallMultiplier * gnd->width);
			}
		}
	}


	stbi_write_png((browEdit->config.ropath + name + ".tilecolor.png").c_str(), gnd->width * wallMultiplier, gnd->height * wallMultiplier, 3, img, gnd->width * wallMultiplier * 3);
	delete[] img;
}

void Map::importTileColors(BrowEdit* browEdit, bool exportWalls)
{
	int wallMultiplier = 2;
	if (!exportWalls)
		wallMultiplier = 1;

	auto lightmapcpy = [&](glm::ivec4& dst, unsigned char* src, int srcx, int srcy, int w)
	{
		for (int c = 0; c < 3; c++)
			dst[c] = src[((srcx)+w * (srcy)) * 3 + c];
	};
	auto gnd = rootNode->getComponent<Gnd>();
	int w, h, c;
	unsigned char* img = stbi_load((browEdit->config.ropath + name + ".colormap.png").c_str(), &w, &h, &c, 1);
	if (w != gnd->width * wallMultiplier ||
		h != gnd->height * wallMultiplier)
	{
		std::cerr << "Image is wrong size" << std::endl;
		return;
	}
	for (auto x = 0; x < gnd->width; x++)
	{
		for (auto y = 0; y < gnd->height; y++)
		{
			auto cube = gnd->cubes[x][y];
			int xx = wallMultiplier * x;
			int yy = wallMultiplier * y;

			if (cube->tileUp > -1)
				lightmapcpy(gnd->tiles[cube->tileUp]->color, img, xx, yy, wallMultiplier * gnd->width);
			if (exportWalls)
			{
				if (cube->tileSide > -1)
					lightmapcpy(gnd->tiles[cube->tileSide]->color, img, xx, yy + 1, wallMultiplier * gnd->width);
				if (cube->tileFront > -1)
					lightmapcpy(gnd->tiles[cube->tileFront]->color, img, xx + 1, yy, wallMultiplier * gnd->width);
			}
		}
	}
	stbi_image_free(img);
}



void Map::recalculateQuadTree(BrowEdit* browEdit)
{
	std::cout << "Recalculating quadtree" << std::endl;
	auto rsw = rootNode->getComponent<Rsw>();
	//if (!rsw->quadtree)
	{
		auto gnd = rootNode->getComponent<Gnd>();
		if(rsw->quadtree)
			delete rsw->quadtree;
		rsw->quadtree = new Rsw::QuadTreeNode(10*-gnd->width/2.0f, 10 * -gnd->height/2.0f, (float)gnd->width*10 - 1.0f, (float)gnd->height*10 - 1.0f, 0);
	}

	rsw->recalculateQuadtree();
	std::cout << "Done recalculating quadtree" << std::endl;
}

void Map::growTileSelection(BrowEdit* browEdit)
{
	auto around = getSelectionAroundTiles();
	around.insert(around.end(), tileSelection.begin(), tileSelection.end());
	auto action = new TileSelectAction(this, around);
	doAction(action, browEdit);
}

void Map::shrinkTileSelection(BrowEdit* browEdit)
{
	auto gnd = rootNode->getComponent<Gnd>();
	std::vector<glm::ivec2> shrunk;
	auto isTileSelected = [&](int x, int y) { return std::find(tileSelection.begin(), tileSelection.end(), glm::ivec2(x, y)) != tileSelection.end(); };

	for (auto& t : tileSelection)
	{
		bool ok = true;
		for (int xx = -1; xx <= 1; xx++)
			for (int yy = -1; yy <= 1; yy++)
				if (gnd->inMap(t + glm::ivec2(xx, yy)) &&
					std::find(shrunk.begin(), shrunk.end(), glm::ivec2(t.x, t.y)) == shrunk.end() &&
					!isTileSelected(t.x + xx, t.y + yy))
					ok = false;
		if(ok)
			shrunk.push_back(t);
	}

	auto action = new TileSelectAction(this, shrunk);
	doAction(action, browEdit);
}


void Map::createPrefab(const std::string& fileName, BrowEdit* browEdit)
{
	std::filesystem::path filePath("data\\prefabs\\" + fileName);
	std::filesystem::create_directories(filePath.parent_path());
	std::ofstream outFile(filePath);
	json clipboard;
	for (auto n : selectedNodes)
		clipboard.push_back(*n);
	outFile << clipboard;
	util::FileIO::reload("data\\prefabs");
}

void WallCalculation::calcUV(const glm::ivec3& position, Gnd* gnd)
{
	auto cube = gnd->cubes[position.x][position.y];
	int index = (offset + position.x + position.y) % wallWidth;

	float minHeight = 0;
	float maxHeight = 0;

	if (position.z == 1 && position.x < gnd->width-1) //tileside
	{
		auto cube2 = gnd->cubes[position.x + 1][position.y];
		index = (gnd->height - position.y - offset) % wallWidth;
		minHeight = glm::min(glm::min(glm::min(-cube->h2, -cube->h4), -cube2->h1), -cube2->h3);
		maxHeight = glm::max(glm::max(glm::max(-cube->h2, -cube->h4), -cube2->h1), -cube2->h3);
	}
	if (position.z == 2 && position.y < gnd->height-1) //tilefront
	{
		auto cube2 = gnd->cubes[position.x][position.y + 1];
		minHeight = glm::min(glm::min(glm::min(-cube->h4, -cube->h3), -cube2->h2), -cube2->h1);
		maxHeight = glm::max(glm::max(glm::max(-cube->h4, -cube->h3), -cube2->h2), -cube2->h1);
	}

	if (!wallTopAuto)
		maxHeight = this->wallTop;
	if (!wallBottomAuto)
		minHeight = this->wallBottom;


	g_uv1 = uvStart + xInc * (float)index + yInc * (float)0.0f;
	g_uv2 = g_uv1 + xInc;
	g_uv3 = g_uv1 + yInc;
	g_uv4 = g_uv1 + xInc + yInc;

	g_uv1.y = 1 - g_uv1.y;
	g_uv2.y = 1 - g_uv2.y;
	g_uv3.y = 1 - g_uv3.y;
	g_uv4.y = 1 - g_uv4.y;

	if (autoStraight)
	{
		float h1 = -cube->h4;
		float h2 = -cube->h2;
		float h3, h4;
		if (position.z == 1 && gnd->inMap(position + glm::ivec3(1, 0, 0)))
		{
			h3 = -gnd->cubes[position.x + 1][position.y]->h3;
			h4 = -gnd->cubes[position.x + 1][position.y]->h1;
		}
		else if (position.z == 2 && gnd->inMap(position + glm::ivec3(0, 1, 0)))
		{
			h1 = -cube->h3;
			h2 = -cube->h4;
			h4 = -gnd->cubes[position.x][position.y + 1]->h2;
			h3 = -gnd->cubes[position.x][position.y + 1]->h1;
		}
		else
			return;

		if (xInc.y == 0) {
			glm::vec2 guv1(g_uv1.x, glm::mix(g_uv1.y, g_uv3.y, (h1 - minHeight) / (maxHeight - minHeight)));
			glm::vec2 guv2(g_uv2.x, glm::mix(g_uv2.y, g_uv4.y, (h2 - minHeight) / (maxHeight - minHeight)));
			glm::vec2 guv3(g_uv3.x, glm::mix(g_uv1.y, g_uv3.y, (h3 - minHeight) / (maxHeight - minHeight)));
			glm::vec2 guv4(g_uv4.x, glm::mix(g_uv2.y, g_uv4.y, (h4 - minHeight) / (maxHeight - minHeight)));

			g_uv1 = guv1;
			g_uv2 = guv2;
			g_uv3 = guv3;
			g_uv4 = guv4;
		}
		else {
			glm::vec2 guv1(glm::mix(g_uv1.x, g_uv3.x, (h1 - minHeight) / (maxHeight - minHeight)), g_uv1.y);
			glm::vec2 guv2(glm::mix(g_uv2.x, g_uv4.x, (h2 - minHeight) / (maxHeight - minHeight)), g_uv2.y);
			glm::vec2 guv3(glm::mix(g_uv1.x, g_uv3.x, (h3 - minHeight) / (maxHeight - minHeight)), g_uv3.y);
			glm::vec2 guv4(glm::mix(g_uv2.x, g_uv4.x, (h4 - minHeight) / (maxHeight - minHeight)), g_uv4.y);

			g_uv1 = guv1;
			g_uv2 = guv2;
			g_uv3 = guv3;
			g_uv4 = guv4;
		}
	}

}


void WallCalculation::prepare(BrowEdit* browEdit)
{
	wallWidth = browEdit->activeMapView->wallWidth;
	offset = browEdit->activeMapView->wallOffset;
	wallTop = browEdit->activeMapView->wallTop;
	wallTopAuto = browEdit->activeMapView->wallTopAuto;
	wallBottom = browEdit->activeMapView->wallBottom;
	wallBottomAuto = browEdit->activeMapView->wallBottomAuto;
	autoStraight = browEdit->activeMapView->wallAutoStraight;


	glm::vec2 uvSize = browEdit->activeMapView->textureEditUv2 - browEdit->activeMapView->textureEditUv1;
	uv1 = glm::vec2(0, 0);
	uv4 = glm::vec2(1, 1);

	uv2 = glm::vec2(uv4.x, uv1.y);
	uv3 = glm::vec2(uv1.x, uv4.y);

	if (browEdit->activeMapView->textureBrushFlipD)
		std::swap(uv1, uv4); // different from texturebrush window due to flipped UV

	if (browEdit->activeMapView->textureBrushFlipH)
	{
		uv1.x = 1 - uv1.x;
		uv2.x = 1 - uv2.x;
		uv3.x = 1 - uv3.x;
		uv4.x = 1 - uv4.x;
	}
	if (browEdit->activeMapView->textureBrushFlipV)
	{
		uv1.y = 1 - uv1.y;
		uv2.y = 1 - uv2.y;
		uv3.y = 1 - uv3.y;
		uv4.y = 1 - uv4.y;
	}
	xInc = (uv2 - uv1) * uvSize / (float)browEdit->activeMapView->wallWidth;
	yInc = (uv3 - uv1) * uvSize / (float)1;
	if (browEdit->activeMapView->textureBrushFlipD)
	{
		if (browEdit->activeMapView->textureBrushFlipV ^ browEdit->activeMapView->textureBrushFlipH)
		{
			xInc = (uv1 - uv2) * uvSize / (float)1;
			yInc = (uv1 - uv3) * uvSize / (float)browEdit->activeMapView->wallWidth;
		}
		if (browEdit->activeMapView->textureBrushFlipH == browEdit->activeMapView->textureBrushFlipV)
		{
			xInc = (uv2 - uv1) * uvSize / (float)1;
			yInc = (uv3 - uv1) * uvSize / (float)browEdit->activeMapView->wallWidth;

		}
		std::swap(xInc, yInc);
		std::swap(xInc.x, xInc.y);
		std::swap(yInc.x, yInc.y);
	}

	uvStart = uv1 + browEdit->activeMapView->textureEditUv1.x * (uv2 - uv1) + browEdit->activeMapView->textureEditUv1.y * (uv3 - uv1);
}

extern std::vector<glm::ivec3> selectedWalls; //ewwww
void Map::wallAddSelected(BrowEdit* browEdit)
{
	auto gnd = rootNode->getComponent<Gnd>();
	auto gndRenderer = rootNode->getComponent<GndRenderer>();

	WallCalculation calculation;
	calculation.prepare(browEdit);

	auto ga = new GroupAction();
	int id = (int)gnd->tiles.size();

	for (const auto& w : selectedWalls)
	{
		auto cube = gnd->cubes[w.x][w.y];

		calculation.calcUV(w, gnd);

		auto t = new Gnd::Tile();
		t->color = glm::ivec4(255, 255, 255, 255);
		t->textureIndex = browEdit->activeMapView->textureSelected;
		t->lightmapIndex = 0;

		t->v1 = calculation.g_uv1;
		t->v2 = calculation.g_uv2;
		t->v3 = calculation.g_uv3;
		t->v4 = calculation.g_uv4;

		if (w.z == 1 && cube->tileSide == -1)
			ga->addAction(new CubeTileChangeAction(w, cube, cube->tileUp, cube->tileFront, id));
		if (w.z == 2 && cube->tileFront == -1)
			ga->addAction(new CubeTileChangeAction(w, cube, cube->tileUp, id, cube->tileSide));
		ga->addAction(new TileNewAction(t));
		id++;
	}
	doAction(ga, browEdit);
	gndRenderer->gndShadowDirty = true;
}


void Map::wallReApplySelected(BrowEdit* browEdit)
{
	auto gnd = rootNode->getComponent<Gnd>();
	auto gndRenderer = rootNode->getComponent<GndRenderer>();

	WallCalculation calculation;
	calculation.prepare(browEdit);

	auto ga = new GroupAction();
	int id = (int)gnd->tiles.size();

	for (const auto& w : selectedWalls)
	{
		auto cube = gnd->cubes[w.x][w.y];

		calculation.calcUV(w, gnd);

		auto t = new Gnd::Tile();
		t->color = glm::ivec4(255, 255, 255, 255);
		t->textureIndex = browEdit->activeMapView->textureSelected;
		t->lightmapIndex = 0;

		t->v1 = calculation.g_uv1;
		t->v2 = calculation.g_uv2;
		t->v3 = calculation.g_uv3;
		t->v4 = calculation.g_uv4;

		if (w.z == 1)
			ga->addAction(new CubeTileChangeAction(w, cube, cube->tileUp, cube->tileFront, id));
		if (w.z == 2)
			ga->addAction(new CubeTileChangeAction(w, cube, cube->tileUp, id, cube->tileSide));
		ga->addAction(new TileNewAction(t));
		id++;
	}
	doAction(ga, browEdit);
	gndRenderer->gndShadowDirty = true;
}



void Map::wallRemoveSelected(BrowEdit* browEdit)
{
	auto gnd = rootNode->getComponent<Gnd>();
	auto gndRenderer = rootNode->getComponent<GndRenderer>();
	auto ga = new GroupAction();
	for (const auto& w : selectedWalls)
	{
		auto cube = gnd->cubes[w.x][w.y];
		if (w.z == 1 && cube->tileSide != -1)
			ga->addAction(new CubeTileChangeAction(w, cube, cube->tileUp, cube->tileFront, -1));
		if (w.z == 2 && cube->tileFront != -1)
			ga->addAction(new CubeTileChangeAction(w, cube, cube->tileUp, -1, cube->tileSide));

	}
	doAction(ga, browEdit);
}


void Map::wallFlipSelectedTextureHorizontal(BrowEdit* browEdit)
{
	auto gnd = rootNode->getComponent<Gnd>();
	auto gndRenderer = rootNode->getComponent<GndRenderer>();
	auto ga = new GroupAction();

	for (const auto& w : selectedWalls)
	{
		auto cube = gnd->cubes[w.x][w.y];
		Gnd::Tile* tile = nullptr;
		if (w.z == 1 && cube->tileSide != -1)
			tile = gnd->tiles[cube->tileSide];
		if (w.z == 2 && cube->tileFront != -1)
			tile = gnd->tiles[cube->tileFront];
		if (!tile)
			continue;
		Gnd::Tile original(*tile);
		std::swap(tile->v1.x, tile->v2.x);
		std::swap(tile->v3.x, tile->v4.x);
		for(int i = 0; i < 4; i++)
			ga->addAction(new TileChangeAction<glm::vec2>(w, tile, &tile->texCoords[i], original.texCoords[i], "Change UV"));
	}
	doAction(ga, browEdit);
}

void Map::wallFlipSelectedTextureVertical(BrowEdit* browEdit)
{
	auto gnd = rootNode->getComponent<Gnd>();
	auto gndRenderer = rootNode->getComponent<GndRenderer>();
	auto ga = new GroupAction();

	for (const auto& w : selectedWalls)
	{
		auto cube = gnd->cubes[w.x][w.y];
		Gnd::Tile* tile = nullptr;
		if (w.z == 1 && cube->tileSide != -1)
			tile = gnd->tiles[cube->tileSide];
		if (w.z == 2 && cube->tileFront != -1)
			tile = gnd->tiles[cube->tileFront];
		if (!tile)
			continue;
		Gnd::Tile original(*tile);
		std::swap(tile->v1.y, tile->v3.y);
		std::swap(tile->v2.y, tile->v4.y);
		for (int i = 0; i < 4; i++)
			ga->addAction(new TileChangeAction<glm::vec2>(w, tile, &tile->texCoords[i], original.texCoords[i], "Change UV"));
	}
	doAction(ga, browEdit);
}
