#pragma once
#include <string>
#include <vector>
#include <xlnt/xlnt.hpp>

struct HeaderInfo {
	std::string name;
	unsigned int col, row;
};

struct SheetInfo {
	std::string name;
	std::vector<std::vector<std::string>> data;
	std::vector<HeaderInfo> headers;
	bool loaded = false;

	void clear();
};

struct FileInfo {
	std::string name;
	std::string path;
	std::vector<std::string> sheets;
	SheetInfo activeSheet;

	void load(const std::string& path);
	void clear();
	bool ready();
};

struct MergeInfo {
	std::string name;
	void clear();
};

struct Project {
	std::string name;
	std::string path;
	std::vector<std::string> files;
	FileInfo activeFile;
	std::vector<std::string> mergeInfos;
	MergeInfo activeMergeInfo;
	bool loaded = false;

	void load(const std::string& name, const std::string& path);
	void loadfile(const std::string& path);
	void addfile(const std::string& path);
	void removefile(const std::string& path);
	void clear();
	void save();
};
