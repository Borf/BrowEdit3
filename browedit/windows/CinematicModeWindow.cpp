#define IMGUI_DEFINE_MATH_OPERATORS
#include <browedit/BrowEdit.h>
#include <browedit/Icons.h>
#include <browedit/Map.h>
#include <browedit/MapView.h>
#include <browedit/Node.h>
#include <browedit/gl/Texture.h>
#include <browedit/gl/FBO.h>
#include <browedit/components/Rsw.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <stb/stb_image_write.h>

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
	ImGui::SameLine();
	if (ImGui::Button("•"))
	{
		ImGui::OpenPopup("RecordVideo");
	}
	if (ImGui::BeginPopupModal("RecordVideo"))
	{
		static float fps = 30;
		static int quality = 25;
		static std::string videoFile = "video.mkv";
		static int resolution[2] = { 1920,1080 };
		ImGui::DragFloat("Fps", &fps, 1, 1, 120);
		ImGui::InputText("Filename", &videoFile);
		ImGui::InputInt2("Resolution", resolution);
		ImGui::InputInt("Quality\n(lower = better)", &quality);
		
		if (ImGui::Button("Record"))
		{
			while(true) {
				std::string path = config.ffmpegPath + " ";
				path += "-y ";
				//path += "-f image2pipe ";
				//path += "-vcodec png ";
				path += "-f rawvideo ";
				path += "-s "+std::to_string(resolution[0]) + "x"+std::to_string(resolution[1]) + " -pix_fmt rgb24 ";
				path += "-r " + std::to_string(fps) + " "; // FPS
				path += "-i - ";
				path += "-pix_fmt yuv420p ";
				path += "-c:v libx265 -crf "+std::to_string(quality) + " ";
				path += "-pix_fmt yuv420p ";
				path += "-nostats ";
				path += videoFile;


				HANDLE hPipeReadStdIn, hPipeWriteStdIn;
				HANDLE hPipeReadStdOut, hPipeWriteStdOut;

				SECURITY_ATTRIBUTES saAttr = { sizeof(SECURITY_ATTRIBUTES) };
				saAttr.bInheritHandle = TRUE; // Pipe handles are inherited by child process.
				saAttr.lpSecurityDescriptor = NULL;

				if (!CreatePipe(&hPipeReadStdOut, &hPipeWriteStdOut, &saAttr, 0))
				{
					std::cout << "Error creating pipe" << std::endl;
					break;
				}

				if (!CreatePipe(&hPipeReadStdIn, &hPipeWriteStdIn, &saAttr, 0))
				{
					std::cout << "Error creating pipe" << std::endl;
					break;
				}
				SetHandleInformation(hPipeWriteStdIn, HANDLE_FLAG_INHERIT, 0);

				STARTUPINFO si = { sizeof(STARTUPINFO) };
				si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
				si.hStdOutput = hPipeWriteStdOut;
				si.hStdError = hPipeWriteStdOut;
				si.hStdInput = hPipeReadStdIn;

				PROCESS_INFORMATION pi = { 0 };

				BOOL fSuccess = CreateProcess(NULL, (LPSTR)path.c_str(), NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
				if (!fSuccess)
				{
					std::cout << "Could not start ffmpeg" << std::endl;
					CloseHandle(hPipeWriteStdIn);
					CloseHandle(hPipeReadStdIn);
					break;
				}

				std::thread t([&]()
				{
						while (true)
						{
							char buf[1024];
							DWORD read;
							auto success = ::ReadFile(hPipeReadStdOut, buf, 1024, &read, nullptr);
							if (!success)
								break;
							std::cout << std::string(buf, read);
						}
				});
				Sleep(100);

				int totalFrames = (int)(fps * rsw->cinematicLength);

				auto& mv = *activeMapView;
				mv.fbo->resize(resolution[0], resolution[1]);




				for (int i = 0; i < totalFrames; i++)
				{
					float time = i * 1.0f / fps;
					std::cout << "Frame\t" << i <<", "<< time << "s\t";

					mv.cinematicPlay = true;
					timeSelected = time;
					int len = resolution[0] * resolution[1] * 3;
					mv.render(this);
					//char* frameData = mv.fbo->saveToMemoryPng(len);
					char* frameData = mv.fbo->saveToMemoryRaw();
					std::cout << len << " bytes" << std::endl;
					DWORD dwWritten;
					::WriteFile(hPipeWriteStdIn, frameData, len, &dwWritten, NULL);
					delete[] frameData;
				}
				CloseHandle(hPipeWriteStdOut);
				CloseHandle(hPipeReadStdOut);
				CloseHandle(hPipeWriteStdIn);
				CloseHandle(hPipeReadStdIn);
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
				t.join();
				break;
			}
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}


	if (rsw->cinematicLength > 0 && ImGui::BeginChild("KeyframeEditor", ImVec2(-1, 200), true, ImGuiWindowFlags_AlwaysHorizontalScrollbar))
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
			if (ImGui::IsMouseHoveringRect(bb.Min + ImVec2(leftAreaSize, rowHeight + rowHeight * trackIndex + 1), bb.Min + ImVec2(leftAreaSize + scale * rsw->cinematicLength, rowHeight + rowHeight * trackIndex + rowHeight - 2)) && ImGui::IsMouseDoubleClicked(0))
			{
				float t = (ImGui::GetMousePos().x - (bb.Min.x + leftAreaSize)) / (bb.GetWidth() - leftAreaSize) * rsw->cinematicLength;
				Rsw::KeyFrame* keyframe = nullptr;
				if (trackIndex == 0)
					keyframe = new Rsw::KeyFrameData<std::pair<glm::vec3, glm::vec3>>(t, std::pair<glm::vec3, glm::vec3>(glm::vec3(0, 0, 0), glm::vec3(0, 0, 0)));
				else if (trackIndex == 1)
					keyframe = new Rsw::KeyFrameData<Rsw::CameraTarget>(t, Rsw::CameraTarget());
				rsw->cinematicTracks[trackIndex].frames.push_back(keyframe);
				selectedTrack = trackIndex;
				std::sort(rsw->cinematicTracks[selectedTrack].frames.begin(), rsw->cinematicTracks[selectedTrack].frames.end(), [](Rsw::KeyFrame* a, Rsw::KeyFrame* b) { return a->time < b->time; });
				for (auto i = 0; i < rsw->cinematicTracks[selectedTrack].frames.size(); i++)
					if (rsw->cinematicTracks[selectedTrack].frames[i] == keyframe)
						selectedKeyFrame = i;
			}
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
				if (ImGui::BeginPopupContextItem("KeyFramePopup"))
				{
					if (ImGui::MenuItem("Delete"))
					{
						ImGui::EndPopup();
						track.frames.erase(track.frames.begin() + keyframeIndex);
						keyframeIndex--;
						if (selected)
							ImGui::PopStyleColor(3);
						ImGui::PopID();
						continue;
					}
					ImGui::EndPopup();
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
			if (timeSelected < 0)
				timeSelected = 0;
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