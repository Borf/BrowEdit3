#include <browedit/BrowEdit.h>
#include <browedit/MapView.h>
#include <browedit/Map.h>
#include <browedit/actions/Action.h>

void BrowEdit::showUndoWindow()
{
	ImGui::Begin("Undo stack", &windowData.undoVisible);
	if (activeMapView && ImGui::BeginListBox("##stack", ImGui::GetContentRegionAvail()))
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.1f, 0.1f, 1.0f));
		for (auto it = activeMapView->map->redoStack.rbegin(); it != activeMapView->map->redoStack.rend(); it++)
		{
			ImGui::Selectable(((*it)->time + " " + (*it)->str()).c_str());
		}
		ImGui::PopStyleColor();

		int c = 0;
		for (auto it = activeMapView->map->undoStack.rbegin(); it != activeMapView->map->undoStack.rend(); it++)
		{
			ImGui::Selectable(((*it)->time + " " + (*it)->str()).c_str());
			c++;
			if (c > 100)
			{
				ImGui::Selectable("...");
				break;
			}
		}
		ImGui::EndListBox();
	}
	ImGui::End();
}
