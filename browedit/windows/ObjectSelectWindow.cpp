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

		float distance = 1.1f * glm::max(glm::abs(rsm->realbbmax.y), glm::max(glm::abs(rsm->realbbmax.z), glm::abs(rsm->realbbmax.x)));

		float ratio = fbo->getWidth() / (float)fbo->getHeight();
		nodeRenderContext.projectionMatrix = glm::perspective(glm::radians(45.0f), ratio, 0.1f, 5000.0f);
		nodeRenderContext.viewMatrix = glm::lookAt(glm::vec3(0.0f, -distance, -distance), glm::vec3(0.0f, rsm->bbrange.y, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		RsmRenderer::RsmRenderContext::getInstance()->viewLighting = false;
		node->getComponent<RsmRenderer>()->matrixCache = glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0, 1, 0));
		NodeRenderer::render(node, nodeRenderContext);
		fbo->unbind();
	}
};
std::map<std::string, ObjectWindowObject*> objectWindowObjects;

void BrowEdit::showObjectWindow()
{
	ImGui::Begin("Object Picker", &windowData.objectWindowVisible);
	static std::string filter;
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
					tagListReverse[util::utf8_to_iso_8859_1(t)].push_back(util::utf8_to_iso_8859_1(tag.first));
			saveTagList();
			std::cout << "TagList merged and saved to file" << std::endl;
		};

		progressThread = std::thread([&]() {
			std::vector<std::string> maps = util::FileIO::listFiles("data");
			maps.erase(std::remove_if(maps.begin(), maps.end(), [](const std::string& map) { return map.substr(map.size() - 4, 4) != ".rsw"; }), maps.end());
			windowData.progressWindowProgres = 0;
			for (const auto& fileName : maps)
			{
				windowData.progressWindowProgres += (1.0f / maps.size());
				Node* node = new Node();
				Rsw* rsw = new Rsw();
				node->addComponent(rsw);
				rsw->load(fileName, false, false);

				std::string mapTag = fileName;
				if (mapTag.substr(0, 5) == "data\\")
					mapTag = mapTag.substr(5);
				if (mapTag.substr(mapTag.size() - 4, 4) == ".rsw")
					mapTag = mapTag.substr(0, mapTag.size() - 4);

				mapTag = "map:" + util::iso_8859_1_to_utf8(mapTag);

				node->traverse([&](Node* node)
					{
						auto rswModel = node->getComponent<RswModel>();
						if (rswModel)
						{
							if (std::find(newTagList[mapTag].begin(), newTagList[mapTag].end(), rswModel->fileName) == newTagList[mapTag].end())
								newTagList[mapTag].push_back(rswModel->fileName);
						}
					});
				delete node;
			}

			windowData.progressWindowProgres = 1;
			windowData.progressWindowVisible = false;
			});
		progressThread.detach();
	}

	static util::FileIO::Node* selectedNode = nullptr;
	std::function<void(util::FileIO::Node*)> buildTreeNodes;
	buildTreeNodes = [&](util::FileIO::Node* node)
	{
		for (auto f : node->directories)
		{
			int flags = ImGuiTreeNodeFlags_OpenOnDoubleClick;
			if (f.second->directories.size() == 0)
				flags |= ImGuiTreeNodeFlags_Bullet;
			if (f.second == selectedNode)
				flags |= ImGuiTreeNodeFlags_Selected;

			if (ImGui::TreeNodeEx(f.second->name.c_str(), flags))
			{
				buildTreeNodes(f.second);
				ImGui::TreePop();
			}
			if (ImGui::IsItemClicked())
				selectedNode = f.second;
		}
	};
	
	ImGui::BeginChild("left pane", ImVec2(250, 0), true);

	auto startTree = [&](const char* nodeName, const std::string& path)
	{
		auto root = util::FileIO::directoryNode(path);
		if (!root)
			return;
		if (ImGui::TreeNodeEx(nodeName, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnDoubleClick))
		{
			if (ImGui::IsItemClicked())
				selectedNode = root;
			buildTreeNodes(root);
			ImGui::TreePop();
		}
		else if (ImGui::IsItemClicked())
			selectedNode = root;
	};
	startTree("Models", "data\\model\\");
	startTree("Sounds", "data\\wav\\");
	startTree("Lights", "data\\lights\\");
	startTree("Effects", "data\\effects\\");
	ImGui::EndChild();
	ImGui::SameLine();

	ImGuiStyle& style = ImGui::GetStyle();
	float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;


	auto buildBox = [&](const std::string& file) {
		if (ImGui::BeginChild(file.c_str(), ImVec2(config.thumbnailSize.x, config.thumbnailSize.y + 50), true, ImGuiWindowFlags_NoScrollbar))
		{
			std::string path = util::utf8_to_iso_8859_1(file);
			auto n = selectedNode;
			while (n)
			{
				if (n->name != "")
					path = util::utf8_to_iso_8859_1(n->name) + "\\" + path;
				n = n->parent;
			}
			
			ImTextureID texture = 0;
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
				texture = (ImTextureID)(long long)soundTexture->id;
			else if (path.substr(path.size() - 5) == ".json")
			{
				if (path.find("data\\lights") != std::string::npos)
					texture = (ImTextureID)(long long)lightTexture->id;
				if (path.find("data\\effects") != std::string::npos)
					texture = (ImTextureID)(long long)effectTexture->id;
			}
			if (ImGui::ImageButton(texture, config.thumbnailSize, ImVec2(0, 0), ImVec2(1, 1), 0))
			{
				if (activeMapView && newNodes.size() == 0)
				{
					if (file.substr(file.size() - 4) == ".rsm")
					{
						Node* newNode = new Node(file);
						newNode->addComponent(util::ResourceManager<Rsm>::load(path));
						newNode->addComponent(new RsmRenderer());
						newNode->addComponent(new RswObject());
						newNode->addComponent(new RswModel(util::iso_8859_1_to_utf8(path)));
						newNode->addComponent(new RsmRenderer());
						newNode->addComponent(new RswModelCollider());
						newNodes.push_back(std::pair<Node*, glm::vec3>(newNode, glm::vec3(0, 0, 0)));
					}
					else if (file.substr(file.size() - 5) == ".json" &&
						path.find("data\\lights") != std::string::npos)
					{
						auto l = new RswLight();
						try {
							from_json(util::FileIO::getJson(path), *l);
						}catch(...) { std::cerr<<"Error loading json"<<std::endl; }
						Node* newNode = new Node(file);
						newNode->addComponent(new RswObject());
						newNode->addComponent(l);
						newNode->addComponent(new BillboardRenderer("data\\light.png", "data\\light_selected.png"));
						newNode->addComponent(new CubeCollider(5));
						newNodes.push_back(std::pair<Node*, glm::vec3>(newNode, glm::vec3(0, 0, 0)));
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
					if (ImGui::Button("Replace selected models") && activeMapView)
					{
						auto ga = new GroupAction();
						for (auto n : activeMapView->map->selectedNodes)
							ga->addAction(new ModelChangeAction(n, path));
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
						tagList[newTag].push_back(util::iso_8859_1_to_utf8(path.substr(11))); //remove data\model\ prefix
						tagListReverse[path.substr(11)].push_back(newTag);
						saveTagList();
					}
					ImGui::Separator();
					ImGui::Text("Current tags on this model");
					for (auto tag : tagListReverse[path.substr(11)])
					{
						ImGui::BulletText(tag.c_str());
						ImGui::SameLine();
						if (ImGui::Button("Remove"))
						{
							tagList[tag].erase(std::remove_if(tagList[tag].begin(), tagList[tag].end(), [&](const std::string& m) { return m == util::iso_8859_1_to_utf8(path.substr(11)); }), tagList[tag].end());
							tagListReverse[path.substr(11)].erase(std::remove_if(tagListReverse[path.substr(11)].begin(), tagListReverse[path.substr(11)].end(), [&](const std::string& t) { return t == tag; }), tagListReverse[path.substr(11)].end());
							saveTagList();
						}

					}
				}

				ImGui::EndPopup();
			}
			else if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip(util::combine(tagListReverse[path.substr(11)], "\n").c_str());
				auto it = objectWindowObjects.find(path);
				if (it != objectWindowObjects.end())
				{
					it->second->rotation = glfwGetTime()*90; //TODO: make this increment based on deltatime
					it->second->draw();
				}
			}
			ImGui::Text(file.c_str());
		}
		ImGui::EndChild();

		float last_button_x2 = ImGui::GetItemRectMax().x;
		float next_button_x2 = last_button_x2 + style.ItemSpacing.x + config.thumbnailSize.x; // Expected position if next button was on same line
		if (next_button_x2 < window_visible_x2)
			ImGui::SameLine();
	};

	if (ImGui::BeginChild("right pane", ImVec2(ImGui::GetContentRegionAvail().x, 0), true))
	{
		if (selectedNode != nullptr && filter == "")
		{
			window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
			for (auto file : selectedNode->files)
			{
				if ((file.find(".rsm") == std::string::npos || 
					file.find(".rsm2") == std::string::npos) &&
					file.find(".wav") == std::string::npos &&
					file.find(".json") == std::string::npos)
					continue;
				buildBox(file);
			}
		}
		else if (filter != "")
		{
			static std::string currentFilterText = "";
			static std::vector<std::string> filteredFiles;
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
						filteredFiles.push_back("data\\model\\" + util::iso_8859_1_to_utf8(t.first));
				}
			}
			for (const auto& file : filteredFiles)
			{
				buildBox(file);
			}

		}
	}
	ImGui::EndChild();
	ImGui::End();
}

