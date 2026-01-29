#pragma once
#include <string>
#include <vector>
#include <xlnt/xlnt.hpp>
#include <variant>
#include <cstdint>
#include "utils.h"

/*struct ExcelDateTime {
	int year, month, day;
	int hour = 0, minute = 0, second = 0;

	bool operator==(const ExcelDateTime& rhs) const noexcept {
		return this == &rhs;
	}
	bool operator!=(const ExcelDateTime& rhs) const noexcept {
		return this != &rhs;
	}
};*/
static std::string ExcelSerialToDate(int serial) {
	int l = serial + 68569 + 2415019;
	int n = 4 * l / 146097;
	l = l - (146097 * n + 3) / 4;
	int i = 4000 * (l + 1) / 1461001;
	l = l - 1461 * i / 4 + 31;
	int j = 80 * l / 2447;
	int day = l - 2447 * j / 80;
	l = j / 11;
	int month = j + 2 - 12 * l;
	int year = 100 * (n - 49) + i + l;

	std::ostringstream oss;
	oss << std::setfill('0') << std::setw(2) << day << "." << std::setw(2) << month << "." << year;
	return oss.str();
}

using ExcelValue = std::variant<
	std::monostate, // empty
	double,					// numbers
	std::int64_t,		// integers
	bool,
	std::string			// strings (also fallback)
	//ExcelDateTime
>;

using ColId = std::uint32_t;

struct HeaderKey {
	std::string name;
	std::uint32_t occurrence; // 0,1,2 for duplicates
};

static std::string to_display(const ExcelValue& v) {
	struct {
		std::string operator()(std::monostate) const { return ""; }
		std::string operator()(double d) const {
			char buf[64];
			std::snprintf(buf, sizeof(buf), "%.15g", d);
			for (char& c : buf) {
				if (c == '.')
					c = ',';
			}
			return buf;
		}
		std::string operator()(std::int64_t i) const { return std::to_string(i); }
		std::string operator()(bool b) const { return b ? "true" : "false"; }
		std::string operator()(const std::string& s) const { return s; }
		/*std::string operator()(const ExcelDateTime& dt) const {
			char buf[32];
			std::snprintf(buf, sizeof(buf), "%02d.%02d.%04d", dt.day, dt.month, dt.year);
			return buf;
		}*/
	} vis;
	return std::visit(vis, v);
}

static std::string header_label(const HeaderKey& k) {
	if (k.occurrence == 0) return k.name;
	return k.name + " (" + std::to_string(k.occurrence + 1) + ")";
}

// parsing helpers
static std::string normalize_decimal(std::string s) {
	for (char& c : s) if (c == ',') c = '.';
	return s;
}

static ExcelValue parse_value_auto(const std::string& input) {
	std::string s = input;
	// Nothing
	if (s.empty()) return std::monostate{};
	// Bool
	if (s == "true") return true;
	if (s == "false") return false;
	// Normalize doubles
	std::string n = normalize_decimal(s);
	// int
	{
		char* end = nullptr;
		long long i = std::strtoll(n.c_str(), &end, 10);
		if (end && *end == '\0') return (std::int64_t)i;
	}
	// double
	{
		char* end = nullptr;
		double d = std::strtod(n.c_str(), &end);
		if (end && *end == '\0') return d;
	}
	// Date
	/*if (isDate(s)) {
		std::string splitter;
		if (s.contains("-"))
			splitter = '-';
		else
			splitter = '.';
		std::string year, month, day;
		std::pair<std::string, std::string> split = Splitlines(s, splitter);
		if (splitter[0] == s[4]) {
			year = split.first;
			month = Splitlines(split.second, splitter).first;
			day = Splitlines(split.second, splitter).second;
		}
		else {
			day = split.first;
			month = Splitlines(split.second, splitter).first;
			year = Splitlines(split.second, splitter).second;
		}
		ExcelDateTime dt;
		dt.year = std::stoi(year);
		dt.month = std::stoi(month);
		dt.day = std::stoi(day);
		return dt;
	}*/
	// fallback to string
	return s;
}

struct CellKey {
	int col;
	int row;
	bool operator==(const CellKey& o) const { return col == o.col && row == o.row; }
};

struct CellKeyHash {
	std::size_t operator()(const CellKey& k) const noexcept {
		return (std::size_t(std::uint32_t(k.col)) << 32) ^ std::size_t(std::uint32_t(k.row));
	}
};

struct Column {
	HeaderKey key;
	std::vector<std::pair<ExcelValue, std::string>> values;
};

struct SheetSettings {
	// Data settings
	int dataRow = -1;	// header row
	bool stopAtEmpty = false;	// stop if row is empty
};

struct SheetTable {
public:
	std::string name;
	std::string path;
	std::string activeSheet;
	std::vector<std::string> sheets;
	std::size_t rowCount = 0;
	std::vector<Column> columns;
	std::unordered_map<std::string, std::vector<ColId>> byName;
	bool loaded = false;
	void clear();

	Column* find_column(const std::string& header, std::uint32_t occurrence = 0){
		auto it = byName.find(header);
		if (it == byName.end()) return nullptr;
		if (occurrence >= it->second.size()) return nullptr;

		return &columns[it->second[occurrence]];
	}
};

struct MergeHeaders {
	HeaderKey srcHeader;
	HeaderKey dstHeader;
};

struct MergeSettings {
	std::string name;
	std::string mergefolder = "";
	SheetTable sourceFile = {};
	SheetSettings sheetSettings = {};
	MergeHeaders key = {};	// used to only fill row if the key matches
	bool reverseKey = false;	// used to reverse the key so only import if key is not present
	std::vector<MergeHeaders> mergeHeaders;
};

struct Project {
	std::string name;
	std::string path;
	std::vector<std::string> files;
	std::unordered_map<std::string, SheetSettings> sheetSettings;
	std::unordered_map<std::string, std::vector<MergeSettings>> mergeSettings;
	SheetTable activeFile;
	bool loaded = false;

	void load(const std::string& name, const std::string& path);
	void loadfile(const std::string& path, const std::string& sheet = "");
	void addfile(const std::string& path);
	void removefile(const std::string& path);
	void clear();
	void save();
	SheetSettings* getCurrentSettingsHandle() {
		return &sheetSettings[sheet_key(activeFile.path, activeFile.activeSheet)];
	}
	std::vector<MergeSettings>* getCurrentMergeSettingsHandle() {
		return &mergeSettings[sheet_key(activeFile.path, activeFile.activeSheet)];
	}
private:
	static std::string sheet_key(const std::string& file, const std::string& sheet);
	bool sheetSettingsLoaded = false;
	bool mergeSettingsLoaded = false;
	void load_all_sheetsettings();
	void save_all_sheetsettings() const;
	void load_all_mergesettings();
	void save_all_mergesettings() const;
};

SheetTable load_sheet(const std::string& filePath, const std::string& sheet, SheetSettings& sheetSettings);

struct SaveReport {
	size_t cellsWritten = 0;
	size_t cellsSkipped = 0;
	size_t conflicts = 0;
	std::vector<std::string> warnings;
	std::vector<std::string> errors;
};

SaveReport save_sheet(const std::string& filePath, SheetTable& table, const SheetSettings& ss);

struct MergeReport {
	std::string type = "";
	size_t rowsRead = 0;
	size_t rowsWritten = 0;
	size_t cellsWritten = 0;
	size_t rowsAppended = 0;
	size_t rowsMatched = 0;
	size_t conflicts = 0;
	size_t skippedHeaders = 0;
	std::vector<std::string> warnings;
	std::vector<std::string> errors;
};

MergeReport MergeTables(SheetTable& dst, SheetTable& src, const MergeSettings& settings);
