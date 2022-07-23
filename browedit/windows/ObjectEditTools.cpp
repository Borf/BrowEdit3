#include <browedit/BrowEdit.h>
#include <browedit/HotkeyRegistry.h>
#include <imgui.h>
#include <browedit/Icons.h>

void BrowEdit::showObjectEditToolsWindow()
{
	ImGui::Begin("Object Edit Tools");
	hotkeyButton("Copy", HotkeyAction::Global_Copy);
	ImGui::SameLine();
	hotkeyButton("Paste", HotkeyAction::Global_Paste);
	ImGui::SameLine();
	hotkeyButton("Delete", HotkeyAction::ObjectEdit_Delete);

	hotkeyButton("Flip horizontally", HotkeyAction::ObjectEdit_FlipHorizontal);
	ImGui::SameLine();
	hotkeyButton("Flip vertically", HotkeyAction::ObjectEdit_FlipVertical);

	hotkeyButton("Invert Scale X", HotkeyAction::ObjectEdit_InvertScaleX);
	ImGui::SameLine();
	hotkeyButton("Invert Scale Y", HotkeyAction::ObjectEdit_InvertScaleY);
	ImGui::SameLine();
	hotkeyButton("Invert Scale Z", HotkeyAction::ObjectEdit_InvertScaleZ);

	hotkeyButton("Create Prefab", HotkeyAction::ObjectEdit_CreatePrefab);
	hotkeyButton("Focus on selection", HotkeyAction::ObjectEdit_FocusOnSelection);

	ImGui::InputFloat("Nudge distance", &nudgeDistance);
	hotkeyButton("Nudge X Negative", HotkeyAction::ObjectEdit_NudgeXNeg);
	ImGui::SameLine();
	hotkeyButton("Nudge X Positive", HotkeyAction::ObjectEdit_NudgeXPos);
	hotkeyButton("Nudge Y Negative", HotkeyAction::ObjectEdit_NudgeYNeg);
	ImGui::SameLine();
	hotkeyButton("Nudge Y Positive", HotkeyAction::ObjectEdit_NudgeYPos);
	hotkeyButton("Nudge Z Negative", HotkeyAction::ObjectEdit_NudgeZNeg);
	ImGui::SameLine();
	hotkeyButton("Nudge Z Positive", HotkeyAction::ObjectEdit_NudgeZPos);

	if (toolBarToggleButton("localglobal", activeMapView->pivotPoint == MapView::PivotPoint::Local ? ICON_PIVOT_ROT_LOCAL : ICON_PIVOT_ROT_GLOBAL, false, "Changes the pivot point for rotations", config.toolbarButtonsObjectEdit))
	{
		if (activeMapView->pivotPoint == MapView::PivotPoint::Local)
			activeMapView->pivotPoint = MapView::PivotPoint::GroupCenter;
		else
			activeMapView->pivotPoint = MapView::PivotPoint::Local;
	}

	ImGui::InputFloat("Rotate distance", &rotateDistance);
	hotkeyButton("Rotate X Negative", HotkeyAction::ObjectEdit_RotXNeg);
	ImGui::SameLine();
	hotkeyButton("Rotate X Positive", HotkeyAction::ObjectEdit_RotXPos);
	hotkeyButton("Rotate Y Negative", HotkeyAction::ObjectEdit_RotYNeg);
	ImGui::SameLine();
	hotkeyButton("Rotate Y Positive", HotkeyAction::ObjectEdit_RotYPos);
	hotkeyButton("Rotate Z Negative", HotkeyAction::ObjectEdit_RotZNeg);
	ImGui::SameLine();
	hotkeyButton("Rotate Z Positive", HotkeyAction::ObjectEdit_RotZPos);

	ImGui::End();
}