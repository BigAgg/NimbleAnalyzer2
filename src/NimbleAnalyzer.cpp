#include "NimbleAnalyzer.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <misc/cpp/imgui_stdlib.h>
#include "utils.h"
#include "fileloader.h"
#include "logging.h"
#include "project.h"
#include <raylib.h>
#include "fileDialog.h"
#include <string>
#include <vector>
#include <charconv>

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


void NimbleAnalyzer::init(){
	loadProjectsAvail();
}

static std::string g_search = "";
static std::string g_search_header = "##NONE_HEADER";
static std::vector<int> filteredRows;

void NimbleAnalyzer::menubar(){
	switch (viewmode) {
	case ViewMode::ProjectSelection:
		if (ImGui::Button("Data view"))
			viewmode = ViewMode::DataView;
		if (ImGui::Button("Just merge"))
			viewmode = ViewMode::JustMerge;
		break;
	case ViewMode::DataView:
		if (ImGui::Button("Project selection"))
			viewmode = ViewMode::ProjectSelection;
		if (ImGui::Button("Just merge"))
			viewmode = ViewMode::JustMerge;
		ImGui::SetNextItemWidth(TEXT_INPUT_WIDTH);
		ImGui::InputTextWithHint("##search", "search", &g_search);
		ImGui::SetItemTooltip("Filter settings:\n\
'<X' Filters for everything smaller X\n\
'>X' Filters for everything bigger X\n\
'<min;max>' Filters for everything inbetween min and max\n\
'%c' At the start of the filter searches for everything that contains your text after\n\
'!' At the start of the filter searches for everything that does not contain your text after\n\
No Filters just searches for everything that starts with your searchtext", '%');
		ImGui::SetNextItemWidth(LISTBOX_WIDTH);
		if (ImGui::BeginCombo("Search only in header", g_search_header.c_str())) {
			bool selected = (g_search_header == "##NONE_HEADER");
			if (ImGui::Selectable("##NONE_HEADER", &selected)) {
				g_search_header = "##NONE_HEADER";
			}
			if (selected)
				ImGui::SetItemDefaultFocus();
			for (int n = 0; n < projectInfo.project.activeFile.columns.size(); n++) {
				const std::string& s = header_label(projectInfo.project.activeFile.columns[n].key);
				bool selected = (s == g_search_header);
				if (ImGui::Selectable(s.c_str(), &selected)) {
					g_search_header = s;
				}
				if (selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		break;
	case ViewMode::JustMerge:
		if (ImGui::Button("Project selection"))
			viewmode = ViewMode::ProjectSelection;
		if (ImGui::Button("Data view"))
			viewmode = ViewMode::DataView;
		break;
	}
}

void NimbleAnalyzer::contentwindow(){
	switch (viewmode) {
	case ViewMode::ProjectSelection:
		ImGui::BeginChild("Project selection", { CHILD_WINDOW_WIDTH, CHILD_WINDOW_HEIGHT }, true);
		projectSelection();
		ImGui::EndChild();
		ImGui::SameLine();
		ImGui::BeginChild("File selection", { CHILD_WINDOW_WIDTH, CHILD_WINDOW_HEIGHT }, true);
		fileSelection();
		ImGui::EndChild();
		ImGui::SameLine();
		ImGui::BeginChild("Sheet selection", { CHILD_WINDOW_WIDTH, CHILD_WINDOW_HEIGHT }, true);
		sheetSelection();
		ImGui::EndChild();
		break;
	case ViewMode::DataView:
		dataView();
		break;
	case ViewMode::JustMerge:
		justMerge();
		break;
	default:
		viewmode = ViewMode::ProjectSelection;
		break;
	}
}

void NimbleAnalyzer::cleanup(){
	if(projectInfo.project.loaded)
		projectInfo.project.save();
	projectInfo.clear();
}

static bool RowMatchesFilter(int r) {
		bool skip = true;
		if (!g_search.empty()) {
			for (int c = 0; c < (int)projectInfo.project.activeFile.columns.size(); ++c) {
				const auto& cell = projectInfo.project.activeFile.columns[c].values[r];
				if (g_search_header != "##NONE_HEADER" && g_search_header != header_label(projectInfo.project.activeFile.columns[c].key))
					continue;
				if (g_search.starts_with("<") && g_search.ends_with(">")) {
					auto* i = std::get_if<std::int64_t>(&cell.first);
					auto* d = std::get_if<double>(&cell.first);
					std::string s = g_search;
					s = normalize_decimal(s);
					s.erase(0, 1);
					s.erase(s.size() - 1, 1);
					auto split = Splitlines(s, ";");
					double value1;
					double value2;
					auto result = std::from_chars(split.first.data(), split.first.data() + split.first.size(), value1);
					if (!(result.ec == std::errc() && result.ptr == split.first.data() + split.first.size())) {
						continue;
					}
					result = std::from_chars(split.second.data(), split.second.data() + split.second.size(), value2);
					if (!(result.ec == std::errc() && result.ptr == split.second.data() + split.second.size())) {
						continue;
					}
					if (i && *i > value1 && *i < value2) {
						skip = false;
						break;
					}
					else if(d && *d > value1 && *d < value2) {
						skip = false;
						break;
					}
				}
				else if (g_search.starts_with("<")) {
					auto* i = std::get_if<std::int64_t>(&cell.first);
					auto* d = std::get_if<double>(&cell.first);
					std::string s = g_search;
					s = normalize_decimal(s);
					s.erase(0, 1);
					double value;
					auto result = std::from_chars(s.data(), s.data() + s.size(), value);
					if (!(result.ec == std::errc() && result.ptr == s.data() + s.size())) {
						continue;
					}
					if (i && *i < value) {
						skip = false;
						break;
					}
					else if(d && *d < value) {
						skip = false;
						break;
					}
				}
				else if (g_search.starts_with(">")) {
					auto* i = std::get_if<std::int64_t>(&cell.first);
					auto* d = std::get_if<double>(&cell.first);
					std::string s = g_search;
					s = normalize_decimal(s);
					s.erase(0, 1);
					double value;
					auto result = std::from_chars(s.data(), s.data() + s.size(), value);
					if (!(result.ec == std::errc() && result.ptr == s.data() + s.size())) {
						continue;
					}
					if (i && *i > value) {
						skip = false;
						break;
					}
					else if(d && *d > value) {
						skip = false;
						break;
					}
				}
				else if (g_search.starts_with("!")) {
					std::string s = g_search;
					s.erase(0, 1);
					if (!cell.second.contains(s)) {
						skip = false;
					}
					else {
						skip = true;
						break;
					}
				}
				else if (g_search.starts_with("%")) {
					std::string s = g_search;
					s.erase(0, 1);
					if (cell.second.contains(s)) {
						skip = false;
						break;
					}
				}
				else if (cell.second.starts_with(g_search)) {
					skip = false;
					break;
				}
			}
		}
		else
			skip = false;
		return !skip;
}

static struct ActiveCell { int row = -1; int col = -1; };
static ActiveCell g_active;
void NimbleAnalyzer::dataView(){
	static std::unordered_map<CellKey, std::string, CellKeyHash> editBuf;

	if (projectInfo.project.activeFile.columns.empty()) {
		ImGui::TextUnformatted("No data available.");
		return;
	}

	ImGuiTableFlags flags =
		ImGuiTableFlags_Borders |
		ImGuiTableFlags_RowBg |
		ImGuiTableFlags_Resizable |
		ImGuiTableFlags_ScrollY |
		ImGuiTableFlags_ScrollX;

	if (ImGui::BeginTable("##sheet_table", (int)projectInfo.project.activeFile.columns.size(), flags)) {
		ImGui::TableSetupScrollFreeze(0, 1);
		for (int c = 0; c < (int)projectInfo.project.activeFile.columns.size(); ++c) {
			const auto& col = projectInfo.project.activeFile.columns[c];
			ImGui::TableSetupColumn(header_label(col.key).c_str(), ImGuiTableColumnFlags_WidthFixed, 140.0f);
		}
		ImGui::TableHeadersRow();

		filterRows();

		ImGuiListClipper clipper;
		clipper.Begin((int)filteredRows.size());

		while (clipper.Step()) {
			for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
				int r = filteredRows[i];
				ImGui::TableNextRow();
				for (int c = 0; c < (int)projectInfo.project.activeFile.columns.size(); ++c) {
					ImGui::TableSetColumnIndex(c);

					// safety
					if ((std::size_t)r >= projectInfo.project.activeFile.columns[c].values.size()) {
						ImGui::TextUnformatted("");
						continue;
					}

					auto& cell = projectInfo.project.activeFile.columns[c].values[r];	// pair<ExcelValue, string>
					ImGui::PushID(r);
					ImGui::PushID(c);

					const bool isActive = (g_active.row == r && g_active.col == c);

					if (!isActive) {
						ImGui::PushID(&cell);
						if(ImGui::Selectable(cell.second.c_str())){
							g_active = { r, c };
							ImGui::SetKeyboardFocusHere();
						}
						ImGui::PopID();
					}
					else {
						ImGuiInputTextFlags inputFlags =
							ImGuiInputTextFlags_EnterReturnsTrue;
						
						bool enterPressed = ImGui::InputText("##cell", &cell.second, inputFlags);

						bool commit = enterPressed || ImGui::IsItemDeactivatedAfterEdit();
						if (commit) {
							cell.first = parse_value_auto(cell.second);
							cell.second = to_display(cell.first);
							g_active = { -1, -1 };
						}
						// escape cancels
						if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
							cell.second = to_display(cell.first);
							g_active = { -1, -1 };
						}
					}
					ImGui::PopID();
					ImGui::PopID();
				}
			}
		}
		ImGui::EndTable();
	}
}

void NimbleAnalyzer::justMerge(){
}

void NimbleAnalyzer::loadProjectsAvail() {
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

void NimbleAnalyzer::generateProject(const std::string& name) {
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

void NimbleAnalyzer::deleteProject(const std::string& name) {
	const std::string path = "projects/" + name;
	fl::del(path);
	loadProjectsAvail();
}

void NimbleAnalyzer::projectSelection() {
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
			projectInfo.project.save();
			fl::copy(projectInfo.project.path, path + "/" + projectInfo.project.name);
		}
	}
}

void NimbleAnalyzer::fileSelection(){
	if (!projectInfo.project.loaded)
		return;
	ImGui::Text("Files");
	if (ImGui::BeginListBox("## File Selection", { LISTBOX_WIDTH, LISTBOX_HEIGHT })) {
		for (const auto& file : projectInfo.project.files) {
			bool selected = (file == projectInfo.project.activeFile.path);
			if (ImGui::Selectable(fl::getFilename(file).c_str(), &selected)) {
				projectInfo.project.save();
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

void NimbleAnalyzer::sheetSelection(){
	if (!projectInfo.project.loaded || !projectInfo.project.activeFile.loaded)
		return;
	ImGui::Text("Sheets");
	if (ImGui::BeginListBox("## Sheet Selection", { LISTBOX_WIDTH, LISTBOX_HEIGHT })) {
		for (const auto& sheet : projectInfo.project.activeFile.sheets) {
			bool selected = (sheet == projectInfo.project.activeFile.activeSheet);
			if (ImGui::Selectable(sheet.c_str(), &selected)) {
				projectInfo.project.save();
				projectInfo.project.loadfile(
					projectInfo.project.activeFile.path,
					sheet,
					projectInfo.project.activeFile.sheetRows[sheet]);
			}
		}
		ImGui::EndListBox();
	}
	if (ImGui::InputInt("Header Row", &projectInfo.project.activeFile.sheetRows[projectInfo.project.activeFile.activeSheet].dataRow)) {
		projectInfo.project.loadfile(
			projectInfo.project.activeFile.path,
			projectInfo.project.activeFile.activeSheet,
			projectInfo.project.activeFile.sheetRows[projectInfo.project.activeFile.activeSheet]);
	}
	if (ImGui::Checkbox("Stop at empty row", &projectInfo.project.activeFile.sheetRows[projectInfo.project.activeFile.activeSheet].stopAtEmpty)) {
		projectInfo.project.loadfile(
			projectInfo.project.activeFile.path,
			projectInfo.project.activeFile.activeSheet,
			projectInfo.project.activeFile.sheetRows[projectInfo.project.activeFile.activeSheet]);
	}
}

void NimbleAnalyzer::filterRows(){
	filteredRows.clear();
	filteredRows.reserve((int)projectInfo.project.activeFile.rowCount);

	for (int r = 0; r < (int)projectInfo.project.activeFile.rowCount; ++r) {
		if (RowMatchesFilter(r))
			filteredRows.push_back(r);
	}
}
