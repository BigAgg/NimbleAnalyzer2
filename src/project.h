#pragma once
#include <string>
#include <vector>
#include <xlnt/xlnt.hpp>
#include <variant>
#include <cstdint>

using ExcelValue = std::variant<
	std::monostate, // empty
	double,					// numbers
	std::int64_t,		// integers
	bool,
	std::string			// strings (also fallback)
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
	int dataRow = -1;
	bool stopAtEmpty = false;
};

struct SheetTable {
public:
	std::string name;
	std::string path;
	std::string activeSheet;
	std::vector<std::string> sheets;
	std::unordered_map<std::string, SheetSettings> sheetRows;
	std::size_t rowCount = 0;
	std::vector<Column> columns;
	std::unordered_map<std::string, std::vector<ColId>> byName;
	bool loaded = false;
	void clear();

	const Column* find_column(const std::string& header, std::uint32_t occurrence = 0) {
		auto it = byName.find(header);
		if (it == byName.end()) return nullptr;
		if (occurrence >= it->second.size()) return nullptr;

		return &columns[it->second[occurrence]];
	}
};

struct MergeInfo {
	std::string name;
	void clear();
};

struct Project {
	std::string name;
	std::string path;
	std::vector<std::string> files;
	SheetTable activeFile;
	std::vector<std::string> mergeInfos;
	MergeInfo activeMergeInfo;
	bool loaded = false;

	void load(const std::string& name, const std::string& path);
	void loadfile(const std::string& path, const std::string& sheet = "", SheetSettings sheetSettings = {});
	void addfile(const std::string& path);
	void removefile(const std::string& path);
	void clear();
	void save();
};
