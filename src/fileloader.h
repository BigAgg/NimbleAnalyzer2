#pragma once

#include <string>
#include <vector>
#include <fstream>

namespace fileloader {
	std::vector<std::string> loadfilelines(const std::string& filename, bool printinfo = true);
	std::string loadfile(const std::string& filename, bool printinfo = true);

	void savefilelines(const std::string& filename, std::vector<std::string> content, bool printinfo = true);
	void savefile(const std::string& filename, const std::string& content, bool printinfo = true);

	std::string fileinfo(const std::string& filename);

	bool exists(const std::string& filename);

	std::vector<std::string> iteratePath(const std::string& path, bool includeDirs = true, bool includeFiles = true);
	std::string getFilename(const std::string& path);
	std::string u8path(const std::string& path);
	std::string u8topath(const std::string& path);
	std::string GetLastWriteTime(const std::string& path);
	std::string GetCurrentPath();

	void copy(const std::string& source, const std::string& dest, bool overwrite=true);
	void createDirs(const std::string& dirs);
	void del(const std::string& path);

	// csv loading
	namespace csv {
		bool read_csv_record(std::ifstream& in, std::string& out_record);
		bool is_line_empty_or_ws(const std::string& s);
		std::string trim_ws(std::string s);
		std::vector<std::string> split_csv_fields(const std::string& record, char delim);
		char sniff_delimiter(const std::string& headerLine);
	};
};
