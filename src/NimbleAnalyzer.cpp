#include "NimbleAnalyzer.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <misc/cpp/imgui_stdlib.h>
#include "utils.h"
#include "fileloader.h"
#include "logging.h"
#include "project.h"
#include "fileDialog.h"
#include <string>
#include <vector>

namespace fl = fileloader;
#define LISTBOX_WIDTH 275.0f
#define LISTBOX_HEIGHT 200.0f
#define TEXT_INPUT_WIDTH LISTBOX_WIDTH 
#define CHILD_WINDOW_WIDTH (LISTBOX_WIDTH + 20)
#define CHILD_WINDOW_HEIGHT 325.0f

// Storing data for project selection
static struct projectData {
	std::string name;
	std::string path;
	void clear() {
		name.clear();
		path.clear();
	}
};

static struct {
	std::vector<projectData> projectsAvail;
	Project project;
	std::string newProject;

	void clear() {
		// Cleaning up the available projects
		for (auto& pd : projectsAvail) {
			pd.clear();
		}
		projectsAvail.clear();
	}
} projectInfo;

static void loadProjectsAvail();
static void generateProject(const std::string& name);
static void deleteProject(const std::string& name);
static void projectSelection();
static void fileSelection();

void NimbleAnalyzer::init(){
	loadProjectsAvail();
}

void NimbleAnalyzer::menubar(){
}

void NimbleAnalyzer::contentwindow(){
	ImGui::BeginChild("Project selection", { CHILD_WINDOW_WIDTH, CHILD_WINDOW_HEIGHT }, true);
	projectSelection();
	ImGui::EndChild();
	ImGui::SameLine();
	ImGui::BeginChild("File selection", { CHILD_WINDOW_WIDTH, CHILD_WINDOW_HEIGHT }, true);
	fileSelection();
	ImGui::EndChild();
}

void NimbleAnalyzer::cleanup(){
	if(projectInfo.project.loaded)
		projectInfo.project.save();
	projectInfo.clear();
}

static void loadProjectsAvail() {
	projectInfo.projectsAvail.clear();
	for (const auto& path : fl::iteratePath("projects", false)) {
		if (!fl::exists(path)) {
			logging::logwarning("[NimbleAnalyzer::loadProjectsAvail] Returned path does not exist: %s", path.c_str());
			continue;
		}
		projectData pdata{};
		pdata.name = fl::getFilename(path);
		pdata.path = path;
		projectInfo.projectsAvail.push_back(pdata);
	}
	logging::loginfo("[NimbleAnalyzer::loadProjectsAvail] Available projects found: %zu", projectInfo.projectsAvail.size());
}

static void generateProject(const std::string& name) {
	const std::string path = "projects/" + name;
	if (fl::exists(path)) {
		logging::logwarning("[NimbleAnalyzer::generateProject] Project does already exist: %s", name.c_str());
		return;
	}
	fl::createDirs(path);
	// Generating the settings file
	Project p{};
	p.load(name, path);
	p.save();
	loadProjectsAvail();
}

static void deleteProject(const std::string& name) {
	const std::string path = "projects/" + name;
	fl::del(path);
	loadProjectsAvail();
}

static void projectSelection() {
	ImGui::Text("Projects");
	if (ImGui::BeginListBox("## Project Selection", { LISTBOX_WIDTH, LISTBOX_HEIGHT })) {
		for (const auto& project : projectInfo.projectsAvail) {
			bool selected = (project.name == projectInfo.project.name);
			if (ImGui::Selectable(project.name.c_str(), &selected)) {
				if (projectInfo.project.loaded)
					projectInfo.project.save();
				projectInfo.project.clear();
				projectInfo.project.load(project.name, project.path);
			}
		}
		ImGui::EndListBox();
	}
	ImGui::SetNextItemWidth(TEXT_INPUT_WIDTH);
	if (ImGui::InputTextWithHint("## New Project", "New Project", &projectInfo.newProject));
	if (ImGui::Button("Create")) {
		if (projectInfo.newProject != "")
			generateProject(projectInfo.newProject);
		projectInfo.newProject = "";
	}
	ImGui::SameLine();
	if (ImGui::Button("Delete")) {
		if (projectInfo.project.name != "") {
			deleteProject(projectInfo.project.name);
			projectInfo.project.clear();
		}
	}
	if (ImGui::Button("Import")) {
		std::string path = OpenDirectoryDialog();
		if (path != "") {
			convertContentToUTF8(&path);
			std::string name = fl::getFilename(path);
			fl::copy(path, "projects/" + name);
			loadProjectsAvail();
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Export")) {
		std::string path = OpenDirectoryDialog();
		if (path != "") {
			convertContentToUTF8(&path);
			fl::copy(projectInfo.project.path, path + "/" + projectInfo.project.name);
		}
	}
}

void fileSelection(){
	if (!projectInfo.project.loaded)
		return;
	ImGui::Text("Files");
	if (ImGui::BeginListBox("## File Selection", { LISTBOX_WIDTH, LISTBOX_HEIGHT })) {
		for (const auto& file : projectInfo.project.files) {
			bool selected = (file == projectInfo.project.activeFile.path);
			if (ImGui::Selectable(fl::getFilename(file).c_str(), &selected)) {
				projectInfo.project.loadfile(file);
			}
			ImGui::SetItemTooltip(file.c_str());
		}
		ImGui::EndListBox();
	}
	if (ImGui::Button("Add")) {
		std::string path = OpenFileDialog("Excel Sheet", "xlsx,csv");
		if (path != "") {
			convertContentToUTF8(&path);
			projectInfo.project.addfile(path);
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Remove")) {
		projectInfo.project.removefile(projectInfo.project.activeFile.path);
		projectInfo.project.activeFile.clear();
	}
}
