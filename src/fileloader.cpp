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
	fs::path path = fs::u8path(filename);
	return fs::exists(path);
}

std::vector<std::string> fileloader::iteratePath(const std::string& path, bool includeDirs, bool includeFiles)
{
	std::vector<std::string> content;
	const fs::path dir = path;
	if (!exists(dir)) {
		logging::logwarning("[fileloader::iteratePath] The selected path does not exist!");
		return content;
	}
	for (auto const& dir_entry : fs::directory_iterator{ dir }) {
		std::string pbuffer = dir_entry.path().string();
		convertContentToUTF8(&pbuffer);
		content.push_back(pbuffer);
	}
	logging::loginfo("[fileloader::iteratePath] Found %zu entries in %s", content.size(), path.c_str());
	return content;
}

std::string fileloader::getFilename(const std::string& path)
{
	fs::path p = path;
	return p.filename().string();
}

std::string fileloader::u8path(const std::string& path){
	fs::path p = fs::u8path(path);
	return p.string();
}

void fileloader::copy(const std::string& source, const std::string& dest, bool overwrite){
	fs::path s = fs::u8path(source);
	fs::path d = fs::u8path(dest);
	if(overwrite)
		fs::copy(s, d, fs::copy_options::overwrite_existing);
	else
		fs::copy(s, d, fs::copy_options::skip_existing);
}

void fileloader::createDirs(const std::string& dirs){
	fs::path path = fs::u8path(dirs);
	fs::create_directories(path);
}

void fileloader::del(const std::string& path){
	fs::path p = fs::u8path(path);
	fs::remove_all(p);
}
