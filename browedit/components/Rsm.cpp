#include "Rsm.h"

#include <browedit/util/Util.h>
#include <browedit/util/FileIO.h>
#include <iostream>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

Rsm::Rsm(const std::string& fileName)
{
	this->fileName = fileName;
	rootMesh = NULL;
	loaded = false;
	reload();
}

Rsm::~Rsm()
{
	if(rootMesh)
		delete rootMesh;
}


void Rsm::reload()
{
	rootMesh = NULL;
	loaded = false;
	textures.clear();
	if (rootMesh)
		delete rootMesh;
	rootMesh = nullptr;


	std::istream* rsmFile = util::FileIO::open(fileName);
	if (!rsmFile)
	{
		std::cerr << "RSM: Unable to open " << fileName << std::endl;
		return;
	}

	char header[4];
	rsmFile->read(header, 4);
	if (header[0] == 'G' && header[1] == 'R' && header[2] == 'G' && header[3] == 'M')
	{
		std::cerr<< "RSM: Unknown RSM header in file " << fileName << ", stopped loading" << std::endl;
		delete rsmFile;
		return;
	}

	//std::cout << "RSM: loading " << fileName << std::endl;
	
	rsmFile->read(reinterpret_cast<char*>(&version), sizeof(short));
	version = util::swapShort(version);
	rsmFile->read(reinterpret_cast<char*>(&animLen), sizeof(int));
	rsmFile->read(reinterpret_cast<char*>(&shadeType), sizeof(int));

	if (version >= 0x0104)
		alpha = rsmFile->get();
	else
		alpha = 255;
	
	std::string mainNodeName;

	if (version >= 0x0203)
	{
		rsmFile->read(reinterpret_cast<char*>(&fps), sizeof(float));
		animLen = (int)ceil(animLen * fps);
		int rootMeshCount;
		rsmFile->read(reinterpret_cast<char*>(&rootMeshCount), sizeof(int));
		if (rootMeshCount > 1)
			std::cerr << "RSM "<<fileName<<" Warning: multiple root meshes, this is not supported yet" << std::endl;
		for (int i = 0; i < rootMeshCount; i++)
		{
			auto rootMeshName = util::FileIO::readStringDyn(rsmFile);
			mainNodeName = rootMeshName;
		}
		rsmFile->read(reinterpret_cast<char*>(&meshCount), sizeof(int));		
	}
	else if (version >= 0x0202)
	{
		rsmFile->read(reinterpret_cast<char*>(&fps), sizeof(float));
		animLen = (int)ceil(animLen * fps);
		int textureCount;
		rsmFile->read(reinterpret_cast<char*>(&textureCount), sizeof(int));;
		if (textureCount > 100 || textureCount < 0)
		{
			std::cerr << "Invalid textureCount, aborting" << std::endl;
			delete rsmFile;
			return;
		}

		for (int i = 0; i < textureCount; i++)
			textures.push_back(util::FileIO::readStringDyn(rsmFile));

		int rootMeshCount;
		rsmFile->read(reinterpret_cast<char*>(&rootMeshCount), sizeof(int));
		if (rootMeshCount > 1)
			std::cerr << "Warning: multiple root meshes, this is not supported yet" << std::endl;
		for (int i = 0; i < rootMeshCount; i++)
		{
			auto rootMeshName = util::FileIO::readStringDyn(rsmFile);
			mainNodeName = rootMeshName;
		}
		rsmFile->read(reinterpret_cast<char*>(&meshCount), sizeof(int));
	}
	else
	{
		rsmFile->read(unknown, 16);
		for (int i = 0; i < 16; i++)
			assert(unknown[i] == 0);

		int textureCount;
		rsmFile->read(reinterpret_cast<char*>(&textureCount), sizeof(int));;
		if (textureCount > 100 || textureCount < 0)
		{
			std::cerr << "Invalid textureCount, aborting" << std::endl;
			delete rsmFile;
			return;
		}

		for (int i = 0; i < textureCount; i++)
		{
			std::string textureFileName = util::FileIO::readString(rsmFile, 40);;
			textures.push_back(textureFileName);
		}

		mainNodeName = util::FileIO::readString(rsmFile, 40);
		rsmFile->read(reinterpret_cast<char*>(&meshCount), sizeof(int));
	}

	std::map<std::string, Mesh* > meshes;
	for (int i = 0; i < meshCount; i++)
	{
		try
		{
			Mesh* mesh = new Mesh(this, rsmFile);
			mesh->index = i;
			while(meshes.find(mesh->name) != meshes.end())
				mesh->name += "(dup)";
			if (mesh->name == "")
				mesh->name = "empty";
			meshes[mesh->name] = mesh;
		}
		catch (const std::exception& e)
		{
			std::cerr << e.what() << std::endl;
			loaded = false;
			return;
		}
	}

	if (meshes.size() == 0)
	{
		std::cerr << "RsmModel " << fileName << ": does not have any meshes in it" << std::endl;
		return;
	}
	else if (meshes.find(mainNodeName) == meshes.end())
	{
		std::cerr << "RsmModel " << fileName << ": Could not locate root mesh" << std::endl;
		rootMesh = meshes.begin()->second;
	}
	else
		rootMesh = meshes[mainNodeName];

	if (!rootMesh)
	{
		std::cerr << "RsmModel " << fileName << ": does not have a rootmesh? make sure this model has meshes in it" << std::endl;
		return;
	}

	rootMesh->parent = NULL;
	rootMesh->fetchChildren(meshes);


	updateMatrices();

	if (version < 0x0106)
	{
		int numKeyFrames;
		rsmFile->read(reinterpret_cast<char*>(&numKeyFrames), sizeof(int));
		for (int i = 0; i < numKeyFrames; i++)
		{
			int frame;
			glm::vec3 scale;
			float data;

			rsmFile->read(reinterpret_cast<char*>(&frame), sizeof(int));
			rsmFile->read(reinterpret_cast<char*>(glm::value_ptr(scale)), sizeof(float) * 3);
			rsmFile->read(reinterpret_cast<char*>(&data), sizeof(float));
		}
	}

	int numVolumeBox;
	rsmFile->read(reinterpret_cast<char*>(&numVolumeBox), sizeof(int));
//	if (numVolumeBox != 0)
//		std::cerr << "WARNING! This model has " << numVolumeBox << " volume boxes!" << std::endl;

	loaded = true;


	delete rsmFile;
}


void Rsm::updateMatrices()
{
	bbmin = glm::vec3(999999, 999999, 999999);
	bbmax = glm::vec3(-999999, -999999, -9999999);
	dynamic_cast<Mesh*>(rootMesh)->setBoundingBox(bbmin, bbmax);
	bbrange = (bbmin + bbmax) / 2.0f;

	rootMesh->calcMatrix1(0);
	rootMesh->calcMatrix2();

	realbbmax = glm::vec3(-999999, -999999, -999999);
	realbbmin = glm::vec3(999999, 999999, 999999);
	glm::mat4 mat = glm::scale(glm::mat4(1.0f), glm::vec3(1, -1, 1));
	dynamic_cast<Mesh*>(rootMesh)->setBoundingBox2(mat, realbbmin, realbbmax);
	realbbrange = (realbbmax + realbbmin) / 2.0f;
	maxRange = glm::max(glm::max(realbbmax.x, -realbbmin.x), glm::max(glm::max(realbbmax.y, -realbbmin.y), glm::max(realbbmax.z, -realbbmin.z)));

	drawnbbmax = glm::vec3(-999999, -999999, -999999);
	drawnbbmin = glm::vec3(999999, 999999, 999999);
	mat = glm::scale(glm::mat4(1.0f), glm::vec3(1, -1, 1));
	dynamic_cast<Mesh*>(rootMesh)->setBoundingBox3(mat, drawnbbmin, drawnbbmax);
	drawnbbrange = (drawnbbmax - drawnbbmin) / 2.0f;

	setAnimated(rootMesh);
}

void Rsm::setAnimated(Rsm::Mesh* mesh, bool isAnimated)
{
	if (!isAnimated) {
		if (!mesh->rotFrames.empty() || !mesh->scaleFrames.empty() || !mesh->posFrames.empty())
			isAnimated = true;
	}

	mesh->isAnimated = isAnimated;

	for (auto child : mesh->children)
		setAnimated(child, isAnimated);
}

Rsm::Mesh::Mesh(Rsm* model)
{
	this->model = model;
}

Rsm::Mesh::Mesh(Rsm* model, std::istream* rsmFile)
{
	this->model = model;
	if (model->version >= 0x0202)
	{
		name = util::FileIO::readStringDyn(rsmFile);
		parentName = util::FileIO::readStringDyn(rsmFile);
	}
	else
	{
		name = util::FileIO::readString(rsmFile, 40);
		parentName = util::FileIO::readString(rsmFile, 40);
	}

	if (model->version >= 0x0203)
	{
		int textureCount;
		rsmFile->read(reinterpret_cast<char*>(&textureCount), sizeof(int));
		for (int i = 0; i < textureCount; i++)
		{
			std::string textureFile = util::FileIO::readStringDyn(rsmFile);
			auto it = std::find(model->textures.begin(), model->textures.end(), textureFile);
			if (it != model->textures.end())
			{
				textures.push_back((int)(it - model->textures.begin()));
			}
			else
			{
				textures.push_back((int)model->textures.size());
				model->textures.push_back(textureFile);
			}
		}
	}
	else
	{
		int textureCount;
		rsmFile->read(reinterpret_cast<char*>(&textureCount), sizeof(int));
		for (int i = 0; i < textureCount; i++)
		{
			int textureId;
			rsmFile->read(reinterpret_cast<char*>(&textureId), sizeof(int));
			textures.push_back(textureId);
		}
	}
#if 1
	offset = glm::mat4(1.0f);
	rsmFile->read(reinterpret_cast<char*>(&offset[0][0]), sizeof(float));
	rsmFile->read(reinterpret_cast<char*>(&offset[0][1]), sizeof(float));
	rsmFile->read(reinterpret_cast<char*>(&offset[0][2]), sizeof(float));

	rsmFile->read(reinterpret_cast<char*>(&offset[1][0]), sizeof(float));
	rsmFile->read(reinterpret_cast<char*>(&offset[1][1]), sizeof(float));
	rsmFile->read(reinterpret_cast<char*>(&offset[1][2]), sizeof(float));

	rsmFile->read(reinterpret_cast<char*>(&offset[2][0]), sizeof(float));
	rsmFile->read(reinterpret_cast<char*>(&offset[2][1]), sizeof(float));
	rsmFile->read(reinterpret_cast<char*>(&offset[2][2]), sizeof(float));
#else
	offset[0][0] = rsmFile->readFloat();//rotation
	offset[1][0] = rsmFile->readFloat();
	offset[2][0] = rsmFile->readFloat();

	offset[0][1] = rsmFile->readFloat();
	offset[1][1] = rsmFile->readFloat();
	offset[2][1] = rsmFile->readFloat();

	offset[0][2] = rsmFile->readFloat();
	offset[1][2] = rsmFile->readFloat();
	offset[2][2] = rsmFile->readFloat();
#endif
	util::decompose(offset, offsetRotation, offsetScale, offsetPosition);

	rsmFile->read(reinterpret_cast<char*>(glm::value_ptr(pos_)), sizeof(float) * 3);

	if (model->version >= 0x0202)
	{
		pos = glm::vec3(0.0f);
		rotangle = 0.0f;
		rotaxis = glm::vec3(0.0f);
		scale = glm::vec3(1.0f, 1.0f, 1.0f);
	}
	else
	{
		rsmFile->read(reinterpret_cast<char*>(glm::value_ptr(pos)), sizeof(float) * 3);
		rsmFile->read(reinterpret_cast<char*>(&rotangle), sizeof(float));
		rsmFile->read(reinterpret_cast<char*>(glm::value_ptr(rotaxis)), sizeof(float) * 3);
		rsmFile->read(reinterpret_cast<char*>(glm::value_ptr(scale)), sizeof(float) * 3);
	}
	int vertexCount;
	rsmFile->read(reinterpret_cast<char*>(&vertexCount), sizeof(int));
	vertices.resize(vertexCount);
	for (int i = 0; i < vertexCount; i++)
		rsmFile->read(reinterpret_cast<char*>(glm::value_ptr(vertices[i])), sizeof(float) * 3);

	int texCoordCount;
	rsmFile->read(reinterpret_cast<char*>(&texCoordCount), sizeof(int));
	texCoords.resize(texCoordCount);
	for (int i = 0; i < texCoordCount; i++)
	{
		float todo = 0;
		if (model->version >= 0x0102)
		{
			rsmFile->read(reinterpret_cast<char*>(&todo), sizeof(float)); //color??
			//assert(todo == 0);
		}
		rsmFile->read(reinterpret_cast<char*>(glm::value_ptr(texCoords[i])), sizeof(float) * 2);
//		texCoords[i].y = texCoords[i].y;
	}

	int faceCount;
	rsmFile->read(reinterpret_cast<char*>(&faceCount), sizeof(int));
	faces.resize(faceCount);
	for (int i = 0; i < faceCount; i++)
	{
		Face* f = &faces[i];
		int len = -1;

		if(model->version >= 0x0202)
			rsmFile->read(reinterpret_cast<char*>(&len), sizeof(int));

		rsmFile->read(reinterpret_cast<char*>(f->vertexIds), sizeof(short) * 3);
		rsmFile->read(reinterpret_cast<char*>(f->texCoordIds), sizeof(short) * 3);
		rsmFile->read(reinterpret_cast<char*>(&f->texId), sizeof(short));
		rsmFile->read(reinterpret_cast<char*>(&f->padding), sizeof(short));
		rsmFile->read(reinterpret_cast<char*>(&f->twoSided), sizeof(int));
		f->twoSided = f->twoSided > 0 ? 1 : 0;

		if (model->version >= 0x0102)
		{
			rsmFile->read(reinterpret_cast<char*>(&f->smoothGroups[0]), sizeof(int));
			if (len > 24)
				rsmFile->read(reinterpret_cast<char*>(&f->smoothGroups[1]), sizeof(int));
			if (len > 28)
				rsmFile->read(reinterpret_cast<char*>(&f->smoothGroups[2]), sizeof(int));
			for(int i = 32; i < len; i+=4)
			{
				// Tokei: those are group masks from 3DS models, actually. So the smoothGroups don't belong to a vertex, but rather is a single big ID flag.
				int tmp;
				rsmFile->read(reinterpret_cast<char*>(&tmp), sizeof(int));
			}
		}
		bool ok = true;
		for (int ii = 0; ii < 3; ii++)
			if (f->vertexIds[ii] >= vertices.size())
				ok = false;
		if (ok)
			f->normal = glm::normalize(glm::cross(vertices[f->vertexIds[1]] - vertices[f->vertexIds[0]], vertices[f->vertexIds[2]] - vertices[f->vertexIds[0]]));
		else
		{
			std::cerr << "There's an error in " << model->fileName << std::endl;
			model->loaded = false;
			throw std::exception("vertex out of bounds");
		}
	}

	if (model->shadeType == ShadeType::SHADE_FLAT)
		for (auto& f : faces)
			for (int ii = 0; ii < 3; ii++)
				f.vertexNormals[ii] = f.normal;
	if (model->shadeType == ShadeType::SHADE_SMOOTH)
	{
		std::map<int, std::map<int, glm::vec3>> vertexNormals;
		for (auto& f : faces)
			for (int i = 0; i < 3; i++)
				for (int ii = 0; ii < 3; ii++)
					vertexNormals[f.smoothGroups[ii]][f.vertexIds[i]] += f.normal;
		for (auto& f : faces)
		{
			for (int i = 0; i < 3; i++)
			{
				f.vertexNormals[i] = glm::normalize(vertexNormals[f.smoothGroups[0]][f.vertexIds[i]]);
			}
		}
	}


	if (model->version >= 0x0106)
	{
		int scaleFrameCount;
		rsmFile->read(reinterpret_cast<char*>(&scaleFrameCount), sizeof(int));
		scaleFrames.resize(scaleFrameCount);
		for (int i = 0; i < scaleFrameCount; i++)
		{
			rsmFile->read(reinterpret_cast<char*>(&scaleFrames[i].time), sizeof(int));
			rsmFile->read(reinterpret_cast<char*>(glm::value_ptr(scaleFrames[i].scale)), sizeof(float) * 3);
			rsmFile->read(reinterpret_cast<char*>(&scaleFrames[i].data), sizeof(float));
			if (model->version >= 0x0202)
				scaleFrames[i].time = (int)ceil(scaleFrames[i].time * model->fps); //TODO: remove this
		}
	}


	int rotFrameCount;
	rsmFile->read(reinterpret_cast<char*>(&rotFrameCount), sizeof(int));
	rotFrames.resize(rotFrameCount);
	for (int i = 0; i < rotFrameCount; i++)
	{
		rsmFile->read(reinterpret_cast<char*>(&rotFrames[i].time), sizeof(int));
		float x, y, z, w;
		rsmFile->read(reinterpret_cast<char*>(&x), sizeof(float));
		rsmFile->read(reinterpret_cast<char*>(&y), sizeof(float));
		rsmFile->read(reinterpret_cast<char*>(&z), sizeof(float));
		rsmFile->read(reinterpret_cast<char*>(&w), sizeof(float));
		rotFrames[i].quaternion = glm::quat(w, x, y, z);
		if (model->version >= 0x0202)
			rotFrames[i].time = (int)ceil(rotFrames[i].time * model->fps); //TODO: remove this

	}

	if (model->version >= 0x0202)
	{
		int posKeyFrameCount;
		rsmFile->read(reinterpret_cast<char*>(&posKeyFrameCount), sizeof(int));
		posFrames.resize(posKeyFrameCount);
		for (int i = 0; i < posKeyFrameCount; i++)
		{
			rsmFile->read(reinterpret_cast<char*>(&posFrames[i].time), sizeof(int));
			rsmFile->read(reinterpret_cast<char*>(glm::value_ptr(posFrames[i].position)), sizeof(float) * 3);
			rsmFile->read(reinterpret_cast<char*>(&posFrames[i].data), sizeof(float));
			posFrames[i].time = (int)ceil(posFrames[i].time * model->fps); //TODO: remove this
		}
		if (model->version >= 0x0203)
		{
			int textureAnimCount;
			rsmFile->read(reinterpret_cast<char*>(&textureAnimCount), sizeof(int));
			for (int i = 0; i < textureAnimCount; i++)
			{
				int textureId;
				rsmFile->read(reinterpret_cast<char*>(&textureId), sizeof(int));

				std::map<int, std::vector<TexFrame>> animationTypes;
				int textureIdAnimationCount;
				rsmFile->read(reinterpret_cast<char*>(&textureIdAnimationCount), sizeof(int));

				for (int i = 0; i < textureIdAnimationCount; i++)
				{
					int type, amountFrames;
					rsmFile->read(reinterpret_cast<char*>(&type), sizeof(int));
					rsmFile->read(reinterpret_cast<char*>(&amountFrames), sizeof(int));

					std::vector<TexFrame> frames;
					frames.resize(amountFrames);

					for (int ii = 0; ii < amountFrames; ii++)
					{
						rsmFile->read(reinterpret_cast<char*>(&frames[ii].time), sizeof(int));
						rsmFile->read(reinterpret_cast<char*>(&frames[ii].data), sizeof(float));
						frames[ii].time = (int)ceil(frames[ii].time * model->fps); //TODO: remove this
					}

					animationTypes[type] = frames;
				}

				textureFrames[textureId] = animationTypes;
			}
		}
	}
}

Rsm::Mesh::~Mesh()
{
	for (auto child : children)
		delete child;
	faces.clear();
	children.clear();

}

void Rsm::Mesh::fetchChildren(std::map<std::string, Mesh* > meshes)
{
	for (std::map<std::string, Mesh*, std::less<std::string> >::iterator it = meshes.begin(); it != meshes.end(); it++)
	{
		if (it->second->parentName == name && it->second != this)
		{
			it->second->parent = this;
			children.push_back(it->second);
		}
	}
	for (unsigned int i = 0; i < children.size(); i++)
		children[i]->fetchChildren(meshes);
}

void Rsm::Mesh::calcMatrix1(int time)
{
	matrix1 = glm::mat4(1.0f);
	float cull = 1;

	if (model->version < 0x0202) {
		if (parent == NULL)
		{
			if (children.size() > 0)
				matrix1 = glm::translate(matrix1, glm::vec3(-model->bbrange.x, -model->bbmax.y, -model->bbrange.z));
			else
				matrix1 = glm::translate(matrix1, glm::vec3(0, -model->bbmax.y + model->bbrange.y, 0));
		}
		else
			matrix1 = glm::translate(matrix1, pos);

		if (rotFrames.size() == 0)
		{
			if (fabs(rotangle) > 0.01)
				matrix1 = glm::rotate(matrix1, glm::radians(rotangle * 180.0f / 3.14159f), rotaxis); //TODO: double conversion
		}
		else
		{
			if (rotFrames[rotFrames.size() - 1].time != 0)
			{
				int tick = time % rotFrames[rotFrames.size() - 1].time;
				int current = 0;
				for (unsigned int i = 0; i < rotFrames.size(); i++)
				{
					if (rotFrames[i].time > tick)
					{
						current = i - 1;
						break;
					}
				}
				if (current < 0)
					current = 0;

				int next = current + 1;
				if (next >= (int)rotFrames.size())
					next = 0;

				float interval = ((float)(tick - rotFrames[current].time)) / ((float)(rotFrames[next].time - rotFrames[current].time));
#if 1
				glm::quat quat = glm::mix(rotFrames[current].quaternion, rotFrames[next].quaternion, interval);
#else
				bEngine::math::cQuaternion quat(
					(1 - interval) * animationFrames[current].quat.x + interval * animationFrames[next].quat.x,
					(1 - interval) * animationFrames[current].quat.y + interval * animationFrames[next].quat.y,
					(1 - interval) * animationFrames[current].quat.z + interval * animationFrames[next].quat.z,
					(1 - interval) * animationFrames[current].quat.w + interval * animationFrames[next].quat.w);
#endif

				quat = glm::normalize(quat);

				matrix1 = matrix1 * glm::toMat4(quat);
			}
			else
			{
				matrix1 *= glm::toMat4(glm::normalize(rotFrames[0].quaternion));
			}
		}

		matrix1 = glm::scale(matrix1, scale);

		// Cull face check
		if (parent != nullptr)
			cull = parent->reverseCullFaceSub ? -1.0f : 1.0f;

		cull = cull * scale.x * scale.y * scale.z;
		reverseCullFaceSub = cull < 0;
		cull = cull * offsetScale.x * offsetScale.y * offsetScale.z;
		reverseCullFace = cull < 0;
	}
	else {
		matrix2 = glm::mat4(1.0f);

		if (scaleFrames.size() > 0) {
			int tick = time % glm::max(1, model->animLen);
			int prevIndex = -1;
			int nextIndex = 0;

			for (; nextIndex < scaleFrames.size() && tick >= scaleFrames[nextIndex].time; nextIndex++) {
			}

			prevIndex = nextIndex - 1;
			float prevTick = (float)(prevIndex < 0 ? 0 : scaleFrames[prevIndex].time);
			float nextTick = (float)(nextIndex == scaleFrames.size() ? model->animLen : scaleFrames[nextIndex].time);
			glm::vec3 prev = prevIndex < 0 ? glm::vec3(1) : scaleFrames[prevIndex].scale;
			glm::vec3 next = nextIndex == scaleFrames.size() ? scaleFrames[nextIndex - 1].scale : scaleFrames[nextIndex].scale;

			float mult = (tick - prevTick) / (nextTick - prevTick);
			matrix1 = glm::scale(matrix1, mult * (next - prev) + prev);
		}

		if (rotFrames.size() > 0)
		{
			int tick = time % glm::max(1, model->animLen);
			int prevIndex = -1;
			int nextIndex = 0;
			
			for (; nextIndex < rotFrames.size() && tick >= rotFrames[nextIndex].time; nextIndex++) {
			}
			
			prevIndex = nextIndex - 1;
			float prevTick = (float)(prevIndex < 0 ? 0 : rotFrames[prevIndex].time);
			float nextTick = (float)(nextIndex == rotFrames.size() ? model->animLen : rotFrames[nextIndex].time);
			glm::quat prev = prevIndex < 0 ? glm::quat(1, 0, 0, 0) : rotFrames[prevIndex].quaternion;
			glm::quat next = nextIndex == rotFrames.size() ? rotFrames[nextIndex - 1].quaternion : rotFrames[nextIndex].quaternion;
			
			float mult = (tick - prevTick) / (nextTick - prevTick);
			glm::quat quat = glm::slerp(prev, next, mult);
			matrix1 = glm::toMat4(quat) * matrix1;
		}
		else
		{
			matrix1 = offset * matrix1;

			if (parent != NULL)
			{
				matrix1 = glm::inverse(parent->offset) * matrix1;
			}
		}

		matrix2 = glm::mat4(matrix1);
		glm::vec3 position;
		
		if (posFrames.size() > 0) {
			int tick = time % glm::max(1, model->animLen);
			int prevIndex = -1;
			int nextIndex = 0;

			for (; nextIndex < posFrames.size() && tick >= posFrames[nextIndex].time; nextIndex++) {
			}

			prevIndex = nextIndex - 1;
			float prevTick = (float)(prevIndex < 0 ? 0 : posFrames[prevIndex].time);
			float nextTick = (float)(nextIndex == posFrames.size() ? model->animLen : posFrames[nextIndex].time);
			glm::vec3 prev = prevIndex < 0 ? pos_ : posFrames[prevIndex].position;
			glm::vec3 next = nextIndex == posFrames.size() ? posFrames[nextIndex - 1].position : posFrames[nextIndex].position;

			float mult = (tick - prevTick) / (nextTick - prevTick);
			position = mult * (next - prev) + prev;
		}
		else
		{
			if (parent != NULL)
			{
				position = pos_ - parent->pos_;
				position = glm::inverse(parent->offset) * glm::vec4(position.x, position.y, position.z, 0);
			}
			else {
				position = pos_;
			}
		}

		matrix2[3].x = position.x;
		matrix2[3].y = position.y;
		matrix2[3].z = position.z;

		Mesh *mesh = this;

		while (mesh->parent != NULL)
		{
			mesh = mesh->parent;
			matrix2 = mesh->matrix1 * matrix2;
		}
		
		if (parent != NULL)
		{
			matrix2[3].x += parent->matrix2[3].x;
			matrix2[3].y += parent->matrix2[3].y;
			matrix2[3].z += parent->matrix2[3].z;
		}

		// Cull face check
		reverseCullFaceSub = cull < 0;
		cull = cull * offsetScale.x * offsetScale.y * offsetScale.z;
		reverseCullFace = cull < 0;
	}

	for (unsigned int i = 0; i < children.size(); i++)
		children[i]->calcMatrix1(time);
}

void Rsm::Mesh::calcMatrix2()
{
	if (model->version < 0x202) {
		matrix2 = glm::mat4(1.0f);

		if (parent == NULL && children.size() == 0)
			matrix2 = glm::translate(matrix2, pos_);

		if (parent != NULL || children.size() != 0)
			matrix2 = glm::translate(matrix2, pos_);

		matrix2 *= offset;

		for (unsigned int i = 0; i < children.size(); i++)
			children[i]->calcMatrix2();
	}
}

bool Rsm::Mesh::getTextureAnimation(int textureId, int time, glm::vec2& texTranslate, glm::vec3& texScale, float& texRot)
{
	auto texEntry = textureFrames.find(textureId);

	if (texEntry == textureFrames.end())
		return false;

	int tick = time % glm::max(1, model->animLen);
	texRot = 0;

	for (const auto& [type, texFrames] : texEntry->second) {
		if (texFrames.size() == 0)
			continue;

		int prevIndex = -1;
		int nextIndex = 0;

		for (; nextIndex < texFrames.size() && tick >= texFrames[nextIndex].time; nextIndex++) {
		}

		prevIndex = nextIndex - 1;
		float prevTick = (float)(prevIndex < 0 ? 0 : texFrames[prevIndex].time);
		float nextTick = (float)(nextIndex == texFrames.size() ? model->animLen : texFrames[nextIndex].time);
		float prev = prevIndex < 0 ? 0 : texFrames[prevIndex].data;
		float next = nextIndex == texFrames.size() ? texFrames[nextIndex - 1].data : texFrames[nextIndex].data;

		float mult = (tick - prevTick) / (nextTick - prevTick);
		float offset = mult * (next - prev) + prev;

		switch (type)
		{
		case 0:
			texTranslate.x += offset;
			break;
		case 1:
			texTranslate.y += offset;
			break;
		case 2:
			texScale.x = offset;
			break;
		case 3:
			texScale.y = offset;
			break;
		case 4:
			texRot = offset;
			break;
		default:
			break;
		}
	}

	return true;
}

void Rsm::Mesh::setBoundingBox(glm::vec3& _bbmin, glm::vec3& _bbmax)
{
	int c;
	bbmin = glm::vec3(9999999, 9999999, 9999999);
	bbmax = glm::vec3(-9999999, -9999999, -9999999);

	if (parent != NULL)
		bbmin = bbmax = glm::vec3(0, 0, 0);

	glm::mat4 myMat = offset;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		for (int ii = 0; ii < 3; ii++)
		{
			glm::vec4 v(vertices[faces[i].vertexIds[ii]], 1);
			v = myMat * v;
			if (parent != NULL || children.size() != 0)
				v += glm::vec4(pos + pos_, 1);
			for (c = 0; c < 3; c++)
			{
				bbmin[c] = glm::min(bbmin[c], v[c]);
				bbmax[c] = glm::max(bbmax[c], v[c]);
			}
		}
	}
	bbrange = (bbmin + bbmax) / 2.0f;

	for (int c = 0; c < 3; c++)
		for (unsigned int i = 0; i < 3; i++)
		{
			_bbmax[c] = glm::max(_bbmax[c], bbmax[c]);
			_bbmin[c] = glm::min(_bbmin[c], bbmin[c]);
		}

	for (unsigned int i = 0; i < children.size(); i++)
		dynamic_cast<Mesh*>(children[i])->setBoundingBox(_bbmin, _bbmax);
}

void Rsm::Mesh::setBoundingBox2(glm::mat4& mat, glm::vec3& bbmin_, glm::vec3& bbmax_)
{
	glm::mat4 mat1 = mat * matrix1;
	glm::mat4 mat2 = mat * matrix1 * matrix2;
	for (unsigned int i = 0; i < vertices.size(); i++)
	{
		glm::vec4 v = mat2 * glm::vec4(vertices[i], 1);
		bbmin_.x = glm::min(bbmin_.x, v.x);
		bbmin_.y = glm::min(bbmin_.y, v.y);
		bbmin_.z = glm::min(bbmin_.z, v.z);

		bbmax_.x = glm::max(bbmax_.x, v.x);
		bbmax_.y = glm::max(bbmax_.y, v.y);
		bbmax_.z = glm::max(bbmax_.z, v.z);
	}

	for (unsigned int i = 0; i < children.size(); i++)
		dynamic_cast<Mesh*>(children[i])->setBoundingBox2(mat1, bbmin_, bbmax_);
}

void Rsm::Mesh::setBoundingBox3(glm::mat4& mat, glm::vec3& bbmin_, glm::vec3& bbmax_)
{
	if (model->version >= 0x202) {
		for (unsigned int i = 0; i < faces.size(); i++)
		{
			for (int ii = 0; ii < 3; ii++)
			{
				glm::vec4 v = matrix2 * glm::vec4(vertices[faces[i].vertexIds[ii]], 1);
				bbmin_.x = glm::min(bbmin_.x, v.x);
				bbmin_.y = glm::min(bbmin_.y, v.y);
				bbmin_.z = glm::min(bbmin_.z, v.z);

				bbmax_.x = glm::max(bbmax_.x, v.x);
				bbmax_.y = glm::max(bbmax_.y, v.y);
				bbmax_.z = glm::max(bbmax_.z, v.z);
			}
		}

		for (unsigned int i = 0; i < children.size(); i++)
			dynamic_cast<Mesh*>(children[i])->setBoundingBox3(mat, bbmin_, bbmax_);
	}
	else {
		glm::mat4 mat1 = mat * matrix1;
		glm::mat4 mat2 = mat * matrix1 * matrix2;
		for (unsigned int i = 0; i < faces.size(); i++)
		{
			for (int ii = 0; ii < 3; ii++)
			{
				glm::vec4 v = mat2 * glm::vec4(vertices[faces[i].vertexIds[ii]], 1);
				bbmin_.x = glm::min(bbmin_.x, v.x);
				bbmin_.y = glm::min(bbmin_.y, v.y);
				bbmin_.z = glm::min(bbmin_.z, v.z);

				bbmax_.x = glm::max(bbmax_.x, v.x);
				bbmax_.y = glm::max(bbmax_.y, v.y);
				bbmax_.z = glm::max(bbmax_.z, v.z);
			}
		}

		for (unsigned int i = 0; i < children.size(); i++)
			dynamic_cast<Mesh*>(children[i])->setBoundingBox3(mat1, bbmin_, bbmax_);
	}
}

/**
 * Saves the current mesh
 */
void Rsm::Mesh::save(std::ostream* pFile)
{
	Mesh* t = this;
	char zeroes[100];
	int todo = 0;
	int len;
	
	memset(&zeroes, 0x0, sizeof(zeroes));
	
	// Mesh name
	len = (int)t->name.length();
	pFile->write(t->name.c_str(), len);
	pFile->write(zeroes, 40 - len);
	// Parent Name
	len = (int)t->parentName.length();
	pFile->write(t->parentName.c_str(), len);
	pFile->write(zeroes, 40 - len);
	// Texture ID count
	auto textureCount = t->textures.size();
	pFile->write((char*)&textureCount, 4);
	// Texture ID
	for (int i = 0; i < textureCount; i++) {
		pFile->write((char*)&t->textures[i], 4);
	}
	pFile->write((char*)&t->offset[0][0], sizeof(float));
	pFile->write((char*)&t->offset[0][1], sizeof(float));
	pFile->write((char*)&t->offset[0][2], sizeof(float));

	pFile->write((char*)&t->offset[1][0], sizeof(float));
	pFile->write((char*)&t->offset[1][1], sizeof(float));
	pFile->write((char*)&t->offset[1][2], sizeof(float));

	pFile->write((char*)&t->offset[2][0], sizeof(float));
	pFile->write((char*)&t->offset[2][1], sizeof(float));
	pFile->write((char*)&t->offset[2][2], sizeof(float));

	pFile->write((char*)&t->pos_, sizeof(float) * 3);
	pFile->write((char*)&t->pos, sizeof(float) * 3);
	pFile->write((char*)&t->rotangle, sizeof(float));
	pFile->write((char*)&t->rotaxis, sizeof(float) * 3);
	pFile->write((char*)&t->scale, sizeof(float) * 3);

	// Vertices
	auto vertexCount = t->vertices.size();
	pFile->write((char*)&vertexCount, sizeof(int));

	for (int i = 0; i < vertexCount; i++) {
		pFile->write((char*)&t->vertices[i], sizeof(float) * 3);
	}

	// Texture Coordinates
	auto texCoordCount = t->texCoords.size();
	pFile->write((char*)&texCoordCount, sizeof(int));
	for (int i = 0; i < texCoordCount; i++)
	{
		pFile->write((char*)&todo, sizeof(int));
		pFile->write((char*)&t->texCoords[i], sizeof(float) * 2);
	}

	// Faces
	auto faceCount = t->faces.size();
	pFile->write((char*)&faceCount, sizeof(int));
	for (int i = 0; i < faceCount; i++)
	{
		pFile->write((char*)&t->faces[i].vertexIds, sizeof(short) * 3);
		pFile->write((char*)&t->faces[i].texCoordIds, sizeof(short) * 3);
		pFile->write((char*)&t->faces[i].texId, sizeof(short));
		pFile->write((char*)&t->faces[i].padding, sizeof(short));
		pFile->write((char*)&t->faces[i].twoSided, sizeof(int));
		// Smooth group
		pFile->write((char*)&todo, sizeof(int));
		//pFile->write((char*)&t->faces[i].smoothGroups[0], sizeof(int));
	}
	// Scale Frame
	pFile->write((char*)&todo, sizeof(int));
	// Rotation Frame
	pFile->write((char*)&todo, sizeof(int));
}

void Rsm::save(const std::string& filename)
{
	char zeroes[100];
	int todo = 0;
	int len;
	std::ofstream pFile( filename, std::ios::out | std::ios::binary);
	memset(&zeroes, 0x0, sizeof(zeroes));
	
	// Header + Version
	pFile.write("GRSM\1\4", 6);
	// Anim Lenght
	pFile.write((char*)&this->animLen, 4);
	// Shade Type
	pFile.write((char*)&this->shadeType, 4);
	// Rsm Alpha
	pFile.write((char*)&this->alpha, 1);
	// Unknown
	pFile.write((char*)zeroes, 16);
	// Materials num (textures)
	int materialsNum = (int)this->textures.size();
	pFile.write((char*)&materialsNum, 4);
	for (auto& t : this->textures)
	{
		std::string texture = t.c_str();
		len = (int)t.length();
		
		pFile.write(t.c_str(), len);
		pFile.write(zeroes, 40 - len);
	}
	
	/**
	 * Meshes
	 */
	pFile.write("root", 4);	pFile.write(zeroes, 36);	//node name
	
	// Mesh Count
	pFile.write((char*)&this->meshCount, 4);
	this->rootMesh->save(&pFile);
	// Saves children
	for (auto& t : this->rootMesh->children ) {
		t->save(&pFile);	
	}
	
	// Volume Box
	pFile.write((char*)&todo, sizeof(int));

	std::cout << "Saved RSM model " << filename << std::endl;
}
