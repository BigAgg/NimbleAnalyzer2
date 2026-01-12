#include "app.h"
#include "logging.h"
#include "fileDialog.h"
#include "themes.h"
#include "utils.h"
#include "NimbleAnalyzer.h"
#include <tinyxml2.h>
#include <filesystem>
#include <fstream>
#include <raylib.h>
#include "ressourcemanager.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <rlImGui.h>
#include <imgui_stdlib.h>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace tinyxml2;

// General App settings
#define WINDOW_SETTINGS_FILE "window.bin"
#define LOG_WINDOW_HEIGHT 400.0f
#define TEXT_INPUT_WIDTH 400.0f

void App::init(const char* name){
	createDirs();
	loadSettings();
	initRaylib(name);
	initRessources();
	initImGui();
	na.init();
}

void App::run() {
	// Main loop
	while (!WindowShouldClose()) {
		render();
		handleDropfiles();
	}
	cleanup();
}

void App::createDirs(){
	if (!fs::exists("projects")) {
		fs::create_directory("projects");
		logging::loginfo("[App::createDirs] projects directory created!");
	}
}

void App::initRaylib(const char* name){
	InitWindow(m_windowSettings.width, m_windowSettings.height, name);
	SetWindowIcon(LoadImage("icon.png"));
	SetWindowState(FLAG_WINDOW_RESIZABLE);
	SetTargetFPS(m_windowSettings.fps);
	if (m_windowSettings.vsync) {
		SetWindowState(FLAG_VSYNC_HINT);
	}
	SetWindowMonitor(m_windowSettings.device);
	SetWindowPosition(m_windowSettings.posx, m_windowSettings.posy);
	if (m_windowSettings.maximized) {
		MaximizeWindow();
	}
	logging::loginfo("[App::initRaylib] Raylib initialized!");
}

void App::initRessources(){
	StoreTexture("themes/Light Purple.png", "theme_light_purple");
	StoreTexture("themes/Light Gold.png", "theme_light_gold");
	StoreTexture("themes/Light Blue.png", "theme_light_blue");
	StoreTexture("themes/Dark Purple.png", "theme_dark_purple");
	StoreTexture("themes/Dark Blue.png", "theme_dark_blue");
	StoreTexture("themes/Dark Gold.png", "theme_dark_gold");
}

void App::initImGui(){
	rlImGuiSetup(false);
	SetTheme(m_windowSettings.theme);
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	static const ImWchar glyphRanges[] = {
		0x0020, 0x00FF,
		0
	};
	if (fs::exists("fonts/JetBrainsMonoNerdFont-Bold.ttf")) {
		ImFont* font = io.Fonts->AddFontFromFileTTF("fonts/JetBrainsMonoNerdFont-Bold.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesDefault());
		if (font)
			io.FontDefault = font;
		else
			logging::logwarning("[App::initImGui] Font could not be loaded: %s", "fonts/JetBrainsMonoNerdFont-Bold.ttf");
	}
	else {
		logging::logwarning("[App::initImGui] Font file does not exist: %s", "fonts/JetBrainsMonoNerdFont-Bold.ttf");
	}
	logging::loginfo("[App::initImGui] ImGui initialized!");
}

void App::loadSettings(){
	// Check if settings exist or not
	if (!fs::exists(WINDOW_SETTINGS_FILE)) {
		loadDefaults();
	}
	else {
		// Open file and check if the file is good
		std::ifstream file(WINDOW_SETTINGS_FILE, std::ios::binary);
		if (!file) {
			loadDefaults();
		}
		else {
			// Load settings from file
			file.read((char*)&m_windowSettings, sizeof(m_windowSettings));
			file.close();
			logging::loginfo("[App::loadSettings] Windowsettings loaded from file!");
		}
	}
	if (m_windowSettings.posy <= 0)
		m_windowSettings.posy = 30;
}

void App::loadDefaults(){
	// Setup default settings for window
	m_windowSettings.width = 640;
	m_windowSettings.height = 480;
	m_windowSettings.fps = 60;
	m_windowSettings.vsync = false;
	m_windowSettings.maximized = false;
	m_windowSettings.posx = 0;
	m_windowSettings.posy = 0;
	m_windowSettings.device = 0;
	m_windowSettings.theme = Themes::DEFAULT;
	logging::loginfo("[App::loadDefaults] Default Windowsettings loaded!");
}

void App::saveSettings() {
	// Getting current window settings
	m_windowSettings.device = GetCurrentMonitor();
	if (IsWindowMinimized()) {
		RestoreWindow();
	}
	m_windowSettings.maximized = IsWindowMaximized();
	if (m_windowSettings.maximized)
		RestoreWindow();
	Vector2 pos = GetWindowPosition();
	m_windowSettings.posx = pos.x;
	m_windowSettings.posy = pos.y;
	m_windowSettings.width = GetScreenWidth();
	m_windowSettings.height = GetScreenHeight();
	m_windowSettings.vsync = IsWindowState(FLAG_VSYNC_HINT);
	// Open and check file
	std::ofstream file(WINDOW_SETTINGS_FILE, std::ios::binary);
	if (!file.is_open() || !file.good()) {
		logging::logerror("[App::saveSettings] Windowsettings could not be saved: %s", WINDOW_SETTINGS_FILE);
		return;
	}
	// Writing settings to file
	file.write((char*)&m_windowSettings, sizeof(m_windowSettings));
	file.close();
	logging::loginfo("[App::saveSettings] Windowsettings saved!");
}

void App::render(){
	BeginDrawing();
	ClearBackground(BLACK);
	rlImGuiBegin();
	taskbar();
	if (m_logviewSettings.displaylog)
		logWindow();
	contentWindow();
	rlImGuiEnd();
	EndDrawing();
}

void App::handleDropfiles(){
	m_dragDrop.hasDroppedFile = false;
	m_dragDrop.files.clear();
	if (IsFileDropped()) {
		ImGui::ClearActiveID();
		m_dragDrop.files.clear();
		FilePathList dropped = LoadDroppedFiles();
		m_dragDrop.hasDroppedFile = true;
		std::string log = "";
		if (dropped.count > 0) {
			for (int i = 0; i < dropped.count; i++) {
				std::string filename = dropped.paths[i];
				ReplaceAllSubstrings(filename, "\\", "/");
				ReplaceAllSubstrings(filename, "\"", "");
				m_dragDrop.files.push_back(filename);
				log += "\n" + filename;
			}
			logging::loginfo("[App::run] New files were dropped onto the program: %zu%s", dropped.count, log.c_str());
		}
		UnloadDroppedFiles(dropped);
	}
}

void App::cleanup() {
	saveSettings();
	rlImGuiShutdown();
	UnloadRessources();
	CloseWindow();
	na.cleanup();
	logging::loginfo("[App::cleanup] Window cleaned up and closed!");
}

void App::taskbar(){
	const float scale = 0.75f;
	ImGui::BeginMainMenuBar();
	if (ImGui::BeginMenu("Theme")) {
		ImGui::SeparatorText("Light themes");
		if (ImGui::Button("Light Blue")) {
			m_windowSettings.theme = Themes::LIGHT;
			SetTheme(m_windowSettings.theme);
		}
		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			Texture* texture = GetTexture("theme_light_blue");
			rlImGuiImageSize(texture, texture->width * scale, texture->height * scale);
			ImGui::EndTooltip();
		}
		if (ImGui::Button("Light Gold")) {
			m_windowSettings.theme = Themes::GOLD_LIGHT;
			SetTheme(m_windowSettings.theme);
		}
		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			Texture* texture = GetTexture("theme_light_gold");
			rlImGuiImageSize(texture, texture->width * scale, texture->height * scale);
			ImGui::EndTooltip();
		}
		if (ImGui::Button("Light Purple")) {
			m_windowSettings.theme = Themes::PURPLE_LIGHT;
			SetTheme(m_windowSettings.theme);
		}
		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			Texture* texture = GetTexture("theme_light_purple");
			rlImGuiImageSize(texture, texture->width * scale, texture->height * scale);
			ImGui::EndTooltip();
		}
		ImGui::SeparatorText("Dark themes");
		if (ImGui::Button("Dark Blue")) {
			m_windowSettings.theme = Themes::DARK;
			SetTheme(m_windowSettings.theme);
		}
		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			Texture* texture = GetTexture("theme_dark_blue");
			rlImGuiImageSize(texture, texture->width * scale, texture->height * scale);
			ImGui::EndTooltip();
		}
		if (ImGui::Button("Dark Gold")) {
			m_windowSettings.theme = Themes::GOLD_DARK;
			SetTheme(m_windowSettings.theme);
		}
		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			Texture* texture = GetTexture("theme_dark_gold");
			rlImGuiImageSize(texture, texture->width * scale, texture->height * scale);
			ImGui::EndTooltip();
		}
		if (ImGui::Button("Dark Purple")) {
			m_windowSettings.theme = Themes::PURPLE_DARK;
			SetTheme(m_windowSettings.theme);
		}
		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			Texture* texture = GetTexture("theme_dark_purple");
			rlImGuiImageSize(texture, texture->width * scale, texture->height * scale);
			ImGui::EndTooltip();
		}
		ImGui::EndMenu();
	}
	ImGui::Checkbox("Log window", &m_logviewSettings.displaylog);
	ImGui::EndMainMenuBar();
}


void App::contentWindow(){
	std::string windowname = "Project window";
	ImGuiViewport* viewport = ImGui::GetMainViewport();

	ImGui::SetNextWindowPos(viewport->WorkPos);
	if (m_logviewSettings.displaylog) {
		ImGui::SetNextWindowSize({ viewport->WorkSize.x, viewport->WorkSize.y - LOG_WINDOW_HEIGHT });
	}
	else {
		ImGui::SetNextWindowSize(viewport->WorkSize);
	}

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_MenuBar |
		ImGuiWindowFlags_NoNavFocus;

	ImGui::Begin(windowname.c_str(), nullptr, flags);
	if (ImGui::BeginMenuBar()) {
		na.menubar();
	}
	ImGui::EndMenuBar();

	if (ImGui::BeginChild("ContentWindow", { 0, 0 }, true, ImGuiWindowFlags_HorizontalScrollbar)) {
		na.contentwindow();
	}
	ImGui::EndChild();
	ImGui::End();
}

void App::logWindow(){
	ImGuiViewport* viewport = ImGui::GetMainViewport();

	ImGui::SetNextWindowPos({ viewport->WorkPos.x, viewport->WorkPos.y + viewport->WorkSize.y - LOG_WINDOW_HEIGHT});
	ImGui::SetNextWindowSize({ viewport->WorkSize.x, LOG_WINDOW_HEIGHT });

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_MenuBar |
		ImGuiWindowFlags_NoNavFocus;

	ImGui::Begin("log", nullptr, flags);
	if (ImGui::BeginMenuBar()) {
		if(ImGui::Checkbox("Info", &m_logviewSettings.displayinfo))
			rebuildLogview();
		if(ImGui::Checkbox("Warning", &m_logviewSettings.displaywarning))
			rebuildLogview();
		if(ImGui::Checkbox("Error", &m_logviewSettings.displayerror))
			rebuildLogview();
	}
	ImGui::EndMenuBar();
	if (ImGui::BeginChild("Logview", { 0, 0 }, true, ImGuiWindowFlags_HorizontalScrollbar)) {
		if (m_logviewSettings.lastLogsize < logging::GetAllMessages().size()) {
			rebuildLogview();
		}
		const int count = (int)m_logviewSettings.log.size();
		for (int i = count-1; i >= 0; i--) {
			ImGui::PushID(i);
			if (ImGui::Selectable(m_logviewSettings.log[i].c_str())) {
				ImGui::SetClipboardText(m_logviewSettings.log[i].c_str());
			}
			ImGui::PopID();
		}
	}
	ImGui::EndChild();
	ImGui::End();
}

void App::rebuildLogview(){
	m_logviewSettings.log = logging::GetAllMessages();
	m_logviewSettings.lastLogsize = m_logviewSettings.log.size();
	
	if (!m_logviewSettings.displayerror) {
		m_logviewSettings.log.erase(
			std::remove_if(m_logviewSettings.log.begin(), m_logviewSettings.log.end(),
				[](const std::string& s) {
					return s.find("[ERROR]") != std::string::npos;
				}),
			m_logviewSettings.log.end()
		);
	}
	if (!m_logviewSettings.displayinfo) {
		m_logviewSettings.log.erase(
			std::remove_if(m_logviewSettings.log.begin(), m_logviewSettings.log.end(),
				[](const std::string& s) {
					return s.find("[INFO]") != std::string::npos;
				}),
			m_logviewSettings.log.end()
		);
	}
	if (!m_logviewSettings.displaywarning) {
		m_logviewSettings.log.erase(
			std::remove_if(m_logviewSettings.log.begin(), m_logviewSettings.log.end(),
				[](const std::string& s) {
					return s.find("[WARNING]") != std::string::npos;
				}),
			m_logviewSettings.log.end()
		);
	}
}
