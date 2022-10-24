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

#include <glm/gtc/matrix_transform.hpp>

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
			auto rsm = m.node->getComponent<Rsm>();
			if (!rsm->loaded)
				return;
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
			nodeRenderContext.viewMatrix = glm::lookAt(glm::vec3(0.0f, -rsm->bbrange.y - distance, -distance), glm::vec3(0.0f, rsm->bbrange.y, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			RsmRenderer::RsmRenderContext::getInstance()->viewLighting = false;
			RsmRenderer::RsmRenderContext::getInstance()->viewTextures = true;
			RsmRenderer::RsmRenderContext::getInstance()->viewFog = false;
			m.node->getComponent<RsmRenderer>()->matrixCache = glm::rotate(glm::mat4(1.0f), glm::radians((float)ImGui::GetTime()*100), glm::vec3(0, 1, 0));
			NodeRenderer::render(m.node, nodeRenderContext);
			m.fbo->unbind();
			auto size = ImGui::GetContentRegionAvail();
			ImGui::ImageButton((ImTextureID)(long long)m.fbo->texid[0], size, ImVec2(1, 0), ImVec2(0, 1), 0, ImVec4(0, 0, 0, 1), ImVec4(1, 1, 1, 1));

		}
		ImGui::End();
	}
	if (ImGui::Begin("ModelEditorProperties"))
	{
		ImGui::Text("Right");

	}
	ImGui::End();
	if (ImGui::Begin("ModelEditorTimeline"))
	{

		ImGui::End();
	}
	if (ImGui::Begin("ModelEditorNodes"))
	{

		ImGui::End();
	}
	


}