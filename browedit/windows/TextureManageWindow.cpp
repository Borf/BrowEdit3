#include <browedit/BrowEdit.h>
#include <browedit/util/ResourceManager.h>
#include <browedit/gl/Texture.h>
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/GndRenderer.h>
#include <browedit/actions/GndTextureActions.h>
#include <misc/cpp/imgui_stdlib.h>
#include <imgui_internal.h>


void BrowEdit::showTextureManageWindow()
{
	if (!windowData.textureManageWindowVisible)
		return;
	if (!ImGui::Begin("Texture Picker", &windowData.textureManageWindowVisible))
		return;
	static std::string tagFilter;
	ImGui::InputText("Filter", &tagFilter);
	ImGui::SameLine();

	if (ImGui::Button("Rescan map filters"))
	{
	}

	std::function<void(util::FileIO::Node*)> buildTreeNodes;
	buildTreeNodes = [&](util::FileIO::Node* node)
	{
		for (auto f : node->directories)
		{
			int flags = ImGuiTreeNodeFlags_OpenOnDoubleClick;
			if (f.second->directories.size() == 0)
				flags |= ImGuiTreeNodeFlags_Bullet;
			if (f.second == windowData.textureManageWindowSelectedTreeNode)
				flags |= ImGuiTreeNodeFlags_Selected;

			if (ImGui::TreeNodeEx(f.second->name.c_str(), flags))
			{
				buildTreeNodes(f.second);
				ImGui::TreePop();
			}
			if (ImGui::IsItemClicked())
				windowData.textureManageWindowSelectedTreeNode = f.second;
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
				windowData.textureManageWindowSelectedTreeNode = root;
			buildTreeNodes(root);
			ImGui::TreePop();
		}
		else if (ImGui::IsItemClicked())
			windowData.textureManageWindowSelectedTreeNode = root;
	};
	startTree("Textures", "data\\texture\\");
	ImGui::EndChild();
	ImGui::SameLine();

	ImGuiStyle& style = ImGui::GetStyle();
	float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;


	auto buildBox = [&](const std::string& file, bool fullPath) {
		if (ImGui::BeginChild(file.c_str(), ImVec2(config.thumbnailSize.x, config.thumbnailSize.y + 50), true, ImGuiWindowFlags_NoScrollbar))
		{
			std::string path = util::utf8_to_iso_8859_1(file);
			if (!fullPath)
			{
				auto n = windowData.textureManageWindowSelectedTreeNode;
				while (n)
				{
					if (n->name != "")
						path = util::utf8_to_iso_8859_1(n->name) + "\\" + path;
					n = n->parent;
				}
			}

			if (windowData.textureManageWindowCache.find(path) == windowData.textureManageWindowCache.end())
				windowData.textureManageWindowCache[path] = util::ResourceManager<gl::Texture>::load(path);
			
			ImTextureID texture = 0;
			if(windowData.textureManageWindowCache[path]->loaded)
				texture = (ImTextureID)(long long)windowData.textureManageWindowCache[path]->id();

			if (ImGui::ImageButtonEx(ImGui::GetID(path.c_str()), texture, config.thumbnailSize, ImVec2(0, 0), ImVec2(1, 1), ImVec2(0, 0), ImVec4(0, 0, 0, 0), ImVec4(1, 1, 1, 1)))
			{
				std::cout << "Click on " << file << std::endl;
				if (activeMapView)
				{
					auto gnd = activeMapView->map->rootNode->getComponent<Gnd>();
					auto gndRenderer = activeMapView->map->rootNode->getComponent<GndRenderer>();
					std::string tex = path.substr(13); // remove "data\texture\"

					bool found = false;
					for (auto i = 0; i < gnd->textures.size(); i++)
					{
						if (gnd->textures[i]->file == tex)
						{
							found = true;
							activeMapView->textureSelected = i;
						}
					}
					if (!found)
						activeMapView->map->doAction(new GndTextureAddAction(tex), this);
				}
			}
			if (ImGui::BeginPopupContextWindow("Texture Tags"))
			{
				if (ImGui::CollapsingHeader("Actions", ImGuiTreeNodeFlags_DefaultOpen))
				{
					if (ImGui::Button("Replace selected Texture"))
					{
						auto gnd = activeMapView->map->rootNode->getComponent<Gnd>();
						auto gndRenderer = activeMapView->map->rootNode->getComponent<GndRenderer>();
						std::string tex = path.substr(13); // remove "data\texture\"

						bool found = false;
						for (auto i = 0; i < gnd->textures.size(); i++)
						{
							if (gnd->textures[i]->file == tex)
							{
								found = true; //replace tiles with texture activeMapView->textureSelected to texture i
							}
						}
						if (!found)
							activeMapView->map->doAction(new GndTextureChangeAction(activeMapView->textureSelected, tex), this);
						
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
					ImGui::Text("Current tags on this texture");
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
				ImGui::SetTooltip(util::combine(tagListReverse[path], "\n").c_str());
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
		if (windowData.textureManageWindowSelectedTreeNode != nullptr && tagFilter == "")
		{
			window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
			for (auto file : windowData.textureManageWindowSelectedTreeNode->files)
			{
				if (file.find(".bmp") == std::string::npos &&
					file.find(".tga") == std::string::npos &&
					file.find(".png") == std::string::npos &&
					file.find(".gif") == std::string::npos)
					continue;
				buildBox(file, false);
			}
		}
		else if (tagFilter != "")
		{
			static std::string currentFilterText = "";
			static std::vector<std::string> filteredFiles;
			if (currentFilterText != tagFilter)
			{
				currentFilterText = tagFilter;
				std::vector<std::string> filterTags = util::split(tagFilter, " ");
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
						filteredFiles.push_back(util::iso_8859_1_to_utf8(t.first));
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