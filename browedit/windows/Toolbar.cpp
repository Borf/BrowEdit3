#include <Windows.h>
#include <glad/glad.h>
#include <browedit/Icons.h>
#include <browedit/BrowEdit.h>
#include <browedit/MapView.h>
#include <browedit/Map.h>
#include <browedit/components/Gnd.h>
#include <browedit/components/Rsw.h>
#include <browedit/util/ResourceManager.h>
#include <browedit/gl/Texture.h>
#include <browedit/Node.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>

#include <Windows.h>
#include <psapi.h>

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
	else
		ImGui::Text("???");
	ImGui::SameLine();
	ImGui::SetCursorPosX(125);

	if (toolBarToggleButton("heightmode", ICON_EDIT_HEIGHT, editMode == EditMode::Height, "Height edit mode", config.toolbarButtonsHeightEdit))
		editMode = EditMode::Height;
	ImGui::SameLine();
	if (toolBarToggleButton("texturemode", ICON_EDIT_TEXTURE, editMode == EditMode::Texture, "Texture edit mode", config.toolbarButtonsTextureEdit))
		editMode = EditMode::Texture;
	ImGui::SameLine();
	if (toolBarToggleButton("objectmode", ICON_EDIT_OBJECT, editMode == EditMode::Object, "Object edit mode", config.toolbarButtonsObjectEdit))
		editMode = EditMode::Object;
	ImGui::SameLine();
	if (toolBarToggleButton("gatmode", ICON_EDIT_GAT, editMode == EditMode::Gat, "Gat edit mode", config.toolbarButtonsGatEdit))
		editMode = EditMode::Gat;
	ImGui::SameLine();
	if (toolBarToggleButton("wallmode", ICON_EDIT_WALL, editMode == EditMode::Wall, "Wall edit mode", config.toolbarButtonsWallEdit))
		editMode = EditMode::Wall;
	ImGui::SameLine(130 + 5 * (config.toolbarButtonSize + 5) + 20 );

	if (editMode == EditMode::Object)
	{
		toolBarToggleButton("showObjectWindow", windowData.objectWindowVisible ? ICON_OBJECTPICKER_OPEN : ICON_OBJECTPICKER_CLOSE, &windowData.objectWindowVisible, "Toggle object window", config.toolbarButtonsObjectEdit);
		ImGui::SameLine();
	}
	if (editMode == EditMode::Texture)
	{
		toolBarToggleButton("showTextureWindow", windowData.textureManageWindowVisible ? ICON_TEXTUREPICKER_OPEN : ICON_TEXTUREPICKER_CLOSE, &windowData.textureManageWindowVisible, "Toggle texture window", config.toolbarButtonsTextureEdit);
		ImGui::SameLine();
	}


	if (toolBarToggleButton("undo", ICON_UNDO, false, "Undo") && activeMapView)
		activeMapView->map->undo(this);
	ImGui::SameLine();
	if (toolBarToggleButton("redo", ICON_REDO, false, "Redo") && activeMapView)
		activeMapView->map->undo(this);
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


	PROCESS_MEMORY_COUNTERS_EX pmc;
	GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
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

	int mem[4] = { 0,0,0,0 };
	glGetIntegerv(0x9048, &mem[1]); //GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX      
	glGetIntegerv(0x9049, &mem[0]); //GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX  
	static char txt[1024];
	sprintf_s(txt, 1024, "Load: Tex(%zu), Models(%zu), %sMem(%zu MB), GPU(%d MB / %d MB)", util::ResourceManager<gl::Texture>::count(), util::ResourceManager<Rsm>::count(), images, pmc.WorkingSetSize / 1024/1024, (mem[1]-mem[0])/1024, mem[1]/1024);
	auto len = ImGui::CalcTextSize(txt);
	ImGui::SameLine(ImGui::GetWindowWidth() - len.x - 6 - ImGui::GetStyle().FramePadding.x);
	ImRect bb(ImGui::GetCursorScreenPos() - ImVec2(3,3), ImGui::GetCursorScreenPos() + len + ImVec2(6,6));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::RenderFrame(bb.Min, bb.Max, 0, true, ImGui::GetStyle().FrameRounding);
	ImGui::Text(txt);

	if (activeMapView)
	{
		int objectCount = 0;
		activeMapView->map->rootNode->traverse([&objectCount](Node* n) { if (n->getComponent<RswObject>()) objectCount++; });
		
		sprintf_s(txt, 1024, "Map: %s, Tiles(%zu), Lightmaps(%zu), Objects(%d)", activeMapView->map->name.c_str(), activeMapView->map->rootNode->getComponent<Gnd>()->tiles.size(), activeMapView->map->rootNode->getComponent<Gnd>()->lightmaps.size(), objectCount);
		auto len2 = ImGui::CalcTextSize(txt);

		ImGui::SameLine(ImGui::GetWindowWidth() - len2.x - len.x - 2*ImGui::GetStyle().FramePadding.x - 14);

		ImRect bb(ImGui::GetCursorScreenPos() - ImVec2(3, 3), ImGui::GetCursorScreenPos() + len2 + ImVec2(6,6));
		ImGui::RenderFrame(bb.Min, bb.Max, 0, true, ImGui::GetStyle().FrameRounding);

		ImGui::Text(txt);
	}
	ImGui::PopStyleVar();
	ImGui::SameLine(150);
	ImGui::End();
}