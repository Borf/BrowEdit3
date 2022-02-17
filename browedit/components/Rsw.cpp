#include <Windows.h>
#include <glad/glad.h>
#include "Rsw.h"
#include "Gnd.h"
#include "Rsm.h"
#include "GndRenderer.h"
#include "RsmRenderer.h"
#include "BillboardRenderer.h"

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
#include <glm/gtc/type_ptr.hpp>
#include <browedit/Node.h>

Rsw::Rsw()
{
}


void Rsw::load(const std::string& fileName, Map* map, bool loadModels, bool loadGnd)
{
	auto file = util::FileIO::open(fileName);
	if (!file)
	{
		std::cerr << "Could not open file " << fileName << std::endl;
		return;
	}
	quadtree = nullptr;

	auto extraProperties = util::FileIO::getJson(fileName + ".extra.json");

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

	if (version >= 0x0203)
	{
		std::cerr << "RSW: Sorry, this version is not supported yet, did not open "<<fileName<< std::endl;
		return;
	}

	if (version == 0x0202) // ???
	{
		file->get();
	}

	iniFile = util::FileIO::readString(file, 40);
	gndFile = util::FileIO::readString(file, 40);
	if (version > 0x0104)
		gatFile = util::FileIO::readString(file, 40);
	else
		gatFile = gndFile; //TODO: convert

	iniFile = util::FileIO::readString(file, 40); // ehh...read inifile twice?

		//TODO: default values
	if (version >= 0x103)
		file->read(reinterpret_cast<char*>(&water.height), sizeof(float));
	if (version >= 0x108)
	{
		file->read(reinterpret_cast<char*>(&water.type), sizeof(int));
		file->read(reinterpret_cast<char*>(&water.amplitude), sizeof(float));
		file->read(reinterpret_cast<char*>(&water.phase), sizeof(float));
		file->read(reinterpret_cast<char*>(&water.surfaceCurve), sizeof(float));
	}
	if (version >= 0x109)
		file->read(reinterpret_cast<char*>(&water.animSpeed), sizeof(int));
	else
	{
		water.animSpeed = 100;
		throw "todo";
	}

	if (loadGnd)
	{
		node->addComponent(new Gnd(gndFile));
		node->addComponent(new GndRenderer());
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

	for (int i = 0; i < objectCount; i++)
	{
		Node* object = new Node("");
		auto rswObject = new RswObject();
		object->addComponent(rswObject);
		rswObject->load(file, version, loadModels);

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

		std::string objPath = object->name;
		if (objPath.find("\\") != std::string::npos)
			objPath = objPath.substr(0, objPath.rfind("\\"));
		else
			objPath = "";

		object->setParent(map->findAndBuildNode(objPath));
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


void Rsw::save(const std::string& fileName)
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
		file.write(reinterpret_cast<char*>(&water.phase), sizeof(float));
		file.write(reinterpret_cast<char*>(&water.surfaceCurve), sizeof(float));
	}
	if (version >= 0x109)
		file.write(reinterpret_cast<char*>(&water.animSpeed), sizeof(int));
	else
	{
		water.animSpeed = 100;
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

	//TODO: write gnd/gat
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

	if (level >= 5)
		return;
	for (size_t i = 0; i < 4; i++)
		children[i] = new QuadTreeNode(it, level + 1);
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
		util::DragInt(browEdit, browEdit->activeMapView->map, node, "Type", &water.type, 1, 0, 1000);
		util::DragFloat(browEdit, browEdit->activeMapView->map, node, "Height", &water.height, 0.1f, -100, 100);
		util::DragFloat(browEdit, browEdit->activeMapView->map, node, "Amplitude", &water.amplitude, 0.1f, -100, 100);
		util::DragInt(browEdit, browEdit->activeMapView->map, node, "Animation Speed", &water.animSpeed, 1, 0, 1000);
		util::DragFloat(browEdit, browEdit->activeMapView->map, node, "Phase", &water.phase, 0.1f, -100, 100);
		util::DragFloat(browEdit, browEdit->activeMapView->map, node, "Surface Curve", &water.surfaceCurve, 0.1f, -100, 100);
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
			verts[ii] = mesh->vertices[mesh->faces[i]->vertexIds[ii]];

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

bool RswModelCollider::collidesTexture(const math::Ray& ray)
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
	return collidesTexture(rsm->rootMesh, ray, rsmRenderer->matrixCache);
}


bool RswModelCollider::collidesTexture(Rsm::Mesh* mesh, const math::Ray& ray, const glm::mat4& matrix)
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
			verts[ii] = mesh->vertices[mesh->faces[i]->vertexIds[ii]];

		if (newRay.LineIntersectPolygon(verts, t))
		{
			Image* img = nullptr;
			auto rsmMesh = dynamic_cast<Rsm::Mesh*>(mesh);
			if (rsmMesh)
			{
				auto rsm = dynamic_cast<Rsm*>(rsmMesh->model);
				if (rsm)
					img = util::ResourceManager<Image>::load("data/texture/" + rsm->textures[mesh->faces[i]->texId]);
			}
			if (img->hasAlpha)
			{
				glm::vec3 hitPoint = newRay.origin + newRay.dir * t;
				auto f1 = verts[0] - hitPoint;
				auto f2 = verts[1] - hitPoint;
				auto f3 = verts[2] - hitPoint;

				float a = glm::length(glm::cross(verts[0] - verts[1], verts[0] - verts[2]));
				float a1 = glm::length(glm::cross(f2, f3)) / a;
				float a2 = glm::length(glm::cross(f3, f1)) / a;
				float a3 = glm::length(glm::cross(f1, f2)) / a;

				glm::vec2 uv1 = mesh->texCoords[mesh->faces[i]->texCoordIds[0]];
				glm::vec2 uv2 = mesh->texCoords[mesh->faces[i]->texCoordIds[1]];
				glm::vec2 uv3 = mesh->texCoords[mesh->faces[i]->texCoordIds[2]];

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
	}

	for (size_t i = 0; i < mesh->children.size(); i++)
		if(collidesTexture(mesh->children[i], ray, matrix))
			return true;
	return false;
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

	for (int i = 0; i < 4; i++)
		if (children[i])
			children[i]->draw(levelLeft-1);
}