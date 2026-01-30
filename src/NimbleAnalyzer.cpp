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
struct projectData {
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
	std::string selectedMerge;
	std::string newMerge;
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
	checkForUpdates();
	if (updateInfo.updateAvail)
		viewmode = ViewMode::Update;
	loadProjectsAvail();
}

static std::string g_search = "";
static std::string g_search_header = "##NONE_HEADER";
static std::vector<int> filteredRows;

void NimbleAnalyzer::menubar(){
	switch (viewmode) {
	case ViewMode::Update:
		if (ImGui::Button("Project selection"))
			viewmode = ViewMode::ProjectSelection;
		if (ImGui::Button("Data view"))
			viewmode = ViewMode::DataView;
		if (ImGui::Button("Just merge"))
			viewmode = ViewMode::JustMerge;
		break;
	case ViewMode::ProjectSelection:
		if (ImGui::Button("Check for Updates")) {
			checkForUpdates();
			viewmode = ViewMode::Update;
		}
		if (ImGui::Button("Data view"))
			viewmode = ViewMode::DataView;
		if (ImGui::Button("Just merge"))
			viewmode = ViewMode::JustMerge;
		break;
	case ViewMode::DataView:
		if (ImGui::Button("Check for Updates")) {
			viewmode = ViewMode::Update;
		}
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
		if (ImGui::Button("Check for Updates")) {
			viewmode = ViewMode::Update;
		}
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
		ImGui::SameLine();
		ImGui::BeginChild("Show headers", { CHILD_WINDOW_WIDTH, CHILD_WINDOW_HEIGHT }, true);
		showHeaders();
		ImGui::EndChild();
		ImGui::BeginChild("Merge settings", { CHILD_WINDOW_WIDTH * 3, CHILD_WINDOW_HEIGHT * 2}, true);
		mergeSettings();
		ImGui::EndChild();
		break;
	case ViewMode::DataView:
		dataView();
		break;
	case ViewMode::JustMerge:
		justMerge();
		break;
	case ViewMode::Update:
		update();
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

struct ActiveCell { int row = -1; int col = -1; };
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

void NimbleAnalyzer::update(){
	if (updateInfo.updateAvail) {
		ImGui::Text("New update avail: %d.%d.%d", updateInfo.version_avail_major, updateInfo.version_avail_minor, updateInfo.version_avail_alpha);
		ImGui::SameLine();
		if (ImGui::Button("Update now")) {
			std::string installerPath = "Y:\\Produktion\\Software & Tools\\NimbleAnalyzer\\src\\output\\setup_NimbleAnalyzer.exe";
			if (fl::exists(installerPath)) {
				fl::copy(installerPath, ".\\installer.exe", true);
				std::string batPath = "update_temp.bat";
				std::string appPath = fl::GetCurrentPath() + "\\installer.exe";
				std::ofstream bat(batPath);
				bat << "@echo off\n";
				bat << "timeout /t 2 /nobreak >nul\n"; // wait for 2 seconds
				//bat << "copy /Y \"" << installerPath << "\" \"" << appPath << "\"\n";
				bat << "start \"\" \"" << appPath << "\"\n";
				bat << "exit";
				bat.close();
				system(("start " + batPath).c_str());
				CloseWindow();
			}
		}
		ImGui::Text("Changes");
		ImGui::InputTextMultiline("##Updateinfo", &updateInfo.updatetext, {}, ImGuiInputTextFlags_ReadOnly);
	}
	else {
		ImGui::Text("No update available");
	}
}

void NimbleAnalyzer::checkForUpdates(){
	updateInfo = {};
	if (!fl::exists("Y:/Produktion/Software & Tools/NimbleAnalyzer/src/output/VERSION"))
		return;
	// Read current version
	std::string version = NIMBLE_ANALYZER_VERSION;
	updateInfo.version_major = std::stoi(Splitlines(version, ".").first);
	updateInfo.version_minor = std::stoi(Splitlines(Splitlines(version, ".").second, ".").first);
	updateInfo.version_alpha = std::stoi(Splitlines(Splitlines(version, ".").second, ".").second);
	std::string version_file = fl::loadfile("Y:/Produktion/Software & Tools/NimbleAnalyzer/src/output/VERSION");
	logging::loginfo("Current version: %d.%d.%d", updateInfo.version_major, updateInfo.version_minor, updateInfo.version_alpha);
	updateInfo.version_avail_major = std::stoi(Splitlines(version_file, ".").first);
	updateInfo.version_avail_minor = std::stoi(Splitlines(Splitlines(version_file, ".").second, ".").first);
	updateInfo.version_avail_alpha = std::stoi(Splitlines(Splitlines(version_file, ".").second, ".").second);
	logging::loginfo("Available version: %d.%d.%d", updateInfo.version_avail_major, updateInfo.version_avail_minor, updateInfo.version_avail_alpha);
	// Read update info
	if (updateInfo.version_major >= updateInfo.version_avail_major) {
		if (updateInfo.version_minor >= updateInfo.version_avail_minor) {
			if (updateInfo.version_alpha >= updateInfo.version_avail_alpha) {
				return;
			}
		}
	}
	updateInfo.updateAvail = true;
	if(fl::exists("Y:/Produktion/Software & Tools/NimbleAnalyzer/src/output/CHANGES"))
		updateInfo.updatetext = fl::loadfile("Y:/Produktion/Software & Tools/NimbleAnalyzer/src/output/CHANGES");
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
				projectInfo.project.load(project.name, project.path);
				projectInfo.selectedMerge.clear();
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
				projectInfo.selectedMerge.clear();
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
	if (ImGui::Button("Save")) {
		SaveReport report = save_sheet(projectInfo.project.activeFile.path, projectInfo.project.activeFile, *projectInfo.project.getCurrentSettingsHandle());
		if (!report.warnings.empty()) {
			for (const auto& warning : report.warnings) {
				logging::logwarning("%s", warning.c_str());
			}
		}
		if (!report.errors.empty()) {
			for (const auto& warning : report.errors) {
				logging::logwarning("%s", warning.c_str());
			}
		}
	}
}

void NimbleAnalyzer::sheetSelection(){
	if (!projectInfo.project.loaded || !projectInfo.project.activeFile.loaded)
		return;
	ImGui::TextUnformatted("Sheets");
	if (ImGui::BeginListBox("## Sheet Selection", { LISTBOX_WIDTH, LISTBOX_HEIGHT })) {
		for (const auto& sheet : projectInfo.project.activeFile.sheets) {
			bool selected = (sheet == projectInfo.project.activeFile.activeSheet);
			if (ImGui::Selectable(sheet.c_str(), &selected)) {
				projectInfo.project.save();
				projectInfo.project.loadfile(
					projectInfo.project.activeFile.path,
					sheet
				);
				projectInfo.selectedMerge.clear();
			}
		}
		ImGui::EndListBox();
	}
	SheetSettings* ss = projectInfo.project.getCurrentSettingsHandle();
	if (!ss)
		return;
	if (ImGui::InputInt("Header Row", &ss->dataRow)) {
		projectInfo.project.loadfile(
			projectInfo.project.activeFile.path,
			projectInfo.project.activeFile.activeSheet
		);
	}
	if (ImGui::Checkbox("Stop at empty row", &ss->stopAtEmpty)) {
		projectInfo.project.loadfile(
			projectInfo.project.activeFile.path,
			projectInfo.project.activeFile.activeSheet
		);
	}
}

void NimbleAnalyzer::showHeaders(){
	if (!projectInfo.project.loaded || !projectInfo.project.activeFile.loaded)
		return;
	ImGui::TextUnformatted("Headers found");
	if (ImGui::BeginListBox("## Show Headers", { LISTBOX_WIDTH, LISTBOX_HEIGHT })) {
		for (const auto& header : projectInfo.project.activeFile.columns) {
			const std::string label = header_label(header.key);
			ImGui::Selectable(label.c_str());
		}
		ImGui::EndListBox();
	}
}

void NimbleAnalyzer::mergeSettings(){
	if (!projectInfo.project.loaded)
		return;
	ImGui::SeparatorText("Merge Settings");
	std::vector<MergeSettings>* msv = projectInfo.project.getCurrentMergeSettingsHandle();
	if (!msv)
		return;
	ImGui::SetNextItemWidth(TEXT_INPUT_WIDTH);
	ImGui::InputTextWithHint("## new mergesettings", "new merge name", &projectInfo.newMerge);
	ImGui::SameLine();
	if (ImGui::Button("Create new setting") && projectInfo.newMerge != "") {
		bool exists = false;
		for (const auto& ms : *msv) {
			if (ms.name == projectInfo.newMerge) {
				exists = true;
				break;
			}
		}
		if (!exists) {
			msv->push_back({ projectInfo.newMerge });
			projectInfo.newMerge = "";
		}
	}
	// Combobox to select mergesettings
	ImGui::Text("Available Merge Settings (%d)", msv->size());
	ImGui::SetNextItemWidth(TEXT_INPUT_WIDTH);
	if (ImGui::BeginCombo("## Mergesetting", projectInfo.selectedMerge.c_str())) {
		for (const auto& ms : *msv) {
			if (ImGui::Selectable(ms.name.c_str())) {
				projectInfo.selectedMerge = ms.name;
			}
		}
		ImGui::EndCombo();
	}
	// Deleting settings
	// retrieving current selected mergesettings
	MergeSettings* ms = nullptr;
	int ms_idx = 0;
	for (auto& x : *msv) {
		ms_idx++;
		if (projectInfo.selectedMerge == x.name) {
			ms = &x;
			break;
		}
	}
	if (!ms)
		return;
	ImGui::SameLine();
	ImGui::PushID(&projectInfo.selectedMerge);
	if (ImGui::Button("X") && projectInfo.selectedMerge != "") {
		if (ms_idx <= msv->size() && ms_idx > 0) {
			msv->erase(msv->begin() + ms_idx - 1);
			projectInfo.selectedMerge = "";
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Merge") && ms->sourceFile.loaded && projectInfo.selectedMerge != "") {
		if (ms->mergefolder.empty()) {
			MergeReport report = MergeTables(projectInfo.project.activeFile, ms->sourceFile, *ms);
			if (!report.warnings.empty()) {
				for (const auto& msg : report.warnings) {
					logging::logwarning("[Merge Report] %s", msg.c_str());
				}
			}
			if (!report.errors.empty()) {
				for (const auto& msg : report.errors) {
					logging::logerror("[Merge Report] %s", msg.c_str());
				}
			}
		}
		else {
			const std::vector<std::string> files = fl::iteratePath(ms->mergefolder, false);
			for (const std::string& file : files) {
				try {
					std::string f = file;
					std::transform(f.begin(), f.end(), f.begin(), ::tolower);
					if (f.ends_with(".xlsx") || f.ends_with(".csv")) {
						SheetTable srcTable = load_sheet(file, ms->sourceFile.activeSheet, ms->sheetSettings);
						MergeReport report = MergeTables(projectInfo.project.activeFile, srcTable, *ms);
						if (!report.warnings.empty()) {
							for (const auto& msg : report.warnings) {
								logging::logwarning("[Merge Report] %s", msg.c_str());
							}
						}
						if (!report.errors.empty()) {
							for (const auto& msg : report.errors) {
								logging::logerror("[Merge Report] %s", msg.c_str());
							}
						}
					}
				}
				catch (const std::exception& e) {
					logging::logerror(e.what());
				}
			}
		}
	}
	ImGui::PopID();
	// sourcefile
	ImGui::SeparatorText("Sourcefile settings");
	if (ImGui::Button("Add sourcefile")) {
		std::string path = OpenFileDialog("Excel Sheet", "xlsx,csv");
		if (path != "") {
			convertContentToUTF8(&path);
			ms->sourceFile = load_sheet(path, "", ms->sheetSettings);
		}
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(TEXT_INPUT_WIDTH / 2);
	if (ImGui::InputInt("Header Row", &ms->sheetSettings.dataRow)) {
		ms->sourceFile = load_sheet(ms->sourceFile.path, ms->sourceFile.activeSheet, ms->sheetSettings);
	}
	ImGui::SameLine();
	if (ImGui::Checkbox("Stop at empty row", &ms->sheetSettings.stopAtEmpty)) {
		ms->sourceFile = load_sheet(ms->sourceFile.path, ms->sourceFile.activeSheet, ms->sheetSettings);
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(TEXT_INPUT_WIDTH / 2);
	if (ImGui::BeginCombo("Sheet", ms->sourceFile.activeSheet.c_str())) {
		for (const auto& sheet : ms->sourceFile.sheets) {
			if (ImGui::Selectable(sheet.c_str())) {
				ms->sourceFile = load_sheet(ms->sourceFile.path, sheet, ms->sheetSettings);
			}
		}
		ImGui::EndCombo();
	}
	ImGui::Text("Sourcefile: %s", ms->sourceFile.name.c_str());
	ImGui::SetItemTooltip("%s", ms->sourceFile.path.c_str());
	// sourcefolder
	ImGui::SeparatorText("Mergefolder settings");
	if (ImGui::Button("Add mergefolder")) {
		std::string path = OpenDirectoryDialog();
		if (path != "") {
			convertContentToUTF8(&path);
			ms->mergefolder = path;
		}
	}
	if (ms->mergefolder != "") {
		ImGui::PushID(&ms->mergefolder);
		ImGui::SameLine();
		if (ImGui::Button("X")) {
			ms->mergefolder = "";
		}
		ImGui::PopID();
	}
	ImGui::Text("Mergefolder: %s", ms->mergefolder.c_str());
	// Key
	ImGui::SeparatorText("Header Key");
	ImGui::TextUnformatted("Dst Header key                     Src Header key");
	ImGui::SetNextItemWidth(TEXT_INPUT_WIDTH);
	if (ImGui::BeginCombo("##Dst Header Key", header_label(ms->key.dstHeader).c_str())) {
		for (const auto& dst_key : projectInfo.project.activeFile.columns) {
			if (ImGui::Selectable(header_label(dst_key.key).c_str())) {
				ms->key.dstHeader = dst_key.key;
			}
		}
		ImGui::EndCombo();
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(TEXT_INPUT_WIDTH);
	if (ImGui::BeginCombo("##Src Header Key", header_label(ms->key.srcHeader).c_str())) {
		for (const auto& src_key : ms->sourceFile.columns) {
			if (ImGui::Selectable(header_label(src_key.key).c_str())) {
				ms->key.srcHeader = src_key.key;
			}
		}
		ImGui::EndCombo();
	}
	ImGui::SameLine();
	if (ImGui::Button("Clear")) {
		ms->key = {};
	}
	ImGui::SameLine();
	ImGui::Checkbox("Reverse header", &ms->reverseKey);
	ImGui::SetItemTooltip("Sets the key headers to work in reverse.\n\
Unticked: Only import if the headers value from the src file does NOT exist in dst file\n\
Ticked: Only import if the headers value from the src file does exist in dst file and fill row with data");
	// add rules
	ImGui::SeparatorText("Merging headers");
	int size = ms->mergeHeaders.size();
	int old_size = size;
	ImGui::SetNextItemWidth(TEXT_INPUT_WIDTH / 2);
	if (ImGui::InputInt("Merging rules", &size)) {
		if (size < 0)
			size = 0;
		if (old_size > size) {
			int diff = old_size - size;
			for (int i = 0; i < diff; i++) {
				ms->mergeHeaders.erase(ms->mergeHeaders.end() - 1);
			}
		}
		else if (old_size < size) {
			int diff = size - old_size;
			for (int i = 0; i < diff; i++) {
				ms->mergeHeaders.push_back({});
			}
		}
	}
	ImGui::TextUnformatted("Dst Header key                     Src Header key");
	for (auto& headers : ms->mergeHeaders) {
		ImGui::PushID(&headers);
		ImGui::SetNextItemWidth(TEXT_INPUT_WIDTH);
		if (ImGui::BeginCombo("##Dst Header Key", header_label(headers.dstHeader).c_str())) {
			for (const auto& dst_key : projectInfo.project.activeFile.columns) {
				if (ImGui::Selectable(header_label(dst_key.key).c_str())) {
					headers.dstHeader = dst_key.key;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(TEXT_INPUT_WIDTH);
		if (ImGui::BeginCombo("##Src Header Key", header_label(headers.srcHeader).c_str())) {
			for (const auto& src_key : ms->sourceFile.columns) {
				if (ImGui::Selectable(header_label(src_key.key).c_str())) {
					headers.srcHeader = src_key.key;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::PopID();
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
