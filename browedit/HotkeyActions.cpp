#include <browedit/BrowEdit.h>
#include <browedit/HotkeyRegistry.h>
#include <browedit/Map.h>
#include <browedit/util/ResourceManager.h>
#include <browedit/gl/Texture.h>
#include <browedit/components/Rsm.h>
#include <browedit/components/RsmRenderer.h>
#include <browedit/Node.h>
#include <GLFW/glfw3.h>

void BrowEdit::registerActions()
{
	auto hasActiveMapView = [this]() {return activeMapView != nullptr; };
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
