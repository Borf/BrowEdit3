#include <browedit/BrowEdit.h>
#include <browedit/HotkeyRegistry.h>
#include <browedit/Map.h>
#include <browedit/util/ResourceManager.h>
#include <browedit/gl/Texture.h>
#include <browedit/components/Rsw.h>
#include <browedit/components/Rsm.h>
#include <browedit/components/RsmRenderer.h>
#include <browedit/Lightmapper.h>
#include <browedit/Node.h>
#include <GLFW/glfw3.h>

void BrowEdit::registerActions()
{
	auto hasActiveMapView = [this]() {return activeMapView != nullptr; };
	auto hasActiveMapViewObjectMode = [this]() { return activeMapView != nullptr && editMode == EditMode::Object; };

	HotkeyRegistry::registerAction(HotkeyAction::Global_Save,			[this]() { saveMap(activeMapView->map); }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::Global_Load,			[this]() { showOpenWindow(); });
	HotkeyRegistry::registerAction(HotkeyAction::Global_Exit,			[this]() { glfwSetWindowShouldClose(window, 1); });

	HotkeyRegistry::registerAction(HotkeyAction::Global_Undo,			[this]() { activeMapView->map->undo(this); }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::Global_Redo,			[this]() { activeMapView->map->redo(this); }, hasActiveMapView);

	HotkeyRegistry::registerAction(HotkeyAction::Global_Settings,		[this]() { windowData.configVisible = true; });
	HotkeyRegistry::registerAction(HotkeyAction::Global_ReloadTextures, [this]() { for (auto t : util::ResourceManager<gl::Texture>::getAll()) { t->reload(); } });
	HotkeyRegistry::registerAction(HotkeyAction::Global_ReloadModels,	[this]() { 
		for (auto rsm : util::ResourceManager<Rsm>::getAll())
			rsm->reload();
		for (auto m : maps)
			m->rootNode->traverse([](Node* n) {
			auto rsmRenderer = n->getComponent<RsmRenderer>();
			if (rsmRenderer)
				rsmRenderer->begin();
				});
		});
	HotkeyRegistry::registerAction(HotkeyAction::Global_Copy,					[this]() { if (editMode == EditMode::Object) { activeMapView->map->copySelection(); } }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::Global_Paste,					[this]() { if (editMode == EditMode::Object) { activeMapView->map->pasteSelection(this); } }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::Global_PasteChangeHeight,		[this]() { if (editMode == EditMode::Object) { newNodeHeight = !newNodeHeight; } }, hasActiveMapView);

	HotkeyRegistry::registerAction(HotkeyAction::Global_CalculateQuadtree,		[this]() { activeMapView->map->recalculateQuadTree(this); }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::Global_CalculateLightmaps,		[this]() {
		lightmapper = new Lightmapper(activeMapView->map, this);
		windowData.openLightmapSettings = true; 
	}, [this]() { return activeMapView && !lightmapper;});
	


	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_FlipHorizontal,		[this]() { activeMapView->map->flipSelection(0, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_FlipVertical,		[this]() { activeMapView->map->flipSelection(2, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_Delete,				[this]() { activeMapView->map->deleteSelection(this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_FocusOnSelection,	[this]() { activeMapView->focusSelection(); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_InvertSelection,	[this]() { activeMapView->map->selectInvert(this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_SelectAll,			[this]() { activeMapView->map->selectAll(this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_SelectAllModels,	[this]() { activeMapView->map->selectAll<RswModel>(this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_SelectAllEffects,	[this]() { activeMapView->map->selectAll<RswEffect>(this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_SelectAllSounds,	[this]() { activeMapView->map->selectAll<RswSound>(this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_SelectAllLights,	[this]() { activeMapView->map->selectAll<RswLight>(this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_InvertScaleX,		[this]() { activeMapView->map->invertScale(0, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_InvertScaleY,		[this]() { activeMapView->map->invertScale(1, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_InvertScaleZ,		[this]() { activeMapView->map->invertScale(2, this); }, hasActiveMapViewObjectMode);


	HotkeyRegistry::registerAction(HotkeyAction::EditMode_Height,		[this]() { editMode = EditMode::Height; });
	HotkeyRegistry::registerAction(HotkeyAction::EditMode_Texture,		[this]() { editMode = EditMode::Texture; });
	HotkeyRegistry::registerAction(HotkeyAction::EditMode_Object,		[this]() { editMode = EditMode::Object; });
	HotkeyRegistry::registerAction(HotkeyAction::EditMode_Wall,			[this]() { editMode = EditMode::Wall; });
	HotkeyRegistry::registerAction(HotkeyAction::EditMode_Gat,			[this]() { editMode = EditMode::Gat; });
}

bool BrowEdit::hotkeyMenuItem(const std::string& title, HotkeyAction action)
{
	auto hotkey = HotkeyRegistry::getHotkey(action);
	bool clicked = ImGui::MenuItem(title.c_str(), hotkey.hotkey.toString().c_str());

	if (clicked && hotkey.callback)
		hotkey.callback();
	return clicked;
}
