#include <windows.h>
#include <browedit/BrowEdit.h>
#include <browedit/MapView.h>
#include <browedit/Map.h>
#include <browedit/gl/FBO.h>
#include <browedit/util/FileIO.h>
#include <browedit/util/Util.h>
#include <GLFW/glfw3.h>

void BrowEdit::menuBar()
{
	ImGui::BeginMainMenuBar();
	if (ImGui::BeginMenu("File"))
	{
		ImGui::MenuItem("New", "Ctrl+n");
		if (ImGui::MenuItem("Open", "Ctrl+o"))
			showOpenWindow();
		if (activeMapView && ImGui::MenuItem(("Save " + activeMapView->map->name).c_str(), "Ctrl+s"))
			saveMap(activeMapView->map);
		if (ImGui::MenuItem("Quit"))
			glfwSetWindowShouldClose(window, 1);
		ImGui::EndMenu();
	}
	if (ImGui::BeginMenu("Edit"))
	{
		if (ImGui::MenuItem("Undo", "Ctrl+z") && activeMapView)
			activeMapView->map->undo(this);
		if (ImGui::MenuItem("Redo", "Ctrl+shift+z") && activeMapView)
			activeMapView->map->redo(this);
		if (ImGui::MenuItem("Undo Window", nullptr, windowData.undoVisible))
			windowData.undoVisible = !windowData.undoVisible;
		if (ImGui::MenuItem("Configure"))
			windowData.configVisible = true;
		if (ImGui::MenuItem("Demo Window", nullptr, windowData.demoWindowVisible))
			windowData.demoWindowVisible = !windowData.demoWindowVisible;
		ImGui::EndMenu();
	}
	if (editMode == EditMode::Object && activeMapView && ImGui::BeginMenu("Selection"))
	{
		if (ImGui::MenuItem("Copy", "Ctrl+c"))
			activeMapView->map->copySelection();
		if (ImGui::MenuItem("Paste", "Ctrl+v"))
			activeMapView->map->pasteSelection(this);
		if (ImGui::BeginMenu("Grow/shrink Selection"))
		{
			if (ImGui::MenuItem("Select same models"))
				activeMapView->map->selectSameModels(this);
			static float nearDistance = 50;
			ImGui::SetNextItemWidth(100.0f);
			ImGui::DragFloat("Near Distance", &nearDistance, 1.0f, 0.0f, 1000.0f);
			if (ImGui::MenuItem("Select objects near"))
				activeMapView->map->selectNear(nearDistance, this);
			if (ImGui::MenuItem("Select all"))
				activeMapView->map->selectAll(this);
			if (ImGui::MenuItem("Invert selection"))
				activeMapView->map->selectInvert(this);
			ImGui::EndMenu();
		}
		if (ImGui::MenuItem("Flip horizontally"))
			activeMapView->map->flipSelection(0, this);
		if (ImGui::MenuItem("Flip vertically"))
			activeMapView->map->flipSelection(2, this);
		if (ImGui::MenuItem("Delete", "Delete"))
			activeMapView->map->deleteSelection(this);
		if (ImGui::MenuItem("Go to selection", "F"))
			activeMapView->focusSelection();
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Maps"))
	{
		for (auto map : maps)
		{
			if (ImGui::BeginMenu(map->name.c_str()))
			{
				if (ImGui::MenuItem("Save as"))
					saveMap(map);
				if (ImGui::MenuItem("Open new view"))
					loadMap(map->name);
				for (auto mv : mapViews)
				{
					if (mv.map == map)
					{
						if (ImGui::BeginMenu(mv.viewName.c_str()))
						{
							if (ImGui::BeginMenu("High Res Screenshot"))
							{
								static int size[2] = { 2048, 2048 };
								ImGui::DragInt2("Size", size);
								if (ImGui::MenuItem("Save"))
								{
									std::string filename = util::SaveAsDialog("screenshot.png", "All\0*.*\0Png\0*.png");
									if (filename != "")
									{
										mv.fbo->resize(size[0], size[1]);
										mv.render(this);
										mv.fbo->saveAsFile(filename);
									}
								}
								ImGui::EndMenu();
							}
							ImGui::EndMenu();
						}
					}
				}
				ImGui::EndMenu();
			}
		}
		ImGui::EndMenu();
	}
	ImGui::EndMainMenuBar();
}

