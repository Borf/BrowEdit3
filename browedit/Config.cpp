#include <windows.h> // for warning
#include "Config.h"
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <json.hpp>
#include <browedit/BrowEdit.h>
#include <browedit/util/FileIO.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>
#include <glm/gtc/type_ptr.hpp>
#include <ShlObj_core.h>

using json = nlohmann::json;

std::string Config::isValid() const
{
	if (ropath == "")
		return "Ro path is empty";

	if (!std::filesystem::exists(ropath))
		return "Ro path does not exist";

	if (ropath[ropath.size() - 1] != '\\')
		return "Ro path should end with a \\";

	if (!std::filesystem::exists(ropath + "data"))
		return "Please create a data directory in your RO directory";


	for (const auto& grf : grfs)
	{
		if (!std::filesystem::exists(grf))
			return "Grf " + grf + " does not exist";
		if (grf.substr(std::max(0, (int)grf.size() - 4), 4) != ".grf")
			return "Grf " + grf + " does not end with .grf";
	}
	return "";
}


int CALLBACK BrowseCallBackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	switch (uMsg)
	{
	case BFFM_INITIALIZED:
		::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
		break;
	}
	return 0;
}

bool Config::showWindow(BrowEdit* browEdit)
{
	bool close = false;
	static bool showStyleEditor = false;
	if (ImGui::Begin("Configuration", 0, ImGuiWindowFlags_NoCollapse))
	{
		std::string error = isValid();
		if (error != "")
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
			ImGui::Text(error.c_str());
			ImGui::PopStyleColor();
		}

		ImGui::Text("RO Path");
		ImGui::InputText("##RO Path", &ropath);
		ImGui::SameLine();
		if (ImGui::Button("Browse"))
		{
			CoInitializeEx(0, 0);
			CHAR szDir[MAX_PATH];
			BROWSEINFO bInfo;
			bInfo.hwndOwner = glfwGetWin32Window(browEdit->window);
			bInfo.pidlRoot = NULL;
			bInfo.pszDisplayName = szDir;
			bInfo.lpszTitle = "Select your RO directory (not data)";
			bInfo.ulFlags = 0;
			bInfo.lpfn = BrowseCallBackProc;
			std::string path = ropath;
			if (path.find(":") == std::string::npos)
				path = std::filesystem::current_path().string() + "\\" + path; //TODO: fix this

			bInfo.lParam = (LPARAM)path.c_str();
			bInfo.iImage = -1;

			LPITEMIDLIST lpItem = SHBrowseForFolder(&bInfo);
			if (lpItem != NULL)
			{
				SHGetPathFromIDList(lpItem, szDir);
				ropath = szDir;
				if (ropath[ropath.size() - 1] != '\\')
					ropath += "\\";
			}

		}
		ImGui::Text("GRF Files");
		if(ImGui::BeginListBox("##GRFs"))
		{
			for (int i = 0; i < grfs.size(); i++)
			{
				ImGui::PushID(i);
				ImGui::InputText("##", &grfs[i]);
				ImGui::SameLine();
				if (ImGui::Button("Browse"))
				{
					CoInitializeEx(0, 0);

					std::string curdir = std::filesystem::current_path().string();

					HWND hWnd = glfwGetWin32Window(browEdit->window);
					OPENFILENAME ofn;
					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = hWnd;

					std::string initial = grfs[i];
					if (initial.find(":") == std::string::npos)
						initial = curdir + "\\" + initial;

					if (!std::filesystem::is_regular_file(initial))
						initial = "";

					char buf[MAX_PATH];
					ZeroMemory(buf, MAX_PATH);
					strcpy_s(buf, MAX_PATH, initial.c_str());
					ofn.lpstrFile = buf;
					ofn.nMaxFile = MAX_PATH;
					ofn.lpstrFilter = "All\0*.*\0GRF files\0*.grf\0";
					ofn.nFilterIndex = 2;
					ofn.lpstrFileTitle = NULL;
					ofn.nMaxFileTitle = 0;
					ofn.lpstrInitialDir = NULL;
					ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ENABLESIZING;
					if (GetOpenFileName(&ofn))
					{
						std::filesystem::current_path(curdir);
						grfs[i] = buf;
					}
					std::filesystem::current_path(curdir);
				}
				ImGui::SameLine();
				if (ImGui::Button("Remove"))
				{
					grfs.erase(grfs.begin() + i);
					i--;
					ImGui::PopID();
					continue;
				}
				ImGui::SameLine();
				if (ImGui::Button("Up"))
				{
					if (i > 0)
						std::swap(grfs[i], grfs[i - 1]);
				}
				ImGui::SameLine();
				if (ImGui::Button("Down"))
				{
					if (i < grfs.size() - 1)
						std::swap(grfs[i], grfs[i + 1]);
				}
				ImGui::PopID();
			}
			ImGui::EndListBox();
		}
		ImGui::SameLine();
		if (ImGui::Button("Add"))
		{
			grfs.push_back("c:\\");
		}

		ImGui::DragFloat("Field of View", &fov, 0.1f, 1.0f, 180.0f);
		ImGui::DragFloat("Camera Mouse Speed", &cameraMouseSpeed, 0.05f, 0.01f, 3.0f);

		if (ImGui::Combo("Skin", &style, "Dark\0Light\0Classic\0"))
		{
			switch (style)
			{
			case 0: ImGui::StyleColorsDark(); break;
			case 1: ImGui::StyleColorsLight(); break;
			case 2: ImGui::StyleColorsClassic(); break;
			}
		}
		if (ImGui::Button("Style editor"))
			showStyleEditor = !showStyleEditor;

		ImGui::ColorEdit3("Background Color", glm::value_ptr(backgroundColor));
		ImGui::DragFloat2 ("Object Window thumbnail size", &thumbnailSize.x, 1, 32, 512);
		ImGui::Checkbox("Close object window when adding a model", &closeObjectWindowOnAdd);
		ImGui::DragFloat("Toolbar Button Size", &toolbarButtonSize, 1, 1, 100);
		ImGui::ColorEdit4("Toolbar Button Tint", &toolbarButtonTint.x);


		if (ImGui::Button("Save"))
		{
			if (isValid() == "")
			{
				json configJson = *this;
				std::ofstream configFile("config.json");
				configFile << configJson;
				close = true;

				setupFileIO();
			}
		}

	}
	ImGui::End();

	if(showStyleEditor)
		ImGui::ShowStyleEditor();

	return close;
}

void Config::setupFileIO()
{
	util::FileIO::begin();
	util::FileIO::addDirectory("./");
	util::FileIO::addDirectory(ropath);
	for (const auto& grf : grfs)
		util::FileIO::addGrf(grf);
	util::FileIO::end();

	switch (style)
	{
	case 0: ImGui::StyleColorsDark(); break;
	case 1: ImGui::StyleColorsLight(); break;
	case 2: ImGui::StyleColorsClassic(); break;
	}
}
