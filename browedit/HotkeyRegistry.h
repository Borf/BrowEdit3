#pragma once

#include <string>
#include <vector>
#include <map>
#include <browedit/Hotkey.h>

class BrowEdit;

enum class HotkeyAction
{
	Global_HotkeyPopup,
	Global_New,
	Global_Save,
	Global_Load,
	Global_Exit,
	Global_Undo,
	Global_Redo,
	Global_Settings,
	Global_ReloadTextures,
	Global_ReloadModels,
	Global_CloseTab,
	Global_ModelEditor_Open,

	Global_Copy,
	Global_Paste,
	Global_PasteChangeHeight,

	Global_CalculateQuadtree,
	Global_CalculateLightmaps,
	Global_ClearZeroHeightWalls,

	ObjectEdit_InvertScaleX,
	ObjectEdit_InvertScaleY,
	ObjectEdit_InvertScaleZ,
	ObjectEdit_FlipHorizontal,
	ObjectEdit_FlipVertical,
	ObjectEdit_Delete,
	ObjectEdit_FocusOnSelection,
	ObjectEdit_HighlightInObjectPicker,
	ObjectEdit_CreatePrefab,

	ObjectEdit_NudgeXNeg,
	ObjectEdit_NudgeXPos,
	ObjectEdit_NudgeYNeg,
	ObjectEdit_NudgeYPos,
	ObjectEdit_NudgeZNeg,
	ObjectEdit_NudgeZPos,

	ObjectEdit_RotXNeg,
	ObjectEdit_RotXPos,
	ObjectEdit_RotYNeg,
	ObjectEdit_RotYPos,
	ObjectEdit_RotZNeg,
	ObjectEdit_RotZPos,
	ObjectEdit_RandomXRotation,
	ObjectEdit_RandomYRotation,
	ObjectEdit_RandomZRotation,

	ObjectEdit_InvertSelection,
	ObjectEdit_SelectAll,
	ObjectEdit_SelectAllModels,
	ObjectEdit_SelectAllEffects,
	ObjectEdit_SelectAllSounds,
	ObjectEdit_SelectAllLights,

	ObjectEdit_Rotate,
	ObjectEdit_Move,
	ObjectEdit_Scale,

	ObjectEdit_ToggleObjectWindow,


	WallEdit_AddWall,
	WallEdit_RemoveWall,
	WallEdit_ReApply,
	WallEdit_Preview,
	WallEdit_OffsetLower,
	WallEdit_OffsetRaise,
	WallEdit_SizeLower,
	WallEdit_SizeRaise,

	WallEdit_FlipSelectedHorizontal,
	WallEdit_FlipSelectedVertical,


	Texture_NextTexture,
	Texture_PrevTexture,
	Texture_SelectFull,
	Texture_Delete,


	GatEdit_NextTileType,
	GatEdit_PrevTileType,
	GatEdit_Tile0,
	GatEdit_Tile1,
	GatEdit_Tile2,
	GatEdit_Tile3,
	GatEdit_Tile4,
	GatEdit_Tile5,
	GatEdit_Tile6,
	GatEdit_Tile7,
	GatEdit_Tile8,
	GatEdit_Tile9,
	GatEdit_AdjustToGround,

	ColorEdit_Dropper,

	HeightEdit_Doodle,
	HeightEdit_Rectangle,
	HeightEdit_Lasso,
	HeightEdit_MagicWandTexture,
	HeightEdit_MagicWandHeight,
	HeightEdit_SelectAllTexture,
	HeightEdit_SelectAllHeight,
	HeightEdit_PasteSpecial,
	HeightEdit_SmoothTiles,
	HeightEdit_FlattenTiles,
	HeightEdit_IncreaseHeight,
	HeightEdit_DecreaseHeight,

	EditMode_Height,
	EditMode_Texture,
	EditMode_Object,
	EditMode_Wall,
	EditMode_Gat,
	EditMode_Color,
	EditMode_Shadow,
	EditMode_Sprite,
	EditMode_Cinematic,
	EditMode_Water,

	View_ShadowMap,
	View_ColorMap,
	View_TileColors,
	View_Lighting,
	View_Textures,
	View_SmoothColormap,
	View_EmptyTiles,
	View_GatTiles,
	View_Fog,
	
	View_Models,
	View_Effects,
	View_Sounds,
	View_Lights,
	View_Water,
	View_EffectIcons,

	Camera_OrthoPerspective,
	Camera_MoveXPositive,
	Camera_MoveXNegative,
	Camera_MoveYPositive,
	Camera_MoveYNegative,
	Camera_MoveZPositive,
	Camera_MoveZNegative,
	Camera_RotateX45Positive,
	Camera_RotateX45Negative,
	Camera_RotateY45Positive,
	Camera_RotateY45Negative,

	TextureEdit_ToggleTextureWindow,
	TextureEdit_SwapBrushSize,
/*	Texture_RotateLeft,
	Texture_RotateRight,
	Texture_FlipHorizontal,
	Texture_FlipVertical,

	Gat_RaiseToHeight,*/
};

class HotkeyCombi
{
public:
	HotkeyAction action;
	Hotkey hotkey;
	std::function<bool()> condition = []() {return true; };
	std::function<void()> callback;
};

class HotkeyRegistry
{
public:
	inline static std::vector<std::string> actions;
	inline static std::map<std::string, Hotkey> defaultHotkeys;
	inline static std::vector<HotkeyCombi> hotkeys;
	static void init(const std::map<std::string, Hotkey>& config);
	static void registerAction(HotkeyAction, const std::function<void()>&, const std::function<bool()>& condition = []() {return true; });
	static const HotkeyCombi& getHotkey(HotkeyAction action);
	static bool checkHotkeys();
	static void runAction(HotkeyAction action);
	static void showHotkeyPopup(BrowEdit* browEdit);
};