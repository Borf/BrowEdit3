#include <browedit/BrowEdit.h>


void BrowEdit::toolbar()
{
	auto viewport = ImGui::GetMainViewport();

	ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y));
	ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, toolbarHeight));
	ImGuiWindowFlags toolbarFlags = 0
		| ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove;
	ImGui::Begin("Toolbar", 0, toolbarFlags);

	if (editMode == EditMode::Texture)
		ImGui::Text("Texture");
	else if (editMode == EditMode::Object)
		ImGui::Text("Object");
	else if (editMode == EditMode::Wall)
		ImGui::Text("Wall");
	ImGui::SameLine();

	if (toolBarToggleButton("texturemode", 4, editMode == EditMode::Texture, "Texture edit mode"))
		editMode = EditMode::Texture;
	ImGui::SameLine();
	if (toolBarToggleButton("objectmode", 5, editMode == EditMode::Object, "Object edit mode"))
		editMode = EditMode::Object;
	ImGui::SameLine();
	if (toolBarToggleButton("wallmode", 6, editMode == EditMode::Wall, "Wall edit mode"))
		editMode = EditMode::Wall;
	ImGui::SameLine();

	if (editMode == EditMode::Object)
		toolBarToggleButton("showObjectWindow", 5, &windowData.objectWindowVisible, "Toggle object window");

	ImGui::End();

	ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + viewport->WorkSize.y - toolbarHeight));
	ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, toolbarHeight));
	toolbarFlags = 0
		| ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove;
	ImGui::Begin("Statusbar", 0, toolbarFlags);
	ImGui::Text("Browedit!");
	ImGui::SameLine();
	ImGui::End();
}