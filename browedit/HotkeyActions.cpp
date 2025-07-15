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
#include <browedit/util/Util.h>
#include <GLFW/glfw3.h>

extern bool previewWall; //move this
extern bool dropperEnabled; //move this


void BrowEdit::registerActions()
{
	auto hasActiveMapView = [this]() {return activeMapView != nullptr; };
	auto hasActiveMapViewObjectMode = [this]() { return activeMapView != nullptr && editMode == EditMode::Object; };
	auto hasActiveMapViewTextureMode = [this]() { return activeMapView != nullptr && editMode == EditMode::Texture; };
	auto hasActiveMapViewWallMode = [this]() { return activeMapView != nullptr && editMode == EditMode::Wall; };
	auto hasActiveMapViewGatMode = [this]() { return activeMapView != nullptr && editMode == EditMode::Gat; };
	auto hasActiveMapViewHeightMode = [this]() { return activeMapView != nullptr && editMode == EditMode::Height; };
	auto hasActiveMapViewTextureWallMode = [this]() { return activeMapView != nullptr && (editMode == EditMode::Texture|| editMode == EditMode::Wall); };

	HotkeyRegistry::registerAction(HotkeyAction::Global_HotkeyPopup,	[this]() { 
		windowData.showHotkeyPopup = true;
		std::cout << "Opening hotkey popup" << std::endl;
	});

	HotkeyRegistry::registerAction(HotkeyAction::Global_New, 			[this]() { windowData.showNewMapPopup = true; });
	HotkeyRegistry::registerAction(HotkeyAction::Global_Save,			[this]() { saveMap(activeMapView->map); }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::Global_Load,			[this]() { showOpenWindow(); });
	HotkeyRegistry::registerAction(HotkeyAction::Global_Exit,			[this]() { 
		bool changed = false;
		for (auto m : maps)
			windowData.showQuitConfirm |= m->changed;
		if(!windowData.showQuitConfirm)
			glfwSetWindowShouldClose(window, GLFW_TRUE);
	});

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
	HotkeyRegistry::registerAction(HotkeyAction::Global_CloseTab,				[this]() { activeMapView->opened = false; }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::Global_ModelEditor_Open,		[this]() { modelEditor.opened = !modelEditor.opened; });
	HotkeyRegistry::registerAction(HotkeyAction::Global_Copy, [this]() {
		if (editMode == EditMode::Object)
			activeMapView->map->copySelection();
		else if (editMode == EditMode::Height && !heightDoodle)
			copyTiles();
		else if (editMode == EditMode::Gat && !heightDoodle)
			copyGat();
	}, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::Global_Paste,					[this]() { 
		if (editMode == EditMode::Object) 
			activeMapView->map->pasteSelection(this); 
		else if (editMode == EditMode::Height && !heightDoodle)
		{
			pasteOptions = PasteOptions::Height | PasteOptions::Walls | PasteOptions::Textures | PasteOptions::Colors | PasteOptions::Lightmaps;
			pasteTiles();
		}
		else if (editMode == EditMode::Gat && !heightDoodle)
			pasteGat();
	}, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::Global_PasteChangeHeight,		[this]() { if (editMode == EditMode::Object) { newNodePlacement = newNodePlacement == BrowEdit::Absolute ? BrowEdit::Ground : BrowEdit::Absolute; } }, hasActiveMapView);

	HotkeyRegistry::registerAction(HotkeyAction::Global_ClearZeroHeightWalls,	[this]() { activeMapView->map->rootNode->getComponent<Gnd>()->removeZeroHeightWalls(); }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::Global_CalculateQuadtree,		[this]() { activeMapView->map->recalculateQuadTree(this); }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::Global_CalculateLightmaps,		[this]() {
		lightmapper = new Lightmapper(activeMapView->map, this);
		windowData.openLightmapSettings = true; 
	}, [this]() { return activeMapView && !lightmapper;});
	


	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_FlipHorizontal,		[this]() { activeMapView->map->flipSelection(0, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_FlipVertical,		[this]() { activeMapView->map->flipSelection(2, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_Delete,				[this]() { activeMapView->map->deleteSelection(this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_FocusOnSelection,	[this]() { activeMapView->focusSelection(); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_HighlightInObjectPicker,	[this]() { 
		if (activeMapView->map->selectedNodes.size() < 1)
			return;
		auto selected = activeMapView->map->selectedNodes[0]->getComponent<RswModel>();
		if (!selected)
			return;
		std::filesystem::path path(util::utf8_to_iso_8859_1(selected->fileName));
		std::string dir = path.parent_path().string();
		windowData.objectWindowSelectedTreeNode = util::FileIO::directoryNode("data\\model\\" + dir);
		windowData.objectWindowScrollToModel = "data\\model\\" + util::utf8_to_iso_8859_1(selected->fileName);


	}, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_InvertSelection,		[this]() { activeMapView->map->selectInvert(this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_SelectAll,				[this]() { activeMapView->map->selectAll(this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_SelectAllModels,		[this]() { activeMapView->map->selectAll<RswModel>(this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_SelectAllEffects,		[this]() { activeMapView->map->selectAll<RswEffect>(this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_SelectAllSounds,		[this]() { activeMapView->map->selectAll<RswSound>(this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_SelectAllLights,		[this]() { activeMapView->map->selectAll<RswLight>(this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_Move,					[this]() { activeMapView->gadget.mode = Gadget::Mode::Translate; }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_Rotate,					[this]() { activeMapView->gadget.mode = Gadget::Mode::Rotate; }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_Scale,					[this]() { activeMapView->gadget.mode = Gadget::Mode::Scale; }, hasActiveMapViewObjectMode);

	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_InvertScaleX,			[this]() { activeMapView->map->invertScale(0, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_InvertScaleY,			[this]() { activeMapView->map->invertScale(1, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_InvertScaleZ,			[this]() { activeMapView->map->invertScale(2, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_CreatePrefab,			[this]() { windowData.openPrefabPopup = true; }, hasActiveMapViewObjectMode);

	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_NudgeXNeg,				[this]() { activeMapView->map->nudgeSelection(0, -1, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_NudgeXPos,				[this]() { activeMapView->map->nudgeSelection(0, 1, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_NudgeYNeg,				[this]() { activeMapView->map->nudgeSelection(1, -1, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_NudgeYPos,				[this]() { activeMapView->map->nudgeSelection(1, 1, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_NudgeZNeg,				[this]() { activeMapView->map->nudgeSelection(2, -1, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_NudgeZPos,				[this]() { activeMapView->map->nudgeSelection(2, 1, this); }, hasActiveMapViewObjectMode);

	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_RotXNeg,				[this]() { activeMapView->map->rotateSelection(0, -1, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_RotXPos,				[this]() { activeMapView->map->rotateSelection(0, 1, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_RotYNeg,				[this]() { activeMapView->map->rotateSelection(1, -1, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_RotYPos,				[this]() { activeMapView->map->rotateSelection(1, 1, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_RotZNeg,				[this]() { activeMapView->map->rotateSelection(2, -1, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_RotZPos,				[this]() { activeMapView->map->rotateSelection(2, 1, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_RandomXRotation,		[this]() { activeMapView->map->randomRotateSelection(0, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_RandomYRotation,		[this]() { activeMapView->map->randomRotateSelection(1, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_RandomZRotation,		[this]() { activeMapView->map->randomRotateSelection(2, this); }, hasActiveMapViewObjectMode);
	HotkeyRegistry::registerAction(HotkeyAction::ObjectEdit_ToggleObjectWindow,		[this]() { windowData.objectWindowVisible = !windowData.objectWindowVisible; }, hasActiveMapViewObjectMode);
	
	HotkeyRegistry::registerAction(HotkeyAction::TextureEdit_ToggleTextureWindow,	[this]() { windowData.textureManageWindowVisible = !windowData.textureManageWindowVisible; }, hasActiveMapViewTextureMode);
	HotkeyRegistry::registerAction(HotkeyAction::TextureEdit_SwapBrushSize,			[this]() { std::swap(activeMapView->textureBrushWidth, activeMapView->textureBrushHeight); }, hasActiveMapViewTextureMode);

	HotkeyRegistry::registerAction(HotkeyAction::WallEdit_AddWall,					[this]() { activeMapView->map->wallAddSelected(this); }, hasActiveMapViewWallMode);
	HotkeyRegistry::registerAction(HotkeyAction::WallEdit_RemoveWall,				[this]() { activeMapView->map->wallRemoveSelected(this); }, hasActiveMapViewWallMode);
	HotkeyRegistry::registerAction(HotkeyAction::WallEdit_ReApply,					[this]() { activeMapView->map->wallReApplySelected(this); }, hasActiveMapViewWallMode);
	HotkeyRegistry::registerAction(HotkeyAction::WallEdit_Preview,					[this]() { previewWall = !previewWall; }, hasActiveMapViewWallMode);
	HotkeyRegistry::registerAction(HotkeyAction::WallEdit_OffsetLower,				[this]() { activeMapView->wallOffset = glm::max(1, activeMapView->wallOffset - 1); }, hasActiveMapViewWallMode);
	HotkeyRegistry::registerAction(HotkeyAction::WallEdit_OffsetRaise,				[this]() { activeMapView->wallOffset++; }, hasActiveMapViewWallMode);
	HotkeyRegistry::registerAction(HotkeyAction::WallEdit_SizeLower,				[this]() { activeMapView->wallWidth = glm::max(1, activeMapView->wallWidth - 1); }, hasActiveMapViewWallMode);
	HotkeyRegistry::registerAction(HotkeyAction::WallEdit_SizeRaise,				[this]() { activeMapView->wallWidth++; }, hasActiveMapViewWallMode);
	HotkeyRegistry::registerAction(HotkeyAction::WallEdit_FlipSelectedHorizontal,	[this]() { activeMapView->map->wallFlipSelectedTextureHorizontal(this); }, hasActiveMapViewWallMode);
	HotkeyRegistry::registerAction(HotkeyAction::WallEdit_FlipSelectedVertical,		[this]() { activeMapView->map->wallFlipSelectedTextureVertical(this); }, hasActiveMapViewWallMode);

	
	HotkeyRegistry::registerAction(HotkeyAction::GatEdit_NextTileType,				[this]() { windowData.gatEdit.gatIndex = (windowData.gatEdit.gatIndex + 1)%10;	heightDoodle = false;	windowData.gatEdit.doodle = true; }, hasActiveMapViewGatMode);
	HotkeyRegistry::registerAction(HotkeyAction::GatEdit_PrevTileType,				[this]() { windowData.gatEdit.gatIndex = (windowData.gatEdit.gatIndex + 11)%10;	heightDoodle = false;	windowData.gatEdit.doodle = true; }, hasActiveMapViewGatMode);
	HotkeyRegistry::registerAction(HotkeyAction::GatEdit_Tile0,						[this]() { windowData.gatEdit.gatIndex = 0;	heightDoodle = false;	windowData.gatEdit.doodle = true; }, hasActiveMapViewGatMode);
	HotkeyRegistry::registerAction(HotkeyAction::GatEdit_Tile1,						[this]() { windowData.gatEdit.gatIndex = 1;	heightDoodle = false;	windowData.gatEdit.doodle = true; }, hasActiveMapViewGatMode);
	HotkeyRegistry::registerAction(HotkeyAction::GatEdit_Tile2,						[this]() { windowData.gatEdit.gatIndex = 2;	heightDoodle = false;	windowData.gatEdit.doodle = true; }, hasActiveMapViewGatMode);
	HotkeyRegistry::registerAction(HotkeyAction::GatEdit_Tile3,						[this]() { windowData.gatEdit.gatIndex = 3;	heightDoodle = false;	windowData.gatEdit.doodle = true; }, hasActiveMapViewGatMode);
	HotkeyRegistry::registerAction(HotkeyAction::GatEdit_Tile4,						[this]() { windowData.gatEdit.gatIndex = 4;	heightDoodle = false;	windowData.gatEdit.doodle = true; }, hasActiveMapViewGatMode);
	HotkeyRegistry::registerAction(HotkeyAction::GatEdit_Tile5,						[this]() { windowData.gatEdit.gatIndex = 5;	heightDoodle = false;	windowData.gatEdit.doodle = true; }, hasActiveMapViewGatMode);
	HotkeyRegistry::registerAction(HotkeyAction::GatEdit_Tile6,						[this]() { windowData.gatEdit.gatIndex = 6;	heightDoodle = false;	windowData.gatEdit.doodle = true; }, hasActiveMapViewGatMode);
	HotkeyRegistry::registerAction(HotkeyAction::GatEdit_Tile7,						[this]() { windowData.gatEdit.gatIndex = 7;	heightDoodle = false;	windowData.gatEdit.doodle = true; }, hasActiveMapViewGatMode);
	HotkeyRegistry::registerAction(HotkeyAction::GatEdit_Tile8,						[this]() { windowData.gatEdit.gatIndex = 8;	heightDoodle = false;	windowData.gatEdit.doodle = true; }, hasActiveMapViewGatMode);
	HotkeyRegistry::registerAction(HotkeyAction::GatEdit_Tile9,						[this]() { windowData.gatEdit.gatIndex = 9;	heightDoodle = false;	windowData.gatEdit.doodle = true; }, hasActiveMapViewGatMode);
	HotkeyRegistry::registerAction(HotkeyAction::GatEdit_AdjustToGround,			[this]() { activeMapView->gatEdit_adjustToGround(this); }, hasActiveMapViewGatMode);
	
	HotkeyRegistry::registerAction(HotkeyAction::ColorEdit_Dropper,					[this]() { dropperEnabled = !dropperEnabled; cursor = dropperEnabled ? dropperCursor : nullptr; }, hasActiveMapViewGatMode);


	HotkeyRegistry::registerAction(HotkeyAction::HeightEdit_Doodle,					[this]() { heightDoodle = true; }, hasActiveMapViewHeightMode);
	HotkeyRegistry::registerAction(HotkeyAction::HeightEdit_Rectangle,				[this]() { selectTool = BrowEdit::SelectTool::Rectangle; heightDoodle = false; }, hasActiveMapViewHeightMode);
	HotkeyRegistry::registerAction(HotkeyAction::HeightEdit_Lasso,					[this]() { selectTool = BrowEdit::SelectTool::Lasso; heightDoodle = false; }, hasActiveMapViewHeightMode);
	HotkeyRegistry::registerAction(HotkeyAction::HeightEdit_MagicWandTexture,		[this]() { selectTool = BrowEdit::SelectTool::WandTex; heightDoodle = false; }, hasActiveMapViewHeightMode);
	HotkeyRegistry::registerAction(HotkeyAction::HeightEdit_MagicWandHeight,		[this]() { selectTool = BrowEdit::SelectTool::WandHeight; heightDoodle = false; }, hasActiveMapViewHeightMode);
	HotkeyRegistry::registerAction(HotkeyAction::HeightEdit_SelectAllTexture,		[this]() { selectTool = BrowEdit::SelectTool::AllTex; heightDoodle = false; }, hasActiveMapViewHeightMode);
	HotkeyRegistry::registerAction(HotkeyAction::HeightEdit_SelectAllHeight,		[this]() { selectTool = BrowEdit::SelectTool::AllHeight; heightDoodle = false; }, hasActiveMapViewHeightMode);
	HotkeyRegistry::registerAction(HotkeyAction::HeightEdit_PasteSpecial,			[this]() { ImGui::OpenPopup("PasteSpecial");  }, hasActiveMapViewHeightMode);
	HotkeyRegistry::registerAction(HotkeyAction::HeightEdit_SmoothTiles,			[this]() {
		auto gnd = activeMapView->map->rootNode->getComponent<Gnd>();

		if (gnd)
			gnd->smoothTiles_Brow1(activeMapView->map, this, activeMapView->map->tileSelection);
	}, hasActiveMapViewHeightMode);
	HotkeyRegistry::registerAction(HotkeyAction::HeightEdit_FlattenTiles,			[this]() {
		auto gnd = activeMapView->map->rootNode->getComponent<Gnd>();

		if (gnd)
			gnd->flattenTiles_Brow1(activeMapView->map, this, activeMapView->map->tileSelection);
	}, hasActiveMapViewHeightMode);
	HotkeyRegistry::registerAction(HotkeyAction::HeightEdit_IncreaseHeight,			[this]() {
		auto gnd = activeMapView->map->rootNode->getComponent<Gnd>();

		if (gnd)
			gnd->increaseTileHeight(activeMapView->map, this, activeMapView->map->tileSelection, -activeMapView->gridSizeTranslate);
	}, hasActiveMapViewHeightMode);
	HotkeyRegistry::registerAction(HotkeyAction::HeightEdit_DecreaseHeight,			[this]() {
		auto gnd = activeMapView->map->rootNode->getComponent<Gnd>();

		if (gnd)
			gnd->increaseTileHeight(activeMapView->map, this, activeMapView->map->tileSelection, activeMapView->gridSizeTranslate);
	}, hasActiveMapViewHeightMode);

	HotkeyRegistry::registerAction(HotkeyAction::Texture_PrevTexture,				[this]() { activeMapView->textureSelected = (activeMapView->textureSelected + activeMapView->map->rootNode->getComponent<Gnd>()->textures.size() - 1) % (int)activeMapView->map->rootNode->getComponent<Gnd>()->textures.size(); }, hasActiveMapViewTextureWallMode);
	HotkeyRegistry::registerAction(HotkeyAction::Texture_NextTexture,				[this]() { activeMapView->textureSelected = (activeMapView->textureSelected + 1) % activeMapView->map->rootNode->getComponent<Gnd>()->textures.size(); }, hasActiveMapViewTextureWallMode);
	HotkeyRegistry::registerAction(HotkeyAction::Texture_SelectFull,				[this]() { activeMapView->textureEditUv1 = glm::vec2(0, 0);	activeMapView->textureEditUv2 = glm::vec2(1, 1); }, hasActiveMapViewTextureWallMode);
	HotkeyRegistry::registerAction(HotkeyAction::Texture_Delete,					[this]() { activeMapView->deleteTiles = true; }, hasActiveMapViewTextureMode);
	

	HotkeyRegistry::registerAction(HotkeyAction::EditMode_Height,					[this]() { editMode = EditMode::Height; });
	HotkeyRegistry::registerAction(HotkeyAction::EditMode_Texture,					[this]() { editMode = EditMode::Texture; });
	HotkeyRegistry::registerAction(HotkeyAction::EditMode_Object,					[this]() { editMode = EditMode::Object; });
	HotkeyRegistry::registerAction(HotkeyAction::EditMode_Wall,						[this]() { editMode = EditMode::Wall; });
	HotkeyRegistry::registerAction(HotkeyAction::EditMode_Gat,						[this]() { editMode = EditMode::Gat; });
	HotkeyRegistry::registerAction(HotkeyAction::EditMode_Color,					[this]() { editMode = EditMode::Color; });
	HotkeyRegistry::registerAction(HotkeyAction::EditMode_Shadow,					[this]() { editMode = EditMode::Shadow; });
	HotkeyRegistry::registerAction(HotkeyAction::EditMode_Sprite,					[this]() { editMode = EditMode::Sprite; });
	HotkeyRegistry::registerAction(HotkeyAction::EditMode_Cinematic,				[this]() { editMode = EditMode::Cinematic; });
	
	HotkeyRegistry::registerAction(HotkeyAction::View_ShadowMap,					[this]() { activeMapView->viewLightmapShadow = !activeMapView->viewLightmapShadow; }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::View_ColorMap,						[this]() { activeMapView->viewLightmapColor = !activeMapView->viewLightmapColor; }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::View_TileColors,					[this]() { activeMapView->viewColors = !activeMapView->viewColors; }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::View_Lighting,						[this]() { activeMapView->viewLighting = !activeMapView->viewLighting; }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::View_Textures,						[this]() { activeMapView->viewTextures = !activeMapView->viewTextures; }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::View_SmoothColormap,				[this]() { activeMapView->smoothColors = !activeMapView->smoothColors; }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::View_EmptyTiles,					[this]() { activeMapView->viewEmptyTiles = !activeMapView->viewEmptyTiles; }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::View_GatTiles,						[this]() { if (editMode == BrowEdit::EditMode::Gat) { activeMapView->viewGatGat = !activeMapView->viewGatGat; } else { activeMapView->viewGat = !activeMapView->viewGat; } }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::View_Models,						[this]() { activeMapView->viewModels = !activeMapView->viewModels; }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::View_Effects,						[this]() { activeMapView->viewEffects = !activeMapView->viewEffects; }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::View_EffectIcons,					[this]() { activeMapView->viewEffectIcons = !activeMapView->viewEffectIcons; }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::View_Sounds,						[this]() { activeMapView->viewSounds = !activeMapView->viewSounds; }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::View_Lights,						[this]() { activeMapView->viewLights= !activeMapView->viewLights; }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::View_Water,						[this]() { activeMapView->viewWater = !activeMapView->viewWater; }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::View_Fog,							[this]() { activeMapView->viewFog = !activeMapView->viewFog; }, hasActiveMapView);



	HotkeyRegistry::registerAction(HotkeyAction::Camera_OrthoPerspective,			[this]() { activeMapView->ortho = !activeMapView->ortho; }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::Camera_MoveXPositive,				[this]() { activeMapView->cameraTargetRot = glm::vec2(0, 270); activeMapView->cameraTargetPos = activeMapView->cameraCenter; activeMapView->cameraAnimating = true; }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::Camera_MoveXNegative,				[this]() { activeMapView->cameraTargetRot = glm::vec2(0, 90); activeMapView->cameraTargetPos = activeMapView->cameraCenter; activeMapView->cameraAnimating = true; }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::Camera_MoveYPositive,				[this]() { activeMapView->cameraTargetRot = glm::vec2(90, 0); activeMapView->cameraTargetPos = activeMapView->cameraCenter; activeMapView->cameraAnimating = true; }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::Camera_MoveYNegative,				[this]() { activeMapView->cameraTargetRot = glm::vec2(270, 0); activeMapView->cameraTargetPos = activeMapView->cameraCenter; activeMapView->cameraAnimating = true; }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::Camera_MoveZPositive,				[this]() { activeMapView->cameraTargetRot = glm::vec2(0, 180); activeMapView->cameraTargetPos = activeMapView->cameraCenter; activeMapView->cameraAnimating = true; }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::Camera_MoveZNegative,				[this]() { activeMapView->cameraTargetRot = glm::vec2(0, 0); activeMapView->cameraTargetPos = activeMapView->cameraCenter; activeMapView->cameraAnimating = true; }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::Camera_RotateX45Positive,			[this]() { activeMapView->cameraTargetRot += glm::vec2(45, 0); activeMapView->cameraTargetPos = activeMapView->cameraCenter; activeMapView->cameraAnimating = true; }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::Camera_RotateX45Negative,			[this]() { activeMapView->cameraTargetRot -= glm::vec2(45, 0); activeMapView->cameraTargetPos = activeMapView->cameraCenter; activeMapView->cameraAnimating = true; }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::Camera_RotateY45Positive,			[this]() { activeMapView->cameraTargetRot += glm::vec2(0, 45); activeMapView->cameraTargetPos = activeMapView->cameraCenter; activeMapView->cameraAnimating = true; }, hasActiveMapView);
	HotkeyRegistry::registerAction(HotkeyAction::Camera_RotateY45Negative,			[this]() { activeMapView->cameraTargetRot -= glm::vec2(0, 45); activeMapView->cameraTargetPos = activeMapView->cameraCenter; activeMapView->cameraAnimating = true; }, hasActiveMapView);





}

bool BrowEdit::hotkeyMenuItem(const std::string& title, HotkeyAction action)
{
	auto hotkey = HotkeyRegistry::getHotkey(action);
	bool clicked = ImGui::MenuItem(title.c_str(), hotkey.hotkey.toString().c_str());

	if (clicked && hotkey.callback)
		hotkey.callback();
	return clicked;
}
bool BrowEdit::hotkeyButton(const std::string& title, HotkeyAction action)
{
	auto hotkey = HotkeyRegistry::getHotkey(action);
	bool clicked = ImGui::Button(title.c_str());
	if (ImGui::IsItemHovered() && hotkey.hotkey.keyCode != 0)
		ImGui::SetTooltip(hotkey.hotkey.toString().c_str());

	if (clicked && hotkey.callback)
		hotkey.callback();
	return clicked;
}
