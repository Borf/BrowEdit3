#include <Windows.h>
#include <glad/glad.h>
#include "Rsw.h"
#include "Gnd.h"
#include "Rsm.h"
#include "GndRenderer.h"
#include "RsmRenderer.h"
#include "LubRenderer.h"
#include "WaterRenderer.h"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <browedit/BrowEdit.h>
#include <browedit/Image.h>
#include <browedit/util/ResourceManager.h>
#include <browedit/util/FileIO.h>
#include <browedit/util/Util.h>
#include <browedit/math/Ray.h>
#include <browedit/Map.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <glm/gtc/type_ptr.hpp>
#include <browedit/Node.h>


Rsw::Rsw()
{
}

Rsw::~Rsw()
{
	if (quadtree)
		delete quadtree;
}



void Rsw::load(const std::string& fileName, Map* map, BrowEdit* browEdit, bool loadModels, bool loadGnd)
{
	std::cout << "Loading " << fileName << std::endl;
	auto file = util::FileIO::open(fileName);
	if (!file)
	{
		std::cerr << "Could not open file " << fileName << std::endl;
		return;
	}
	quadtree = nullptr;

	json lubInfo = json::array();

	std::string mapName = fileName;
	mapName = mapName.substr(0, mapName.size() - 4);
	mapName = mapName.substr(mapName.rfind("\\") + 1);
	auto lub = util::FileIO::open("data\\lua files\\effecttool\\" + mapName + ".lub");
	if (!lub)
		lub = util::FileIO::open("data\\luafiles514\\lua files\\effecttool\\" + mapName + ".lub");
	if (!lub)
		lub = util::FileIO::open("data\\LuaFiles514\\Lua Files\\effecttool\\" + mapName + ".lub");
	if (lub)
	{
		char c = lub->get();
		lub->seekg(0, std::ios_base::beg);
		std::string data = "";
		if (c == 0x1b)
		{
			std::ofstream out("tmp.lub", std::ios_base::binary | std::ios_base::out);
			char buf[1024];
			while (!lub->eof())
			{
				lub->read(buf, 1024);
				auto count = lub->gcount();
				out.write(buf, count);
			}
			out.close();

			STARTUPINFO info = { sizeof(info) };
			PROCESS_INFORMATION processInfo;
			std::string cmd = browEdit->config.grfEditorPath + "GrfCL.exe -lub .\\tmp.lub .\\tmp.lua";

			if (CreateProcess(nullptr, (LPSTR)cmd.c_str(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &info, &processInfo))
			{
				WaitForSingleObject(processInfo.hProcess, INFINITE);
				CloseHandle(processInfo.hProcess);
				CloseHandle(processInfo.hThread);
			}
			data = "";
			std::ifstream lua("tmp.lua", std::ios_base::binary | std::ios_base::in);
			if (lua.is_open())
			{
				char buf[1024];
				while (!lua.eof())
				{
					lua.read(buf, 1024);
					data += std::string(buf, lua.gcount());
				}
				lua.close();
			}
			std::filesystem::remove("tmp.lub");
			std::filesystem::remove("tmp.lua");
		}
		else //this is a nasty renamed lua file to lub. Shame on you mappers!
		{
			char buf[1024];
			while (!lub->eof())
			{
				lub->read(buf, 1024);
				data += std::string(buf, lub->gcount());
			}
		}
		delete lub;
		
		if (data != "" && data.find("{") != std::string::npos)
		{
			std::string ver = data;
			ver = ver.substr(ver.find("version ="));
			ver = ver.substr(0, ver.find("\n"));
			if (ver.find("\r"))
				ver = ver.substr(0, ver.find("\r"));
			ver = ver.substr(ver.rfind("=")+1);
			ver = util::trim(ver);
			lubVersion = std::stoi(ver);
			//this is a dirty hack to change the main lua array into a json structure for automatic parsing...I'm wondering if it's not easier to just write a lua parser
			data = data.substr(data.find("{")); // strip beginning;
			data = util::replace(data, "\r\n", "\n");
			while (data.find("\t\n") != std::string::npos)//omg mina, clean up your newlines
				data = util::replace(data, "\t\n", "\n");
			while (data.find(" \n") != std::string::npos)
				data = util::replace(data, " \n", "\n");
			while (data.find("\n\n") != std::string::npos)
				data = util::replace(data, "\n\n", "\n");
			data = util::replace(data, "\\", "\\\\");
			data = util::replace(data, "[[", "\"");
			data = util::replace(data, "]]", "\"");
			std::replace(data.begin(), data.end(), '[', '\"');
			std::replace(data.begin(), data.end(), ']', '\"');
			data = util::replace(data, "\"\"", "");
			std::replace(data.begin(), data.end(), '=', ':');
			data[0] = '[';
			auto lines = util::split(data, "\n");
			bool done = false;
			for (auto i = 0; i < lines.size(); i++)
			{
				if (lines[i].find("--") != std::string::npos)
					lines[i] = util::rtrim(lines[i].substr(0, lines[i].find("--")));//remove comments
				if (lines[i].size() > 2 && lines[i][0] == '\t' && lines[i][1] == '\"')
				{
					if (lines[i].find("{") != std::string::npos)
						lines[i] = lines[i].substr(lines[i].find(":") + 2);
					else
						lines[i] = "";
				}
				if (lines[i].size() > 3 && lines[i][0] == '\t' && lines[i][1] == '\t' && lines[i][2] != '\t')
				{
					lines[i] = "\t\t\"" + lines[i].substr(2, lines[i].find(" ") - 2) + "\"" + lines[i].substr(lines[i].find(" ") + 1);
					std::replace(lines[i].begin(), lines[i].end(), '{', '[');
					std::replace(lines[i].begin(), lines[i].end(), '}', ']');
				}
				auto trimmed = util::trim(lines[i]); // why you do this to me mina? remove comma at end of list
				if (i < lines.size() - 1 && trimmed.size() > 0 && trimmed[trimmed.size() - 1] == ',' && (util::trim(lines[i + 1]) == "}," || util::trim(lines[i + 1]) == "}"))
					lines[i] = lines[i].substr(0, lines[i].rfind(",")) + lines[i].substr(lines[i].rfind(",") + 1);

				if (done)
					lines[i] = "";
				if (util::rtrim(lines[i]) == "}")
					done = true;
			}

			std::string jsondata = util::combine(lines, "\n");
			while (jsondata.find("\n\n") != std::string::npos)
				jsondata = util::replace(jsondata, "\n\n", "\n");

			jsondata = util::trim(jsondata);
			jsondata[jsondata.length() - 1] = ']';

			try
			{
				lubInfo = json::parse(jsondata);
			}
			catch (const std::exception& e)
			{
				std::cerr << "Error loading json from lub data: " << e.what() << std::endl;
				std::cout << jsondata << std::endl;
			}
		}
	}


	json extraProperties;
	try {
		extraProperties = util::FileIO::getJson(fileName + ".extra.json");
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error loading extra properties " << fileName << ".extra.json" << std::endl;
		std::cerr << e.what() << std::endl;
	}

	char header[4];
	file->read(header, 4);
	if (!(header[0] == 'G' || header[1] == 'R' || header[2] == 'S' || header[3] == 'W'))
	{
		std::cerr << "RSW: Error loading rsw: invalid header" << std::endl;
		delete file;
		return;
	}

	file->read(reinterpret_cast<char*>(&version), sizeof(short));
	version = util::swapShort(version);
	std::cout << std::hex<<"RSW: Version 0x" << version << std::endl <<std::dec;

	if (version >= 0x0202)
	{
		buildNumber = file->get();
		std::cout << "Build number " << (int)buildNumber << std::endl;
	}
	if (version >= 0x0205)
	{
		int u;
		file->read(reinterpret_cast<char*>(&u), sizeof(int));
		std::cout << "205 unknown value: " << u << std::endl;
	}

	iniFile = util::FileIO::readString(file, 40);
	gndFile = util::FileIO::readString(file, 40);
	if (version > 0x0104)
		gatFile = util::FileIO::readString(file, 40);
	else
		gatFile = gndFile; //TODO: convert

	iniFile = util::FileIO::readString(file, 40); // ehh...read inifile twice?

	//version 0x0206
	if (version < 0x0206)
	{
		//TODO: default values
		if (version >= 0x103)
			file->read(reinterpret_cast<char*>(&water.height), sizeof(float));
		if (version >= 0x108)
		{
			file->read(reinterpret_cast<char*>(&water.type), sizeof(int));
			file->read(reinterpret_cast<char*>(&water.amplitude), sizeof(float));
			file->read(reinterpret_cast<char*>(&water.waveSpeed), sizeof(float));
			file->read(reinterpret_cast<char*>(&water.wavePitch), sizeof(float));
		}
		if (version >= 0x109)
			file->read(reinterpret_cast<char*>(&water.textureAnimSpeed), sizeof(int));
		else
		{
			water.textureAnimSpeed = 100;
			throw "todo";
		}
	}

	if (loadGnd)
	{
		node->addComponent(new Gnd(gndFile));
		node->addComponent(new GndRenderer());
		node->addComponent(new WaterRenderer());
		//TODO: read GND & GAT here
	}


	light.longitude = 45;//TODO: remove the defaults here and put defaults of the water somewhere too
	light.latitude = 45;
	light.diffuse = glm::vec3(1, 1, 1);
	light.ambient = glm::vec3(0.3f, 0.3f, 0.3f);
	light.intensity = 0.5f;

	if (version >= 0x105)
	{
		file->read(reinterpret_cast<char*>(&light.longitude), sizeof(int));
		file->read(reinterpret_cast<char*>(&light.latitude), sizeof(int));
		file->read(reinterpret_cast<char*>(glm::value_ptr(light.diffuse)), sizeof(float) * 3);
		file->read(reinterpret_cast<char*>(glm::value_ptr(light.ambient)), sizeof(float) * 3);
	}
	if (version >= 0x107)
		file->read(reinterpret_cast<char*>(&light.intensity), sizeof(float)); //shadowOpacity;
	
	if (extraProperties.find("mapproperties") != extraProperties.end())
	{
		if (extraProperties["mapproperties"].find("lightmapAmbient") != extraProperties["mapproperties"].end())
			light.lightmapAmbient = extraProperties["mapproperties"]["lightmapAmbient"];
		if (extraProperties["mapproperties"].find("lightmapIntensity") != extraProperties["mapproperties"].end())
			light.lightmapIntensity = extraProperties["mapproperties"]["lightmapIntensity"];
	}

	if (version >= 0x106)
	{
		file->read(reinterpret_cast<char*>(&unknown[0]), sizeof(int)); //m_groundTop
		file->read(reinterpret_cast<char*>(&unknown[1]), sizeof(int)); //m_groundBottom
		file->read(reinterpret_cast<char*>(&unknown[2]), sizeof(int)); //m_groundLeft
		file->read(reinterpret_cast<char*>(&unknown[3]), sizeof(int)); //m_groundRight
	}
	else
	{
		unknown[0] = unknown[2] = -500;
		unknown[1] = unknown[3] = 500;
	}

	int objectCount;
	file->read(reinterpret_cast<char*>(&objectCount), sizeof(int));
	std::cout << "RSW: Loading " << objectCount << " objects" << std::endl;

	int lubIndex = 0;
	for (int i = 0; i < objectCount; i++)
	{
		Node* object = new Node("");
		auto rswObject = new RswObject();
		object->addComponent(rswObject);
		rswObject->load(file, version, buildNumber, loadModels);

		if (object->getComponent<RswLight>() && extraProperties.is_object() && extraProperties["light"].is_array())
		{
			for (const json& l : extraProperties["light"])
				if (l["id"] == i)
					object->getComponent<RswLight>()->loadExtra(l);
		}
		if (object->getComponent<RswModel>() && extraProperties.is_object() && extraProperties["model"].is_array())
		{
			for (const json& l : extraProperties["model"])
				if (l["id"] == i)
					object->getComponent<RswModel>()->loadExtra(l);
		}
		auto rswEffect = object->getComponent<RswEffect>();
		if (rswEffect && rswEffect->id == 974)
		{
			auto lubEffect = new LubEffect();
			if (lubIndex <= lubInfo.size())
				lubEffect->load(lubInfo[lubIndex]);
			object->addComponent(lubEffect);
			object->addComponent(new LubRenderer());
			lubIndex++;
		}

		std::string objPath = object->name;
		if (objPath.find("\\") != std::string::npos)
			objPath = objPath.substr(0, objPath.rfind("\\"));
		else
			objPath = "";

		if(map)
			object->setParent(map->findAndBuildNode(objPath));
		else
			object->setParent(node);
	}


	while (!file->eof())
	{
		glm::vec3 p;
		file->read(reinterpret_cast<char*>(glm::value_ptr(p)), sizeof(float) * 3);
		if(file->gcount() > 0) //if data is actually read
			quadtreeFloats.push_back(p);
	}
	if (quadtreeFloats.size() > 1)
	{
		auto it = quadtreeFloats.cbegin();
		quadtree = new QuadTreeNode(it);
	}
	else
		quadtree = nullptr;//TODO:

	delete file;
}


void Rsw::save(const std::string& fileName, BrowEdit* browEdit)
{
	std::cout << "Saving to " << fileName << std::endl;
	std::ofstream file(fileName.c_str(), std::ios_base::out | std::ios_base::binary);
	nlohmann::json extraProperties;

	char header[4] = { 'G','R','S','W' };
	file.write(header, 4);
	short versionFlipped = util::swapShort(version);
	file.write(reinterpret_cast<char*>(&versionFlipped), sizeof(short));

	if (version == 0x0202)
		file.put(0); // ???
	util::FileIO::writeString(file, iniFile, 40);

	util::FileIO::writeString(file, gndFile, 40);

	if (version > 0x0104)
	{
		util::FileIO::writeString(file, gatFile, 40);
	}
	util::FileIO::writeString(file, iniFile, 40);

	if (version >= 0x103)
		file.write(reinterpret_cast<char*>(&water.height), sizeof(float));
	if (version >= 0x108)
	{
		file.write(reinterpret_cast<char*>(&water.type), sizeof(int));
		file.write(reinterpret_cast<char*>(&water.amplitude), sizeof(float));
		file.write(reinterpret_cast<char*>(&water.waveSpeed), sizeof(float));
		file.write(reinterpret_cast<char*>(&water.wavePitch), sizeof(float));
	}
	if (version >= 0x109)
		file.write(reinterpret_cast<char*>(&water.textureAnimSpeed), sizeof(int));
	else
	{
		water.textureAnimSpeed = 100;
		throw "todo";
	}

	if (version >= 0x105)
	{
		file.write(reinterpret_cast<char*>(&light.longitude), sizeof(int));
		file.write(reinterpret_cast<char*>(&light.latitude), sizeof(int));
		file.write(reinterpret_cast<char*>(glm::value_ptr(light.diffuse)), sizeof(float) * 3);
		file.write(reinterpret_cast<char*>(glm::value_ptr(light.ambient)), sizeof(float) * 3);
	}
	if (version >= 0x107)
		file.write(reinterpret_cast<char*>(&light.intensity), sizeof(float));

	extraProperties["light"] = json();
	extraProperties["mapproperties"]["lightmapAmbient"] = light.lightmapAmbient;
	extraProperties["mapproperties"]["lightmapIntensity"] = light.lightmapIntensity;


	if (version >= 0x106)
	{
		file.write(reinterpret_cast<char*>(&unknown[0]), sizeof(int));
		file.write(reinterpret_cast<char*>(&unknown[1]), sizeof(int));
		file.write(reinterpret_cast<char*>(&unknown[2]), sizeof(int));
		file.write(reinterpret_cast<char*>(&unknown[3]), sizeof(int));
	}

	std::vector<Node*> objects;
	node->traverse([&objects](Node* n)
	{
		if (n->getComponent<RswObject>())
			objects.push_back(n);
	});
	int objectCount = (int)objects.size();
	file.write(reinterpret_cast<char*>(&objectCount), sizeof(int));
	std::vector<LubEffect*> lubEffects;
	for (auto i = 0; i < objects.size(); i++)
	{
		objects[i]->getComponent<RswObject>()->save(file, version);
		auto light = objects[i]->getComponent<RswLight>();
		if (light)
		{
			auto j = light->saveExtra();
			j["id"] = i;
			extraProperties["light"].push_back(j);
		}
		auto model = objects[i]->getComponent<RswModel>();
		if (model)
		{
			auto j = model->saveExtra();
			j["id"] = i;
			extraProperties["model"].push_back(j);
		}
		auto lubEffect = objects[i]->getComponent<LubEffect>();
		if (lubEffect)
			lubEffects.push_back(lubEffect);
	}
	quadtree->foreach([&file](QuadTreeNode* n)
	{
		file.write(reinterpret_cast<char*>(glm::value_ptr(n->bbox.bounds[1])), sizeof(float) * 3);
		file.write(reinterpret_cast<char*>(glm::value_ptr(n->bbox.bounds[0])), sizeof(float) * 3);
		file.write(reinterpret_cast<char*>(glm::value_ptr(n->range[0])), sizeof(float) * 3);
		file.write(reinterpret_cast<char*>(glm::value_ptr(n->range[1])), sizeof(float) * 3);
	});

	std::ofstream extraFile((fileName + ".extra.json").c_str(), std::ios_base::out | std::ios_base::binary);
	extraFile << std::setw(2)<<extraProperties;
	extraFile.close();

	std::string mapName = fileName;
	if (mapName.find(".rsw") != std::string::npos)
		mapName = mapName.substr(0, mapName.find(".rsw"));
	if (mapName.find("\\") != std::string::npos)
		mapName = mapName.substr(mapName.rfind("\\")+1);
	std::string luaMapName = util::replace("@", "", mapName);
	
	if (lubEffects.size() > 0)
	{
		std::cout << "Lub effects found, saving to " << browEdit->config.ropath << "data\\luafiles514\\lua files\\effecttool\\" << mapName << ".lub" << std::endl;
		std::ofstream lubFile((browEdit->config.ropath + "data\\luafiles514\\lua files\\effecttool\\" + mapName + ".lub").c_str(), std::ios_base::out | std::ios_base::binary);
		lubFile << "_" << luaMapName << "_emitterInfo_version = "<<lubVersion<<".0" << std::endl;
		lubFile << "_" << luaMapName << "_emitterInfo =" << std::endl;
		lubFile << "{" << std::endl;

#define SAVEPROP0(x,y) lubFile<<"\t\t[\""<<x<<"\"] = { "<<y<<" }"
#define SAVEPROP2(x,y) lubFile<<"\t\t[\""<<x<<"\"] = { "<<y[0]<<", "<<y[1]<<" }"
#define SAVEPROP3(x,y) lubFile<<"\t\t[\""<<x<<"\"] = { "<<y[0]<<", "<<y[1]<<", "<<y[2]<<" }"
#define SAVEPROP4(x,y) lubFile<<"\t\t[\""<<x<<"\"] = { "<<y[0]<<", "<<y[1]<<", "<<y[2]<<", "<<y[3]<<" }"
#define SAVEPROPS(x,y) lubFile<<"\t\t[\""<<x<<"\"] = [["<<y<<"]]"

		for (auto i = 0; i < lubEffects.size(); i++)
		{
			lubFile << "\t[" << i << "] = " << std::endl;
			lubFile << "\t{" << std::endl;
			SAVEPROP3("dir1", lubEffects[i]->dir1) << "," << std::endl;
			SAVEPROP3("dir2", lubEffects[i]->dir2) << "," << std::endl;
			SAVEPROP3("gravity", lubEffects[i]->gravity) << "," << std::endl;
			SAVEPROP3("pos", lubEffects[i]->pos) << "," << std::endl;
			SAVEPROP3("radius", lubEffects[i]->radius) << "," << std::endl;
			SAVEPROP4("color", glm::round(lubEffects[i]->color * 255.0f)) << "," << std::endl;
			SAVEPROP2("rate", lubEffects[i]->rate) << "," << std::endl;
			SAVEPROP2("size", lubEffects[i]->size) << "," << std::endl;
			SAVEPROP2("life", lubEffects[i]->life) << "," << std::endl;
			SAVEPROPS("texture", lubEffects[i]->texture) << "," << std::endl;
			SAVEPROP0("speed", lubEffects[i]->speed) << "," << std::endl;
			SAVEPROP0("srcmode", lubEffects[i]->srcmode) << "," << std::endl;
			SAVEPROP0("destmode", lubEffects[i]->destmode) << "," << std::endl;
			SAVEPROP0("maxcount", lubEffects[i]->maxcount) << "," << std::endl;
			SAVEPROP0("zenable", lubEffects[i]->zenable) << std::endl;
			SAVEPROP3("rotate_angle", lubEffects[i]->rotate_angle) << std::endl;


			lubFile << "\t}";
			if (i < lubEffects.size() - 1)
				lubFile << ",";
			lubFile << std::endl;
		}
		lubFile << "}" << std::endl;
		lubFile.close();
	}

	std::cout << "Done saving" << std::endl;
}





Rsw::QuadTreeNode::QuadTreeNode(std::vector<glm::vec3>::const_iterator &it, int level /*= 0*/) : bbox(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0))
{
	bbox.bounds[1] = *it;
	it++;
	bbox.bounds[0] = *it;
	it++;

	range[0] = *it;
	it++;
	range[1] = *it;
	it++;

	for (size_t i = 0; i < 4; i++)
		children[i] = nullptr;
	if (level >= 5)
		return;
	for (size_t i = 0; i < 4; i++)
		children[i] = new QuadTreeNode(it, level + 1);
}

Rsw::QuadTreeNode::QuadTreeNode(float x, float y, float width, float height, int level) : bbox(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0))
{
	bbox.min.x = x;
	bbox.max.x = x+width;

	bbox.min.z = y;
	bbox.max.z = y+height;

	for (int i = 0; i < 4; i++)
		children[i] = nullptr;

	if (level >= 5)
		return;
	children[0] = new QuadTreeNode(x, y, width / 2, height / 2, level + 1);
	children[1] = new QuadTreeNode(x+width/2, y, width / 2, height / 2, level + 1);
	children[2] = new QuadTreeNode(x, y+height/2, width / 2, height / 2, level + 1);
	children[3] = new QuadTreeNode(x+width/2, y+height/2, width / 2, height / 2, level + 1);

}


Rsw::QuadTreeNode::~QuadTreeNode()
{
	for (int i = 0; i < 4; i++)
		if (children[i])
			delete children[i];
}



void Rsw::buildImGui(BrowEdit* browEdit)
{
	ImGui::Text("RSW");
	char versionStr[10];
	sprintf_s(versionStr, 10, "%04x", version);
	if (ImGui::BeginCombo("Version", versionStr))
	{
		if (ImGui::Selectable("0103", version == 0x0103))
			version = 0x0103;
		if (ImGui::Selectable("0104", version == 0x0104))
			version = 0x0104;
		if (ImGui::Selectable("0108", version == 0x0108))
			version = 0x0108;
		if (ImGui::Selectable("0109", version == 0x0109))
			version = 0x0109;
		if (ImGui::Selectable("0201", version == 0x0201))
			version = 0x0201;
		if (ImGui::Selectable("0202", version == 0x0202))
			version = 0x0202;
		if (ImGui::Selectable("0203", version == 0x0203))
			version = 0x0203;
		if (ImGui::Selectable("0204", version == 0x0204))
			version = 0x0204;
		ImGui::EndCombo();
	}
	
	
	if (ImGui::BeginCombo("LubEffect Version", std::to_string(lubVersion).c_str()))
	{
		if (ImGui::Selectable("1", version == 1))
			lubVersion = 1;
		if (ImGui::Selectable("2", version == 2))
			lubVersion = 2;
		if (ImGui::Selectable("3", version == 3))
			lubVersion = 3;
		ImGui::EndCombo();
	}
	

	if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen))
	{
		util::DragInt(browEdit, browEdit->activeMapView->map, node, "Longitude", &light.longitude, 1, 0, 360);
		util::DragInt(browEdit, browEdit->activeMapView->map, node, "Latitude", &light.latitude, 1, 0, 360);
		util::DragFloat(browEdit, browEdit->activeMapView->map, node, "Intensity", &light.intensity, 0.01f, 0, 1);
		util::ColorEdit3(browEdit, browEdit->activeMapView->map, node, "Diffuse", &light.diffuse);
		util::ColorEdit3(browEdit, browEdit->activeMapView->map, node, "Ambient", &light.ambient);


		util::DragFloat(browEdit, browEdit->activeMapView->map, node, "lightmapAmbient", &light.lightmapAmbient, 0.01f, 0, 1);
		util::DragFloat(browEdit, browEdit->activeMapView->map, node, "lightmapIntensity", &light.lightmapIntensity, 0.01f, 0, 1);

	}
	if (ImGui::CollapsingHeader("Water", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (util::DragInt(browEdit, browEdit->activeMapView->map, node, "Type", &water.type, 1, 0, 1000))
		{
			auto waterRenderer = node->getComponent<WaterRenderer>();
			waterRenderer->reloadTextures();
		}
		util::DragFloat(browEdit, browEdit->activeMapView->map, node, "Height", &water.height, 0.1f, -100, 100);
		util::DragFloat(browEdit, browEdit->activeMapView->map, node, "Wave Height", &water.amplitude, 0.1f, -100, 100);
		util::DragInt(browEdit, browEdit->activeMapView->map, node, "Texture Animation Speed", &water.textureAnimSpeed, 1, 0, 1000);
		util::DragFloat(browEdit, browEdit->activeMapView->map, node, "Wave Speed", &water.waveSpeed, 0.1f, -100, 100);
		util::DragFloat(browEdit, browEdit->activeMapView->map, node, "Wave Pitch", &water.wavePitch, 0.1f, -100, 100);
	}
}

extern std::vector<std::vector<glm::vec3>> debugPoints;

std::vector<std::vector<glm::vec2>> heights;
void Rsw::recalculateQuadtree(QuadTreeNode* node)
{
	static Gnd* gnd;
	static Rsw* rsw;
//	static std::vector<std::vector<glm::vec2>> heights;
	bool rootNode = false;
	if (!node)
	{
		rootNode = true;
		node = quadtree;
		gnd = this->node->getComponent<Gnd>();
		rsw = this->node->getComponent<Rsw>();

		heights.clear();
		heights.resize(gnd->width);
		for (int i = 0; i < gnd->width; i++)
			heights[i].resize(gnd->height, glm::vec2(rsw->water.height, rsw->water.height));

		debugPoints.clear();
		debugPoints.resize(2);

		this->node->traverse([&](Node* n) {
			auto rswModel = n->getComponent<RswModel>();
			auto rsmRenderer = n->getComponent<RsmRenderer>();
			if (rswModel)
			{
				auto collider = n->getComponent<RswModelCollider>();
				auto vertices = collider->getVerticesWorldSpace();
				for (auto i = 0; i < vertices.size(); i += 3)
				{
					math::AABB aabb(std::span<glm::vec3>(vertices.begin() + i, 3));
					for (int x = (int)glm::floor(aabb.min.x / 10); x < (int)glm::ceil(aabb.max.x / 10); x++)
						for (int y = (int)glm::floor(aabb.min.z / 10); y < (int)glm::ceil(aabb.max.z / 10); y++)
						{
							if (x >= 0 && x < gnd->width && y >= 0 && y < gnd->height)
							{
								heights[x][y].x = glm::min(heights[x][y].x, aabb.min.y);
								heights[x][y].y = glm::max(heights[x][y].y, aabb.max.y);
							}
						}

				}
			}
			});


#if 0
		for (auto x = 0; x < heights.size(); x++)
			for (auto y = 0; y < heights[x].size(); y++)
			{
				debugPoints[0].push_back(glm::vec3(10*x, heights[x][y].x, 10 * y));
				debugPoints[1].push_back(glm::vec3(10 * x, heights[x][y].y, 10 * y));
			}
#endif
	} //end startup

	for (int i = 0; i < 4; i++)
		if(node->children[i])
			recalculateQuadtree(node->children[i]);

	if (!node->children[0]) // leaf
	{
		node->bbox.min.y = 99999;
		node->bbox.max.y = -99999;
		for (int x = gnd->width / 2 + (int)floor(node->bbox.min.x / 10); x < gnd->width / 2 + (int)ceil(node->bbox.max.x / 10); x++)
		{
			for (int y = gnd->height / 2 + (int)floor(node->bbox.min.z / 10); y < gnd->height / 2 + (int)ceil(node->bbox.max.z / 10); y++)
			{
				if (x >= 0 && x < gnd->width && y >= 0 && y < gnd->height)
				{
					for (auto i = 0; i < 4; i++)
					{
						node->bbox.min.y = glm::min(gnd->cubes[x][y]->heights[i], node->bbox.min.y);
						node->bbox.max.y = glm::max(gnd->cubes[x][y]->heights[i], node->bbox.max.y);
					}
					node->bbox.min.y = glm::min(-heights[x][heights[x].size() - 1 - y].y, node->bbox.min.y);
					node->bbox.max.y = glm::max(-heights[x][heights[x].size() - 1 - y].x, node->bbox.max.y);
				}
			}
		}
	}
	else
	{
		node->bbox.min.y = 9999999;
		node->bbox.max.y = -9999999;
		for (int i = 0; i < 4; i++)
		{
			node->bbox.min.y = glm::min(node->bbox.min.y, node->children[i]->bbox.min.y);
			node->bbox.max.y = glm::max(node->bbox.max.y, node->children[i]->bbox.max.y);
		}
	}

	node->range[0] = (node->bbox.max - node->bbox.min) / 2.0f;
	node->range[1] = node->bbox.max - node->range[0];

	if (rootNode)
	{
		heights.clear();
	}

}


void RswModelCollider::begin()
{
	rswModel = nullptr;
	rsm = nullptr;
	rsmRenderer = nullptr;
}

std::vector<glm::vec3> RswModelCollider::getCollisions(const math::Ray& ray)
{
	std::vector<glm::vec3> ret;

	if (!rswModel)
		rswModel = node->getComponent<RswModel>();
	if (!rsm)
		rsm = node->getComponent<Rsm>();
	if (!rsmRenderer)
		rsmRenderer = node->getComponent<RsmRenderer>();
	if (!rswModel || !rsm || !rsmRenderer)
		return std::vector<glm::vec3>();

	if (!rsm->loaded)
		rsm = RsmRenderer::errorModel;

	if (!rswModel->aabb.hasRayCollision(ray, 0, 10000000))
		return std::vector<glm::vec3>();
	return getCollisions(rsm->rootMesh, ray, rsmRenderer->matrixCache);
}

std::vector<glm::vec3> RswModelCollider::getCollisions(Rsm::Mesh* mesh, const math::Ray& ray, const glm::mat4& matrix)
{
	std::vector<glm::vec3> ret;

	glm::mat4 newMatrix = matrix * rsmRenderer->renderInfo[mesh->index].matrix;
	newMatrix = glm::inverse(newMatrix);
	math::Ray newRay(ray * newMatrix);

	std::vector<glm::vec3> verts;
	verts.resize(3);
	float t;
	for (size_t i = 0; i < mesh->faces.size(); i++)
	{
		for (size_t ii = 0; ii < 3; ii++)
			verts[ii] = mesh->vertices[mesh->faces[i].vertexIds[ii]];

		if (newRay.LineIntersectPolygon(verts, t))
			ret.push_back(glm::vec3(matrix * rsmRenderer->renderInfo[mesh->index].matrix * glm::vec4(newRay.origin + t * newRay.dir, 1)));
	}

	for (size_t i = 0; i < mesh->children.size(); i++)
	{
		std::vector<glm::vec3> other = getCollisions(mesh->children[i], ray, matrix);
		if (!other.empty())
			ret.insert(ret.end(), other.begin(), other.end());
	}
	return ret;
}

bool RswModelCollider::collidesTexture(const math::Ray& ray, float maxDistance)
{
	std::vector<glm::vec3> ret;

	if (!rswModel)
		rswModel = node->getComponent<RswModel>();
	if (!rsm)
		rsm = node->getComponent<Rsm>();
	if (!rsmRenderer)
		rsmRenderer = node->getComponent<RsmRenderer>();
	if (!rswModel || !rsm || !rsmRenderer)
		return false;

	if (!rswModel->aabb.hasRayCollision(ray, 0, 10000000))
		return false;
	return collidesTexture(rsm->rootMesh, ray, rsmRenderer->matrixCache, maxDistance);
}


bool RswModelCollider::collidesTexture(Rsm::Mesh* mesh, const math::Ray& ray, const glm::mat4& matrix, float maxDistance)
{
	std::vector<glm::vec3> ret;

	glm::mat4 newMatrix = matrix * rsmRenderer->renderInfo[mesh->index].matrix;
	//math::Ray newRay(ray * glm::inverse(newMatrix)); //would rather work with the inversed ray here, but that doesn't work for scaled models

	std::vector<glm::vec3> verts;
	verts.resize(3);
	float t;
	for (size_t i = 0; i < mesh->faces.size(); i++)
	{
		for (size_t ii = 0; ii < 3; ii++)
			verts[ii] = newMatrix * glm::vec4(mesh->vertices[mesh->faces[i].vertexIds[ii]],1.0f);

		if (ray.LineIntersectPolygon(verts, t) && t > 0)
		{
			Image* img = nullptr;
			auto rsmMesh = dynamic_cast<Rsm::Mesh*>(mesh);
			if (rsmMesh)
			{
				auto rsm = dynamic_cast<Rsm*>(rsmMesh->model);
				if (rsm && rsm->textures.size() > mesh->faces[i].texId)
					img = util::ResourceManager<Image>::load("data/texture/" + rsm->textures[mesh->faces[i].texId]);
			}
			if (img && img->hasAlpha)
			{
				glm::vec3 hitPoint = ray.origin + ray.dir * t;
				if (maxDistance - t > 0)
				{
					auto f1 = verts[0] - hitPoint;
					auto f2 = verts[1] - hitPoint;
					auto f3 = verts[2] - hitPoint;

					float a = glm::length(glm::cross(verts[0] - verts[1], verts[0] - verts[2]));
					float a1 = glm::length(glm::cross(f2, f3)) / a;
					float a2 = glm::length(glm::cross(f3, f1)) / a;
					float a3 = glm::length(glm::cross(f1, f2)) / a;

					glm::vec2 uv1 = mesh->texCoords[mesh->faces[i].texCoordIds[0]];
					glm::vec2 uv2 = mesh->texCoords[mesh->faces[i].texCoordIds[1]];
					glm::vec2 uv3 = mesh->texCoords[mesh->faces[i].texCoordIds[2]];

					glm::vec2 uv = uv1 * a1 + uv2 * a2 + uv3 * a3;

					if (uv.x > 1 || uv.x < 0)
						uv.x -= glm::floor(uv.x);
					if (uv.y > 1 || uv.y < 0)
						uv.y -= glm::floor(uv.y);

					if (std::isnan(uv.x))
					{
						std::cerr << "Error calculating lightmap for model " << node->name << ", " << rswModel->fileName << std::endl;
						return false;
					}

					if (img && img->get(uv) > 0.01)
						return true;
				}
			}
			else
				return true;

		}
	}

	for (size_t i = 0; i < mesh->children.size(); i++)
		if(collidesTexture(mesh->children[i], ray, matrix, maxDistance))
			return true;
	return false;
}

std::vector<glm::vec3> RswModelCollider::getVerticesWorldSpace(Rsm::Mesh* mesh, const glm::mat4& matrix)
{
	if (!rswModel)
		rswModel = node->getComponent<RswModel>();
	if (!rsm)
		rsm = node->getComponent<Rsm>();
	if (!rsmRenderer)
		rsmRenderer = node->getComponent<RsmRenderer>();
	if (!rswModel || !rsm || !rsmRenderer)
		return std::vector<glm::vec3>();

	glm::mat4 mat = matrix;
	if (mesh == nullptr)
	{
		mesh = rsm->rootMesh;
		mat = rsmRenderer->matrixCache;
	}

	glm::mat4 newMatrix = mat * rsmRenderer->renderInfo[mesh->index].matrix;
	std::vector<glm::vec3> verts;
	for (size_t i = 0; i < mesh->faces.size(); i++)
		for (size_t ii = 0; ii < 3; ii++)
			verts.push_back(newMatrix * glm::vec4(mesh->vertices[mesh->faces[i].vertexIds[ii]],1));

	for (size_t i = 0; i < mesh->children.size(); i++)
	{
		std::vector<glm::vec3> other = getVerticesWorldSpace(mesh->children[i], mat);
		if (!other.empty())
			verts.insert(verts.end(), other.begin(), other.end());
	}
	return verts;

}

CubeCollider::CubeCollider(int size) : aabb(glm::vec3(-size,-size,-size), glm::vec3(size,size,size))
{
	begin();
}

void CubeCollider::begin()
{
	rswObject = nullptr;
	gnd = nullptr;
}

std::vector<glm::vec3> CubeCollider::getCollisions(const math::Ray& ray)
{
	if (!rswObject)
		rswObject = node->getComponent<RswObject>();
	if (!gnd)
		gnd = node->root->getComponent<Gnd>();
	if (!rswObject || !gnd)
		return std::vector<glm::vec3>();

	glm::mat4 modelMatrix(1.0f);
	modelMatrix = glm::scale(modelMatrix, glm::vec3(1, 1, -1));
	modelMatrix = glm::translate(modelMatrix, glm::vec3(5 * gnd->width + rswObject->position.x, -rswObject->position.y, -10 - 5 * gnd->height + rswObject->position.z));

	modelMatrix = glm::inverse(modelMatrix);
	math::Ray newRay(ray * modelMatrix);

	std::vector<glm::vec3> ret;
	if (aabb.hasRayCollision(newRay, 0, 100000))
		ret.push_back(glm::vec3(5 * gnd->width + rswObject->position.x, -rswObject->position.y, -(- 10 - 5 * gnd->height + rswObject->position.z)));
	return ret;
}


void Rsw::QuadTreeNode::draw(int levelLeft)
{
	if (levelLeft <= 0)
		return;
	if (levelLeft == 1)
	{
		glColor4f(1, 0, 0, 1);
		glBegin(GL_LINES);
		glVertex3f(bbox.min.x, bbox.min.y, bbox.min.z);
		glVertex3f(bbox.min.x, bbox.min.y, bbox.max.z);
		glVertex3f(bbox.min.x, bbox.min.y, bbox.max.z);
		glVertex3f(bbox.min.x, bbox.max.y, bbox.max.z);
		glVertex3f(bbox.min.x, bbox.max.y, bbox.max.z);
		glVertex3f(bbox.min.x, bbox.max.y, bbox.min.z);
		glVertex3f(bbox.min.x, bbox.max.y, bbox.min.z);
		glVertex3f(bbox.min.x, bbox.min.y, bbox.min.z);

		glVertex3f(bbox.max.x, bbox.min.y, bbox.min.z);
		glVertex3f(bbox.max.x, bbox.min.y, bbox.max.z);
		glVertex3f(bbox.max.x, bbox.min.y, bbox.max.z);
		glVertex3f(bbox.max.x, bbox.max.y, bbox.max.z);
		glVertex3f(bbox.max.x, bbox.max.y, bbox.max.z);
		glVertex3f(bbox.max.x, bbox.max.y, bbox.min.z);
		glVertex3f(bbox.max.x, bbox.max.y, bbox.min.z);
		glVertex3f(bbox.max.x, bbox.min.y, bbox.min.z);

		glVertex3f(bbox.min.x, bbox.min.y, bbox.min.z);
		glVertex3f(bbox.max.x, bbox.min.y, bbox.min.z);
		glVertex3f(bbox.min.x, bbox.min.y, bbox.max.z);
		glVertex3f(bbox.max.x, bbox.min.y, bbox.max.z);
		glVertex3f(bbox.min.x, bbox.max.y, bbox.min.z);
		glVertex3f(bbox.max.x, bbox.max.y, bbox.min.z);
		glVertex3f(bbox.min.x, bbox.max.y, bbox.max.z);
		glVertex3f(bbox.max.x, bbox.max.y, bbox.max.z);
		glEnd();
	}

	for (int i = 0; i < 4; i++)
		if (children[i])
			children[i]->draw(levelLeft-1);
}