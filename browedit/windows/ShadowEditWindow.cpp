#include <browedit/BrowEdit.h>
#include <browedit/Icons.h>
#include <browedit/MapView.h>
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/components/Rsw.h>
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>

bool shadowDropperEnabled = false;
void BrowEdit::showShadowEditWindow()
{
	if (!activeMapView)
		return;
	ImGui::Begin("Shadow Edit Window");

	ImGui::SliderInt("Brush Alpha", &shadowEditBrushAlpha, 0, 255);

	if (ImGui::IsKeyPressed(GLFW_KEY_KP_ADD))
		shadowEditBrushSize++;
	if (ImGui::IsKeyPressed(GLFW_KEY_KP_SUBTRACT))
		shadowEditBrushSize = glm::max(shadowEditBrushSize - 1, 1);

	ImGui::InputInt("Brush Size", &shadowEditBrushSize);
	ImGui::Checkbox("Smooth shadow edges", &shadowSmoothEdges);

	if (toolBarButton("Color Picker", ICON_DROPPER, "Picks a color from the shadowmap", ImVec4(1, 1, 1, 1)))
	{
		shadowDropperEnabled = !shadowDropperEnabled;
		cursor = shadowDropperEnabled ? dropperCursor : nullptr;
	}

	ImGui::End();
}