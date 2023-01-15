#include <windows.h>
#include <browedit/BrowEdit.h>
#include <misc/cpp/imgui_stdlib.h>
#include <browedit/util/FileIO.h>
#include <browedit/gl/Texture.h>
#include <browedit/util/ResourceManager.h>
#include <iostream>

void BrowEdit::openWindow()
{
	static gl::Texture* preview = nullptr;
	if (windowData.openJustVisible)
		ImGui::OpenPopup("Open Map");
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("Open Map", 0, ImGuiWindowFlags_NoDocking))
	{
		/*if (ImGui::Button("Browse for file"))
		{
			std::cout << "Sorry, not working yet" << std::endl;
		}*/

		static bool selectFirst = false;
		ImGui::Text("Filter");
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		if(windowData.openJustVisible)
			ImGui::SetKeyboardFocusHere();
		windowData.openJustVisible = false;
		static std::string lastOpenFilter = windowData.openFilter;
		if (ImGui::InputText("##filter", &windowData.openFilter, ImGuiInputTextFlags_EnterReturnsTrue))
		{
			loadMap(windowData.openFiles[windowData.openFileSelected]);
			ImGui::CloseCurrentPopup();
		}
		if (lastOpenFilter != windowData.openFilter)
		{
			selectFirst = true;
			lastOpenFilter = windowData.openFilter;
		}

		ImGui::Text("Recent Files");
		if (ImGui::BeginListBox("##Recent", ImVec2(ImGui::GetContentRegionAvail().x, 100)))
		{
			for (const auto& m : config.recentFiles)
				if (ImGui::Selectable(m.c_str()))
				{
					loadMap(m);
					ImGui::CloseCurrentPopup();
					break;
				}
			ImGui::EndListBox();
		}
		
		if (ImGui::BeginListBox("##Files", ImVec2(ImGui::GetContentRegionAvail().x-200, ImGui::GetContentRegionAvail().y-40)))
		{
			for (std::size_t i = 0; i < windowData.openFiles.size(); i++)
			{
				if (windowData.openFilter == "" || windowData.openFiles[i].find(windowData.openFilter) != std::string::npos)
				{
					if (selectFirst)
					{
						selectFirst = false;
						windowData.openFileSelected = i;
					}
					if (ImGui::Selectable(windowData.openFiles[i].c_str(), windowData.openFileSelected == i))
					{
						windowData.openFileSelected = i;
						if (preview)
							util::ResourceManager<gl::Texture>::unload(preview);
						std::string filename = windowData.openFiles[i];
						if (filename.find(".") != std::string::npos)
							filename = filename.substr(0, filename.rfind("."));
						if (filename.find("data\\") != std::string::npos)
							filename = filename.substr(filename.find("data\\") + 5);

						filename = "data\\texture\\유저인터페이스\\map\\" + filename + ".bmp";
						preview = util::ResourceManager<gl::Texture>::load(filename);
					}
					if (ImGui::IsItemClicked(0) && ImGui::IsMouseDoubleClicked(0))
					{
						loadMap(windowData.openFiles[windowData.openFileSelected]);
						ImGui::CloseCurrentPopup();
					}
				}
			}
			ImGui::EndListBox();
		}
		if (preview)
		{
			ImGui::SameLine();
			ImGui::Image((ImTextureID)(long long)preview->id(), ImVec2(200, 200));
		}

		if (ImGui::Button("Open"))
		{
			loadMap(windowData.openFiles[windowData.openFileSelected]);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}
}



void BrowEdit::showOpenWindow()
{
	windowData.openFiles = util::FileIO::listAllFiles();
	windowData.openFiles.erase(std::remove_if(windowData.openFiles.begin(), windowData.openFiles.end(), [](const std::string& map) { return map.substr(map.size() - 4, 4) != ".rsw"; }), windowData.openFiles.end());
	std::sort(windowData.openFiles.begin(), windowData.openFiles.end());
	auto last = std::unique(windowData.openFiles.begin(), windowData.openFiles.end());
	windowData.openFiles.erase(last, windowData.openFiles.end());
	windowData.openJustVisible = true;
	ImGui::OpenPopup("Open Map");
}