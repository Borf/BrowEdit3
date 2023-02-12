#define IMGUI_DEFINE_MATH_OPERATORS
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE
#include "ModelEditor.h"
#include <browedit/BrowEdit.h>
#include <browedit/Config.h>
#include <browedit/Node.h>
#include <browedit/Icons.h>
#include <browedit/util/ResourceManager.h>
#include <browedit/components/RsmRenderer.h>
#include <browedit/components/Rsm.h>
#include <browedit/gl/FBO.h>
#include <browedit/gl/Texture.h>
#include <browedit/shaders/SimpleShader.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <functional>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <magic_enum.hpp>
#include <imGuIZMOquat.h>
#include <lib/tinygltf/tiny_gltf.h>
#include <glm/glm.hpp>


void ModelEditor::load(const std::string& fileName)
{
	auto node = new Node();
	node->addComponent(util::ResourceManager<Rsm>::load(fileName));
	node->addComponent(new RsmRenderer());

	ModelView mv{ node, new gl::FBO(1024, 1024, true) }; //TODO: resolution? };
	models.push_back(mv);

	simpleShader = util::ResourceManager<gl::Shader>::load<SimpleShader>();
	gridTexture = util::ResourceManager<gl::Texture>::load("data\\grid.png");
	gridTexture->setWrapMode(GL_REPEAT);
}

void ModelEditor::run(BrowEdit* browEdit)
{
	if (!opened)
		return;
	static bool openOpenPopup = false;
	static bool saveModel = false;

	if (ImGui::Begin("ModelEditor", &opened, ImGuiWindowFlags_MenuBar))
	{
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New"))
				{
				}
				if (ImGui::MenuItem("Open"))
					openOpenPopup = true;
				if (ImGui::MenuItem("Save As")) {
					saveModel = true;
				}
				ImGui::EndMenu();
			}
		}
		static std::vector<std::string> modelFiles;
		if (openOpenPopup)
		{
			ImGui::OpenPopup("OpenModel");
			modelFiles = util::FileIO::listAllFiles();
			modelFiles.erase(std::remove_if(modelFiles.begin(), modelFiles.end(), [](const std::string& map) { return map.substr(map.size() - 4, 4) != ".rsm" && map.substr(map.size() - 5, 5) != ".rsm2"; }), modelFiles.end());
			std::sort(modelFiles.begin(), modelFiles.end());
			auto last = std::unique(modelFiles.begin(), modelFiles.end());
			modelFiles.erase(last, modelFiles.end());
		}
		ImGui::SetNextWindowSize(ImVec2(400, 600));
		if (ImGui::BeginPopup("OpenModel"))
		{
			static bool selectFirst = false;
			ImGui::Text("Filter");
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			if(openOpenPopup)
				ImGui::SetKeyboardFocusHere();
			openOpenPopup = false;
			static std::string openFilter = "";
			static std::string lastOpenFilter = "";
			static std::size_t openFileSelected = 0;
			if (ImGui::InputText("##filter", &openFilter, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				load(modelFiles[openFileSelected]);
				ImGui::CloseCurrentPopup();
			}
			if (lastOpenFilter != openFilter)
			{
				selectFirst = true;
				lastOpenFilter = openFilter;
			}
			if (ImGui::BeginListBox("##Files", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - 40)))
			{
				for (std::size_t i = 0; i < modelFiles.size(); i++)
				{
					if (openFilter == "" || modelFiles[i].find(openFilter) != std::string::npos)
					{
						if (selectFirst)
						{
							selectFirst = false;
							openFileSelected = i;
						}
						if (ImGui::Selectable(util::iso_8859_1_to_utf8(modelFiles[i]).c_str(), openFileSelected == i))
							openFileSelected = i;
						if (ImGui::IsItemClicked(0) && ImGui::IsMouseDoubleClicked(0))
						{
							load(modelFiles[openFileSelected]);
							ImGui::CloseCurrentPopup();
						}
					}
				}
				ImGui::EndListBox();
			}

			if (ImGui::Button("Open"))
			{
				load(modelFiles[openFileSelected]);
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
				ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}
		ImGui::EndMenuBar();

		ImGui::DockSpace(ImGui::GetID("ModelEdit"));
	}
	ImGui::End();

	static float timeSelected = 0.0f;
	static bool playing = true;
	static ModelView* activeModelViewPtr = nullptr; // :(

	if (playing)
		timeSelected += ImGui::GetIO().DeltaTime*1000;

	for (auto &m : models)
	{
		auto rsm = m.node->getComponent<Rsm>();
		auto rsmRenderer = m.node->getComponent<RsmRenderer>();
		if (ImGui::Begin(util::iso_8859_1_to_utf8(rsm->fileName).c_str()))
		{
			if (ImGui::IsWindowFocused()) {
				activeModelViewPtr = &m;
			}
			auto size = ImGui::GetContentRegionAvail();
			auto rsm = m.node->getComponent<Rsm>();
			if (!rsm->loaded)
			{
				ImGui::End();
				continue;
			}
			// Save current model
			if (saveModel && activeModelViewPtr == &m ) {
				saveModel = false;
				std::string path = util::SaveAsDialog("", "Rsm\0*.rsm\0");
				if (path != "") {
					if (path.size() < 4 || path.substr(path.size() - 4) != ".rsm")
						path += ".rsm";
					rsm->save(path);
				}
			}
			if (m.fbo->getWidth() != size.x || m.fbo->getHeight() != size.y)
				m.fbo->resize((int)size.x, (int)size.y);

			rsmRenderer->time = fmod(timeSelected/1000.0f, rsm->animLen/1000.0f);

			if (size.x > 0 && size.y > 0)
			{
				m.fbo->bind();
				glViewport(0, 0, m.fbo->getWidth(), m.fbo->getHeight());
				glClearColor(browEdit->config.backgroundColor.r, browEdit->config.backgroundColor.g, browEdit->config.backgroundColor.b, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				glDisable(GL_CULL_FACE);
				glEnable(GL_DEPTH_TEST);
				glEnable(GL_BLEND);
				glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);


				float distance = 1.5f * glm::max(glm::abs(rsm->realbbmax.y), glm::max(glm::abs(rsm->realbbmax.z), glm::abs(rsm->realbbmax.x)));
				float ratio = m.fbo->getWidth() / (float)m.fbo->getHeight();
				nodeRenderContext.projectionMatrix = glm::perspective(glm::radians(45.0f), ratio, 0.1f, 5000.0f);

				nodeRenderContext.viewMatrix = glm::mat4(1.0f);
				nodeRenderContext.viewMatrix = glm::translate(nodeRenderContext.viewMatrix, glm::vec3(0, 0, -m.camera.z));
				nodeRenderContext.viewMatrix = glm::rotate(nodeRenderContext.viewMatrix, -glm::radians(m.camera.x), glm::vec3(1, 0, 0));
				nodeRenderContext.viewMatrix = glm::rotate(nodeRenderContext.viewMatrix, -glm::radians(m.camera.y), glm::vec3(0, 1, 0));
				if(rsm->version >= 0x0202)
					nodeRenderContext.viewMatrix = glm::translate(nodeRenderContext.viewMatrix, glm::vec3(0, rsm->bbrange.y, 0));
				else
					nodeRenderContext.viewMatrix = glm::translate(nodeRenderContext.viewMatrix, glm::vec3(0, -rsm->bbrange.y, 0));

				// draw grid
				simpleShader->use();
				simpleShader->setUniform(SimpleShader::Uniforms::projectionMatrix, nodeRenderContext.projectionMatrix);
				simpleShader->setUniform(SimpleShader::Uniforms::viewMatrix, nodeRenderContext.viewMatrix);
				simpleShader->setUniform(SimpleShader::Uniforms::modelMatrix, glm::mat4(1.0f));
				simpleShader->setUniform(SimpleShader::Uniforms::textureFac, 1.0f);
				simpleShader->setUniform(SimpleShader::Uniforms::colorMult, glm::vec4(1.0f));
				simpleShader->setUniform(SimpleShader::Uniforms::lightMin, 1.0f);
				gridTexture->bind();

				std::vector<VertexP3T2N3> verts;
				verts.push_back(VertexP3T2N3(glm::vec3(-100, 0, -100), glm::vec2(0, 0), glm::vec3(0.0f, 1.0f, 0.0f)));
				verts.push_back(VertexP3T2N3(glm::vec3( 100, 0, -100), glm::vec2(8, 0), glm::vec3(0.0f, 1.0f, 0.0f)));
				verts.push_back(VertexP3T2N3(glm::vec3( 100, 0,  100), glm::vec2(8, 8), glm::vec3(0.0f, 1.0f, 0.0f)));
				verts.push_back(VertexP3T2N3(glm::vec3(-100, 0,  100), glm::vec2(0, 8), glm::vec3(0.0f, 1.0f, 0.0f)));
				glEnableVertexAttribArray(0);
				glEnableVertexAttribArray(1);
				glEnableVertexAttribArray(2);
				glDisableVertexAttribArray(3);
				glDisableVertexAttribArray(4); //TODO: vao
				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data);
				glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data + 3);
				glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(VertexP3T2N3), verts[0].data + 5);
				glDrawArrays(GL_QUADS, 0, (int)verts.size());




				RsmRenderer::RsmRenderContext::getInstance()->viewLighting = false;
				RsmRenderer::RsmRenderContext::getInstance()->viewTextures = true;
				RsmRenderer::RsmRenderContext::getInstance()->viewFog = false;
				auto rsmRenderer = m.node->getComponent<RsmRenderer>();
				for (auto i = 0; i < rsmRenderer->renderInfo.size(); i++)
					rsmRenderer->renderInfo[i].selected = m.selectedMesh && i == m.selectedMesh->index;
				rsmRenderer->matrixCache = glm::mat4(1.0f);// rotate(glm::mat4(1.0f), glm::radians((float)ImGui::GetTime() * 100), glm::vec3(0, 1, 0));
				NodeRenderer::render(m.node, nodeRenderContext);
				m.fbo->unbind();
				ImGui::ImageButton((ImTextureID)(long long)m.fbo->texid[0], size, ImVec2(1, 0), ImVec2(0, 1), 0, ImVec4(0, 0, 0, 1), ImVec4(1, 1, 1, 1));
				if (ImGui::IsItemHovered())
				{
					if ((ImGui::IsMouseDown(ImGuiMouseButton_Right) || ImGui::IsMouseDown(ImGuiMouseButton_Middle)))
					{
						m.camera.x += ImGui::GetIO().MouseDelta.y;
						m.camera.y += ImGui::GetIO().MouseDelta.x;
					}
					m.camera.z *= (1 - (ImGui::GetIO().MouseWheel * 0.1f));
				}
			}
		}
		ImGui::End();
	}

	if(models.size() == 0 || !activeModelViewPtr)
		return;

	auto& activeModelView = *activeModelViewPtr;
	auto rsm = activeModelView.node->getComponent<Rsm>();
	if (!rsm->loaded)
		return;
	auto rsmRenderer = activeModelView.node->getComponent<RsmRenderer>();

	static float leftAreaSize = 200.0f;
	static float scale = 0.1f;
	static float rowHeight = 25;
	static Rsm::Mesh::Frame* selectedFrame = nullptr;

	int rowPerTrack = 1;
	if (rsm->version >= 0x0106)
		rowPerTrack = 2;
	if (rsm->version >= 0x0202)
		rowPerTrack = 3;

	timeSelected = (float)std::fmod(timeSelected, rsm->animLen);

	if (ImGui::Begin("ModelEditorTimeline"))
	{
		//TODO: check if the RSM version supports translation / rotation / scale animations

		const ImGuiStyle& style = ImGui::GetStyle();
		const ImGuiIO& IO = ImGui::GetIO();


		ImGui::SetNextItemWidth(150);
		ImGui::DragFloat("##CurrentTime", &timeSelected, 0.1f, 0, 10000.0f);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Animation time");
		ImGui::SameLine();
		if (browEdit->toolBarButton("Prev", ICON_PREV, "Previous Keyframe", ImVec4(1, 1, 1, 1)))
		{
			//if (selectedTrack >= 0 && selectedTrack < rsw->cinematicTracks.size() && selectedKeyFrame > 0)
			//	selectedKeyFrame--;
		}
		ImGui::SameLine();
		if (browEdit->toolBarButton("PlayPause", playing ? ICON_PAUSE : ICON_PLAY, "Play / Pause", ImVec4(1, 1, 1, 1)))
		{
			playing = !playing;
		}
		ImGui::SameLine();
		if (browEdit->toolBarButton("Next", ICON_NEXT, "Nexts Keyframe", ImVec4(1, 1, 1, 1)))
		{
			//if (selectedTrack >= 0 && selectedTrack < rsw->cinematicTracks.size() && selectedKeyFrame < rsw->cinematicTracks[selectedTrack].frames.size() - 1)
			//	selectedKeyFrame++;
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(150);
		ImGui::DragFloat("##Scale", &scale, 0.01f, 0.00001f, 10, "%.3f", ImGuiSliderFlags_Logarithmic);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Scale");
		if (scale <= 0)
			scale = 0.000001f;

		int trackCount = 0;
		if (rsm)
			rsm->rootMesh->foreach([&trackCount](Rsm::Mesh*) {trackCount++; });


		if (rsm->animLen > 0 && ImGui::BeginChild("KeyframeEditor", ImVec2(-1, (trackCount*3+1) * rowHeight + 34 ), true, ImGuiWindowFlags_AlwaysHorizontalScrollbar))
		{
			ImDrawList* DrawList = ImGui::GetWindowDrawList();
			auto window = ImGui::GetCurrentWindow();

			ImRect bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(leftAreaSize + scale * rsm->animLen, (trackCount*3+1)*rowHeight));
			ImGui::ItemSize(bb);
			if (ImGui::ItemAdd(bb, NULL))
			{
				ImVec2 cursorPos = ImGui::GetCursorScreenPos();
				ImGui::RenderFrame(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_PopupBg, 1), true, style.FrameRounding);

				int trackIndex = 0;
				std::function<void(Rsm::Mesh* mesh, int level)> drawTrack;
				//TODO: put the labels in a second subitem so only the keyframe things scroll
				drawTrack = [&](Rsm::Mesh* mesh, int level)
				{
					ImGui::RenderText(bb.Min + ImVec2(10.0f + 20.0f * level, rowHeight + rowPerTrack * rowHeight * trackIndex + 2.0f), mesh->name.c_str());
					ImGui::RenderText(bb.Min + ImVec2(130.0f, 1*rowHeight + rowPerTrack * rowHeight * trackIndex + 2.0f), "Rotation");
					if(rsm->version >= 0x0106)
						ImGui::RenderText(bb.Min + ImVec2(130.0f, 2*rowHeight + rowPerTrack * rowHeight * trackIndex + 2.0f), "Scale");
					if (rsm->version >= 0x0202)
						ImGui::RenderText(bb.Min + ImVec2(130.0f, 3*rowHeight + rowPerTrack * rowHeight * trackIndex + 2.0f), "Position");
					ImGui::RenderFrame(bb.Min + ImVec2(leftAreaSize, rowHeight + rowPerTrack *rowHeight * trackIndex + 1),
										bb.Min + ImVec2(leftAreaSize + scale * rsm->animLen, rowHeight + rowPerTrack *rowHeight * trackIndex + rowHeight - 2), ImGui::GetColorU32(ImGuiCol_FrameBg, 1), true, style.FrameRounding);
					trackIndex++;
					for (auto m : mesh->children)
						drawTrack(m, level + 1);
				};
				drawTrack(rsm->rootMesh, 0);

				float lastLabel = -10000;
				for (int i = 0; i < rsm->animLen; i += 100)
				{
					if (scale >= 1)
					{
						for (int ii = 0; ii < 100; ii += 10)
						{
							window->DrawList->AddLine(bb.Min + ImVec2(leftAreaSize + scale * i + scale * ii, rowHeight - 5), bb.Min + ImVec2(leftAreaSize + scale * i + scale * ii, rowHeight), ImGui::GetColorU32(ImGuiCol_TextDisabled, 1.0f));
							window->DrawList->AddLine(bb.Min + ImVec2(leftAreaSize + scale * i + scale * ii, rowHeight), bb.Min + ImVec2(leftAreaSize + scale * i + scale * ii, rowHeight * (rowPerTrack * trackCount + 1)), ImGui::GetColorU32(ImGuiCol_TextDisabled, 0.5f));
						}
					}
					if (scale * i - lastLabel > 100)
					{
						ImGui::RenderText(bb.Min + ImVec2(leftAreaSize + scale * i, 0), std::to_string(i).c_str());
						window->DrawList->AddLine(bb.Min + ImVec2(leftAreaSize + scale * i, 0), bb.Min + ImVec2(leftAreaSize + scale * i, rowHeight * (rowPerTrack * trackCount + 1)), ImGui::GetColorU32(ImGuiCol_Text, 1));
						lastLabel = scale * i;
					}
				}


				window->DrawList->AddLine(bb.Min + ImVec2(leftAreaSize + scale * timeSelected, 0), bb.Min + ImVec2(leftAreaSize + scale * timeSelected, rowHeight * (rowPerTrack * trackCount + 1)), ImGui::GetColorU32(ImGuiCol_Text, 1));
				ImGui::SetCursorScreenPos(bb.Min + ImVec2(leftAreaSize, 0));
				ImGui::InvisibleButton("##", ImVec2(scale* rsm->animLen, rowHeight));
				if (ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
				{
					timeSelected = (ImGui::GetMousePos().x - (bb.Min.x + leftAreaSize)) / (bb.GetWidth() - leftAreaSize) * rsm->animLen;
					if (timeSelected > rsm->animLen)
						timeSelected = (float)rsm->animLen;
					if (timeSelected < 0)
						timeSelected = 0;
				}



				trackIndex = 0;
				std::function<void(Rsm::Mesh* mesh)> drawKeyframes;
				//TODO: put the labels in a second subitem so only the keyframe things scroll
				drawKeyframes = [&](Rsm::Mesh* mesh)
				{
					float y = (3 * trackIndex + 1) * rowHeight;
					auto drawFrame = [&](Rsm::Mesh::Frame& frame, int yoffset)
					{
						ImGui::PushID(&frame);
						ImGui::SetCursorScreenPos(bb.Min + ImVec2(leftAreaSize + scale * frame.time - 5, rowHeight + (rowPerTrack * rowHeight + yoffset) * trackIndex));
						if (ImGui::Button("##", ImVec2(10, rowHeight)))
						{
							selectedFrame = &frame;
						}
						ImGui::PopID();
					};


					for (auto& f : mesh->rotFrames)
						drawFrame(f, 0);
					if(rsm->version >= 0x0106)
						for (auto& f : mesh->scaleFrames)
							drawFrame(f, 1);
					if(rsm->version >= 0x0202)
						for (auto& f : mesh->posFrames)
							drawFrame(f, 2);


					trackIndex++;
					for (auto m : mesh->children)
						drawKeyframes(m);
				};
				ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_Button, 10));
				drawKeyframes(rsm->rootMesh);
				ImGui::PopStyleColor();


			}
			ImGui::EndChild();
		}
	}
	ImGui::End();

	if (ImGui::Begin("ModelEditorTimelineProperties"))
	{
		if (selectedFrame != nullptr)
		{
			ImGui::InputInt("Time", &selectedFrame->time);
			auto rotFrame = dynamic_cast<Rsm::Mesh::RotFrame*>(selectedFrame);
			if (rotFrame)
			{
				ImGui::InputFloat4("Quaternion", glm::value_ptr(rotFrame->quaternion));
				ImGui::gizmo3D("Rotation", rotFrame->quaternion);
			}
		}
	}
	ImGui::End();

	if (ImGui::Begin("ModelEditorNodes"))
	{
		std::function<void(Rsm::Mesh*)> buildTree;
		buildTree = [&buildTree,&activeModelView, &rsm, &rsmRenderer](Rsm::Mesh* mesh) {
			bool empty = mesh->children.empty();

			int flags = empty ? (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet) : (ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnDoubleClick);
			if (activeModelView.selectedMesh == mesh)
				flags |= ImGuiTreeNodeFlags_Selected;

			bool opened = ImGui::TreeNodeEx(mesh->name.c_str(), flags);
			if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
				activeModelView.selectedMesh = mesh;
			if (ImGui::BeginPopupContextItem())
			{
				if (ImGui::MenuItem("Add child node"))
				{
					auto subMesh = new Rsm::Mesh(rsm);
					subMesh->name = "new";
					subMesh->index = 0;
					subMesh->scale = glm::vec3(1.0f);
					subMesh->offset = glm::mat4(1.0f);
					rsm->rootMesh->foreach([&subMesh](Rsm::Mesh* m) { subMesh->index = glm::max(subMesh->index, m->index + 1); });
					mesh->children.push_back(subMesh);
					rsm->meshCount++;
					rsmRenderer->setMeshesDirty();
				}
				ImGui::MenuItem("Copy");
				ImGui::MenuItem("Paste");
				ImGui::MenuItem("Delete");
				ImGui::EndPopup();
			}
			if (opened)
			{
				for (auto c : mesh->children)
					buildTree(c);
				ImGui::TreePop();
			}
		};


		bool opened = ImGui::TreeNodeEx(util::iso_8859_1_to_utf8(rsm->fileName).c_str(), (ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnDoubleClick) | (activeModelView.selectedMesh == nullptr ? ImGuiTreeNodeFlags_Selected : 0));
		if (ImGui::IsItemClicked())
			activeModelView.selectedMesh = nullptr;
		if(opened)
		{
			buildTree(rsm->rootMesh);
			ImGui::TreePop();
		}
	}
	ImGui::End();

	if (ImGui::Begin("ModelEditorProperties"))
	{
		if (activeModelView.selectedMesh == nullptr)
		{
			char versionStr[10];
			sprintf_s(versionStr, 10, "%04x", rsm->version);
			if (ImGui::BeginCombo("Version", versionStr))
			{
				if (ImGui::Selectable("0101", rsm->version == 0x0101))
				{
					rsm->version = 0x0101;
					rsmRenderer->setMeshesDirty();
				}
				if (ImGui::Selectable("0102", rsm->version == 0x0102))
				{
					rsm->version = 0x0102;
					rsmRenderer->setMeshesDirty();
				}
				if (ImGui::Selectable("0103", rsm->version == 0x0103))
				{
					rsm->version = 0x0103;
					rsmRenderer->setMeshesDirty();
				}
				if (ImGui::Selectable("0104", rsm->version == 0x0104))
				{
					rsm->version = 0x0104;
					rsmRenderer->setMeshesDirty();
				}
				if (ImGui::Selectable("0105", rsm->version == 0x0105))
				{
					rsm->version = 0x0105;
					rsmRenderer->setMeshesDirty();
				}
				if (ImGui::Selectable("0106", rsm->version == 0x0106))
				{
					rsm->version = 0x0106;
					rsmRenderer->setMeshesDirty();
				}
				if (ImGui::Selectable("0202", rsm->version == 0x0202))
				{
					rsm->version = 0x0202;
					rsmRenderer->setMeshesDirty();
				}
				if (ImGui::Selectable("0203", rsm->version == 0x0203))
				{
					rsm->version = 0x0203;
					rsmRenderer->setMeshesDirty();
				}
				ImGui::EndCombo();
			}
			if (rsm->version > 0x0104)
				ImGui::InputScalar("Alpha", ImGuiDataType_U8, &rsm->alpha);

			ImGui::InputInt("Animation Length", &rsm->animLen);

			if(rsm->version >= 0x0202)
				ImGui::InputFloat("FPS", &rsm->fps);

			if (ImGui::BeginCombo("Shade Type", std::string(magic_enum::enum_name(rsm->shadeType)).c_str()))
			{
				for (const auto& e : magic_enum::enum_entries<Rsm::ShadeType>())
				{
					if (ImGui::Selectable(std::string(e.second).c_str(), rsm->shadeType == e.first))
						rsm->shadeType = e.first;
				}
				ImGui::EndCombo();
			}

			if (ImGui::CollapsingHeader("Textures", ImGuiTreeNodeFlags_DefaultOpen))
			{
				int i = 0;
				for (auto& t : rsm->textures)
				{
					ImGui::PushID( "texture" + i++);
					std::string texture = util::iso_8859_1_to_utf8(t);
					if (ImGui::InputText("Texture", &texture)) {
						t = util::utf8_to_iso_8859_1(texture);
					}
					
					if (ImGui::Button("Load")) 
					{
						rsmRenderer->setMeshesDirty();
					}
					ImGui::PopID();
				}
			}

		}
		else //activeModelView.selectedMesh selected
		{
			ImGui::InputText("Name", &activeModelView.selectedMesh->name);
			if(rsm->version < 0x0202)
				if (ImGui::DragFloat3("Position", glm::value_ptr(activeModelView.selectedMesh->pos), 0.1f))
					rsmRenderer->setMeshesDirty();
			if (ImGui::InputFloat4("Offset1", glm::value_ptr(activeModelView.selectedMesh->offset) + 0))
				rsmRenderer->setMeshesDirty();
			if(ImGui::InputFloat4("Offset2", glm::value_ptr(activeModelView.selectedMesh->offset) + 4))
				rsmRenderer->setMeshesDirty();
			if(ImGui::InputFloat4("Offset3", glm::value_ptr(activeModelView.selectedMesh->offset) + 8))
				rsmRenderer->setMeshesDirty();
			if(ImGui::InputFloat4("Offset4", glm::value_ptr(activeModelView.selectedMesh->offset) + 12))
				rsmRenderer->setMeshesDirty();

			if(ImGui::DragFloat3("Position_", glm::value_ptr(activeModelView.selectedMesh->pos_), 0.1f))
				rsmRenderer->setMeshesDirty();
			if (rsm->version < 0x0202)
				if(ImGui::DragFloat("Rotation Angle", &activeModelView.selectedMesh->rotangle, 0.02f, 0.0f, 2 * glm::pi<float>()))
					rsmRenderer->setMeshesDirty();
			if (rsm->version < 0x0202)
				if(ImGui::InputFloat3("Rotation Axis", glm::value_ptr(activeModelView.selectedMesh->rotaxis)))
					rsmRenderer->setMeshesDirty();
			if (rsm->version < 0x0202)
				if(ImGui::DragFloat3("Scale", glm::value_ptr(activeModelView.selectedMesh->scale), 0.1f))
					rsmRenderer->setMeshesDirty();

			ImGui::LabelText("Face count", "%d", activeModelView.selectedMesh->faces.size());

			bool differentValues = false;
			for (const auto& f : activeModelView.selectedMesh->faces)
				if (f.twoSided != activeModelView.selectedMesh->faces[0].twoSided)
					differentValues = true;
			bool twoSided = false;
			if(activeModelView.selectedMesh->faces.size() > 0)
				twoSided = activeModelView.selectedMesh->faces[0].twoSided != 0;
			ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, differentValues);
			if (ImGui::Checkbox("Two-sided faces", &twoSided))
				for (auto& f : activeModelView.selectedMesh->faces)
					f.twoSided = twoSided ? 1 : 0;
			ImGui::PopItemFlag();

			if (ImGui::CollapsingHeader("Textures", ImGuiTreeNodeFlags_DefaultOpen))
			{
				int i = 0;
				for (auto& t : activeModelView.selectedMesh->textures)
				{
					ImGui::PushID(t);
					if (ImGui::BeginCombo(("Texture " + std::to_string(i)).c_str(), util::iso_8859_1_to_utf8(rsm->textures[t]).c_str()))
					{
						for (auto ii = 0; ii < rsm->textures.size(); ii++)
						{
							if (ImGui::Selectable(util::iso_8859_1_to_utf8(rsm->textures[ii]).c_str(), t == ii))
							{
								activeModelView.selectedMesh->textures[i] = ii;
								rsmRenderer->setMeshesDirty();
							}
						}
						ImGui::EndCombo();
					}					
					ImGui::PopID();
					i++;
				}


			}

			if (ImGui::Button("Import file"))
			{
				bool loaded = false;
				std::string filename = "";
				std::string path = util::SelectFileDialog(filename, "Glb\0*.glb\0");
				tinygltf::Model model;
				std::string err, warn;
				if (path != "") {
					tinygltf::TinyGLTF loader;
					loaded = loader.LoadBinaryFromFile(&model, &err, &warn, path);
				}
				if(loaded)
				{
					std::cout << "Loaded file " << filename << warn << std::endl;
					bool ok = true;
					if (model.meshes.size() != 1)
					{
						std::cout << "This file has too many models" << std::endl;
						ok = false;
					}

					if (ok)
					{
						auto& mesh = activeModelView.selectedMesh;
						mesh->vertices.clear();
						mesh->texCoords.clear();
						mesh->faces.clear();
						mesh->textures.clear();
						std::set<int> texturesUsed;
						std::map<int, int> textureMap;
						for (auto& primitive : model.meshes[0].primitives)
							texturesUsed.insert(model.materials[primitive.material].values["baseColorTexture"].TextureIndex());
						for (auto& tex : texturesUsed)
						{
							textureMap[tex] = (int)mesh->textures.size();
							mesh->textures.push_back((int)rsm->textures.size());
							rsm->textures.push_back("texture.bmp");
						}


						for (auto& primitive : model.meshes[0].primitives)
						{
							// primitive.mode 4 = triangles
							auto& material = model.materials[primitive.material];

							int posAccessorIndex = primitive.attributes["POSITION"];
							int texCoordAccessorIndex = primitive.attributes["TEXCOORD_0"];

							auto& posAccessor = model.accessors[posAccessorIndex];
							auto& texCoordAccessor = model.accessors[texCoordAccessorIndex];
							auto& indicesAccessor = model.accessors[primitive.indices];

							auto& posBufferView = model.bufferViews[posAccessor.bufferView];
							auto& texCoordBufferView = model.bufferViews[texCoordAccessor.bufferView];
							auto& indicesBufferView = model.bufferViews[indicesAccessor.bufferView];

							auto& posBuffer = model.buffers[posBufferView.buffer];
							auto& texCoordBuffer = model.buffers[texCoordBufferView.buffer];
							auto& indicesBuffer = model.buffers[indicesBufferView.buffer];

							if (ok && (
								texCoordAccessor.type != TINYGLTF_TYPE_VEC2 ||
								posAccessor.type != TINYGLTF_TYPE_VEC3 ||
								texCoordAccessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT ||
								posAccessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT))
							{
								std::cout << "Weird vertex format" << std::endl;
								ok = false;
							}
							if (ok)
							{
								auto vertexStart = mesh->vertices.size();
								unsigned char* pos = posBuffer.data.data() + posBufferView.byteOffset + posAccessor.byteOffset;
								auto posStride = posBufferView.byteStride;
								posStride += 3 * sizeof(float);
								unsigned char* tex = texCoordBuffer.data.data() + texCoordBufferView.byteOffset + texCoordAccessor.byteOffset;
								auto texStride = texCoordBufferView.byteStride;
								texStride += 2 * sizeof(float);

								for (unsigned char* p = pos; p < pos + posBufferView.byteLength; p += posStride)
									mesh->vertices.push_back(glm::make_vec3((float*)p));
								for (unsigned char* t = tex; t < tex + texCoordBufferView.byteLength; t += texStride)
									mesh->texCoords.push_back(glm::make_vec2((float*)t));

								unsigned char* index = indicesBuffer.data.data() + indicesAccessor.byteOffset + indicesBufferView.byteOffset;
								auto indicesStride = indicesBufferView.byteStride;
								if (indicesStride == 0)
									indicesStride = indicesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ? sizeof(short) : sizeof(int);

								Rsm::Mesh::Face f;

								auto texIndex = material.values["baseColorTexture"].TextureIndex();

								f.texId = textureMap[texIndex];
								f.twoSided = material.doubleSided;
								int faceIndex = 0;
								for (size_t i = 0; i < indicesBufferView.byteLength; i += indicesStride)
								{
									int ind = 0;
									if (indicesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_SHORT)
										ind = *((short*)(index + i));
									else if (indicesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
										ind = *((unsigned short*)(index + i));
									else if (indicesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_INT)
										ind = *((int*)(index + i));
									else if (indicesAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
										ind = *((unsigned int*)(index + i));

									f.vertexIds[faceIndex] = (short)(vertexStart + ind);
									f.texCoordIds[faceIndex] = (short)(vertexStart + ind);
									faceIndex++;
									if (primitive.mode == TINYGLTF_MODE_TRIANGLES)
									{
										if (faceIndex == 3)
										{
											f.normal = glm::normalize(glm::cross(mesh->vertices[f.vertexIds[1]] - mesh->vertices[f.vertexIds[0]], mesh->vertices[f.vertexIds[2]] - mesh->vertices[f.vertexIds[0]]));
											mesh->faces.push_back(f);
											faceIndex = 0;
										}
									}
									else
										std::cout << "Unknown primitive mode" << std::endl;
								}
								rsm->updateMatrices();
								rsmRenderer->setMeshesDirty();
							}
						}
					}
				}
				else
					std::cout << "Error loading file " << filename << " " << err << ", " << warn << std::endl;
			}

		}

	}
	ImGui::End();
}

