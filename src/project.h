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

struct Column {
	HeaderKey key;
	std::vector<ExcelValue> values;
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
	void loadfile(const std::string& path, const std::string& sheet = "");
	void addfile(const std::string& path);
	void removefile(const std::string& path);
	void clear();
	void save();
};
