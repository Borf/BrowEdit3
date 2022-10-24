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
				m.fbo->resize(size.x, size.y);

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

	if (ImGui::Begin("ModelEditorTimeline"))
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
				if (ImGui::Selectable("0103", rsm->version == 0x0103))
					rsm->version = 0x0103;
				if (ImGui::Selectable("0104", rsm->version == 0x0104))
					rsm->version = 0x0104;
				if (ImGui::Selectable("0108", rsm->version == 0x0108))
					rsm->version = 0x0108;
				if (ImGui::Selectable("0109", rsm->version == 0x0109))
					rsm->version = 0x0109;
				if (ImGui::Selectable("0201", rsm->version == 0x0201))
					rsm->version = 0x0201;
				if (ImGui::Selectable("0202", rsm->version == 0x0202))
					rsm->version = 0x0202;
				if (ImGui::Selectable("0203", rsm->version == 0x0203))
					rsm->version = 0x0203;
				if (ImGui::Selectable("0204", rsm->version == 0x0204))
					rsm->version = 0x0204;
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