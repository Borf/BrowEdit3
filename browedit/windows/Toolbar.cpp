#include <Windows.h>
#include <glad/gl.h>
#include <browedit/Icons.h>
#include <browedit/BrowEdit.h>
#include <browedit/MapView.h>
#include <browedit/Map.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/Rsw.h>
#include <browedit/util/ResourceManager.h>
#include <browedit/gl/Texture.h>
#include <browedit/Node.h>
#include <browedit/HotkeyRegistry.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>

class Image;

void BrowEdit::toolbar()
{
	auto viewport = ImGui::GetMainViewport();
	statusBarHeight = ImGui::CalcTextSize("").y + 12 + 2 * ImGui::GetStyle().FramePadding.y;

	ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y));
	ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, config.toolbarHeight()));
	ImGuiWindowFlags toolbarFlags = 0
		| ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove;
	ImGui::Begin("Toolbar", 0, toolbarFlags);

		if (editMode == EditMode::Texture)
		ImGui::Text("Texture");
	else if (editMode == EditMode::Height)
		ImGui::Text("Height");
	else if (editMode == EditMode::Object)
		ImGui::Text("Object");
	else if (editMode == EditMode::Gat)
		ImGui::Text("GAT");
	else if (editMode == EditMode::Wall)
		ImGui::Text("Wall");
	else if (editMode == EditMode::Color)
		ImGui::Text("Color");
	else if (editMode == EditMode::Shadow)
		ImGui::Text("Shadow");
	else if (editMode == EditMode::Sprite)
		ImGui::Text("Sprite");
	else if (editMode == EditMode::Cinematic)
		ImGui::Text("Cinematic");
	else
		ImGui::Text("???");
	ImGui::SameLine();
	ImGui::SetCursorPosX(125);

	toolBarToggleButton("heightmode", ICON_EDIT_HEIGHT, editMode == EditMode::Height, "Height edit mode", HotkeyAction::EditMode_Height, config.toolbarButtonsHeightEdit);
	ImGui::SameLine();
	toolBarToggleButton("texturemode", ICON_EDIT_TEXTURE, editMode == EditMode::Texture, "Texture edit mode", HotkeyAction::EditMode_Texture, config.toolbarButtonsTextureEdit);
	ImGui::SameLine();
	toolBarToggleButton("objectmode", ICON_EDIT_OBJECT, editMode == EditMode::Object, "Object edit mode", HotkeyAction::EditMode_Object, config.toolbarButtonsObjectEdit);
	ImGui::SameLine();
	toolBarToggleButton("gatmode", ICON_EDIT_GAT, editMode == EditMode::Gat, "Gat edit mode", HotkeyAction::EditMode_Gat, config.toolbarButtonsGatEdit);
	ImGui::SameLine();
	toolBarToggleButton("wallmode", ICON_EDIT_WALL, editMode == EditMode::Wall, "Wall edit mode", HotkeyAction::EditMode_Wall, config.toolbarButtonsWallEdit);
	ImGui::SameLine();
	toolBarToggleButton("color", ICON_EDIT_COLOR, editMode == EditMode::Color, "Color edit mode", HotkeyAction::EditMode_Color, config.toolbarButtonsWallEdit);
	ImGui::SameLine();
	toolBarToggleButton("shadow", ICON_EDIT_SHADOW, editMode == EditMode::Shadow, "Shadow edit mode", HotkeyAction::EditMode_Shadow, config.toolbarButtonsWallEdit);
	ImGui::SameLine();
	toolBarToggleButton("sprite", ICON_EDIT_SPRITE, editMode == EditMode::Sprite, "Sprite edit mode", HotkeyAction::EditMode_Sprite, config.toolbarButtonsWallEdit);
	ImGui::SameLine();
	toolBarToggleButton("cinematic", ICON_EDIT_CINEMATIC, editMode == EditMode::Cinematic, "Cinematic Mode", HotkeyAction::EditMode_Cinematic, config.toolbarButtonsWallEdit);
	ImGui::SameLine(130 + 9 * (config.toolbarButtonSize + 5) + 20 );

	if (editMode == EditMode::Object)
	{
		toolBarToggleButton("showObjectWindow", windowData.objectWindowVisible ? ICON_OBJECTPICKER_OPEN : ICON_OBJECTPICKER_CLOSE, windowData.objectWindowVisible, "Toggle object window", HotkeyAction::ObjectEdit_ToggleObjectWindow, config.toolbarButtonsObjectEdit);
		ImGui::SameLine();
	}
	if (editMode == EditMode::Texture)
	{
		toolBarToggleButton("showTextureWindow", windowData.textureManageWindowVisible ? ICON_TEXTUREPICKER_OPEN : ICON_TEXTUREPICKER_CLOSE, windowData.textureManageWindowVisible, "Toggle texture window", HotkeyAction::TextureEdit_ToggleTextureWindow, config.toolbarButtonsTextureEdit);
		ImGui::SameLine();
	}


	if (toolBarToggleButton("undo", ICON_UNDO, false, "Undo") && activeMapView)
		HotkeyRegistry::runAction(HotkeyAction::Global_Undo);
	ImGui::SameLine();
	if (toolBarToggleButton("redo", ICON_REDO, false, "Redo") && activeMapView)
		HotkeyRegistry::runAction(HotkeyAction::Global_Redo);
	ImGui::SameLine();
	if (toolBarToggleButton("copy", ICON_COPY, false, "Copy") && activeMapView)
		activeMapView->map->copySelection();
	ImGui::SameLine();
	if (toolBarToggleButton("paste", ICON_PASTE, false, "Paste") && activeMapView)
		activeMapView->map->pasteSelection(this);
	ImGui::SameLine();



	ImGui::End();




	ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + viewport->WorkSize.y - statusBarHeight));
	ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, statusBarHeight));
	toolbarFlags = 0
		| ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove;
	ImGui::Begin("Statusbar", 0, toolbarFlags);
	ImGui::Text("Browedit!");

	/*static ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
	static int numProcessors = -1;
	static HANDLE self;
	if (numProcessors == -1)
	{
		SYSTEM_INFO sysInfo;
		FILETIME ftime, fsys, fuser;

		GetSystemInfo(&sysInfo);
		numProcessors = sysInfo.dwNumberOfProcessors;

		GetSystemTimeAsFileTime(&ftime);
		memcpy(&lastCPU, &ftime, sizeof(FILETIME));

		self = GetCurrentProcess();
		GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
		memcpy(&lastSysCPU, &fsys, sizeof(FILETIME));
		memcpy(&lastUserCPU, &fuser, sizeof(FILETIME));
	}

	FILETIME ftime, fsys, fuser;
	ULARGE_INTEGER now, sys, user;
	double percent;

	GetSystemTimeAsFileTime(&ftime);
	memcpy(&now, &ftime, sizeof(FILETIME));

	GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
	memcpy(&sys, &fsys, sizeof(FILETIME));
	memcpy(&user, &fuser, sizeof(FILETIME));
	percent = (sys.QuadPart - lastSysCPU.QuadPart) +
		(user.QuadPart - lastUserCPU.QuadPart);
	percent /= (now.QuadPart - lastCPU.QuadPart);
	percent /= numProcessors;
	lastCPU = now;
	lastUserCPU = user;
	lastSysCPU = sys;*/

	static char images[100];
	if (util::ResourceManager<Image>::count() > 0)
		sprintf_s(images, 100, "Images(%zu), ", util::ResourceManager<Image>::count());
	else
		sprintf_s(images, 100, "");

	std::string stats = std::format("Load: Tex({}), Models({}), {}Mem({:.3g} {} / {:.3g} {}), GPU({:.3g} {} / {:.3g} {})",
		util::ResourceManager<gl::Texture>::count(), util::ResourceManager<Rsm>::count(), images,
		util::toUnitSize(memoryLimits.systemUsed), util::toSizeUnit(memoryLimits.systemUsed), util::toUnitSize(memoryLimits.systemSize), util::toSizeUnit(memoryLimits.systemSize),
		util::toUnitSize(memoryLimits.gpuUsed), util::toSizeUnit(memoryLimits.gpuUsed), util::toUnitSize(memoryLimits.gpuSize), util::toSizeUnit(memoryLimits.gpuSize)
	);
	auto len = ImGui::CalcTextSize(stats.c_str());
	ImGui::SameLine(ImGui::GetWindowWidth() - len.x - 6 - ImGui::GetStyle().FramePadding.x);
	ImRect bb(ImGui::GetCursorScreenPos() - ImVec2(3,3), ImGui::GetCursorScreenPos() + len + ImVec2(6,6));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::RenderFrame(bb.Min, bb.Max, 0, true, ImGui::GetStyle().FrameRounding);
	ImGui::Text(stats.c_str());

	if (activeMapView)
	{
		if (!activeMapView->map->rootNode->getComponent<Gnd>())
			std::cerr << "Map has no gnd" << std::endl;
		else
		{
			int objectCount = 0;
			activeMapView->map->rootNode->traverse([&objectCount](Node* n) { if (n->getComponent<RswObject>()) objectCount++; });

			stats = std::format("Map: {}, Tiles({}), Lightmaps({}), Objects({})", activeMapView->map->name.c_str(), activeMapView->map->rootNode->getComponent<Gnd>()->tiles.size(), activeMapView->map->rootNode->getComponent<Gnd>()->lightmaps.size(), objectCount);
			auto len2 = ImGui::CalcTextSize(stats.c_str());

			ImGui::SameLine(ImGui::GetWindowWidth() - len2.x - len.x - 2 * ImGui::GetStyle().FramePadding.x - 14);

			ImRect bb(ImGui::GetCursorScreenPos() - ImVec2(3, 3), ImGui::GetCursorScreenPos() + len2 + ImVec2(6, 6));
			ImGui::RenderFrame(bb.Min, bb.Max, 0, true, ImGui::GetStyle().FrameRounding);

			ImGui::Text(stats.c_str());
		}
	}
	ImGui::PopStyleVar();
	ImGui::SameLine(150);
	ImGui::End();
}