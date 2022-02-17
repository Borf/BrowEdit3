#include <browedit/BrowEdit.h>
#include <browedit/Node.h>
#include <browedit/Map.h>
#include <browedit/actions/SelectAction.h>
#include <browedit/components/Rsw.h>
#include <browedit/components/Gnd.h>


void BrowEdit::showObjectTree()
{
	ImGui::Begin("Objects");
	ImGui::PushFont(font);
	for (auto m : maps)
	{
		buildObjectTree(m->rootNode, m);
	}
	ImGui::PopFont();

	ImGui::End();
}


void BrowEdit::buildObjectTree(Node* node, Map* map)
{
	if (node->children.size() > 0)
	{
		int flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnDoubleClick;
		if (std::find(map->selectedNodes.begin(), map->selectedNodes.end(), node) != map->selectedNodes.end())
			flags |= ImGuiTreeNodeFlags_Selected;
		bool opened = ImGui::TreeNodeEx(node->name.c_str(), flags);
		if (ImGui::IsItemClicked())
		{
			map->doAction(new SelectAction(map, node, false, false), this);
		}
		if (opened)
		{
			for (auto n : node->children)
				buildObjectTree(n, map);
			ImGui::TreePop();
		}
	}
	else
	{
		int flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		if (std::find(map->selectedNodes.begin(), map->selectedNodes.end(), node) != map->selectedNodes.end())
			flags |= ImGuiTreeNodeFlags_Selected;


		ImVec4 textColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);
		float h, s, v;
		ImGui::ColorConvertRGBtoHSV(textColor.x, textColor.y, textColor.z, h, s, v);
		if (node->getComponent<RswModel>()) { h = 0;		s = glm::min(1.0f, s + 0.5f);	v = glm::min(1.0f, v + 0.5f); }
		if (node->getComponent<RswEffect>()) { h = 0.25f;	s = glm::min(1.0f, s + 0.5f);	v = glm::min(1.0f, v + 0.5f); }
		if (node->getComponent<RswSound>()) { h = 0.5f;	s = glm::min(1.0f, s + 0.5f);	v = glm::min(1.0f, v + 0.5f); }
		if (node->getComponent<RswLight>()) { h = 0.75f;	s = glm::min(1.0f, s + 0.5f);	v = glm::min(1.0f, v + 0.5f); }
		ImGui::ColorConvertHSVtoRGB(h, s, v, textColor.x, textColor.y, textColor.z);

		ImGui::PushStyleColor(ImGuiCol_Text, textColor);
		bool opened = ImGui::TreeNodeEx(node->name.c_str(), flags);
		ImGui::PopStyleColor();

		if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0))
		{
			auto rswObject = node->getComponent<RswObject>();
			if (rswObject)
			{
				auto gnd = map->rootNode->getComponent<Gnd>();
				for (auto& m : mapViews)
				{
					if (m.map->rootNode == node->root)
					{
						m.cameraCenter.x = 5 * gnd->width + rswObject->position.x;
						m.cameraCenter.y = -1 * (-10 - 5 * gnd->height + rswObject->position.z);
					}
				}
			}
		}
		if (ImGui::IsItemClicked())
		{
			bool ctrl = ImGui::GetIO().KeyCtrl;
			bool selected = std::find(map->selectedNodes.begin(), map->selectedNodes.end(), node) != map->selectedNodes.end();
			map->doAction(new SelectAction(map, node, ctrl, ctrl && selected), this);
		}
	}
}



