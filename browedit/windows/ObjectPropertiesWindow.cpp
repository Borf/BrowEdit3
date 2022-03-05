#include <browedit/BrowEdit.h>
#include <browedit/MapView.h>
#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/util/Util.h>
#include <browedit/components/ImguiProps.h>
#include <browedit/components/Rsw.h>

void BrowEdit::showObjectProperties()
{
	ImGui::Begin("Properties");
	if (activeMapView && activeMapView->map->selectedNodes.size() == 1)
	{
		if (util::InputText(this, activeMapView->map, activeMapView->map->selectedNodes[0], "Name", &activeMapView->map->selectedNodes[0]->name, 0, "Renaming", 40))
		{
			activeMapView->map->selectedNodes[0]->onRename(activeMapView->map);
		}
		if(activeMapView->map->selectedNodes.size() == 1)
			for (auto c : activeMapView->map->selectedNodes[0]->components)
			{
				auto props = dynamic_cast<ImguiProps*>(c);
				if (props)
					props->buildImGui(this);
			}
	}
	if (activeMapView && activeMapView->map->selectedNodes.size() > 1)
	{
		RswObject::buildImGuiMulti(this, activeMapView->map->selectedNodes);
		RswLight::buildImGuiMulti(this, activeMapView->map->selectedNodes);
		RswModel::buildImGuiMulti(this, activeMapView->map->selectedNodes);
		RswEffect::buildImGuiMulti(this, activeMapView->map->selectedNodes);
		RswSound::buildImGuiMulti(this, activeMapView->map->selectedNodes);
	}


	ImGui::End();
}
