#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/actions/Action.h>
#include <browedit/actions/ObjectChangeAction.h>
#include <browedit/actions/GroupAction.h>
#include <browedit/actions/SelectAction.h>
#include <browedit/actions/DeleteObjectAction.h>
#include <browedit/components/Rsw.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/GndRenderer.h>
#include <browedit/components/BillboardRenderer.h>
#include <browedit/BrowEdit.h>
#include <browedit/util/ResourceManager.h>

#include <json.hpp>
using json = nlohmann::json;

#include <stb/stb_image_write.h>
#include <stb/stb_image.h>
#include <set>
#include <iostream>
#include <thread>
#include <mutex>


Map::Map(const std::string& name) : name(name)
{
	rootNode = new Node(name);
	auto rsw = new Rsw();
	rootNode->addComponent(rsw);	
	rsw->load(name);
}



void Map::doAction(Action* action, BrowEdit* browEdit)
{
	if (std::find(undoStack.begin(), undoStack.end(), action) != undoStack.end())
	{
		std::cerr << "Double undo!" << std::endl;
		throw "Oops";
		return;
	}
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
	glm::vec3 center;
	for (auto n : selectedNodes)
		if (n->getComponent<RswObject>())
		{
			center += n->getComponent<RswObject>()->position;
			count++;
		}
	center /= count;
	return center;
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

void Map::selectNear(float nearDistance, BrowEdit* browEdit)
{
	auto selection = selectedNodes;
	auto ga = new GroupAction();
	rootNode->traverse([&](Node* n) {
		auto rswModel = n->getComponent<RswModel>();
		auto rswObject = n->getComponent<RswObject>();
		if (!rswObject)
			return;
		auto distance = 999999999.0f;
		for (auto nn : selection)
			if (glm::distance(nn->getComponent<RswObject>()->position, rswObject->position) < distance)
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
void Map::selectAll(BrowEdit* browEdit)
{
	auto ga = new GroupAction();
	rootNode->traverse([&](Node* n) {
		if (std::find(selectedNodes.begin(), selectedNodes.end(), n) != selectedNodes.end())
			return;
		auto rswModel = n->getComponent<RswModel>();
		if (rswModel)
		{
			auto sa = new SelectAction(this, n, true, false);
			sa->perform(this, browEdit);
			ga->addAction(sa);
		}
		});
	doAction(ga, browEdit);
}
void Map::selectInvert(BrowEdit* browEdit)
{
	auto ga = new GroupAction();
	rootNode->traverse([&](Node* n) {
		bool selected = std::find(selectedNodes.begin(), selectedNodes.end(), n) != selectedNodes.end();

		auto rswModel = n->getComponent<RswModel>();
		if (rswModel)
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
		std::string cb = ImGui::GetClipboardText();
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
				for (auto c : n["components"])
				{
					if (c["type"] == "rswobject")
					{
						auto rswObject = new RswObject();
						from_json(c, *rswObject);
						newNode->addComponent(rswObject);
					}
					if (c["type"] == "rswmodel")
					{
						auto rswModel = new RswModel();
						from_json(c, *rswModel);
						newNode->addComponent(rswModel);
						newNode->addComponent(util::ResourceManager<Rsm>::load("data\\model\\" + util::utf8_to_iso_8859_1(rswModel->fileName)));
						newNode->addComponent(new RsmRenderer());
						newNode->addComponent(new RswModelCollider());
					}
					if (c["type"] == "rswlight")
					{
						auto rswLight = new RswLight();
						from_json(c, *rswLight);
						newNode->addComponent(rswLight);
						newNode->addComponent(new BillboardRenderer("data\\light.png", "data\\light_selected.png"));
						newNode->addComponent(new CubeCollider(5));
					}
					if (c["type"] == "rsweffect")
					{
						auto rswEffect = new RswEffect();
						from_json(c, *rswEffect);
						newNode->addComponent(rswEffect);
						newNode->addComponent(new BillboardRenderer("data\\effect.png", "data\\effect_selected.png"));
						newNode->addComponent(new CubeCollider(5));
					}
					if (c["type"] == "rswsound")
					{
						auto rswSound = new RswSound();
						from_json(c, *rswSound);
						newNode->addComponent(rswSound);
						newNode->addComponent(new BillboardRenderer("data\\sound.png", "data\\sound_selected.png"));
						newNode->addComponent(new CubeCollider(5));
					}
				}
				browEdit->newNodes.push_back(std::pair<Node*, glm::vec3>(newNode, newNode->getComponent<RswObject>()->position));
			}
			glm::vec3 center(0, 0, 0);
			for (auto& n : browEdit->newNodes)
				center += n.second;
			center /= browEdit->newNodes.size();

			for (auto& n : browEdit->newNodes)
				n.second = n.second - center;
		}
	}
	catch (...) {
		std::cerr << "!!!!!!!!!!! Some sort of error occurred???" << std::endl;
	};
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
	int w, h, c;
	unsigned char* img = stbi_load((browEdit->config.ropath + name + ".colormap.png").c_str(), &w, &h, &c, 1);
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


	stbi_write_png((browEdit->config.ropath + name + ".tilecolor.png").c_str(), gnd->width * 2, gnd->height * 2, 3, img, gnd->width * 2 * 3);
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
	rootNode->getComponent<GndRenderer>()->gndTileColorDirty = true;

	stbi_image_free(img);
}