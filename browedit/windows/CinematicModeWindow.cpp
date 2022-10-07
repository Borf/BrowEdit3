#define IMGUI_DEFINE_MATH_OPERATORS
#include <browedit/BrowEdit.h>
#include <browedit/Icons.h>
#include <browedit/Map.h>
#include <browedit/MapView.h>
#include <browedit/Node.h>
#include <browedit/gl/Texture.h>
#include <browedit/components/Rsw.h>
#include <imgui.h>
#include <imgui_internal.h>


float length = 20;
float scale = 100;
float rowHeight = 25;
float leftAreaSize = 200;
float timeSelected = 1;
static int selectedKeyFrame = -1;
static int selectedTrack = -1;

void BrowEdit::showCinematicModeWindow()
{
	if (!activeMapView)
		return;
	auto rsw = activeMapView->map->rootNode->getComponent<Rsw>();

	timeSelected = (float)std::fmod(ImGui::GetTime(), length);
	if (rsw->tracks.size() == 0) // stub data
	{
		Rsw::Track t1{ "Camera Position" };
		Rsw::Track t2{ "Camera Rotation" };
		t1.frames.push_back(new Rsw::KeyFrameData<std::pair<glm::vec3, glm::vec3>>(0, std::pair<glm::vec3, glm::vec3>(glm::vec3(50, 50, 50), glm::vec3(0,0,100))));
		t1.frames.push_back(new Rsw::KeyFrameData<std::pair<glm::vec3, glm::vec3>>(2.5f, std::pair<glm::vec3, glm::vec3>(glm::vec3(50, 50, 150), glm::vec3(100, 0, 0))));
		t1.frames.push_back(new Rsw::KeyFrameData<std::pair<glm::vec3, glm::vec3>>(5, std::pair<glm::vec3, glm::vec3>(glm::vec3(150, 75, 150), glm::vec3(0, 0, -100))));
		t1.frames.push_back(new Rsw::KeyFrameData<std::pair<glm::vec3, glm::vec3>>(7.5, std::pair<glm::vec3, glm::vec3>(glm::vec3(150, 75, 50), glm::vec3(-100, 0, 0))));
		t1.frames.push_back(new Rsw::KeyFrameData<std::pair<glm::vec3, glm::vec3>>(7.5, std::pair<glm::vec3, glm::vec3>(glm::vec3(50, 50, 50), glm::vec3(-100, 0, 0))));
		t1.frames.push_back(new Rsw::KeyFrameData<std::pair<glm::vec3, glm::vec3>>(20, std::pair<glm::vec3, glm::vec3>(glm::vec3(50, 50, 150), glm::vec3(100, 0, 0))));
		
		t2.frames.push_back(new Rsw::KeyFrameData<Rsw::CameraTarget>(0, Rsw::CameraTarget(glm::vec3(100,10,100), 0.01f)));

		rsw->tracks.push_back(t1);
		rsw->tracks.push_back(t2);
	}

	ImGui::Begin("Cinematic Mode");
	if (!activeMapView)
	{
		ImGui::End();
		return;
	}

	ImGui::SetNextItemWidth(200);
	ImGui::DragFloat("##Length", &length, 0.1f, 0, 10000.0f);
	ImGui::SameLine();
	if (ImGui::Button("<"))
	{
		if (selectedTrack >= 0 && selectedTrack < rsw->tracks.size() && selectedKeyFrame > 0)
			selectedKeyFrame--;
	}
	ImGui::SameLine();
	if (ImGui::Button(">"))
	{
		if (selectedTrack >= 0 && selectedTrack < rsw->tracks.size() && selectedKeyFrame < rsw->tracks[selectedTrack].frames.size()-1)
			selectedKeyFrame++;
	}
	ImGui::SameLine();

	toolBarButton("PlayPause", ICON_PAUSE, "Play / Pause", ImVec4(1, 1, 1, 1));
	ImGui::SameLine();
	ImGui::Checkbox("##preview", &activeMapView->cinematicPlay);


	if (ImGui::BeginChild("KeyframeEditor", ImVec2(-1, 200), true, ImGuiWindowFlags_AlwaysHorizontalScrollbar))
	{
		const ImGuiStyle& style = ImGui::GetStyle();
		const ImGuiIO& IO = ImGui::GetIO();
		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		auto window = ImGui::GetCurrentWindow();

		ImRect bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(leftAreaSize + scale * length, 100));
		ImGui::ItemSize(bb);
		if (!ImGui::ItemAdd(bb, NULL))
		{
			ImGui::EndChild();
			ImGui::End();
			return;
		}
		ImVec2 cursorPos = ImGui::GetCursorScreenPos();
		ImGui::RenderFrame(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_PopupBg, 1), true, style.FrameRounding);

		int trackIndex = 0;
		for (auto& track : rsw->tracks)
		{
			ImGui::RenderText(bb.Min + ImVec2(10, rowHeight + rowHeight * trackIndex + 2), track.name.c_str()); //todo: center
			ImGui::RenderFrame(bb.Min + ImVec2(leftAreaSize, rowHeight + rowHeight * trackIndex  +1), bb.Min + ImVec2(leftAreaSize + scale * length, rowHeight + rowHeight * trackIndex + rowHeight-2), ImGui::GetColorU32(ImGuiCol_FrameBg, 1), true, style.FrameRounding);
			trackIndex++;
		}

		for (int i = 0; i < length; i++)
		{
			ImGui::RenderText(bb.Min + ImVec2(leftAreaSize + scale * i + 4, 0), std::to_string(i).c_str());
			for (int ii = 0; ii < 10; ii++)
			{
				window->DrawList->AddLine(bb.Min + ImVec2(leftAreaSize + scale * i + scale / 10.0f * ii, rowHeight - 5), bb.Min + ImVec2(leftAreaSize + scale * i + scale / 10.0f * ii, rowHeight), ImGui::GetColorU32(ImGuiCol_TextDisabled, 1.0f));
				window->DrawList->AddLine(bb.Min + ImVec2(leftAreaSize + scale * i + scale / 10.0f * ii, rowHeight), bb.Min + ImVec2(leftAreaSize + scale * i + scale / 10.0f * ii, rowHeight * (rsw->tracks.size() + 1)), ImGui::GetColorU32(ImGuiCol_TextDisabled, 0.5f));
			}
			window->DrawList->AddLine(bb.Min + ImVec2(leftAreaSize + scale * i, 0), bb.Min + ImVec2(leftAreaSize + scale * i, rowHeight * (rsw->tracks.size() + 1)), ImGui::GetColorU32(ImGuiCol_Text, 1));

		}

		trackIndex = 0;
		for (auto& track : rsw->tracks)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_Button, 10));
			int keyframeIndex = 0;
			for (auto& keyframe : track.frames)
			{
				ImGui::PushID(keyframe);
				bool selected = selectedTrack >= 0 && selectedTrack < rsw->tracks.size() && selectedKeyFrame >= 0 && selectedKeyFrame < rsw->tracks[selectedTrack].frames.size() && keyframe == rsw->tracks[selectedTrack].frames[selectedKeyFrame];
				if (selected)
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_PlotHistogram, 10));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetColorU32(ImGuiCol_PlotHistogram, 10));
				}
				ImGui::SetCursorScreenPos(bb.Min + ImVec2(leftAreaSize + scale * keyframe->time - 5, rowHeight + rowHeight * trackIndex));
				if (ImGui::Button("##", ImVec2(10, rowHeight)))
				{
					selectedTrack = trackIndex;
					selectedKeyFrame = keyframeIndex;
				}
				if (selected)
					ImGui::PopStyleColor(2);
				keyframeIndex++;
				ImGui::PopID();
			}
			ImGui::PopStyleColor();
			trackIndex++;
		}

		ImGui::SetCursorScreenPos(bb.Min + ImVec2(leftAreaSize + scale * timeSelected -5, 0));
		window->DrawList->AddLine(bb.Min + ImVec2(leftAreaSize + scale * timeSelected, 0), bb.Min + ImVec2(leftAreaSize + scale * timeSelected, rowHeight * (rsw->tracks.size() + 1)), ImGui::GetColorU32(ImGuiCol_Text, 1));
		ImGui::Button("##", ImVec2(10, rowHeight));


		ImGui::SetCursorScreenPos(cursorPos);


		ImGui::EndChild();
	}

	ImGui::End();


	ImGui::Begin("Cinematic Mode Properties");
	bool selected = selectedTrack >= 0 && selectedTrack < rsw->tracks.size() && selectedKeyFrame >= 0 && selectedKeyFrame < rsw->tracks[selectedTrack].frames.size();
	if (selected)
	{
		auto keyFrame = rsw->tracks[selectedTrack].frames[selectedKeyFrame];

		keyFrame->buildEditor();
	}
	ImGui::End();

}