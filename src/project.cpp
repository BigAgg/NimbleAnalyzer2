#include "project.h"
#include <fstream>
#include "logging.h"
#include "utils.h"
#include "fileloader.h"
#include <tinyxml2.h>
#include <xlnt/xlnt.hpp>
#include <xlnt/xlnt_config.hpp>

static const char* SETTINGS_FILE = "project.na";
static const char* SHEET_SETTINGS_FILE = "sheets.na";
static const char* MERGE_SETTINGS_FILE = "merges.na";
namespace fl = fileloader;

// loading functions predefs
SheetTable load_sheet_csv(const std::string& filePath, const std::string& sheet, SheetSettings& sheetSettings);

void SheetTable::clear() {
	name.clear();
	path.clear();
	sheets.clear();
	activeSheet.clear();
	columns.clear();
}

void Project::load(const std::string& name, const std::string& path){
	clear();
	this->name = name;
	this->path = path;
	const std::string filepath = fl::u8path(path + "/" + SETTINGS_FILE);
	std::ifstream file(filepath, std::ios::binary);
	if (!file) {
		logging::logwarning("Could not open and load settings file: %s", filepath.c_str());
		return;
	}
	std::string line;
	std::string fileToLoad;
	std::string sheetToLoad;
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
			fileToLoad = Splitlines(line, " = ").second;
		}
		if (line.starts_with("selected_sheet = ")) {
			sheetToLoad = Splitlines(line, " = ").second;
		}
	}
	load_all_mergesettings();
	load_all_sheetsettings();
	if (fileToLoad != "" && sheetToLoad != "")
		loadfile(fileToLoad, sheetToLoad);
	loaded = true;
}

void Project::loadfile(const std::string& path, const std::string& sheet) {
	if (path.empty())
		return;
	if (!fl::exists(path)) {
		auto it = std::find(files.begin(), files.end(), path);
		if (it != files.end()) {
			files.erase(it);
			logging::loginfo("[Project::loadfile] Deleted file from project as it no longer exists: %s", path.c_str());
		}
		return;
	}
	// getting active sheet
	std::string activeSheet = sheet;
	if (activeSheet == "") {
		if (path.ends_with(".xlsx") || path.ends_with(".XLSX")) {
			xlnt::workbook wb;
			std::ifstream file(fl::u8topath(path), std::ios::binary);
			if (file) {
				wb.load(file);
				activeSheet = wb.active_sheet().title();
			}
		}
		else {
			activeSheet = "main";
		}
	}
	SheetSettings ss = {};
	auto it = sheetSettings.find(sheet_key(path, activeSheet));
	if (it != sheetSettings.end()) {
		activeFile = load_sheet(path, activeSheet, it->second);
	}
	else {
		activeFile = load_sheet(path, activeSheet, ss);
		const std::string sn = activeFile.activeSheet;
		sheetSettings[sheet_key(path, sn)] = ss;
	}
	save_all_sheetsettings();
	save_all_mergesettings();
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
	sheetSettingsLoaded = false;
	sheetSettings.clear();
	mergeSettingsLoaded = false;
	mergeSettings.clear();
	activeFile = {};
	loaded = false;
}

void Project::save() {
	save_all_sheetsettings();
	save_all_mergesettings();
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
	file << "selected_sheet = " << activeFile.activeSheet << "\n";
}

void Project::load_all_sheetsettings(){
	sheetSettingsLoaded = true;
	sheetSettings.clear();

	const std::string filename = fl::u8path(path + "/" + SHEET_SETTINGS_FILE);
	std::ifstream in(filename, std::ios::binary);
	if (!in) return; // no settings yet is fine

	std::string line;

	auto read_value = [](const std::string& s) -> std::string {
		auto p = Splitlines(s, " = ");
		return p.second;
		};

	int expectedCount = -1;
	while (std::getline(in, line))
	{
		ReplaceAllSubstrings(line, "\r", "");
		ReplaceAllSubstrings(line, "\n", "");
		if (line.empty()) continue;

		if (line.starts_with("count = "))
		{
			expectedCount = std::stoi(read_value(line));
			continue;
		}

		if (line == "BEGIN")
		{
			std::string file, sheet;
			SheetSettings ss;

			while (std::getline(in, line))
			{
				ReplaceAllSubstrings(line, "\r", "");
				ReplaceAllSubstrings(line, "\n", "");
				if (line == "END") break;

				if (line.starts_with("file = ")) file = read_value(line);
				else if (line.starts_with("sheet = ")) sheet = read_value(line);
				else if (line.starts_with("dataRow = ")) ss.dataRow = std::stoi(read_value(line));
				else if (line.starts_with("stopAtEmpty = ")) ss.stopAtEmpty = (read_value(line) == "1");
			}

			if (!file.empty() && !sheet.empty())
				sheetSettings[sheet_key(file, sheet)] = ss;
		}
	}
}

void Project::load_all_mergesettings() {
	mergeSettingsLoaded = true;
	mergeSettings.clear();

	const std::string filename = fl::u8path(path + "/" + MERGE_SETTINGS_FILE);
	std::ifstream in(filename, std::ios::binary);
	if (!in) return;	// no settings yet is fine

	std::string line;

	auto read_value = [](const std::string& s) -> std::string {
		auto p = Splitlines(s, " = ");
		return p.second;
		};

	int expectedCount = -1;
	while (std::getline(in, line)) {
		ReplaceAllSubstrings(line, "\r", "");
		ReplaceAllSubstrings(line, "\n", "");
		if (line.empty()) continue;

		if (line.starts_with("profile_count = ")) {
			expectedCount = std::stoi(read_value(line));
			continue;
		}

		if (line == "BEGIN_PROFILE") {
			std::string file;
			std::string sheet;
			std::string mergeSettingsKey;
			while (std::getline(in, line)) {
				if ((!file.empty() && !sheet.empty()) && mergeSettingsKey.empty()) {
					mergeSettingsKey = sheet_key(file, sheet);
					mergeSettings[mergeSettingsKey] = {};
				}
				int ruleCount = -1;
				ReplaceAllSubstrings(line, "\r", "");
				ReplaceAllSubstrings(line, "\n", "");
				if (line == "END_PROFILE") {
					break;
				}
				if (line.starts_with("file = ")) file = read_value(line);
				else if (line.starts_with("sheet = ")) sheet = read_value(line);
				else if (line.starts_with("rule_count = ")) ruleCount = std::stoi(read_value(line));
				else if (line == "BEGIN_RULE") {
					MergeSettings rule;
					while (std::getline(in, line)) {
						int expectedHeaders = -1;
						ReplaceAllSubstrings(line, "\r", "");
						ReplaceAllSubstrings(line, "\n", "");
						if (line == "END_RULE") {
							rule.sourceFile = load_sheet(rule.sourceFile.path, rule.sourceFile.activeSheet, rule.sheetSettings);
							mergeSettings[mergeSettingsKey].push_back(rule);
							break;
						}
						if (line.starts_with("rule_name = ")) rule.name = read_value(line);
						else if (line.starts_with("mergefolder = ")) rule.mergefolder = read_value(line);
						else if (line.starts_with("src_file = ")) rule.sourceFile.path = read_value(line);
						else if (line.starts_with("src_sheet = ")) rule.sourceFile.activeSheet = read_value(line);
						else if (line.starts_with("dataRow = ")) rule.sheetSettings.dataRow = std::stoi(read_value(line));
						else if (line.starts_with("stopAtEmpty = ")) rule.sheetSettings.stopAtEmpty = (read_value(line) == "1");
						else if (line.starts_with("dst_key_name = ")) rule.key.dstHeader.name = read_value(line);
						else if (line.starts_with("dst_key_occ = ")) rule.key.dstHeader.occurrence = std::stoi(read_value(line));
						else if (line.starts_with("src_key_name = ")) rule.key.srcHeader.name = read_value(line);
						else if (line.starts_with("src_key_occ = ")) rule.key.srcHeader.occurrence = std::stoi(read_value(line));
						else if (line.starts_with("reverseKey = ")) rule.reverseKey = (read_value(line) == "1");
						else if (line.starts_with("mergeHeaders_count = ")) expectedHeaders = std::stoi(read_value(line));
						else if (line == "BEGIN_HEADER") {
							MergeHeaders headers;
							while (std::getline(in, line)) {
								ReplaceAllSubstrings(line, "\r", "");
								ReplaceAllSubstrings(line, "\n", "");

								if (line == "END_HEADER") {
									rule.mergeHeaders.push_back(headers);
									break;
								}
								if (line.starts_with("dst_name = ")) headers.dstHeader.name = read_value(line);
								else if (line.starts_with("dst_occ = ")) headers.dstHeader.occurrence = std::stoi(read_value(line));
								else if (line.starts_with("src_name = ")) headers.srcHeader.name = read_value(line);
								else if (line.starts_with("src_occ = ")) headers.srcHeader.occurrence = std::stoi(read_value(line));
							}
						}
					}
				}
			}
		}
	}
}

void Project::save_all_mergesettings() const {
	const std::string filename = fl::u8path(path + "/" + MERGE_SETTINGS_FILE);
	std::ofstream out(filename, std::ios::binary);
	if (!out) return;

	out << "version = 1\n";
	out << "profile_count = " << mergeSettings.size() << "\n";

	for (const auto& [k, ms] : mergeSettings) {
		auto pos = k.find('\n');
		if (pos == std::string::npos) continue;
		std::string file = k.substr(0, pos);
		std::string sheet = k.substr(pos + 1);
		out << "BEGIN_PROFILE\n";
		out << "file = " << file << "\n";
		out << "sheet = " << sheet << "\n";
		out << "rule_count = " << ms.size() << "\n";
		for (const auto& rule : ms) {
			out << "BEGIN_RULE\n";
			out << "rule_name = " << rule.name << "\n";
			out << "mergefolder = " << rule.mergefolder << "\n";
			out << "src_file = " << rule.sourceFile.path << "\n";
			out << "src_sheet = " << rule.sourceFile.activeSheet << "\n";
			out << "dataRow = " << rule.sheetSettings.dataRow << "\n";
			out << "stopAtEmpty = " << (rule.sheetSettings.stopAtEmpty ? 1 : 0) << "\n";
			out << "dst_key_name = " << rule.key.dstHeader.name << "\n";
			out << "dst_key_occ = " << rule.key.dstHeader.occurrence << "\n";
			out << "src_key_name = " << rule.key.srcHeader.name << "\n";
			out << "src_key_occ = " << rule.key.srcHeader.occurrence << "\n";
			out << "reverseKey = " << (rule.reverseKey ? 1 : 0) << "\n";
			out << "mergeHeaders_count = " << rule.mergeHeaders.size() << "\n";
			for (const auto& headers : rule.mergeHeaders) {
				out << "BEGIN_HEADER\n";
				out << "dst_name = " << headers.dstHeader.name << "\n";
				out << "dst_occ = " << headers.dstHeader.occurrence << "\n";
				out << "src_name = " << headers.srcHeader.name << "\n";
				out << "src_occ = " << headers.srcHeader.occurrence << "\n";
				out << "END_HEADER\n";
			}
			out << "END_RULE\n";
		}
		out << "END_PROFILE\n";
	}
}

void Project::save_all_sheetsettings() const{
	const std::string filename = fl::u8path(path + "/" + SHEET_SETTINGS_FILE);
	std::ofstream out(filename, std::ios::binary);
	if (!out) return;

	out << "version = 1\n";
	out << "count = " << sheetSettings.size() << "\n";

	for (const auto& [k, ss] : sheetSettings)
	{
		// split key back into file + sheet
		auto pos = k.find('\n');
		if (pos == std::string::npos) continue;
		std::string file = k.substr(0, pos);
		std::string sheet = k.substr(pos + 1);

		out << "BEGIN\n";
		out << "file = " << file << "\n";
		out << "sheet = " << sheet << "\n";
		out << "dataRow = " << ss.dataRow << "\n";
		out << "stopAtEmpty = " << (ss.stopAtEmpty ? 1 : 0) << "\n";
		out << "END\n";
	}
}

std::string Project::sheet_key(const std::string& file, const std::string& sheet){
	return file + "\n" + sheet;
}

bool convertOldProject(const std::string& path){
	Project project;
	project.name = fl::getFilename(path);
	project.path = path;
	const std::string proFile = path + "/.pro";
	if (!fl::exists(proFile))
		return false;
	const auto& file = fl::loadfilelines(proFile);
	// loading selected file
	const size_t file_count = std::stoi(file[1]);
	for (int i = file_count; i < file.size(); i++) {
		project.addfile(file[i]);
	}
	// Now everything for project.na is ready already
	// building sheets.na
	for (const auto& f : project.files) {
		const std::string& fileini = path + "/" + fl::getFilename(f) + ".ini";
		if (!fl::exists(fileini))
			continue;
		project.loadfile(f);
		SheetSettings* ss = project.getCurrentSettingsHandle();
		if (!ss)
			continue;
		ss->stopAtEmpty = true;
		// go on and build the mergesettings for each
		auto msv = project.getCurrentMergeSettingsHandle();
		if (!msv)
			continue;
		msv->push_back({ "MergeFolder" });
		MergeSettings& ms = msv->back();
		const auto& ini = fl::loadfilelines(fileini);
		int line_idx = -1;
		for (const std::string& line : ini) {
			line_idx++;
			if (line.starts_with("m_mergefolder = "))
				ms.mergefolder = Splitlines(line, " = ").second;
			if (line.starts_with("m_mergefolderfile = "))
				ms.sourceFile = load_sheet(Splitlines(line, " = ").second, "", ms.sheetSettings);
			if (line.starts_with("m_dontimportifexistsheader = ") && Splitlines(line, " = ").second.size() > 3) {
				ms.reverseKey = true;
			}
			if (line.starts_with("m_megeheadersfolder = ")) {
				int count = std::stoi(Splitlines(line, " = ").second);
				for (int i = line_idx + 1; i < line_idx + count + 1; i++) {
					MergeHeaders mh;
					std::string mh_line = ini[i];
					std::pair<std::string, std::string> values = Splitlines(mh_line, " := ");
					int index = std::stoi(Splitlines(values.first, " ##").second);
					if (index >= project.activeFile.columns.size())
						continue;
					mh.dstHeader.name = project.activeFile.columns[index].key.name;
					mh.dstHeader.occurrence = project.activeFile.columns[index].key.occurrence;
					index = std::stoi(Splitlines(values.second, " ##").second);
					if (index >= ms.sourceFile.columns.size())
						continue;
					mh.srcHeader.name = ms.sourceFile.columns[index].key.name;
					mh.srcHeader.occurrence = ms.sourceFile.columns[index].key.occurrence;
					ms.mergeHeaders.push_back(std::move(mh));
				}
			}
			if (line.starts_with("m_mergefolderif = ")) {
				std::pair<std::string, std::string> values = Splitlines(Splitlines(line, " = ").second, " := ");
				int index = std::stoi(Splitlines(values.first, " ##").second);
				if (index >= project.activeFile.columns.size())
					continue;
				ms.key.dstHeader.name = project.activeFile.columns[index].key.name;
				ms.key.dstHeader.occurrence = project.activeFile.columns[index].key.occurrence;
				index = std::stoi(Splitlines(values.second, " ##").second);
				if (index >= ms.sourceFile.columns.size())
					continue;
				ms.key.srcHeader.name = ms.sourceFile.columns[index].key.name;
				ms.key.srcHeader.occurrence = ms.sourceFile.columns[index].key.occurrence;
			}
		}
		line_idx = -1;
		msv->push_back({ "MergeFile" });
		ms = msv->back();
		for (const std::string& line : ini) {
			line_idx++;
			if (line.starts_with("m_mergefile = "))
				ms.sourceFile = load_sheet(Splitlines(line, " = ").second, "", ms.sheetSettings);
			if (line.starts_with("m_mergeif = ")) {
				std::pair<std::string, std::string> values = Splitlines(Splitlines(line, " = ").second, " := ");
				int index = std::stoi(Splitlines(values.first, " ##").second);
				if (index >= project.activeFile.columns.size())
					continue;
				ms.key.dstHeader.name = project.activeFile.columns[index].key.name;
				ms.key.dstHeader.occurrence = project.activeFile.columns[index].key.occurrence;
				index = std::stoi(Splitlines(values.second, " ##").second);
				if (index >= ms.sourceFile.columns.size())
					continue;
				ms.key.srcHeader.name = ms.sourceFile.columns[index].key.name;
				ms.key.srcHeader.occurrence = ms.sourceFile.columns[index].key.occurrence;
			}
			if (line.starts_with("m_mergeheaders = ")) {
				int count = std::stoi(Splitlines(line, " = ").second);
				for (int i = line_idx + 1; i < line_idx + count + 1; i++) {
					MergeHeaders mh;
					std::pair<std::string, std::string> values = Splitlines(ini[i], " := ");
					int index = std::stoi(Splitlines(values.first, " ##").second);
					if (index >= project.activeFile.columns.size())
						continue;
					mh.dstHeader.name = project.activeFile.columns[index].key.name;
					mh.dstHeader.occurrence = project.activeFile.columns[index].key.occurrence;
					index = std::stoi(Splitlines(values.second, " ##").second);
					if (index >= ms.sourceFile.columns.size())
						continue;
					mh.srcHeader.name = ms.sourceFile.columns[index].key.name;
					mh.srcHeader.occurrence = ms.sourceFile.columns[index].key.occurrence;
					ms.mergeHeaders.push_back(std::move(mh));
				}
			}
		}
		project.save();
	}

	return true;
}

// loading functions defs
SheetTable load_sheet(const std::string& filePath, const std::string& sheet, SheetSettings& sheetSettings) {
	if (filePath.empty())
		return {};
	if (filePath.ends_with(".csv") || filePath.ends_with(".CSV"))
		return load_sheet_csv(filePath, sheet, sheetSettings);
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
	int headerIndex = -1;
	auto rows = ws.rows(false);
	if (sheetSettings.dataRow < 0) {
		sheetSettings.dataRow = 0;
		for (auto r : rows) {
			if (r[0].to_string() == "DATA") {
				headerIndex = sheetSettings.dataRow;
				break;
			}
			sheetSettings.dataRow++;
		}
	}
	else {
		headerIndex = sheetSettings.dataRow;
	}
	sheetSettings.dataRow = headerIndex;
	table.loaded = true;
	table.path = filePath;
	table.name = fl::getFilename(filePath);
	table.activeSheet = ws.title();
	if (headerIndex < 0) {
		return table;
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
		std::size_t emptyCount = 0;
		for (std::size_t c = 0; c < table.columns.size(); ++c) {
			ExcelValue value = std::monostate{};
			if (c < row.length()) {
				auto cell = row[c];
				if (cell.has_value()) {
					std::string keyname = table.columns[c].key.name;
					std::transform(keyname.begin(), keyname.end(), keyname.begin(), ::tolower);
					switch (cell.data_type()) {
					case xlnt::cell_type::number:
						if (keyname.contains("date") || keyname.contains("datum")) {
							value = ExcelSerialToDate(cell.value<double>());
							break;
						}
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
				else {
					emptyCount++;
				}
			}
			table.columns[c].values.push_back(std::make_pair(std::move(value), to_display(value)));
		}
		if (sheetSettings.stopAtEmpty && emptyCount == row.length()) {
			for (std::size_t c = 0; c < table.columns.size(); ++c) {
				table.columns[c].values.pop_back();
			}
			break;
		}
	}
	
	t.Stop();
	logging::loginfo("[project::load_sheet] SheetTable loaded:\n\
							File:\t\t%s\n\
							Sheet:\t\t%s\n\
							Time:\t\t%.2fs", table.path.c_str(), table.activeSheet.c_str(), t.GetElapsedSeconds());
	return table;
}

static HeaderKey make_header_key(std::unordered_map<std::string, std::uint32_t>& seen, const std::string& raw) {
	std::string name = fl::csv::trim_ws(raw);
	if (name.empty()) name = "";
	auto& count = seen[name];
	HeaderKey k{ name, count };
	count++;
	return k;
}

SheetTable load_sheet_csv(const std::string& filePath, const std::string& sheet, SheetSettings& sheetSettings){
	if (filePath.ends_with(".xlsx") || filePath.ends_with(".XLSX"))
		return load_sheet(filePath, sheet, sheetSettings);

	Timer t;
	t.Start();

	SheetTable table;

	std::ifstream file(fl::u8topath(filePath), std::ios::binary);
	if (!file)
	{
		logging::loginfo("[project::load_sheet_csv] File not found: %s", filePath.c_str());
		return {};
	}

	table.sheets.push_back("main");
	table.loaded = true;
	table.path = filePath;
	table.name = fl::getFilename(filePath);
	table.activeSheet = "main";

	// ---- Read all CSV records (streaming, but we need to find header row first) ----
	std::vector<std::string> records;
	records.reserve(256);

	std::string rec;
	while (fl::csv::read_csv_record(file, rec))
	{
		convertContentToUTF8(&rec);
		// Strip BOM on first record if present
		if (records.empty() && rec.size() >= 3 &&
			(unsigned char)rec[0] == 0xEF && (unsigned char)rec[1] == 0xBB && (unsigned char)rec[2] == 0xBF)
		{
			rec.erase(0, 3);
		}
		records.push_back(rec);
	}

	if (records.empty())
		return table;

	// ---- Determine header row index ----
	int headerIndex = sheetSettings.dataRow; // interpret as 0-based "header row"
	if (headerIndex < 0)
	{
		// auto: first non-empty record
		headerIndex = 0;
		for (const auto& rec : records) {
			if (rec.contains("DATA;"))
				break;
			headerIndex++;
		}
		/*while (headerIndex < (int)records.size() && fl::csv::is_line_empty_or_ws(records[headerIndex]))
			++headerIndex;*/

		if (headerIndex >= (int)records.size())
		{
			sheetSettings.dataRow = -1;
			return table;
		}
		sheetSettings.dataRow = headerIndex;
	}

	if (headerIndex < 0 || headerIndex >= (int)records.size())
	{
		// invalid header row setting; keep table loaded but empty columns
		sheetSettings.dataRow = -1;
		return table;
	}

	// ---- Parse header row, create columns ----
	const char delim = fl::csv::sniff_delimiter(records[headerIndex]);
	auto headerFields = fl::csv::split_csv_fields(records[headerIndex], delim);

	std::unordered_map<std::string, std::uint32_t> seen;
	table.columns.clear();
	table.columns.reserve(headerFields.size());

	for (const auto& hf : headerFields)
	{
		Column col;
		col.key = make_header_key(seen, hf);
		ColId id = static_cast<ColId>(table.columns.size());
		table.byName[col.key.name].push_back(id);
		table.columns.push_back(std::move(col));
	}

	// If header has 0 columns, treat as "no header"
	if (table.columns.empty())
		return table;

	// ---- Parse data rows ----
	const int dataStart = headerIndex + 1;
	std::size_t rowCount = 0;

	for (int r = dataStart; r < (int)records.size(); ++r)
	{
		auto fields = fl::csv::split_csv_fields(records[r], delim);

		// stopAtEmpty: empty row means all fields empty/whitespace (or no fields)
		bool allEmpty = true;
		for (const auto& f : fields)
		{
			if (!fl::csv::trim_ws(f).empty()) { allEmpty = false; break; }
		}
		if (sheetSettings.stopAtEmpty && allEmpty)
			break;

		// Ensure we have at least as many fields as columns (missing fields => empty)
		if (fields.size() < table.columns.size())
			fields.resize(table.columns.size());

		// For extra fields beyond header count: ignore by default
		// (If you want, you could auto-create extra columns here.)

		for (std::size_t c = 0; c < table.columns.size(); ++c)
		{
			std::string s = fl::csv::trim_ws(fields[c]);

			ExcelValue v;
			if (s.empty())
			{
				v = std::monostate{};
			}
			else
			{
				// Use your existing parsing (which supports comma decimals etc)
				v = parse_value_auto(s);
			}

			table.columns[c].values.emplace_back(v, to_display(v));
		}

		++rowCount;
	}

	table.rowCount = rowCount;

	// ---- Pad all columns to rowCount (critical for your UI/filter safety) ----
	for (auto& col : table.columns)
	{
		while (col.values.size() < table.rowCount)
		{
			ExcelValue v = std::monostate{};
			col.values.emplace_back(v, to_display(v));
		}
	}

	t.Stop();
	logging::loginfo("[project::load_sheet_csv] SheetTable loaded:\n\
                        File:\t\t%s\n\
                        Sheet:\t\t%s\n\
                        Time:\t\t%.2fs\n\
                        Rows:\t\t%zu\n\
                        Cols:\t\t%zu\n\
                        Delim:\t\t%s",
		table.path.c_str(),
		table.activeSheet.c_str(),
		t.GetElapsedSeconds(),
		table.rowCount,
		table.columns.size(),
		(delim == '\t' ? "\\t" : std::string(1, delim).c_str()));

	return table;
}

static std::string csv_escape(const std::string& s, char delim)
{
	// RFC4180-ish escaping:
	// If field contains delim, quote, or newline => wrap in quotes and escape quotes as ""
	bool needQuotes = false;
	for (char ch : s)
	{
		if (ch == delim || ch == '"' || ch == '\n' || ch == '\r')
		{
			needQuotes = true;
			break;
		}
	}
	if (!needQuotes) return s;

	std::string out;
	out.reserve(s.size() + 8);
	out.push_back('"');
	for (char ch : s)
	{
		if (ch == '"') out += "\"\"";
		else out.push_back(ch);
	}
	out.push_back('"');
	return out;
}

static void write_csv_line(std::ostream& out, const std::vector<std::string>& fields, char delim)
{
	for (size_t i = 0; i < fields.size(); ++i)
	{
		if (i) out << delim;
		out << csv_escape(fields[i], delim);
	}
	out << "\n";
}

static void set_xlnt_cell_value(xlnt::cell& c, const ExcelValue& v)
{
	if (std::holds_alternative<std::monostate>(v))
	{
		// keep formatting, clear value
		c.clear_value();
		return;
	}
	if (auto* d = std::get_if<double>(&v)) { c.value(*d); return; }
	if (auto* i = std::get_if<std::int64_t>(&v)) { c.value(static_cast<long long>(*i)); return; }
	if (auto* b = std::get_if<bool>(&v)) { c.value(*b); return; }
	if (auto* s = std::get_if<std::string>(&v)) { c.value(*s); return; }

	// fallback
	c.value(to_display(v));
}


SaveReport save_sheet(const std::string& filePath, SheetTable& table, const SheetSettings& ss)
{
	SaveReport report;

	if (filePath.empty())
	{
		report.errors.push_back("save_sheet: empty filePath");
		return report;
	}
	if (!table.loaded || table.columns.empty())
	{
		report.errors.push_back("save_sheet: table not loaded or has no columns");
		return report;
	}

	// Ensure rectangular data (critical for correctness)
	for (auto& col : table.columns)
	{
		while (col.values.size() < table.rowCount)
		{
			ExcelValue v = std::monostate{};
			col.values.emplace_back(v, to_display(v));
		}
	}

	// ---------------- CSV ----------------
	if (filePath.ends_with(".csv") || filePath.ends_with(".CSV"))
	{
		// Use ';' because you display decimal comma.
		const char delim = ';';

		std::ofstream out(fl::u8topath(filePath), std::ios::binary | std::ios::trunc);
		if (!out)
		{
			report.errors.push_back("save_sheet: could not open CSV for writing: " + filePath);
			return report;
		}

		// CSV cannot preserve "rows above header" unless you stored them; export as a clean table.
		// header row:
		{
			std::vector<std::string> hdr;
			hdr.reserve(table.columns.size());
			for (const auto& col : table.columns)
			{
				// Keep the original header name; if you want to disambiguate duplicates in CSV use header_label(col.key)
				hdr.push_back(col.key.name);
			}
			write_csv_line(out, hdr, delim);
		}

		// data rows:
		for (size_t r = 0; r < table.rowCount; ++r)
		{
			std::vector<std::string> row;
			row.reserve(table.columns.size());

			for (size_t c = 0; c < table.columns.size(); ++c)
			{
				const auto& cell = table.columns[c].values[r];
				row.push_back(cell.second); // cached display string
			}

			write_csv_line(out, row, delim);
		}

		report.cellsWritten = table.rowCount * table.columns.size();
		return report;
	}

	// ---------------- XLSX ----------------
	try
	{
		xlnt::workbook wb;

		// Load existing workbook if present (so rows above header can be preserved)
		{
			std::ifstream in(fl::u8topath(filePath), std::ios::binary);
			if (in) wb.load(in);
		}

		std::string sheetTitle = table.activeSheet.empty() ? "main" : table.activeSheet;

		xlnt::worksheet ws;
		{
			bool exists = false;
			for (const auto& t : wb.sheet_titles())
			{
				if (t == sheetTitle) { exists = true; break; }
			}
			if (exists) ws = wb.sheet_by_title(sheetTitle);
			else
			{
				ws = wb.active_sheet();
				ws.title(sheetTitle);
			}
		}

		// Decide header row position using SheetSettings.
		// IMPORTANT: Your current loader increments dataRow starting from 0, so treat dataRow as 0-based.
		// If in your UI it is 1-based, change to: excelHeaderRow = (ss.dataRow <= 0 ? 1 : (size_t)ss.dataRow);
		const std::size_t excelHeaderRow = (ss.dataRow >= 0) ? (std::size_t(ss.dataRow) + 1) : 1;
		const std::size_t excelDataStart = excelHeaderRow + 1;

		const std::size_t newMaxRow = excelHeaderRow + table.rowCount; // header row + data rows below
		const std::size_t newMaxCol = table.columns.size();

		// Clear only the table region (from header row down), preserve everything above it.
		{
			const std::size_t oldMaxRow = (std::size_t)ws.highest_row();

			// In some xlnt forks this is highest_column_or_props(); if highest_column() doesn't compile, swap it.
			const std::size_t oldMaxCol = ws.highest_column().index;

			const std::size_t clearRows = std::max(oldMaxRow, newMaxRow);
			const std::size_t clearCols = std::max(oldMaxCol, newMaxCol);

			for (std::size_t r = excelHeaderRow; r <= clearRows; ++r)
			{
				for (std::size_t c = 1; c <= clearCols; ++c)
				{
					ws.cell(xlnt::cell_reference((int)c, (int)r)).clear_value();
				}
			}
		}

		// Write headers at excelHeaderRow
		for (std::size_t c = 0; c < table.columns.size(); ++c)
		{
			ws.cell(xlnt::cell_reference((int)(c + 1), (int)excelHeaderRow)).value(table.columns[c].key.name);
		}

		// Write data starting at excelDataStart
		for (std::size_t r = 0; r < table.rowCount; ++r)
		{
			for (std::size_t c = 0; c < table.columns.size(); ++c)
			{
				auto& cell = table.columns[c].values[r];
				xlnt::cell xcell = ws.cell(xlnt::cell_reference((int)(c + 1), (int)(excelDataStart + r)));
				set_xlnt_cell_value(xcell, cell.first);
			}
		}

		// Save workbook
		{
			std::ofstream out(fl::u8topath(filePath), std::ios::binary | std::ios::trunc);
			if (!out)
			{
				report.errors.push_back("save_sheet: could not open XLSX for writing: " + filePath);
				return report;
			}
			wb.save(out);
		}

		report.cellsWritten = table.rowCount * table.columns.size();
		return report;
	}
	catch (const std::exception& e)
	{
		report.errors.push_back(std::string("save_sheet: exception: ") + e.what());
		return report;
	}
}

MergeReport MergeTables(SheetTable& dst, SheetTable& src, const MergeSettings& settings) {
	MergeReport report;
	Timer t;
	// Check if there is something to merge. Skip if there is none
	if (!dst.loaded || !src.loaded)
		return report;
	if (dst.columns.empty() || src.columns.empty())
		return report;
	if (settings.mergeHeaders.empty())
		return report;
	t.Start();
	int startCount = dst.rowCount;
	const bool useKey = (!settings.key.dstHeader.name.empty() && !settings.key.srcHeader.name.empty());
	if (useKey) {
		// Merging only if value does not exist in header
		Column* dstkeyCol = dst.find_column(settings.key.dstHeader.name, settings.key.dstHeader.occurrence);
		Column* srckeyCol = src.find_column(settings.key.srcHeader.name, settings.key.srcHeader.occurrence);
		if (dstkeyCol && srckeyCol) {
			if (settings.reverseKey) {
				report.type = "None Matching Key";
				// Loop each row and insert the row into dst if they keys value does not alrdy exist in file
				for (int i = 0; i < srckeyCol->values.size(); i++) {
					const std::pair<ExcelValue, std::string> value = srckeyCol->values[i];
					if (std::holds_alternative<std::monostate>(value.first))
						continue;
					auto it = std::find(dstkeyCol->values.begin(), dstkeyCol->values.end(), value);
					if (it != dstkeyCol->values.end())
						continue;
					// Looping all headers
					for (const auto& header : settings.mergeHeaders) {
						Column* dstCol = dst.find_column(header.dstHeader.name, header.dstHeader.occurrence);
						if (!dstCol) {
							report.warnings.push_back("Destination Header not found: " + header_label(header.dstHeader));
							report.conflicts++;
						}
						Column* srcCol = src.find_column(header.srcHeader.name, header.srcHeader.occurrence);
						if (!srcCol) {
							report.warnings.push_back("Source Header not found: " + header_label(header.srcHeader));
							report.conflicts++;
						}
						if (!srcCol || !dstCol) {
							report.skippedHeaders++;
							continue;
						}
						// inserting the value
						dstCol->values.emplace_back(srcCol->values[i]);
						if (dstCol->values.size() > dst.rowCount)
							dst.rowCount = dstCol->values.size() + 1;
						if (!srcCol->values[i].second.empty())
							report.cellsWritten++;
					}
				}
			}
			// Merging only if value does exist in header
			else {
				report.type = "Matching Key";
				// Loop each row and insert the row into dst if the keys value does exist
				for (int i = 0; i < srckeyCol->values.size(); i++) {
					const std::pair<ExcelValue, std::string> value = srckeyCol->values[i];
					auto it = std::find(dstkeyCol->values.begin(), dstkeyCol->values.end(), value);
					if (it == dstkeyCol->values.end())
						continue;
					// Looping all headers
					for (const auto& header : settings.mergeHeaders) {
						Column* dstCol = dst.find_column(header.dstHeader.name, header.dstHeader.occurrence);
						if (!dstCol) {
							report.warnings.push_back("Destination Header not found: " + header_label(header.dstHeader));
							report.conflicts++;
						}
						Column* srcCol = src.find_column(header.srcHeader.name, header.srcHeader.occurrence);
						if (!srcCol) {
							report.warnings.push_back("Source Header not found: " + header_label(header.srcHeader));
							report.conflicts++;
						}
						if (!srcCol || !dstCol) {
							report.skippedHeaders++;
							continue;
						}
						// inserting the value inside the row
						dstCol->values[it - dstkeyCol->values.begin()] = srcCol->values[i];
						if (!srcCol->values[i].second.empty())
							report.cellsWritten++;
					}
					report.rowsMatched++;
					report.rowsWritten++;
				}
			}
		}
		else {
			if (!dstkeyCol) {
				report.errors.push_back("Destination Key not found: " + header_label(settings.key.dstHeader));
			}
			if (!srckeyCol) {
				report.errors.push_back("Source Key not found: " + header_label(settings.key.srcHeader));
			}
		}
	}
	// Merging in append mode
	else {
		report.type = "Append";
		// looping all merge header rules
		for (const auto& header : settings.mergeHeaders) {
			Column* dstCol = dst.find_column(header.dstHeader.name, header.dstHeader.occurrence);
			if (!dstCol) {
				report.warnings.push_back("Destination Header not found: " + header_label(header.dstHeader));
				report.conflicts++;
			}
			Column* srcCol = src.find_column(header.srcHeader.name, header.srcHeader.occurrence);
			if (!srcCol) {
				report.warnings.push_back("Source Header not found: " + header_label(header.srcHeader));
				report.conflicts++;
			}
			if (!srcCol || !dstCol) {
				report.skippedHeaders++;
				continue;
			}
			for (const auto& value : srcCol->values) {
				dstCol->values.emplace_back(std::move(value));
				if (!value.second.empty())
					report.cellsWritten++;
				if (dstCol->values.size() > dst.rowCount)
					dst.rowCount = dstCol->values.size() + 1;
			}
			report.rowsMatched++;
		}
	}
	for (auto& col : dst.columns) {
		while (col.values.size() < dst.rowCount - 1) {
			col.values.emplace_back(std::monostate{}, "");
		}
	}
	report.rowsAppended = dst.rowCount - startCount;
	t.Stop();
	logging::loginfo("[project::MergeTables] Tables merged:\n\
					Destination:\t%s\n\
					Source:\t\t%s\n\
					Time:\t\t\t%.2fms (%.2fS)\n\
					MergeReport:\n\
							Merging Type:\t\t%s\n\
							Rows read:\t\t\t%zu\n\
							Rows written:\t\t %zu\n\
							Cells written:\t\t%zu\n\
							Rows appended:\t\t%zu\n\
							Rows matched:\t\t %zu\n\
							Conflicts:\t\t\t%zu\n\
							Skipped headers:\t  %zu\n\
							Warnings:\t\t\t %zu\n\
							Errors:\t\t\t   %zu",
		dst.name.c_str(),
		src.name.c_str(),
		t.GetElapsedMilliseconds(), t.GetElapsedSeconds(),
		report.type.c_str(),
		report.rowsRead,
		report.rowsWritten,
		report.cellsWritten,
		report.rowsAppended,
		report.rowsMatched,
		report.conflicts,
		report.skippedHeaders,
		report.warnings.size(),
		report.errors.size());
	return report;
}
