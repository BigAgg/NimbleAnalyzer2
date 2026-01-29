#pragma once

#include <string>

enum class ViewMode {
	ProjectSelection,
	DataView,
	JustMerge,
	Update
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
	void update();
	void loadProjectsAvail();
	void generateProject(const std::string& name);
	void deleteProject(const std::string& name);
	void projectSelection();
	void fileSelection();
	void sheetSelection();
	void showHeaders();
	void mergeSettings();
	void filterRows();
	
	ViewMode viewmode = ViewMode::ProjectSelection;
};
