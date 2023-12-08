#include <windows.h>
#include <browedit/BrowEdit.h>
#include <browedit/Node.h>
#include <browedit/MapView.h>
#include <browedit/Map.h>
#include <browedit/components/RsmRenderer.h>
#include <browedit/components/BillboardRenderer.h>
#include <browedit/components/Rsm.h>
#include <browedit/components/Rsw.h>
#include <browedit/gl/FBO.h>
#include <browedit/gl/Texture.h>
#include <browedit/util/Util.h>
#include <browedit/util/FileIO.h>
#include <browedit/util/ResourceManager.h>
#include <browedit/actions/GroupAction.h>
#include <browedit/actions/SelectAction.h>
#include <browedit/actions/ModelChangeAction.h>

#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <thread>
#include <iostream>

//TODO: this file is a mess

class ObjectWindowObject
{
public:
	Node* node;
	gl::FBO* fbo;
	NodeRenderContext nodeRenderContext;
	float rotation = 0;
	BrowEdit* browEdit;

	ObjectWindowObject(const std::string& fileName, BrowEdit* browEdit) : browEdit(browEdit)
	{
		fbo = new gl::FBO((int)browEdit->config.thumbnailSize.x, (int)browEdit->config.thumbnailSize.y, true); //TODO: resolution?
		node = new Node();
		node->addComponent(util::ResourceManager<Rsm>::load(fileName));
		node->addComponent(new RsmRenderer());
	}

	void draw()
	{
		auto rsm = node->getComponent<Rsm>();
		if (!rsm->loaded)
			return;
		fbo->bind();
		glViewport(0, 0, fbo->getWidth(), fbo->getHeight());
		glClearColor(browEdit->config.backgroundColor.r, browEdit->config.backgroundColor.g, browEdit->config.backgroundColor.b, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glDisable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

		float distance = 1.5f * glm::max(glm::abs(rsm->realbbmax.y), glm::max(glm::abs(rsm->realbbmax.z), glm::abs(rsm->realbbmax.x)));

		float ratio = fbo->getWidth() / (float)fbo->getHeight();
		nodeRenderContext.projectionMatrix = glm::perspective(glm::radians(45.0f), ratio, 0.1f, 5000.0f);
		nodeRenderContext.viewMatrix = glm::lookAt(glm::vec3(-distance * glm::sin(glm::radians(rotation)), -distance, -distance * glm::cos(glm::radians(rotation))), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		RsmRenderer::RsmRenderContext::getInstance()->viewLighting = false;
		glm::mat4 mat = glm::mat4(1.0f);
		if (rsm->version >= 0x202) {
			mat = glm::scale(mat, glm::vec3(1, -1, 1));
		}
		mat = glm::translate(mat, glm::vec3(-rsm->realbbrange.x, rsm->realbbrange.y, -rsm->realbbrange.z));
		node->getComponent<RsmRenderer>()->matrixCache = mat;
		node->getComponent<RsmRenderer>()->matrixCached = true;
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

	std::function<void(util::FileIO::Node*)> buildTreeNodes;
	buildTreeNodes = [&](util::FileIO::Node* node)
	{
		for (auto f : node->directories)
		{
			int flags = ImGuiTreeNodeFlags_OpenOnDoubleClick;
			if (f.second->directories.size() == 0)
				flags |= ImGuiTreeNodeFlags_Bullet;
			if (f.second == windowData.objectWindowSelectedTreeNode)
				flags |= ImGuiTreeNodeFlags_Selected;

			if (ImGui::TreeNodeEx(f.second->name.c_str(), flags))
			{
				buildTreeNodes(f.second);
				ImGui::TreePop();
			}
			if (ImGui::IsItemClicked())
				windowData.objectWindowSelectedTreeNode = f.second;
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
			buildTreeNodes(root);
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
							newNodeHeight = false;
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
							newNodeHeight = false;
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
							newNodes.push_back(std::pair<Node*, glm::vec3>(newNode, glm::vec3(0, 0, 0)));
							newNodesCenter = glm::vec3(0, 0, 0);
							newNodeHeight = false;
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
								newNodeHeight = false;
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
							newNodeHeight = false;
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
			util::tolowerInPlace(filter);
			if (currentFilterText != filter)
			{
				currentFilterText = filter;
				std::vector<std::string> filterTags = util::split(filter, " ");
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

