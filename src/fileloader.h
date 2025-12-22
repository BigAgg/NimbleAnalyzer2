#pragma once

#include <string>
#include <vector>

namespace fileloader {
	std::vector<std::string> loadfilelines(const std::string& filename, bool printinfo = true);
	std::string loadfile(const std::string& filename, bool printinfo = true);

	void savefilelines(const std::string& filename, std::vector<std::string> content, bool printinfo = true);
	void savefile(const std::string& filename, const std::string& content, bool printinfo = true);

	std::string fileinfo(const std::string& filename);

	bool exists(const std::string& filename);
};
