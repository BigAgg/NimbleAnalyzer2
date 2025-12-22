#include "fileloader.h"
#include <fstream>
#include <filesystem>
#include "timer.h"
#include "logging.h"
#include "utils.h"

namespace fs = std::filesystem;

std::vector<std::string> fileloader::loadfilelines(const std::string& filename, bool printinfo) {
	std::vector<std::string> content;
	// retrieve content from file
	const std::string rawcontent = loadfile(filename, false);
	// Split into separate lines
	Timer t;
	t.Start();
	std::string::size_type start = 0;
	while (true) {
		auto pos = rawcontent.find('\n', start);

		if (pos == std::string::npos) {
			if (start < rawcontent.size()) {
				content.emplace_back(rawcontent.substr(start));
				content.back().pop_back();
			}
			break;
		}
		content.emplace_back(rawcontent.substr(start, pos - start));
		start = pos + 1;
	}
	t.Stop();
	// Printing info
	if (printinfo) {
		logging::loginfo("[fileloader::loadfile] File loaded:\n\
							Filename:\t%s\n\
							Filesize:\t%zu Bytes (%.2f Megabytes)\n\
							Lines loaded:\t%zu\n\
							Loading time:\t%.2f ms (%.4f Seconds)",
			AnsiToUtf8(filename).c_str(),
			rawcontent.size(), (float)((rawcontent.size() / 1024.0f) / 1024.0f),
			content.size(),
			t.GetElapsedMilliseconds(), t.GetElapsedSeconds());
	}
	return content;
}

std::string fileloader::loadfile(const std::string& filename, bool printinfo) {
	std::string content;
	// checking if file exists
	if (!exists(filename)) {
		logging::logwarning("[fileloader::loadfile] file does not exist: %s", filename.c_str());
		return content;
	}
	Timer t;
	t.Start();
	// Open the file
	std::ifstream file(filename, std::ios::binary);
	if (!file) {
		logging::logwarning("[fileloader::loadfile] file could not be opened: %s", filename.c_str());
		return content;
	}
	content = std::string((std::istreambuf_iterator<char>(file)),
		std::istreambuf_iterator<char>());
	t.Stop();
	// Printing info
	if (printinfo) {
		logging::loginfo("[fileloader::loadfile] File loaded:\n\
							Filename:\t%s\n\
							Filesize:\t%zu Bytes (%.2f Megabytes)\n\
							Loading time:\t%.2fms (%.4f Seconds)",
			AnsiToUtf8(filename).c_str(),
			content.size(), (float)((content.size() / 1024.0f) / 1024.0f),
			t.GetElapsedMilliseconds(), t.GetElapsedSeconds());
	}
	convertContentToUTF8(&content);
	return content;
}

void fileloader::savefilelines(const std::string& filename, std::vector<std::string> content, bool printinfo){
	Timer t;
	t.Start();
	std::ofstream file(filename, std::ios::binary);
	if (!file) {
		logging::logwarning("[fileloader::savefilelines] file could not be opened: %s", filename.c_str());
		return;
	}
	size_t size = 0;
	for (const std::string& line : content) {
		file << line << "\n";
		size += line.size() + 1;
	}
	file.close();
	t.Stop();
	if (printinfo) {
		logging::loginfo("[fileloader::savefilelines] File saved:\n\
							Filename:\t%s\n\
							Filesize:\t%zu Bytes (%.2f Megabytes)\n\
							Lines:\t%zu\n\
							Saving time:\t%.2fms (%.4f Seconds)",
							filename.c_str(),
							size, (float)size/1024.0f,
							content.size(),
							t.GetElapsedMilliseconds(), t.GetElapsedSeconds());
	}
}

void fileloader::savefile(const std::string& filename, const std::string& content, bool printinfo){
	Timer t;
	t.Start();
	std::ofstream file(filename, std::ios::binary);
	if (!file) {
		logging::logwarning("[fileloader::savefile] file could not be opened: %s", filename.c_str());
		return;
	}
	file << content;
	file.close();
	t.Stop();
	if (printinfo) {
		logging::loginfo("[fileloader::savefile] File saved:\n\
							Filename:\t%s\n\
							Filesize:\t%zu Bytes (%.2f Megabytes)\n\
							Saving time:\t%.2fms (%.4f Seconds)",
							filename.c_str(),
							content.size(), (float)content.size() / 1024.0f,
							t.GetElapsedMilliseconds(), t.GetElapsedSeconds());
	}
}

std::string fileloader::fileinfo(const std::string& filename) {
	return std::string();
}

bool fileloader::exists(const std::string& filename) {
	return fs::exists(filename);
}
