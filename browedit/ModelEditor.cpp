#define IMGUI_DEFINE_MATH_OPERATORS
#include "ModelEditor.h"
#include <browedit/BrowEdit.h>
#include <browedit/Config.h>
#include <browedit/util/ResourceManager.h>
#include <browedit/components/RsmRenderer.h>
#include <browedit/components/Rsm.h>
#include <browedit/Node.h>
#include <browedit/gl/FBO.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <functional>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <magic_enum.hpp>
#include <browedit/Icons.h>

void ModelEditor::load(const std::string& fileName)
{
	auto node = new Node();
	node->addComponent(util::ResourceManager<Rsm>::load(fileName));
	node->addComponent(new RsmRenderer());

	ModelView mv{ node, new gl::FBO(1024, 1024, true) }; //TODO: resolution? };
	models.push_back(mv);

}


void ModelEditor::run(BrowEdit* browEdit)
{
	if (!opened)
		return;

	if (ImGui::Begin("ModelEditor", &opened, ImGuiWindowFlags_MenuBar))
	{
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New"))
					;
				if (ImGui::MenuItem("Open"))
					;
				if (ImGui::MenuItem("Save"))
					;
				ImGui::EndMenu();
			}
		}
		ImGui::EndMenuBar();

		ImGui::DockSpace(ImGui::GetID("ModelEdit"));
	}
	ImGui::End();

	
	for (auto &m : models)
	{
		auto rsm = m.node->getComponent<Rsm>();
		if (ImGui::Begin(rsm->fileName.c_str()))
		{
			auto size = ImGui::GetContentRegionAvail();
			auto rsm = m.node->getComponent<Rsm>();
			if (!rsm->loaded)
				return;

			if (m.fbo->getWidth() != size.x || m.fbo->getHeight() != size.y)
				m.fbo->resize((int)size.x, (int)size.y);

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
			nodeRenderContext.viewMatrix = glm::translate(nodeRenderContext.viewMatrix, glm::vec3(0,-rsm->bbrange.y,0));
	
			RsmRenderer::RsmRenderContext::getInstance()->viewLighting = false;
			RsmRenderer::RsmRenderContext::getInstance()->viewTextures = true;
			RsmRenderer::RsmRenderContext::getInstance()->viewFog = false;
			m.node->getComponent<RsmRenderer>()->matrixCache = glm::mat4(1.0f);// rotate(glm::mat4(1.0f), glm::radians((float)ImGui::GetTime() * 100), glm::vec3(0, 1, 0));
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
		ImGui::End();
	}

	auto& activeModelView = models[0];
	auto rsm = activeModelView.node->getComponent<Rsm>();

	static int leftAreaSize = 200;
	static float scale = 1;
	static float timeSelected = 0.0f;
	static float rowHeight = 25;


	if (ImGui::Begin("ModelEditorTimeline"))
	{
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
		if (browEdit->toolBarButton("PlayPause", ICON_PLAY, "Play / Pause", ImVec4(1, 1, 1, 1)))
			;// playing = !playing;
		ImGui::SameLine();
		if (browEdit->toolBarButton("Next", ICON_NEXT, "Nexts Keyframe", ImVec4(1, 1, 1, 1)))
		{
			//if (selectedTrack >= 0 && selectedTrack < rsw->cinematicTracks.size() && selectedKeyFrame < rsw->cinematicTracks[selectedTrack].frames.size() - 1)
			//	selectedKeyFrame++;
		}


		if (rsm->animLen > 0 && ImGui::BeginChild("KeyframeEditor", ImVec2(-1, 200), true, ImGuiWindowFlags_AlwaysHorizontalScrollbar))
		{
			ImDrawList* DrawList = ImGui::GetWindowDrawList();
			auto window = ImGui::GetCurrentWindow();

			ImRect bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(leftAreaSize + scale * rsm->animLen, 100));
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
					ImGui::RenderText(bb.Min + ImVec2(10 + 20 * level, rowHeight + rowHeight * trackIndex + 2), mesh->name.c_str()); //todo: center
					ImGui::RenderFrame(bb.Min + ImVec2(leftAreaSize, rowHeight + rowHeight * trackIndex + 1), bb.Min + ImVec2(leftAreaSize + scale * rsm->animLen, rowHeight + rowHeight * trackIndex + rowHeight - 2), ImGui::GetColorU32(ImGuiCol_FrameBg, 1), true, style.FrameRounding);
					trackIndex++;
					for (auto m : mesh->children)
						drawTrack(m, level + 1);
				};
				drawTrack(rsm->rootMesh, 0);
				int trackCount = trackIndex;

				for (int i = 0; i < rsm->animLen; i+=100)
				{
					ImGui::RenderText(bb.Min + ImVec2(leftAreaSize + scale * i + 4, 0), std::to_string(i).c_str());
					for (int ii = 0; ii < 100; ii += 10)
					{
						window->DrawList->AddLine(bb.Min + ImVec2(leftAreaSize + scale * i + scale * ii, rowHeight - 5), bb.Min + ImVec2(leftAreaSize + scale * i + scale * ii, rowHeight), ImGui::GetColorU32(ImGuiCol_TextDisabled, 1.0f));
						window->DrawList->AddLine(bb.Min + ImVec2(leftAreaSize + scale * i + scale * ii, rowHeight), bb.Min + ImVec2(leftAreaSize + scale * i + scale * ii, rowHeight * (trackCount + 1)), ImGui::GetColorU32(ImGuiCol_TextDisabled, 0.5f));
					}
					window->DrawList->AddLine(bb.Min + ImVec2(leftAreaSize + scale * i, 0), bb.Min + ImVec2(leftAreaSize + scale * i, rowHeight * (trackCount + 1)), ImGui::GetColorU32(ImGuiCol_Text, 1));
				}

			}
			ImGui::EndChild();
		}
	}
	ImGui::End();

	if (ImGui::Begin("ModelEditorTimelineProperties"))
	{

	}
	ImGui::End();

	if (ImGui::Begin("ModelEditorNodes"))
	{
		std::function<void(Rsm::Mesh*)> buildTree;
		buildTree = [&buildTree,&activeModelView](Rsm::Mesh* mesh) {
			bool empty = mesh->children.empty();

			int flags = empty ? (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet) : (ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnDoubleClick);
			if (activeModelView.selectedMesh == mesh)
				flags |= ImGuiTreeNodeFlags_Selected;

			bool opened = ImGui::TreeNodeEx(mesh->name.c_str(), flags);
			if (ImGui::IsItemClicked())
				activeModelView.selectedMesh = mesh;
			if (opened)
			{
				for (auto c : mesh->children)
					buildTree(c);
				ImGui::TreePop();
			}
		};


		bool opened = ImGui::TreeNodeEx(rsm->fileName.c_str(), (ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnDoubleClick) | (activeModelView.selectedMesh == nullptr ? ImGuiTreeNodeFlags_Selected : 0));
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
			ImGui::InputScalar("Alpha", ImGuiDataType_U8, &rsm->alpha);
			char versionStr[10];
			sprintf_s(versionStr, 10, "%04x", rsm->version);
			if (ImGui::BeginCombo("Version", versionStr))
			{
				if (ImGui::Selectable("0101", rsm->version == 0x0101))
					rsm->version = 0x0101;
				if (ImGui::Selectable("0102", rsm->version == 0x0102))
					rsm->version = 0x0102;
				if (ImGui::Selectable("0103", rsm->version == 0x0103))
					rsm->version = 0x0103;
				if (ImGui::Selectable("0104", rsm->version == 0x0104))
					rsm->version = 0x0104;
				if (ImGui::Selectable("0105", rsm->version == 0x0105))
					rsm->version = 0x0105;
				if (ImGui::Selectable("0202", rsm->version == 0x0202))
					rsm->version = 0x0202;
				if (ImGui::Selectable("0203", rsm->version == 0x0203))
					rsm->version = 0x0203;
				ImGui::EndCombo();
			}
			ImGui::InputInt("Animation Length", &rsm->animLen);

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
				for (auto& t : rsm->textures)
				{
					ImGui::PushID(t.c_str());
					ImGui::InputText("Texture", &t);
					ImGui::PopID();
				}
			}

		}
		else //activeModelView.selectedMesh selected
		{
			ImGui::InputText("Name", &activeModelView.selectedMesh->name);
			ImGui::InputFloat3("Position", glm::value_ptr(activeModelView.selectedMesh->pos));
			ImGui::InputFloat4("Offset1", glm::value_ptr(activeModelView.selectedMesh->offset) + 0);
			ImGui::InputFloat4("Offset2", glm::value_ptr(activeModelView.selectedMesh->offset) + 4);
			ImGui::InputFloat4("Offset3", glm::value_ptr(activeModelView.selectedMesh->offset) + 8);
			ImGui::InputFloat4("Offset4", glm::value_ptr(activeModelView.selectedMesh->offset) + 12);

			ImGui::InputFloat3("Position_", glm::value_ptr(activeModelView.selectedMesh->pos_));
			ImGui::InputFloat("Rotation Angle", &activeModelView.selectedMesh->rotangle);
			ImGui::InputFloat3("Rotation Axis", glm::value_ptr(activeModelView.selectedMesh->rotaxis));
			ImGui::InputFloat3("Scale", glm::value_ptr(activeModelView.selectedMesh->scale));

			ImGui::LabelText("Face count", "%d", activeModelView.selectedMesh->faces.size());


			bool differentValues = false;
			for (const auto& f : activeModelView.selectedMesh->faces)
				if (f.twoSided != activeModelView.selectedMesh->faces[0].twoSided)
					differentValues = true;
			bool twoSided = activeModelView.selectedMesh->faces[0].twoSided != 0;
			ImGui::PushItemFlag(ImGuiItemFlags_MixedValue, differentValues);
			if (ImGui::Checkbox("Two-sided faces", &twoSided))
				for (auto& f : activeModelView.selectedMesh->faces)
					f.twoSided = twoSided ? 1 : 0;
			ImGui::PopItemFlag();

			if (ImGui::CollapsingHeader("Textures", ImGuiTreeNodeFlags_DefaultOpen))
			{

			}

		}

	}
	ImGui::End();

	


}