#ifdef _WIN32
	#include <windows.h>
	#include <mmsystem.h>
#endif
#include <browedit/BrowEdit.h>
#include <browedit/Node.h>
#include <browedit/MapView.h>
#include <browedit/Map.h>
#include <browedit/components/RsmRenderer.h>
#include <browedit/components/StrRenderer.h>
#include <browedit/components/BillboardRenderer.h>
#include <browedit/components/Rsm.h>
#include <browedit/components/Str.h>
#include <browedit/components/Rsw.h>
#include <browedit/components/LubRenderer.h>
#include <browedit/shaders/SimpleShader.h>
#include <browedit/gl/FBO.h>
#include <browedit/gl/Texture.h>
#include <browedit/util/Util.h>
#include <browedit/util/FileIO.h>
#include <browedit/util/ResourceManager.h>
#include <browedit/actions/GroupAction.h>
#include <browedit/actions/SelectAction.h>
#include <browedit/actions/ModelChangeAction.h>
#include <sol.hpp>

#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <thread>
#include <iostream>

//TODO: this file is a mess
//Tokei: yes, yes it is (and I made it worse!)

class ObjectWindowObject
{
public:
	enum ObjectType {
		Object_Rsm,
		Object_Str,
		Object_Lub
	} objectType = ObjectType::Object_Rsm;

	Node* node;
	gl::FBO* fbo;
	NodeRenderContext nodeRenderContext;
	static inline gl::Texture* texture;
	static inline SimpleShader* simpleShader;
	float time = 0.0f;
	float offsetTime = 0.0f;
	float rotation = 0;
	BrowEdit* browEdit;

	ObjectWindowObject(const std::string& fileName, BrowEdit* browEdit) : browEdit(browEdit)
	{
		fbo = new gl::FBO((int)browEdit->config.thumbnailSize.x, (int)browEdit->config.thumbnailSize.y, true); //TODO: resolution?
		node = new Node();

		if (fileName.rfind(".rsm") != std::string::npos ||
			fileName.rfind(".rsm2") != std::string::npos) {
			objectType = ObjectType::Object_Rsm;
		}
		else if (fileName.rfind(".str") != std::string::npos) {
			objectType = ObjectType::Object_Str;
		}

		switch (objectType) {
		case ObjectType::Object_Rsm:
			node->addComponent(util::ResourceManager<Rsm>::load(fileName));
			node->addComponent(new RsmRenderer());
			break;
		case ObjectType::Object_Str:
			auto strEffect = new StrEffect();
			strEffect->str = fileName.substr(20); // remove data\texture\effect\ 
			node->addComponent(strEffect);
			node->addComponent(new StrRenderer());
			break;
		}
	}

	ObjectWindowObject(const sol::table& table, BrowEdit* browEdit) : browEdit(browEdit)
	{
		fbo = new gl::FBO((int)browEdit->config.thumbnailSize.x, (int)browEdit->config.thumbnailSize.y, true); //TODO: resolution?
		node = new Node();

		objectType = ObjectType::Object_Lub;
		auto lubEffect = new LubEffect();
		lubEffect->load(table);

		// For proper preview, always display with billboard and boost the alpha value a bit.
		lubEffect->billboard_off = 0;
		float r = lubEffect->color.r;
		float g = lubEffect->color.g;
		float b = lubEffect->color.b;
		float a = lubEffect->color.a;

		if (a < 0.5f)
			a *= 2;
		if (a < 0.5f)
			a += 0.5f;

		a = glm::min(1.0f, a);

		lubEffect->color = glm::vec4(r, g, b, a);
		node->addComponent(lubEffect);
		node->addComponent(new LubRenderer());
	}

	void draw()
	{
		fbo->bind();
		glViewport(0, 0, fbo->getWidth(), fbo->getHeight());
		glClearColor(browEdit->config.backgroundColor.r, browEdit->config.backgroundColor.g, browEdit->config.backgroundColor.b, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glDisable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

		switch (objectType) {
		case ObjectType::Object_Rsm: {
			auto rsm = node->getComponent<Rsm>();
			if (!rsm->loaded)
				return;

			float distance = 2.5f * glm::max(rsm->drawnbbrange.x, glm::max(rsm->drawnbbrange.y, rsm->drawnbbrange.z));

			float ratio = fbo->getWidth() / (float)fbo->getHeight();
			nodeRenderContext.projectionMatrix = glm::perspective(glm::radians(45.0f), ratio, 0.1f, 5000.0f);
			nodeRenderContext.viewMatrix = glm::lookAt(glm::vec3(-distance * glm::sin(glm::radians(rotation)), -distance, -distance * glm::cos(glm::radians(rotation))), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			RsmRenderer::RsmRenderContext::getInstance()->viewLighting = false;
			glm::mat4 mat = glm::mat4(1.0f);
			if (rsm->version >= 0x202) {
				mat = glm::scale(mat, glm::vec3(1, -1, 1));
				node->getComponent<RsmRenderer>()->reverseCullFace = true;
			}
			mat = glm::translate(mat, glm::vec3(-rsm->realbbrange.x, rsm->realbbrange.y, -rsm->realbbrange.z));
			node->getComponent<RsmRenderer>()->matrixCache = mat;
			node->getComponent<RsmRenderer>()->matrixCached = true;
			nodeRenderContext.time = (float)glfwGetTime();
		}
			break;
		case ObjectType::Object_Str: {
			// Str coordinates are scaled to the world coordinates system (so divided by 5).
			// There's also... the weirdo 0.7f constant that seems to be what Gravity does for some reason.
			const float strScaling = 0.2f * 0.7f;

			// Ideally, centering around around the dimensions of the animation would be best, but that would require rendering each frame one by one and that's just too much.
			// Instead, approximate to 500x500, it's "close enough".
			float distance = 500.0f * strScaling;
			float ratio = fbo->getWidth() / (float)fbo->getHeight();

			nodeRenderContext.projectionMatrix = glm::ortho(-distance / 2 * ratio, distance / 2 * ratio, distance / 2, -distance / 2, -100.0f, 100.0f);
			nodeRenderContext.viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0, -60 * strScaling, 0));
			nodeRenderContext.time = (float)glfwGetTime();
		}
			break;
		case ObjectType::Object_Lub: {
			auto lubEffect = node->getComponent<LubEffect>();
			float distance = glm::max(lubEffect->size.x, lubEffect->size.y) * glm::max(lubEffect->scale.x, lubEffect->scale.y) * 2 + glm::max(lubEffect->radius.x, lubEffect->radius.y);
			float ratio = fbo->getWidth() / (float)fbo->getHeight();

			// Draw a dummy texture on the background
			if (texture == nullptr)
				texture = util::ResourceManager<gl::Texture>::load("data\\texture\\lubeffect_background.bmp");
			
			if (simpleShader == nullptr)
				simpleShader = util::ResourceManager<gl::Shader>::load<SimpleShader>();

			nodeRenderContext.projectionMatrix = glm::ortho(-distance / 2 * ratio, distance / 2 * ratio, distance / 2, -distance / 2, -100.0f, 100.0f);
			nodeRenderContext.viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0));

			if (texture) {
				texture->bind();
				simpleShader->use();
				simpleShader->setUniform(SimpleShader::Uniforms::modelMatrix, glm::mat4(1.0f));
				simpleShader->setUniform(SimpleShader::Uniforms::textureFac, 1.0f);
				simpleShader->setUniform(SimpleShader::Uniforms::shadeType, 1);
				simpleShader->setUniform(SimpleShader::Uniforms::color, glm::vec4(0));
				simpleShader->setUniform(SimpleShader::Uniforms::colorMult, glm::vec4(0.8f, 0.8f, 0.8f, 1));
				simpleShader->setUniform(SimpleShader::Uniforms::projectionMatrix, nodeRenderContext.projectionMatrix);
				simpleShader->setUniform(SimpleShader::Uniforms::viewMatrix, nodeRenderContext.viewMatrix);
			
				float size = distance;
				std::vector<VertexP3T2> verts;
				verts.push_back(VertexP3T2(glm::vec3(-size, -size, 0), glm::vec2(0, 0)));
				verts.push_back(VertexP3T2(glm::vec3(-size, size, 0), glm::vec2(0, 1)));
				verts.push_back(VertexP3T2(glm::vec3(size, size, 0), glm::vec2(1, 1)));
				verts.push_back(VertexP3T2(glm::vec3(size, -size, 0), glm::vec2(1, 0)));
			
				glDisable(GL_DEPTH_TEST);
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glDepthMask(0);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				glEnableVertexAttribArray(0);
				glEnableVertexAttribArray(1);
				glDisableVertexAttribArray(2);
				glDisableVertexAttribArray(3);
				glDisableVertexAttribArray(4);
			
				glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2), verts[0].data);
				glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2), verts[0].data + 3);
			
				glDrawArrays(GL_QUADS, 0, 4);
				glEnable(GL_DEPTH_TEST);
			
				simpleShader->setUniform(SimpleShader::Uniforms::shadeType, 0);
				simpleShader->setUniform(SimpleShader::Uniforms::textureFac, 0.0f);
				simpleShader->setUniform(SimpleShader::Uniforms::colorMult, glm::vec4(1));
			
				float currentTime = (float)glfwGetTime();
			
				if (currentTime - time > 2.0f) {
					offsetTime += currentTime - time;
				}
			
				nodeRenderContext.time = currentTime - offsetTime;
				time = currentTime;
			}

			break;
		}
		}

		nodeRenderContext.fbo = fbo;
		NodeRenderer::render(node, nodeRenderContext);
		
		fbo->unbind();
	}
};
std::map<std::string, ObjectWindowObject*> objectWindowObjects;

void BrowEdit::showObjectWindow()
{
	if (!ImGui::Begin("Object Picker", &windowData.objectWindowVisible))
	{
		ImGui::End();
		return;
	}
	static bool verticalLayout = ImGui::GetContentRegionAvail().x < 300;

	static std::string filter;
	ImGui::SetNextItemWidth(ImGui::GetWindowSize().x * 0.65f - 50);
	ImGui::InputText("Filter", &filter);
	ImGui::SameLine();

	static std::thread progressThread;
	static std::map<std::string, std::vector<std::string>> newTagList; // tag -> [ file ], utf8

	if (ImGui::Button("Rescan map filters"))
	{
		windowData.progressWindowVisible = true;
		windowData.progressWindowText = "Loading maps...";
		windowData.progressWindowProgres = 0;
		windowData.progressWindowOnDone = [&]() {
			std::cout << "Done!" << std::endl;
			for (auto it = tagList.begin(); it != tagList.end(); )
				if (it->first.size() > 4 && it->first.substr(0, 4) == "map:")
					it = tagList.erase(it);
				else
					it++;
			for (auto t : newTagList)
				tagList[t.first] = t.second;
			for (auto tag : tagList)
				for (const auto& t : tag.second)
					tagListReverse[util::tolower(util::utf8_to_iso_8859_1(t))].push_back(util::utf8_to_iso_8859_1(tag.first));
			saveTagList();
			std::cout << "TagList merged and saved to file" << std::endl;
		};

		progressThread = std::thread([&]() {
			std::vector<std::string> maps = util::FileIO::listFiles("data");
			maps.erase(std::remove_if(maps.begin(), maps.end(), [](const std::string& map) { return map.substr(map.size() - 4, 4) != ".rsw"; }), maps.end());
			windowData.progressWindowProgres = 0;

			std::map<int, std::vector<std::string>> effectFiles;
			for (auto& f : util::FileIO::listFiles("data\\effects"))
				if(f.rfind(".json") != std::string::npos)
					effectFiles[util::FileIO::getJson(f)["id"]].push_back(f);

			for (const auto& fileName : maps)
			{
				windowData.progressWindowProgres += (1.0f / maps.size());
				Node* node = new Node();
				Rsw* rsw = new Rsw();
				node->addComponent(rsw);
				rsw->load(fileName, nullptr, this, false, true);

				std::string mapTag = fileName;
				if (mapTag.substr(0, 5) == "data\\")
					mapTag = mapTag.substr(5);
				if (mapTag.substr(mapTag.size() - 4, 4) == ".rsw")
					mapTag = mapTag.substr(0, mapTag.size() - 4);

				mapTag = "map:" + util::iso_8859_1_to_utf8(mapTag);

				auto gnd = node->getComponent<Gnd>();
				for (auto t : gnd->textures)
				{
					if (std::find(newTagList[mapTag].begin(), newTagList[mapTag].end(), "data\\texture\\" + t->file) == newTagList[mapTag].end())
						newTagList[mapTag].push_back("data\\texture\\" + util::iso_8859_1_to_utf8(t->file));
				}

				node->traverse([&](Node* node)
					{
						auto rswModel = node->getComponent<RswModel>();
						if (rswModel)
						{
							if (std::find(newTagList[mapTag].begin(), newTagList[mapTag].end(), "data\\model\\" + rswModel->fileName) == newTagList[mapTag].end())
								newTagList[mapTag].push_back("data\\model\\"+rswModel->fileName);
						}
						auto rswEffect = node->getComponent<RswEffect>();
						if (rswEffect)
						{
							for(auto effectFile : effectFiles[rswEffect->id])
								if (std::find(newTagList[mapTag].begin(), newTagList[mapTag].end(), effectFile) == newTagList[mapTag].end())
									newTagList[mapTag].push_back(effectFile);
						}
						auto rswSound = node->getComponent<RswSound>();
						if (rswSound)
						{
							if (std::find(newTagList[mapTag].begin(), newTagList[mapTag].end(), "data\\wav\\" + rswSound->fileName) == newTagList[mapTag].end())
								newTagList[mapTag].push_back("data\\wav\\" + rswSound->fileName);
						}
					});
				delete node;
			}

			windowData.progressWindowProgres = 1;
			windowData.progressWindowVisible = false;
			});
		progressThread.detach();
	}

	ImGui::SameLine();
	ImGui::Checkbox("##vertical", &verticalLayout);

	std::function<void(util::FileIO::Node*, std::string parent)> buildTreeNodes;
	buildTreeNodes = [&](util::FileIO::Node* node, std::string parent)
	{
		for (auto f : node->directories)
		{
			int flags = ImGuiTreeNodeFlags_OpenOnDoubleClick;
			if (f.second->directories.size() == 0)
				flags |= ImGuiTreeNodeFlags_Bullet;
			if (f.second == windowData.objectWindowSelectedTreeNode)
				flags |= ImGuiTreeNodeFlags_Selected;

			bool open = ImGui::TreeNodeEx((f.second->name + "##" + parent + f.second->name).c_str(), flags);

			if (ImGui::IsItemClicked())
				windowData.objectWindowSelectedTreeNode = f.second;

			if (open) {
				buildTreeNodes(f.second, parent + f.second->name + "\\");
				ImGui::TreePop();
			}
		}
	};

	
	ImGui::BeginChild("left pane", ImVec2(verticalLayout ? 0.0f : 250.0f, verticalLayout ? 200.0f : 0.0f), true);

	auto startTree = [&](const char* nodeName, const std::string& path)
	{
		auto root = util::FileIO::directoryNode(path);
		if (!root)
			return;
		if (ImGui::TreeNodeEx(nodeName, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnDoubleClick))
		{
			if (ImGui::IsItemClicked())
				windowData.objectWindowSelectedTreeNode = root;
			buildTreeNodes(root, std::string(nodeName));
			ImGui::TreePop();
		}
		else if (ImGui::IsItemClicked())
			windowData.objectWindowSelectedTreeNode = root;
	};
	startTree("Models", "data\\model\\");
	startTree("Sounds", "data\\wav\\");
	startTree("Lights", "data\\lights\\");
	startTree("Effects", "data\\effects\\");
	startTree("Prefabs", "data\\prefabs\\");
	ImGui::EndChild();
	if(!verticalLayout)
		ImGui::SameLine();

	ImGuiStyle& style = ImGui::GetStyle();
	float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

	auto buildBox = [&](const std::string& file, bool fullPath) {
		std::string path = util::utf8_to_iso_8859_1(file);
		if (!fullPath)
		{
			auto n = windowData.objectWindowSelectedTreeNode;
			while (n)
			{
				if (n->name != "")
					path = util::utf8_to_iso_8859_1(n->name) + "\\" + path;
				n = n->parent;
			}
		}

		if (path == windowData.objectWindowScrollToModel)
		{
			windowData.objectWindowScrollToModel = "";
			ImGui::SetScrollHereY();
		}
		if (ImGui::BeginChild(file.c_str(), ImVec2(config.thumbnailSize.x, config.thumbnailSize.y + 50), true, ImGuiWindowFlags_NoScrollbar))
		{
			auto g = ImGui::GetCurrentContext();
			if (g->CurrentWindow->ParentWindow->ClipRect.Overlaps(g->CurrentWindow->ClipRect))
			{
				ImTextureID texture = 0;
				gl::Texture* textureOrigin = nullptr;
				if (path.substr(path.size() - 4) == ".rsm" ||
					path.substr(path.size() - 5) == ".rsm2")
				{
					auto it = objectWindowObjects.find(path);
					if (it == objectWindowObjects.end())
					{
						objectWindowObjects[path] = new ObjectWindowObject(path, this);
						it = objectWindowObjects.find(path);
						it->second->draw();
					}
					texture = (ImTextureID)(long long)it->second->fbo->texid[0];
				}
				else if (path.substr(path.size() - 4) == ".wav")
					texture = (ImTextureID)(long long)soundTexture->id();
				else if (path.substr(path.size() - 5) == ".json")
				{
					if (path.find("data\\lights") != std::string::npos)
						texture = (ImTextureID)(long long)lightTexture->id();
					if (path.find("data\\prefabs") != std::string::npos)
						texture = (ImTextureID)(long long)prefabTexture->id();
					if (path.find("data\\effects") != std::string::npos)
					{
						static std::map<std::string, int> effectIds;
						if (effectIds.find(path) == effectIds.end())
						{
							auto json = util::FileIO::getJson(path);
							effectIds[path] = json["id"].get<int>();
						}
						int effectId = effectIds[path];
						if (RswEffect::previews.find(effectId) == RswEffect::previews.end())
							RswEffect::previews[effectId] = util::ResourceManager<gl::Texture>::load("data\\texture\\effect\\" + std::to_string(effectId) + ".gif.png");
						if (RswEffect::previewAnim.find(effectId) == RswEffect::previewAnim.end())
							RswEffect::previewAnim[effectId] = util::ResourceManager<gl::Texture>::load("data\\texture\\effect\\" + std::to_string(effectId) + ".gif");

						if (RswEffect::previewAnim[effectId]->loaded)
							texture = (ImTextureID)(long long)RswEffect::previewAnim[effectId]->getAnimatedTextureId();
						else if (RswEffect::previews[effectId]->loaded)
						{
							texture = (ImTextureID)(long long)RswEffect::previews[effectId]->id();
							textureOrigin = RswEffect::previewAnim[effectId];
						}
						else
						{
							if (!RswEffect::previewAnim[effectId]->tryLoaded)
								RswEffect::previewAnim[effectId]->reload();
							if (RswEffect::previewAnim[effectId]->loaded)
								texture = (ImTextureID)(long long)RswEffect::previewAnim[effectId]->getAnimatedTextureId();
							texture = (ImTextureID)(long long)effectTexture->id();
						}
					}
				}
				if (ImGui::ImageButtonEx(ImGui::GetID(path.c_str()), texture, config.thumbnailSize, ImVec2(0, 0), ImVec2(1, 1), ImVec2(0, 0), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1)))
				{
					// A new item is selected, clear the clipboard
					for (auto n : newNodes)
						delete n.first;
					newNodes.clear();

					if (activeMapView && newNodes.size() == 0)
					{
						if (file.substr(file.size() - 4) == ".rsm" ||
							file.substr(file.size() - 5) == ".rsm2")
						{
							std::string name = path.substr(0, path.rfind(".")); //remove .rsm
							name = name.substr(11); // remove data\model\ 
							Node* newNode = new Node(util::iso_8859_1_to_utf8(name));
							newNode->addComponent(util::ResourceManager<Rsm>::load(path));
							newNode->addComponent(new RsmRenderer());
							newNode->addComponent(new RswObject());
							newNode->addComponent(new RswModel(util::iso_8859_1_to_utf8(path.substr(11)))); //remove data\model\ 
							newNode->addComponent(new RswModelCollider());
							newNodes.push_back(std::pair<Node*, glm::vec3>(newNode, glm::vec3(0, 0, 0)));
							newNodesCenter = glm::vec3(0, 0, 0);
							newNodePlacement = BrowEdit::Ground;
						}
						else if (file.substr(file.size() - 5) == ".json" &&
							path.find("data\\lights") != std::string::npos)
						{
							auto l = new RswLight();
							try {
								from_json(util::FileIO::getJson(path), *l);
							}
							catch (...) { std::cerr << "Error loading json" << std::endl; }
							Node* newNode = new Node(file);
							newNode->addComponent(new RswObject());
							newNode->addComponent(l);
							newNode->addComponent(new BillboardRenderer("data\\light.png", "data\\light_selected.png"));
							newNode->addComponent(new CubeCollider(5));
							newNodes.push_back(std::pair<Node*, glm::vec3>(newNode, glm::vec3(0, 0, 0)));
							newNodesCenter = glm::vec3(0, 0, 0);
							newNodePlacement = BrowEdit::Ground;
						}
						else if (file.substr(file.size() - 5) == ".json" &&
							path.find("data\\effects") != std::string::npos)
						{
							auto e = new RswEffect();
							try {
								from_json(util::FileIO::getJson(path), *e);
							}
							catch (...) { std::cerr << "Error loading json" << std::endl; }
							Node* newNode = new Node(file);
							newNode->addComponent(new RswObject());
							newNode->addComponent(e);
							newNode->addComponent(new BillboardRenderer("data\\effect.png", "data\\effect_selected.png"));
							newNode->addComponent(new CubeCollider(5));
							
							// Tokei: Effect 974 needs to have an LubEffect component attached
							// This could be made in the json directly, but it will break if the lub effect
							// properties are changed (which is somewhat often), so it's done in the source instead.
							if (e->id == 974) {
								auto lubEffect = new LubEffect();
								newNode->addComponent(lubEffect);
								// Add dummy data to show something
								lubEffect->texture = "smoke2.bmp";
								lubEffect->gravity = glm::vec3(0, -5, 0);
								lubEffect->color = glm::vec4(1);
								lubEffect->rate = glm::vec2(5, 15);
								lubEffect->size = glm::vec2(3, 8);
								lubEffect->life = glm::vec2(1, 5);
								lubEffect->scale = glm::vec2(1, 1);
								lubEffect->speed = 0.5f;
								lubEffect->srcmode = 10;
								lubEffect->destmode = 2;
								lubEffect->maxcount = 30;
								lubEffect->zenable = 1;
								lubEffect->eternity = 0;

								newNode->addComponent(new LubRenderer());
							}

							newNodes.push_back(std::pair<Node*, glm::vec3>(newNode, glm::vec3(0, 0, 0)));
							newNodesCenter = glm::vec3(0, 0, 0);
							newNodePlacement = BrowEdit::Ground;
						}
						else if (file.substr(file.size() - 5) == ".json" &&
							path.find("data\\prefabs") != std::string::npos)
						{
							json clipboard = util::FileIO::getJson(path);
							if (clipboard.size() > 0)
							{
								for (auto n : newNodes) //TODO: should I do this?
									delete n.first;
								newNodes.clear();

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
										if (c["type"] == "lubeffect")
										{
											auto lubEffect = new LubEffect();
											from_json(c, *lubEffect);
											newNode->addComponent(lubEffect);
										}
										if (c["type"] == "streffect")
										{
											auto strEffect = new StrEffect();
											from_json(c, *strEffect);
											newNode->addComponent(strEffect);
										}
										if (c["type"] == "lubwindeffect")
										{
											auto lubEffect = new LubWindEffect();
											from_json(c, *lubEffect);
											newNode->addComponent(lubEffect);
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
									newNodes.push_back(std::pair<Node*, glm::vec3>(newNode, newNode->getComponent<RswObject>()->position));
								}
								glm::vec3 center(0, 0, 0);
								for (auto& n : newNodes)
									center += n.second;
								center /= newNodes.size();

								newNodesCenter = center;
								for (auto& n : newNodes)
									n.second = n.second - center;
								newNodePlacement = BrowEdit::Ground;
							}
						}
						else if (file.substr(file.size() - 4) == ".wav")
						{
							auto s = new RswSound(util::iso_8859_1_to_utf8(path.substr(9))); //remove data\wav\ 
							Node* newNode = new Node(file);
							newNode->addComponent(new RswObject());
							newNode->addComponent(s);
							newNode->addComponent(new BillboardRenderer("data\\sound.png", "data\\sound_selected.png"));
							newNode->addComponent(new CubeCollider(5));
							newNodes.push_back(std::pair<Node*, glm::vec3>(newNode, glm::vec3(0, 0, 0)));
							newNodesCenter = glm::vec3(0, 0, 0);
							newNodePlacement = BrowEdit::Ground;
						}
					}
					std::cout << "Click on " << file << std::endl;
				}
				if (ImGui::BeginPopupContextWindow("Object Tags"))
				{
					if (file.substr(file.size() - 4) == ".wav")
					{
						if (ImGui::Button("Play"))
						{
							auto is = util::FileIO::open(path);
							is->seekg(0, std::ios_base::end);
							std::size_t len = is->tellg();
							char* buffer = new char[len];
							is->seekg(0, std::ios_base::beg);
							is->read(buffer, len);
							delete is;

							PlaySound(buffer, NULL, SND_MEMORY | SND_ASYNC);
							delete[] buffer;
						}
					}
					if (ImGui::CollapsingHeader("Actions", ImGuiTreeNodeFlags_DefaultOpen))
					{
						if (ImGui::Button("Add to map"))
							std::cout << "Just click the thing" << std::endl;
						ImGui::SameLine();
						if (file.size() > 5 && (file.substr(file.size() - 4) == ".rsm" || file.substr(file.size() - 5) == ".rsm2") && ImGui::Button("Replace selected models") && activeMapView)
						{
							auto ga = new GroupAction();
							for (auto n : activeMapView->map->selectedNodes)
								ga->addAction(new ModelChangeAction(n, path)); //path is in ISO
							activeMapView->map->doAction(ga, this);
						}
						ImGui::SameLine();
						if (ImGui::Button("Select this model") && activeMapView)
						{
							bool first = true;
							auto ga = new GroupAction();
							activeMapView->map->rootNode->traverse([&](Node* n)
								{
									auto rswModel = n->getComponent<RswModel>();
									if (rswModel && rswModel->fileName == util::iso_8859_1_to_utf8(path))
									{
										auto sa = new SelectAction(activeMapView->map, n, !first, false);
										ga->addAction(sa);
										first = false;
									}
								});
							activeMapView->map->doAction(ga, this);
						}
					}
					if (ImGui::CollapsingHeader("Tags", ImGuiTreeNodeFlags_DefaultOpen))
					{
						static std::string newTag;
						ImGui::SetNextItemWidth(100);
						ImGui::InputText("Add Tag", &newTag);
						ImGui::SameLine();
						if (ImGui::Button("Add"))
						{
							tagList[newTag].push_back(util::iso_8859_1_to_utf8(path)); //remove data\model\ prefix
							tagListReverse[path].push_back(newTag);
							saveTagList();
						}
						ImGui::Separator();
						ImGui::Text("Current tags on this model");
						for (auto tag : tagListReverse[path])
						{
							ImGui::BulletText(tag.c_str());
							ImGui::SameLine();
							if (ImGui::Button("Remove"))
							{
								tagList[tag].erase(std::remove_if(tagList[tag].begin(), tagList[tag].end(), [&](const std::string& m) { return m == util::iso_8859_1_to_utf8(path); }), tagList[tag].end());
								tagListReverse[path].erase(std::remove_if(tagListReverse[path].begin(), tagListReverse[path].end(), [&](const std::string& t) { return t == tag; }), tagListReverse[path].end());
								saveTagList();
							}

						}
					}

					ImGui::EndPopup();
				}
				else if (ImGui::IsItemHovered())
				{
					if (textureOrigin && !textureOrigin->tryLoaded)
						textureOrigin->reload();
					static std::string lastPopup;
					static std::string desc;
					if (lastPopup != path)
					{
						desc = "";
						lastPopup = path;
						if (path.substr(path.size() - 5) == ".json" &&
							path.find("data\\effects") != std::string::npos)
						{
							auto data = util::FileIO::getJson(path);
							if (data.find("desc") != data.end() && data["desc"].is_string())
								desc = data["desc"].get<std::string>() + "\n\n";
						}
					}
					ImGui::SetTooltip((desc + util::combine(tagListReverse[path], "\n")).c_str());
					auto it = objectWindowObjects.find(path);
					if (it != objectWindowObjects.end())
					{
						it->second->rotation = (float)(glfwGetTime() * 90); //TODO: make this increment based on deltatime
						it->second->draw();
					}
				}
				ImGui::Text(file.c_str());
			}
		}
		ImGui::EndChild();

		float last_button_x2 = ImGui::GetItemRectMax().x;
		float next_button_x2 = last_button_x2 + style.ItemSpacing.x + config.thumbnailSize.x; // Expected position if next button was on same line
		if (next_button_x2 < window_visible_x2)
			ImGui::SameLine();
	};
	if (ImGui::BeginChild("right pane", ImVec2(verticalLayout ? 0 : ImGui::GetContentRegionAvail().x, 0), true))
	{
		if (windowData.objectWindowSelectedTreeNode != nullptr && filter == "")
		{
			window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
			for (auto file : windowData.objectWindowSelectedTreeNode->files)
			{
				if (file.find(".rsm") == std::string::npos && 
					file.find(".rsm2") == std::string::npos &&
					file.find(".wav") == std::string::npos &&
					file.find(".json") == std::string::npos)
					continue;
				buildBox(file, false);
			}
		}
		else if (filter != "")
		{
			static std::string currentFilterText = "";
			static std::vector<std::string> filteredFiles;
			std::string filter8859 = util::utf8_to_iso_8859_1(filter);
			util::tolowerInPlace(filter8859);
			if (currentFilterText != filter8859)
			{
				currentFilterText = filter8859;
				std::vector<std::string> filterTags = util::split(filter8859, " ");
				filteredFiles.clear();

				for (auto t : tagListReverse)
				{
					bool match = true;
					for (auto tag : filterTags)
					{
						bool tagOk = false;
						if (t.first.find(tag) != std::string::npos)
							tagOk = true;
						for (auto fileTag : t.second)
							if (fileTag.find(tag) != std::string::npos)
							{
								tagOk = true;
								break;
							}
						if (!tagOk)
						{
							match = false;
							break;
						}
					}
					if (match)
					{
						if (t.first.find(".bmp") == std::string::npos &&
							t.first.find(".tga") == std::string::npos &&
							t.first.find(".png") == std::string::npos &&
							t.first.find(".gif") == std::string::npos)
							filteredFiles.push_back(util::iso_8859_1_to_utf8(t.first));
					}
				}
			}
			for (const auto& file : filteredFiles)
			{
				buildBox(file, true);
			}

		}
	}
	ImGui::EndChild();
	ImGui::End();
}

void BrowEdit::showStrPickerWindow()
{
	if (!ImGui::Begin("Str Picker", &windowData.objectWindowVisible))
	{
		ImGui::End();
		return;
	}
	static bool verticalLayout = ImGui::GetContentRegionAvail().x < 300;

	static std::string filter;
	ImGui::SetNextItemWidth(ImGui::GetWindowSize().x * 0.65f - 50);
	ImGui::InputText("Filter", &filter);
	ImGui::SameLine();
	ImGui::Checkbox("##verticalStrPicker", &verticalLayout);

	std::function<void(util::FileIO::Node*, std::string parent)> buildTreeNodes;
	buildTreeNodes = [&](util::FileIO::Node* node, std::string parent)
		{
			for (auto f : node->directories)
			{
				int flags = ImGuiTreeNodeFlags_OpenOnDoubleClick;
				if (f.second->directories.size() == 0)
					flags |= ImGuiTreeNodeFlags_Bullet;
				if (f.second == windowData.objectWindowSelectedTreeNode)
					flags |= ImGuiTreeNodeFlags_Selected;

				bool open = ImGui::TreeNodeEx((f.second->name + "##" + parent + f.second->name).c_str(), flags);
				
				if (ImGui::IsItemClicked())
					windowData.objectWindowSelectedTreeNode = f.second;
				
				if (open) {
					buildTreeNodes(f.second, parent + f.second->name + "\\");
					ImGui::TreePop();
				}
			}
		};


	ImGui::BeginChild("left pane", ImVec2(verticalLayout ? 0.0f : 250.0f, verticalLayout ? 200.0f : 0.0f), true);

	auto startTree = [&](const char* nodeName, const std::string& path)
		{
			auto root = util::FileIO::directoryNode(path);
			if (!root)
				return;
			if (ImGui::TreeNodeEx(nodeName, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnDoubleClick))
			{
				if (ImGui::IsItemClicked())
					windowData.objectWindowSelectedTreeNode = root;
				buildTreeNodes(root, std::string(nodeName) + "\\");
				ImGui::TreePop();
			}
			else if (ImGui::IsItemClicked())
				windowData.objectWindowSelectedTreeNode = root;
		};
	startTree("Str", "data\\texture\\effect\\");
	ImGui::EndChild();
	if (!verticalLayout)
		ImGui::SameLine();

	ImGuiStyle& style = ImGui::GetStyle();
	float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

	auto buildBox = [&](const std::string& file, bool fullPath) {
		std::string path = util::utf8_to_iso_8859_1(file);
		if (!fullPath)
		{
			auto n = windowData.objectWindowSelectedTreeNode;
			while (n)
			{
				if (n->name != "")
					path = util::utf8_to_iso_8859_1(n->name) + "\\" + path;
				n = n->parent;
			}
		}

		if (path == windowData.objectWindowScrollToModel)
		{
			windowData.objectWindowScrollToModel = "";
			ImGui::SetScrollHereY();
		}
		if (ImGui::BeginChild(file.c_str(), ImVec2(config.thumbnailSize.x, config.thumbnailSize.y + 50), true, ImGuiWindowFlags_NoScrollbar))
		{
			auto g = ImGui::GetCurrentContext();
			if (g->CurrentWindow->ParentWindow->ClipRect.Overlaps(g->CurrentWindow->ClipRect))
			{
				ImTextureID texture = 0;
				gl::Texture* textureOrigin = nullptr;
				if (path.substr(path.size() - 4) == ".str")
				{
					auto it = objectWindowObjects.find(path);
					if (it == objectWindowObjects.end())
					{
						objectWindowObjects[path] = new ObjectWindowObject(path, this);
						it = objectWindowObjects.find(path);
						it->second->draw();
					}
					texture = (ImTextureID)(long long)it->second->fbo->texid[0];
				}
				if (ImGui::ImageButtonEx(ImGui::GetID(path.c_str()), texture, config.thumbnailSize, ImVec2(0, 0), ImVec2(1, 1), ImVec2(0, 0), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1)))
				{
					// A new item is selected, clear the clipboard
					for (auto n : newNodes)
						delete n.first;
					newNodes.clear();

					if (activeMapView && newNodes.size() == 0)
					{
						if (file.substr(file.size() - 4) == ".str")
						{
							std::string name = path.substr(0, path.rfind(".")); //remove .str
							name = name.substr(20); // remove data\texture\effect\ 
							Node* newNode = new Node(util::iso_8859_1_to_utf8(name));
							auto strEffect = new StrEffect();
							strEffect->str = util::iso_8859_1_to_utf8(name + ".str");
							auto e = new RswEffect();
							e->id = RswEffect::EffectType::Ez2Str;
							newNode->addComponent(strEffect);
							newNode->addComponent(new StrRenderer());
							newNode->addComponent(new RswObject());
							newNode->addComponent(e);
							newNode->addComponent(new BillboardRenderer("data\\effect.png", "data\\effect_selected.png"));
							newNode->addComponent(new CubeCollider(5));
							newNodes.push_back(std::pair<Node*, glm::vec3>(newNode, glm::vec3(0, 0, 0)));
							newNodesCenter = glm::vec3(0, 0, 0);
							newNodePlacement = BrowEdit::Ground;
						}
					}
					std::cout << "Click on " << file << std::endl;
				}
				if (ImGui::BeginPopupContextWindow("Object Tags"))
				{
					if (ImGui::CollapsingHeader("Actions", ImGuiTreeNodeFlags_DefaultOpen))
					{
						if (ImGui::Button("Add to map"))
							std::cout << "Just click the thing" << std::endl;
						ImGui::SameLine();
						if (ImGui::Button("Select this STR") && activeMapView)
						{
							bool first = true;
							auto ga = new GroupAction();
							activeMapView->map->rootNode->traverse([&](Node* n)
								{
									auto str = n->getComponent<Str>();
									if (str && str->fileName == util::iso_8859_1_to_utf8(path))
									{
										auto sa = new SelectAction(activeMapView->map, n, !first, false);
										ga->addAction(sa);
										first = false;
									}
								});
							activeMapView->map->doAction(ga, this);
						}
					}
					// Do we need tags for STRs...?
					if (ImGui::CollapsingHeader("Tags", ImGuiTreeNodeFlags_DefaultOpen))
					{
						static std::string newTag;
						ImGui::SetNextItemWidth(100);
						ImGui::InputText("Add Tag", &newTag);
						ImGui::SameLine();
						if (ImGui::Button("Add"))
						{
							tagList[newTag].push_back(util::iso_8859_1_to_utf8(path)); //remove data\model\ prefix
							tagListReverse[path].push_back(newTag);
							saveTagList();
						}
						ImGui::Separator();
						ImGui::Text("Current tags on this model");
						for (auto tag : tagListReverse[path])
						{
							ImGui::BulletText(tag.c_str());
							ImGui::SameLine();
							if (ImGui::Button("Remove"))
							{
								tagList[tag].erase(std::remove_if(tagList[tag].begin(), tagList[tag].end(), [&](const std::string& m) { return m == util::iso_8859_1_to_utf8(path); }), tagList[tag].end());
								tagListReverse[path].erase(std::remove_if(tagListReverse[path].begin(), tagListReverse[path].end(), [&](const std::string& t) { return t == tag; }), tagListReverse[path].end());
								saveTagList();
							}

						}
					}

					ImGui::EndPopup();
				}
				else if (ImGui::IsItemHovered())
				{
					if (textureOrigin && !textureOrigin->tryLoaded)
						textureOrigin->reload();
					auto it = objectWindowObjects.find(path);
					if (it != objectWindowObjects.end())
					{
						it->second->draw();
					}
				}
				ImGui::Text(file.c_str());
			}
		}
		ImGui::EndChild();

		float last_button_x2 = ImGui::GetItemRectMax().x;
		float next_button_x2 = last_button_x2 + style.ItemSpacing.x + config.thumbnailSize.x; // Expected position if next button was on same line
		if (next_button_x2 < window_visible_x2)
			ImGui::SameLine();
		};
	if (ImGui::BeginChild("right pane", ImVec2(verticalLayout ? 0 : ImGui::GetContentRegionAvail().x, 0), true))
	{
		if (windowData.objectWindowSelectedTreeNode != nullptr && filter == "")
		{
			window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
			for (auto file : windowData.objectWindowSelectedTreeNode->files)
			{
				if (file.find(".str") == std::string::npos)
					continue;
				buildBox(file, false);
			}
		}
		else if (filter != "")
		{
			static std::string currentFilterText = "";
			static std::vector<std::string> filteredFiles;
			std::string filter8859 = util::utf8_to_iso_8859_1(filter);
			util::tolowerInPlace(filter8859);
			if (currentFilterText != filter8859)
			{
				currentFilterText = filter8859;
				std::vector<std::string> filterTags = util::split(filter8859, " ");
				filteredFiles.clear();

				for (auto t : tagListReverse)
				{
					bool match = true;
					for (auto tag : filterTags)
					{
						bool tagOk = false;
						if (t.first.find(tag) != std::string::npos)
							tagOk = true;
						for (auto fileTag : t.second)
							if (fileTag.find(tag) != std::string::npos)
							{
								tagOk = true;
								break;
							}
						if (!tagOk)
						{
							match = false;
							break;
						}
					}
					if (match)
					{
						if (t.first.find(".bmp") == std::string::npos &&
							t.first.find(".tga") == std::string::npos &&
							t.first.find(".png") == std::string::npos &&
							t.first.find(".gif") == std::string::npos)
							filteredFiles.push_back(util::iso_8859_1_to_utf8(t.first));
					}
				}
			}
			for (const auto& file : filteredFiles)
			{
				buildBox(file, true);
			}

		}
	}
	ImGui::EndChild();
	ImGui::End();
}


void BrowEdit::showLubEffectPickerWindow()
{
	if (!ImGui::Begin("Lub Picker", &windowData.objectWindowVisible))
	{
		ImGui::End();
		return;
	}
	static bool verticalLayout = ImGui::GetContentRegionAvail().x < 300;

	static std::string filter;
	ImGui::SetNextItemWidth(ImGui::GetWindowSize().x * 0.65f - 50);
	ImGui::InputText("Filter", &filter);
	ImGui::SameLine();
	ImGui::Checkbox("##verticalLubPicker", &verticalLayout);

	std::function<void(util::FileIO::Node*, std::string parent)> buildTreeNodes;
	buildTreeNodes = [&](util::FileIO::Node* node, std::string parent)
		{
			for (auto f : node->directories)
			{
				int flags = ImGuiTreeNodeFlags_OpenOnDoubleClick;
				if (f.second->directories.size() == 0)
					flags |= ImGuiTreeNodeFlags_Bullet;
				if (f.second == windowData.objectWindowSelectedTreeNode)
					flags |= ImGuiTreeNodeFlags_Selected;

				bool open = ImGui::TreeNodeEx((f.second->name + "##" + parent + f.second->name).c_str(), flags);

				if (ImGui::IsItemClicked())
					windowData.objectWindowSelectedTreeNode = f.second;

				if (open) {
					buildTreeNodes(f.second, parent + f.second->name + "\\");
					ImGui::TreePop();
				}
			}
		};

	ImGuiStyle& style = ImGui::GetStyle();
	float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

	static bool lubLoaded = false;
	static sol::state lua;
	static std::map<int, sol::table> emitters;

	if (!lubLoaded) {
		lua.open_libraries(sol::lib::base);

		auto lub = util::FileIO::open("data\\lubEffects.lua");
		std::string data = util::loadLubFileToString(lub);
		delete lub;

		try {
			auto load_result = lua.load(data);
			auto run_result = load_result();
			sol::object obj = lua["emitterInfo"];
			if (obj.is<sol::table>()) {
				for (auto const& [key, value] : obj.as<sol::table>()) {
					emitters[key.as<int>()] = value.as<sol::table>();
				}
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "Error loading lub effect data: " << e.what() << std::endl;
			std::cout << data << std::endl;
		}

		lubLoaded = true;
	}

	auto buildBox = [&](int idx, bool fullPath) {
		if (ImGui::BeginChild(("lubeffect_" + std::to_string(idx)).c_str(), ImVec2(config.thumbnailSize.x, config.thumbnailSize.y + 50), true, ImGuiWindowFlags_NoScrollbar))
		{
			std::string path = "lubeffect_" + std::to_string(idx) + ".lub";
			auto g = ImGui::GetCurrentContext();
			if (g->CurrentWindow->ParentWindow->ClipRect.Overlaps(g->CurrentWindow->ClipRect))
			{
				ImTextureID texture = 0;
				gl::Texture* textureOrigin = nullptr;
				auto it = objectWindowObjects.find(path);
				if (it == objectWindowObjects.end())
				{
					objectWindowObjects[path] = new ObjectWindowObject(emitters[idx], this);
					it = objectWindowObjects.find(path);
					it->second->draw();
				}
				texture = (ImTextureID)(long long)it->second->fbo->texid[0];

				if (ImGui::ImageButtonEx(ImGui::GetID(path.c_str()), texture, config.thumbnailSize, ImVec2(0, 0), ImVec2(1, 1), ImVec2(0, 0), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1)))
				{
					// A new item is selected, clear the clipboard
					for (auto n : newNodes)
						delete n.first;
					newNodes.clear();

					if (activeMapView && newNodes.size() == 0)
					{
						Node* newNode = new Node(util::iso_8859_1_to_utf8(path));
						auto lubEffect = new LubEffect();
						lubEffect->load(emitters[idx]);
						auto e = new RswEffect();
						e->id = RswEffect::EffectType::Emitter;
						newNode->addComponent(lubEffect);
						newNode->addComponent(new LubRenderer());
						newNode->addComponent(new RswObject());
						newNode->addComponent(e);
						newNode->addComponent(new BillboardRenderer("data\\effect.png", "data\\effect_selected.png"));
						newNode->addComponent(new CubeCollider(5));
						newNodes.push_back(std::pair<Node*, glm::vec3>(newNode, glm::vec3(0, 0, 0)));
						newNodesCenter = glm::vec3(0, 0, 0);
						newNodePlacement = BrowEdit::Ground;
					}
				}
				//else if (ImGui::IsItemHovered())
				{
					if (textureOrigin && !textureOrigin->tryLoaded)
						textureOrigin->reload();
					auto it = objectWindowObjects.find(path);
					if (it != objectWindowObjects.end())
					{
						it->second->draw();
					}
				}
				ImGui::Text(path.c_str());
			}
		}
		ImGui::EndChild();

		float last_button_x2 = ImGui::GetItemRectMax().x;
		float next_button_x2 = last_button_x2 + style.ItemSpacing.x + config.thumbnailSize.x; // Expected position if next button was on same line
		if (next_button_x2 < window_visible_x2)
			ImGui::SameLine();
		};
	if (ImGui::BeginChild("right pane", ImVec2(verticalLayout ? 0 : ImGui::GetContentRegionAvail().x, 0), true))
	{
		window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
		for (int i = 0; i < emitters.size(); i++) {
			buildBox(i, false);
		}
	}
	ImGui::EndChild();
	ImGui::End();
}