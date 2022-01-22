#include <Windows.h>
#include "Rsw.h"
#include "Gnd.h"
#include "Rsm.h"
#include "GndRenderer.h"
#include "RsmRenderer.h"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <browedit/util/ResourceManager.h>
#include <browedit/util/FileIO.h>
#include <browedit/util/Util.h>
#include <browedit/math/Ray.h>
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <browedit/Node.h>

Rsw::Rsw()
{
}


void Rsw::load(const std::string& fileName)
{
	auto file = util::FileIO::open(fileName);
	quadtree = nullptr;


	char header[4];
	file->read(header, 4);
	if (header[0] == 'G' && header[1] == 'R' && header[2] == 'G' && header[3] == 'W')
	{
		std::cerr << "RSW: Error loading rsw: invalid header" << std::endl;
		delete file;
		return;
	}

	file->read(reinterpret_cast<char*>(&version), sizeof(short));
	version = util::swapShort(version);
	std::cout << std::hex<<"RSW: Version 0x" << version << std::endl <<std::dec;

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

	node->addComponent(new Gnd(gndFile));
	node->addComponent(new GndRenderer());
	//TODO: read GND & GAT here


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
		file->read(reinterpret_cast<char*>(&light.intensity), sizeof(float));


	if (version >= 0x106)
	{
		file->read(reinterpret_cast<char*>(&unknown[0]), sizeof(int));
		file->read(reinterpret_cast<char*>(&unknown[1]), sizeof(int));
		file->read(reinterpret_cast<char*>(&unknown[2]), sizeof(int));
		file->read(reinterpret_cast<char*>(&unknown[3]), sizeof(int));
	}
	else
	{
		unknown[0] = unknown[2] = -500;
		unknown[1] = unknown[3] = 500;
	}

	int objectCount;
	file->read(reinterpret_cast<char*>(&objectCount), sizeof(int));
	std::cout << "RSW: Loading " << objectCount << " objects" << std::endl;

	Node* models = new Node("Models", node);
	Node* lights = new Node("Lights", node);
	Node* effects = new Node("Effects", node);
	Node* sounds = new Node("Sounds", node);

	for (int i = 0; i < objectCount; i++)
	{
		Node* object = new Node("");
		auto rswObject = new RswObject();
		object->addComponent(rswObject);
		rswObject->load(file, version);

		if (object->getComponent<RswModel>())
			object->setParent(models);
		if (object->getComponent<RswEffect>())
			object->setParent(effects);
		if (object->getComponent<RswSound>())
			object->setParent(sounds);
		if (object->getComponent<RswLight>())
			object->setParent(lights);
	}


	while (!file->eof())
	{
		glm::vec3 p;
		file->read(reinterpret_cast<char*>(glm::value_ptr(p)), sizeof(float) * 3);
		quadtreeFloats.push_back(p);
	}
	quadtree = new QuadTreeNode(quadtreeFloats.cbegin());


	delete file;
}

void RswObject::load(std::istream* is, int version)
{
	int type;
	is->read(reinterpret_cast<char*>(&type), sizeof(int));
	if (type == 1)
	{
		node->addComponent(new RswModel());
		node->addComponent(new RswModelCollider());
		node->getComponent<RswModel>()->load(is, version);
	}
	else if (type == 2)
	{
		node->addComponent(new RswLight());
		node->getComponent<RswLight>()->load(is);
	}
	else if (type == 3)
	{
		node->addComponent(new RswSound());
		node->getComponent<RswSound>()->load(is, version);
	}
	else if (type == 4)
	{
		node->addComponent(new RswEffect());
		node->getComponent<RswEffect>()->load(is);
	}
	else
		std::cerr << "RSW: Error loading object in RSW, objectType=" << type << std::endl;


}

std::string iso_8859_1_to_utf8(const std::string& str)
{
	std::string strOut;
	for (auto it = str.cbegin(); it != str.cend(); ++it)
	{
		uint8_t ch = *it;
		if (ch < 0x80) {
			strOut.push_back(ch);
		}
		else {
			strOut.push_back(0xc0 | ch >> 6);
			strOut.push_back(0x80 | (ch & 0x3f));
		}
	}
	return strOut;
}

void RswModel::load(std::istream* is, int version)
{
	auto rswObject = node->getComponent<RswObject>();
	if (version >= 0x103)
	{
		node->name = iso_8859_1_to_utf8(util::FileIO::readString(is, 40));

		is->read(reinterpret_cast<char*>(&animType), sizeof(int));
		is->read(reinterpret_cast<char*>(&animSpeed), sizeof(float));
		is->read(reinterpret_cast<char*>(&blockType), sizeof(int));
	}
	fileName = util::FileIO::readString(is, 80);
	util::FileIO::readString(is, 80); // TODO: Unknown?
	is->read(reinterpret_cast<char*>(glm::value_ptr(rswObject->position)), sizeof(float) * 3);
	is->read(reinterpret_cast<char*>(glm::value_ptr(rswObject->rotation)), sizeof(float) * 3);
	is->read(reinterpret_cast<char*>(glm::value_ptr(rswObject->scale)), sizeof(float) * 3);

	//TODO: move rswObject->position to transform->position

	node->addComponent(util::ResourceManager<Rsm>::load("data\\model\\" + fileName));
	node->addComponent(new RsmRenderer());
}



void RswLight::load(std::istream* is)
{
	auto rswObject = node->getComponent<RswObject>();

	node->name = util::FileIO::readString(is, 40);
	is->read(reinterpret_cast<char*>(glm::value_ptr(rswObject->position)), sizeof(float) * 3);
	rswObject->position *= glm::vec3(1, -1, 1);
	is->read(reinterpret_cast<char*>(todo), sizeof(float) * 10);

	is->read(reinterpret_cast<char*>(glm::value_ptr(color)), sizeof(float) * 3);
	is->read(reinterpret_cast<char*>(&todo2), sizeof(float));

}

void RswSound::load(std::istream* is, int version)
{
	auto rswObject = node->getComponent<RswObject>();

	node->name = util::FileIO::readString(is, 80);

	fileName = util::FileIO::readString(is, 40);

	is->read(reinterpret_cast<char*>(&unknown7), sizeof(float));
	is->read(reinterpret_cast<char*>(&unknown8), sizeof(float));
	is->read(reinterpret_cast<char*>(glm::value_ptr(rswObject->rotation)), sizeof(float) * 3);
	is->read(reinterpret_cast<char*>(glm::value_ptr(rswObject->scale)), sizeof(float) * 3);

	is->read(unknown6, 8);

	is->read(reinterpret_cast<char*>(glm::value_ptr(rswObject->position)), sizeof(float) * 3);

	is->read(reinterpret_cast<char*>(&vol), sizeof(float));
	is->read(reinterpret_cast<char*>(&width), sizeof(int));
	is->read(reinterpret_cast<char*>(&height), sizeof(int));
	is->read(reinterpret_cast<char*>(&range), sizeof(float));

	if (version >= 0x0200)
		is->read(reinterpret_cast<char*>(&cycle), sizeof(float));
}

void RswEffect::load(std::istream* is)
{
	auto rswObject = node->getComponent<RswObject>();
	node->name = util::FileIO::readString(is, 80);
	is->read(reinterpret_cast<char*>(glm::value_ptr(rswObject->position)), sizeof(float) * 3);
	is->read(reinterpret_cast<char*>(&id), sizeof(int));
	is->read(reinterpret_cast<char*>(&loop), sizeof(float));
	is->read(reinterpret_cast<char*>(&param1), sizeof(float));
	is->read(reinterpret_cast<char*>(&param2), sizeof(float));
	is->read(reinterpret_cast<char*>(&param3), sizeof(float));
	is->read(reinterpret_cast<char*>(&param4), sizeof(float));
}

Rsw::QuadTreeNode::QuadTreeNode(std::vector<glm::vec3>::const_iterator it, int level /*= 0*/) : bbox(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0))
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
	if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragInt("Longitude", &light.longitude, 1, 0, 360);
		ImGui::DragInt("Latitude", &light.latitude, 1, 0, 360);
		ImGui::DragFloat("Intensity", &light.intensity, 0.01f, 0, 1);
		ImGui::ColorEdit3("Diffuse", glm::value_ptr(light.diffuse));
		ImGui::ColorEdit3("Ambient", glm::value_ptr(light.ambient));
	}
	if (ImGui::CollapsingHeader("Water", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragInt("Type", &water.type, 1, 0, 1000);
		ImGui::DragFloat("Height", &water.height, 0.1f, -100, 100);
		ImGui::DragFloat("Amplitude", &water.amplitude, 0.1f, -100, 100);
		ImGui::DragInt("Animation Speed", &water.animSpeed, 1, 0, 1000);
		ImGui::DragFloat("Phase", &water.phase, 0.1f, -100, 100);
		ImGui::DragFloat("Surface Curve", &water.surfaceCurve, 0.1f, -100, 100);
	}
}


void RswObject::buildImGui(BrowEdit* browEdit)
{
	auto renderer = node->getComponent<RsmRenderer>();
	ImGui::Text("Object");

	if (util::DragFloat3(browEdit, node, "Position", &position, 1.0f, 0.0f, 0.0f, "Moving") && renderer)
		renderer->setDirty();
	if (util::DragFloat3(browEdit, node, "Scale", &scale, 1.0f, 0.0f, 0.0f, "Resizing") && renderer)
		renderer->setDirty();
	if (util::DragFloat3(browEdit, node, "Rotation", &scale, 1.0f, 0.0f, 0.0f, "Rotating") && renderer)
		renderer->setDirty();
}

void RswModel::buildImGui(BrowEdit* browEdit)
{
	ImGui::Text("Model");
	ImGui::DragInt("Animation Type", &animType, 1, 0, 100);
	ImGui::DragFloat("Animation Speed", &animSpeed, 0.01f, 0.0f, 100.0f);
	ImGui::DragInt("Block Type", &blockType, 1, 0, 100);
	ImGui::InputText("Filename", &fileName, ImGuiInputTextFlags_ReadOnly);
}

void RswEffect::buildImGui(BrowEdit* browEdit)
{
	ImGui::Text("Effect");
	ImGui::DragInt("Type", &id, 1, 0, 500); //TODO: change this to a combobox
	ImGui::DragFloat("Loop", &loop, 0.01f, 0.0f, 100.0f);
	ImGui::DragFloat("Param 1", &param1, 0.01f, 0.0f, 100.0f);
	ImGui::DragFloat("Param 2", &param2, 0.01f, 0.0f, 100.0f);
	ImGui::DragFloat("Param 3", &param3, 0.01f, 0.0f, 100.0f);
	ImGui::DragFloat("Param 4", &param4, 0.01f, 0.0f, 100.0f);
}

void RswSound::buildImGui(BrowEdit* browEdit)
{
	ImGui::Text("Sound");
	ImGui::InputText("Filename", &fileName);
	ImGui::DragFloat("Volume", &vol, 0.01f, 0.0f, 100.0f);

	ImGui::DragInt("Width", (int*)&width, 1, 0, 10000); //TODO: remove cast
	ImGui::DragInt("Height", (int*)&height, 1, 0, 10000); //TODO: remove cast
	ImGui::DragFloat("Range", &range, 0.01f, 0.0f, 100.0f);
	ImGui::DragFloat("Cycle", &cycle, 0.01f, 0.0f, 100.0f);

	ImGui::InputText("unknown6", unknown6, 8, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);

	ImGui::DragFloat("Unknown7", &unknown7, 0.01f, 0.0f, 100.0f);
	ImGui::DragFloat("Unknown8", &unknown8, 0.01f, 0.0f, 100.0f);
}

void RswLight::buildImGui(BrowEdit* browEdit)
{
	ImGui::Text("Light");

	for (int i = 0; i < 10; i++)
	{
		ImGui::PushID(i);
		ImGui::DragFloat("Unknown", &todo[i], 0.01f, -100.0f, 100.0f);
		ImGui::PopID();
	}

	ImGui::ColorEdit3("Color", glm::value_ptr(color));
	ImGui::DragFloat("Todo2", &todo2, 0.01f, -100.0f, 100.0f);
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