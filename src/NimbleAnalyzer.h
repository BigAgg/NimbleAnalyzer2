#pragma once

#include <string>

enum class ViewMode {
	ProjectSelection,
	DataView,
	JustMerge
};

class NimbleAnalyzer {
public:
	void init();
	void menubar();
	void contentwindow();
	void cleanup();
private:
	
	void dataView();
	void justMerge();
	void loadProjectsAvail();
	void generateProject(const std::string& name);
	void deleteProject(const std::string& name);
	void projectSelection();
	void fileSelection();
	void sheetSelection();
	
	ViewMode viewmode = ViewMode::ProjectSelection;
};
