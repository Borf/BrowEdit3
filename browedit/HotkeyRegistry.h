#pragma once

#include <string>
#include <vector>
#include <map>
#include <browedit/Hotkey.h>

class BrowEdit;

enum class HotkeyAction
{
	Global_HotkeyPopup,
	Global_Save,
	Global_Load,
	Global_Exit,
	Global_Undo,
	Global_Redo,
	Global_Settings,
	Global_ReloadTextures,
	Global_ReloadModels,

	Global_Copy,
	Global_Paste,
	Global_PasteChangeHeight,

	Global_CalculateQuadtree,
	Global_CalculateLightmaps,

	ObjectEdit_InvertScaleX,
	ObjectEdit_InvertScaleY,
	ObjectEdit_InvertScaleZ,
	ObjectEdit_FlipHorizontal,
	ObjectEdit_FlipVertical,
	ObjectEdit_Delete,
	ObjectEdit_FocusOnSelection,
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

	ObjectEdit_InvertSelection,
	ObjectEdit_SelectAll,
	ObjectEdit_SelectAllModels,
	ObjectEdit_SelectAllEffects,
	ObjectEdit_SelectAllSounds,
	ObjectEdit_SelectAllLights,


	EditMode_Height,
	EditMode_Texture,
	EditMode_Object,
	EditMode_Wall,
	EditMode_Gat,


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
	inline static std::vector<HotkeyCombi> hotkeys;
	static void init(const std::map<std::string, Hotkey>& config);
	static void registerAction(HotkeyAction, const std::function<void()>&, const std::function<bool()>& condition = []() {return true; });
	static const HotkeyCombi& getHotkey(HotkeyAction action);
	static bool checkHotkeys();
	static void runAction(HotkeyAction action);
	static void showHotkeyPopup(BrowEdit* browEdit);
};