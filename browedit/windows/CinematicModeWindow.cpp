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

float scale = 100;
float rowHeight = 25;
float leftAreaSize = 200;
float timeSelected = 1;
int selectedKeyFrame = -1;
int selectedTrack = -1;
bool playing = true;


void BrowEdit::showCinematicModeWindow()
{
	if (!activeMapView)
		return;
	auto rsw = activeMapView->map->rootNode->getComponent<Rsw>();

	if(playing)
		timeSelected = (float)std::fmod(timeSelected + ImGui::GetIO().DeltaTime, rsw->cinematicLength);
	if (rsw->cinematicTracks.size() == 0) // stub data
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

		rsw->cinematicTracks.push_back(t1);
		rsw->cinematicTracks.push_back(t2);
	}

	ImGui::Begin("Cinematic Mode");
	if (!activeMapView)
	{
		ImGui::End();
		return;
	}

	ImGui::SetNextItemWidth(200);
	ImGui::DragFloat("##Length", &rsw->cinematicLength, 0.1f, 0, 10000.0f);
	ImGui::SameLine();
	if (ImGui::Button("<"))
	{
		if (selectedTrack >= 0 && selectedTrack < rsw->cinematicTracks.size() && selectedKeyFrame > 0)
			selectedKeyFrame--;
	}
	ImGui::SameLine();
	if (ImGui::Button(">"))
	{
		if (selectedTrack >= 0 && selectedTrack < rsw->cinematicTracks.size() && selectedKeyFrame < rsw->cinematicTracks[selectedTrack].frames.size()-1)
			selectedKeyFrame++;
	}
	ImGui::SameLine();

	if (toolBarButton("PlayPause", playing ? ICON_PAUSE : ICON_PLAY, "Play / Pause", ImVec4(1, 1, 1, 1)))
		playing = !playing;
	ImGui::SameLine();
	ImGui::Checkbox("##preview", &activeMapView->cinematicPlay);


	if (ImGui::BeginChild("KeyframeEditor", ImVec2(-1, 200), true, ImGuiWindowFlags_AlwaysHorizontalScrollbar))
	{
		const ImGuiStyle& style = ImGui::GetStyle();
		const ImGuiIO& IO = ImGui::GetIO();
		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		auto window = ImGui::GetCurrentWindow();

		ImRect bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(leftAreaSize + scale * rsw->cinematicLength, 100));
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
		for (auto& track : rsw->cinematicTracks)
		{
			ImGui::RenderText(bb.Min + ImVec2(10, rowHeight + rowHeight * trackIndex + 2), track.name.c_str()); //todo: center
			ImGui::RenderFrame(bb.Min + ImVec2(leftAreaSize, rowHeight + rowHeight * trackIndex  +1), bb.Min + ImVec2(leftAreaSize + scale * rsw->cinematicLength, rowHeight + rowHeight * trackIndex + rowHeight-2), ImGui::GetColorU32(ImGuiCol_FrameBg, 1), true, style.FrameRounding);
			trackIndex++;
		}

		for (int i = 0; i < rsw->cinematicLength; i++)
		{
			ImGui::RenderText(bb.Min + ImVec2(leftAreaSize + scale * i + 4, 0), std::to_string(i).c_str());
			for (int ii = 0; ii < 10; ii++)
			{
				window->DrawList->AddLine(bb.Min + ImVec2(leftAreaSize + scale * i + scale / 10.0f * ii, rowHeight - 5), bb.Min + ImVec2(leftAreaSize + scale * i + scale / 10.0f * ii, rowHeight), ImGui::GetColorU32(ImGuiCol_TextDisabled, 1.0f));
				window->DrawList->AddLine(bb.Min + ImVec2(leftAreaSize + scale * i + scale / 10.0f * ii, rowHeight), bb.Min + ImVec2(leftAreaSize + scale * i + scale / 10.0f * ii, rowHeight * (rsw->cinematicTracks.size() + 1)), ImGui::GetColorU32(ImGuiCol_TextDisabled, 0.5f));
			}
			window->DrawList->AddLine(bb.Min + ImVec2(leftAreaSize + scale * i, 0), bb.Min + ImVec2(leftAreaSize + scale * i, rowHeight * (rsw->cinematicTracks.size() + 1)), ImGui::GetColorU32(ImGuiCol_Text, 1));

		}

		trackIndex = 0;
		for (auto& track : rsw->cinematicTracks)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_Button, 10));
			int keyframeIndex = 0;
			for (auto keyframe : track.frames)
			{
				ImGui::PushID(keyframe);
				bool selected = selectedTrack >= 0 && selectedTrack < rsw->cinematicTracks.size() && selectedKeyFrame >= 0 && selectedKeyFrame < rsw->cinematicTracks[selectedTrack].frames.size() && keyframe == rsw->cinematicTracks[selectedTrack].frames[selectedKeyFrame];
				if (selected)
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_PlotHistogram, 10));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetColorU32(ImGuiCol_PlotHistogram, 10));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetColorU32(ImGuiCol_PlotHistogram, 10));
				}
				ImGui::SetCursorScreenPos(bb.Min + ImVec2(leftAreaSize + scale * keyframe->time - 5, rowHeight + rowHeight * trackIndex));
				if (ImGui::Button("##", ImVec2(10, rowHeight)))
				{
					selectedTrack = trackIndex;
					selectedKeyFrame = keyframeIndex;
				}
				if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && ImGui::IsItemActive() && selected)
				{
					keyframe->time += ImGui::GetIO().MouseDelta.x / (bb.GetWidth()-leftAreaSize) * rsw->cinematicLength;
					std::sort(rsw->cinematicTracks[selectedTrack].frames.begin(), rsw->cinematicTracks[selectedTrack].frames.end(), [](Rsw::KeyFrame* a, Rsw::KeyFrame* b) { return a->time < b->time; });
					for (auto i = 0; i < rsw->cinematicTracks[selectedTrack].frames.size(); i++)
						if (rsw->cinematicTracks[selectedTrack].frames[i] == keyframe)
							selectedKeyFrame = i;
				}
				if (selected)
					ImGui::PopStyleColor(3);
				keyframeIndex++;
				ImGui::PopID();
			}
			ImGui::PopStyleColor();
			trackIndex++;
		}

		window->DrawList->AddLine(bb.Min + ImVec2(leftAreaSize + scale * timeSelected, 0), bb.Min + ImVec2(leftAreaSize + scale * timeSelected, rowHeight * (rsw->cinematicTracks.size() + 1)), ImGui::GetColorU32(ImGuiCol_Text, 1));
		ImGui::SetCursorScreenPos(bb.Min + ImVec2(leftAreaSize, 0));
		ImGui::InvisibleButton("##", ImVec2(scale * rsw->cinematicLength, rowHeight));
		if (ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
		{
			timeSelected = (ImGui::GetMousePos().x - (bb.Min.x+leftAreaSize)) / (bb.GetWidth() - leftAreaSize) * rsw->cinematicLength;
			if (timeSelected > rsw->cinematicLength)
				timeSelected = rsw->cinematicLength;
		}


		ImGui::SetCursorScreenPos(cursorPos);


		ImGui::EndChild();
	}

	ImGui::End();


	if (ImGui::Begin("Cinematic Mode Properties"))
	{
		bool selected = selectedTrack >= 0 && selectedTrack < rsw->cinematicTracks.size() && selectedKeyFrame >= 0 && selectedKeyFrame < rsw->cinematicTracks[selectedTrack].frames.size();
		if (selected)
		{
			auto keyFrame = rsw->cinematicTracks[selectedTrack].frames[selectedKeyFrame];

			keyFrame->buildEditor();
		}
		ImGui::End();
	}

}