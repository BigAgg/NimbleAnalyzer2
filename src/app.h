#pragma once
#include <string>
#include <vector>

class App {
public:
	void init(const char* name);
	void run();

private:
	// Initializing functions
	void createDirs();
	void initRaylib(const char* name);
	void initRessources();
	void initImGui();
	void loadSettings();
	void loadDefaults();
	// Logic functions
	void render();
	void handleDropfiles();
	// Shutdown functions
	void saveSettings();
	void cleanup();

	// ImGui menus
	void taskbar();
	void contentWindow();
	void logWindow();

	// Rebuilding logview
	void rebuildLogview();
	
	// window settings
	struct {
		int width, height;
		int fps;
		bool vsync;
		bool maximized;
		int posx, posy;
		int device;
		unsigned char theme;
	} m_windowSettings;

	struct {
		bool hasDroppedFile = false;
		std::vector<std::string> files;

		std::string getFirstFile(std::string endswith = "") {
			if (!hasDroppedFile || files.size() == 0) {
				hasDroppedFile = false;
				return "";
			}
			hasDroppedFile = false;
			if (endswith == "") {
				std::string file = files[0];
				files.clear();
				return file;
			}
			for (std::string file : files) {
				if (file.ends_with(endswith)) {
					files.clear();
					return file;
				}
			}
			files.clear();
			return "";
		}

		std::vector<std::string> getFiles(std::string endswith = "") {
			if(!hasDroppedFile)
			hasDroppedFile = false;
			std::vector<std::string> outfiles;
			for (std::string file : files) {
				if (file.ends_with(endswith)) {
					outfiles.push_back(file);
				}
			}
			files.clear();
			return outfiles;
		}
	} m_dragDrop;

	// Logwindow settings
	struct {
#ifdef NDEBUG
		bool displaylog = false;
		bool displayinfo = false;
		bool displaywarning = true;
		bool displayerror = true;
#else
		bool displaylog = true;
		bool displayinfo = true;
		bool displaywarning = true;
		bool displayerror = true;
#endif
		std::vector<std::string> log;
		size_t lastLogsize = 0;
	} m_logviewSettings;
};
