#include "project.h"
#include <fstream>
#include "logging.h"
#include "utils.h"
#include "fileloader.h"
#include <xlnt/xlnt.hpp>
#include <xlnt/xlnt_config.hpp>

static const char* SETTINGS_FILE = "project.na";
namespace fl = fileloader;

// loading functions predefs
SheetTable load_sheet(const std::string& filePath, const std::string& sheet, int row = -1);

void SheetTable::clear() {
	name.clear();
	path.clear();
	sheets.clear();
	activeSheet.clear();
	columns.clear();
}

void Project::load(const std::string& name, const std::string& path){
	this->name = name;
	this->path = path;
	const std::string filepath = fl::u8path(path + "/" + SETTINGS_FILE);
	std::ifstream file(filepath, std::ios::binary);
	if (!file) {
		logging::logwarning("Could not open and load settings file: %s", filepath.c_str());
		return;
	}
	std::string line;
	while (std::getline(file, line)) {
		// Replacing the newline
		ReplaceAllSubstrings(line, "\n", "");
		// Getting project files
		if (line.starts_with("file_count = ")) {
			const int filecount = std::stoi(Splitlines(line, " = ").second);
			for (int i = 0; i < filecount; i++) {
				std::getline(file, line);
				ReplaceAllSubstrings(line, "\n", "");
				files.push_back(line);
			}
		}
		// Getting last selected file
		if (line.starts_with("selected_file = ")) {
			loadfile(Splitlines(line, " = ").second);
		}
	}

	loaded = true;
}

void Project::loadfile(const std::string& path, const std::string& sheet) {
	if (path == "")
		return;
	if (!fl::exists(path)) {
		auto it = std::find(files.begin(), files.end(), path);
		if (it != files.end()) {
			files.erase(it);
			logging::loginfo("[Project::loadfile] Deleted file from project as it no longer exists: %s", path.c_str());
		}
		return;
	}
	activeFile = load_sheet(path, sheet);
}

void Project::addfile(const std::string& path){
	auto it = std::find(files.begin(), files.end(), path);
	if (it != files.end()) {
		logging::loginfo("[Project::addfile] File does already exist in current project: %s", path.c_str());
		return;
	}
	files.push_back(path);
	logging::loginfo("[Project::loadfile] New file added:\n\
							Project:\t%s\n\
							File:\t\t%s", name.c_str(), path.c_str());
}

void Project::removefile(const std::string& path){
	auto it = std::find(files.begin(), files.end(), path);
	if (it == files.end()) {
		logging::loginfo("[Project::removefile] File does not exist in Project: \n\
							Project:\t%s\n\
							File:\t\t%s",
							name.c_str(), path.c_str());
		return;
	}
	files.erase(it);
}

void Project::clear(){
	name.clear();
	path.clear();
	files.clear();
	activeFile.clear();
	mergeInfos.clear();
	activeMergeInfo.clear();
	loaded = false;
}

void Project::save() {
	const std::string filepath = fl::u8path(path + "/" + SETTINGS_FILE);
	std::ofstream file(filepath, std::ios::binary);
	if (!file) {
		logging::logwarning("Could not open settings file: %s", filepath.c_str());
		return;
	}
	// Saving project files
	file << "file_count = " << files.size() << "\n";
	for (const auto& f : files) {
		file << f << "\n";
	}
	// saving last opened settings
	file << "selected_file = " << activeFile.path << "\n";
}

void MergeInfo::clear() {
	name.clear();
}

// loading functions defs
SheetTable load_sheet(const std::string& filePath, const std::string& sheet, int row) {
	Timer t;
	t.Start();
	// Setting variables
	SheetTable table;
	xlnt::workbook wb;
	xlnt::worksheet ws;
	// Loading file
	std::ifstream file(fl::u8topath(filePath), std::ios::binary);
	if (!file) {
		logging::loginfo("[project::load_sheet] File not found: %s", filePath.c_str());
		return {};
	}
	wb.load(file);
	// Retrieving all available sheets
	table.sheets = wb.sheet_titles();
	auto it = std::find(table.sheets.begin(), table.sheets.end(), sheet);
	if (sheet == "" || it == table.sheets.end()) {
		ws = wb.active_sheet();
	}
	else {
		ws = wb.sheet_by_title(sheet);
	}
	// Retrieve the header row by finding "Data"
	std::size_t headerIndex = 0;
	auto rows = ws.rows(false);
	if (row < 0) {
		for (auto row : rows) {
			if (row[0].to_string() == "DATA") {
				break;
			}
			headerIndex++;
		}
	}
	else {
		headerIndex = row;
	}

	// Retrieving headers
	std::unordered_map<std::string, std::uint32_t> seen;
	auto headerRow = ws.rows(false)[headerIndex];
	for (auto cell : headerRow) {
		std::string name = cell.to_string();
		auto& count = seen[name];
		HeaderKey key{ name, count++ };
		ColId id = static_cast<ColId>(table.columns.size());
		table.columns.push_back({ key, {} });
		table.byName[name].push_back(id);
	}

	// Data rows
	for (std::size_t r = headerIndex + 1; r < rows.length(); ++r) {
		auto row = rows[r];
		table.rowCount++;
		// single cells
		for (std::size_t c = 0; c < table.columns.size(); ++c) {
			ExcelValue value = std::monostate{};
			if (c < row.length()) {
				auto cell = row[c];
				if (cell.has_value()) {
					switch (cell.data_type()) {
					case xlnt::cell_type::number:
						value = cell.value<double>();
						break;
					case xlnt::cell_type::boolean:
						value = cell.value<bool>();
						break;
					case xlnt::cell_type::date:
					case xlnt::cell_type::empty:
					case xlnt::cell_type::formula_string:
					default:
						value = cell.to_string();
						break;
					}
				}
			}
			table.columns[c].values.push_back(std::move(value));
		}
	}
	
	table.loaded = true;
	table.path = filePath;
	table.name = fl::getFilename(filePath);
	table.activeSheet = ws.title();
	t.Stop();
	logging::loginfo("[project::load_sheet] SheetTable loaded:\n\
							File:\t\t%s\n\
							Sheet:\t\t%s\n\
							Time:\t\t%.2fs", table.path.c_str(), table.activeSheet.c_str(), t.GetElapsedSeconds());
	return table;
}
