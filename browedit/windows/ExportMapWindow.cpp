#include <windows.h>
#include <browedit/BrowEdit.h>
#include <misc/cpp/imgui_stdlib.h>
#include <browedit/util/FileIO.h>
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/components/Rsm.h>
#include <browedit/components/Rsw.h>
#include <browedit/components/Gnd.h>
#include <iostream>
#include <fstream>
#include <filesystem>

void BrowEdit::showExportWindow()
{
	if (windowData.exportVisible)
		ImGui::OpenPopup("Export Map");
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("Export Map", 0, ImGuiWindowFlags_NoDocking))
	{
		ImGui::InputText("Export folder", &windowData.exportFolder);
		ImGui::SameLine();
		if (ImGui::Button("Browse"))
		{
			windowData.exportFolder = util::iso_8859_1_to_utf8(util::SelectPathDialog(util::utf8_to_iso_8859_1(windowData.exportFolder)));
		}
		if (ImGui::Button("Export!"))
		{
			char buf[1024];
			for (const auto& e : windowData.exportToExport)
			{
				if (!e.enabled)
					continue;
				std::string outFileName = util::utf8_to_iso_8859_1(windowData.exportFolder) + e.filename;
				std::filesystem::path path(outFileName);
				auto p = path.parent_path().string();
				std::filesystem::create_directories(p);

				auto is = util::FileIO::open(e.filename);
				if (!is)
				{
					std::cout << "Error opening " << e.filename << std::endl;
					continue;
				}
				auto os = std::ofstream(outFileName, std::ios_base::out | std::ios_base::binary);
				while (!is->eof())
				{
					is->read(buf, 1024);
					auto count = is->gcount();
					os.write(buf, count);
				}
				delete is;
			}
			windowData.exportVisible = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			windowData.exportVisible = false;
			ImGui::CloseCurrentPopup();
		}

		if (ImGui::BeginTable("Data", 4, ImGuiTableFlags_ScrollY | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
		{
			for (auto& e : windowData.exportToExport)
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				if (ImGui::Checkbox(util::iso_8859_1_to_utf8(e.filename).c_str(), &e.enabled))
				{
					for (auto c : e.linkedForward)
					{
						bool canDisabled = true;
						for (auto cc : c->linkedBackward)
							if (cc->enabled)
								canDisabled = false;
						c->enabled = !canDisabled;
					}
				}
				ImGui::TableNextColumn();
				ImGui::Text(e.type.c_str());
				ImGui::TableNextColumn();
				ImGui::Text(e.source.c_str());
				ImGui::TableNextColumn();
				for(auto i : e.linkedBackward)
					if(i->enabled)
						ImGui::Text(util::iso_8859_1_to_utf8(i->filename).c_str());
			}

			ImGui::EndTable();
		}

		ImGui::EndPopup();
	}
}


void BrowEdit::exportMap(Map* map)
{
	windowData.exportToExport.clear();
	windowData.exportToExport.push_back(WindowData::ExportInfo{ map->name,"rsw", util::FileIO::getSrc(map->name), true });
	windowData.exportToExport.push_back(WindowData::ExportInfo{ map->name.substr(0, map->name.size() - 4) + ".gnd","gnd", util::FileIO::getSrc(map->name.substr(0, map->name.size() - 4) + ".gnd"), true });
	windowData.exportToExport.push_back(WindowData::ExportInfo{ map->name.substr(0, map->name.size() - 4) + ".gat","gat", util::FileIO::getSrc(map->name.substr(0, map->name.size() - 4) + ".gat"), true });
	map->rootNode->traverse([&](Node* n)
	{
		auto gnd = n->getComponent<Gnd>();
		if (gnd)
		{
			for (const auto& t : gnd->textures)
				windowData.exportToExport.push_back(WindowData::ExportInfo{ "data\\texture\\" + t->file,"gndTexture", util::FileIO::getSrc("data\\texture\\" + t->file), true});
		}
		auto rsm = n->getComponent<Rsm>();
		if (rsm)
		{
			if (std::find_if(windowData.exportToExport.begin(), windowData.exportToExport.end(), [rsm](const WindowData::ExportInfo& ei) { return ei.filename == rsm->fileName; }) != windowData.exportToExport.end())
				return;
			windowData.exportToExport.push_back(WindowData::ExportInfo{ rsm->fileName ,"rsm", util::FileIO::getSrc(rsm->fileName), true});
			auto modelExport = &windowData.exportToExport.back();
			//find all textures used by this model
			std::vector<std::string> textures;
			textures.insert(textures.end(), rsm->textures.begin(), rsm->textures.end());
			//std::function<void(Rsm::Mesh*)> addTex;
			//addTex = [&textures, &addTex](Rsm::Mesh* m)
			//{
			//	textures.insert(textures.end(), m->textureFiles.begin(), m->textureFiles.end());
			//	for (auto n : m->children)
			//		addTex(n);
			//};
			//addTex(rsm->rootMesh);
			std::sort(textures.begin(), textures.end());
			auto last = std::unique(textures.begin(), textures.end());
			textures.erase(last, textures.end());
			//add textures
			for (const auto& t : textures)
			{
				auto textureFound = std::find_if(windowData.exportToExport.begin(), windowData.exportToExport.end(), [t](const WindowData::ExportInfo& ei) { return ei.filename == "data\\model\\" + t; });
				if (textureFound == windowData.exportToExport.end())
				{
					windowData.exportToExport.push_back(WindowData::ExportInfo{ "data\\texture\\" + t, "rsmTexture", util::FileIO::getSrc("data\\texture\\" + t), true });
					windowData.exportToExport.back().linkedBackward.push_back(modelExport);
					modelExport->linkedForward.push_back(&windowData.exportToExport.back());
				}
				else
				{
					textureFound->linkedBackward.push_back(modelExport);
					modelExport->linkedForward.push_back(&(*textureFound));
				}
			}

		}
	});


	windowData.exportVisible = true;
	windowData.exportMap = map;
	ImGui::OpenPopup("Export Map");
}