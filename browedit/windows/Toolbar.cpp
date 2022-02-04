#include <browedit/BrowEdit.h>
#include <browedit/MapView.h>
#include <browedit/Map.h>

void BrowEdit::toolbar()
{
	auto viewport = ImGui::GetMainViewport();

	ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y));
	ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, config.toolbarHeight()));
	ImGuiWindowFlags toolbarFlags = 0
		| ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove;
	ImGui::Begin("Toolbar", 0, toolbarFlags);

		if (editMode == EditMode::Texture)
		ImGui::Text("Texture");
	else if (editMode == EditMode::Object)
		ImGui::Text("Object");
	else if (editMode == EditMode::Wall)
		ImGui::Text("Wall");
	else
		ImGui::Text("???");
	ImGui::SameLine();
	ImGui::SetCursorPosX(125);

	if (toolBarToggleButton("heightmode", 0, editMode == EditMode::Height, "Height edit mode"))
		editMode = EditMode::Texture;
	ImGui::SameLine();
	if (toolBarToggleButton("texturemode", 1, editMode == EditMode::Texture, "Texture edit mode"))
		editMode = EditMode::Texture;
	ImGui::SameLine();
	if (toolBarToggleButton("objectmode", 2, editMode == EditMode::Object, "Object edit mode"))
		editMode = EditMode::Object;
	ImGui::SameLine();
	if (toolBarToggleButton("wallmode", 3, editMode == EditMode::Wall, "Wall edit mode"))
		editMode = EditMode::Wall;
	ImGui::SameLine();

	if (editMode == EditMode::Object)
	{
		toolBarToggleButton("showObjectWindow", 2, &windowData.objectWindowVisible, "Toggle object window", ImVec4(1, 0, 0, 1));
		ImGui::SameLine();
	}


	if (toolBarToggleButton("undo", 16, false, "Undo") && activeMapView)
		activeMapView->map->undo(this);
	ImGui::SameLine();
	if (toolBarToggleButton("redo", 17, false, "Redo") && activeMapView)
		activeMapView->map->undo(this);
	ImGui::SameLine();
	if (toolBarToggleButton("copy", 18, false, "Copy") && activeMapView)
		activeMapView->map->copySelection();
	ImGui::SameLine();
	if (toolBarToggleButton("paste", 19, false, "Paste") && activeMapView)
		activeMapView->map->pasteSelection(this);
	ImGui::SameLine();



	ImGui::End();




	ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + viewport->WorkSize.y - config.toolbarHeight()));
	ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, config.toolbarHeight()));
	toolbarFlags = 0
		| ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove;
	ImGui::Begin("Statusbar", 0, toolbarFlags);
	ImGui::Text("Browedit!");
	ImGui::SameLine();
	ImGui::End();
}