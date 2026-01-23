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
		logging::loginfo("read_value: line: %s value: %s", s.c_str(), p.second.c_str());
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
					switch (cell.data_type()) {
					case xlnt::cell_type::number:
						value = cell.value<double>();
						break;
					case xlnt::cell_type::boolean:
						value = cell.value<bool>();
						break;
					case xlnt::cell_type::empty:
					case xlnt::cell_type::date:
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
		while (headerIndex < (int)records.size() && fl::csv::is_line_empty_or_ws(records[headerIndex]))
			++headerIndex;

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

void MergeTables(SheetTable* dst, const SheetTable* src, const MergeSettings* settings) {

}
