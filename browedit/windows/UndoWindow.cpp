#include <browedit/BrowEdit.h>
#include <browedit/MapView.h>
#include <browedit/Map.h>
#include <browedit/actions/Action.h>

void BrowEdit::showUndoWindow()
{
	ImGui::Begin("Undo stack", &windowData.undoVisible);
	if (activeMapView && ImGui::BeginListBox("##stack", ImGui::GetContentRegionAvail()))
	{
		for (auto action : activeMapView->map->undoStack)
		{
			ImGui::Selectable(action->str().c_str());
		}
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.1f, 0.1f, 1.0f));
		for (auto action : activeMapView->map->redoStack)
		{
			ImGui::Selectable(action->str().c_str());
		}
		ImGui::PopStyleColor();
		if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
			ImGui::SetScrollHereY(1.0f);
		ImGui::EndListBox();
	}
	ImGui::End();
}
