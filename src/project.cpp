#include "project.h"
#include <fstream>
#include "logging.h"
#include "utils.h"
#include "fileloader.h"

static const char* SETTINGS_FILE = "project.na";
namespace fl = fileloader;

void FileInfo::load(const std::string& path) {
	name = fl::getFilename(path);
	this->path = path;
}

void FileInfo::clear() {
	name.clear();
	path.clear();
	sheets.clear();
	activeSheet.clear();
}

bool FileInfo::ready() {
	return false;
}

void Project::load(const std::string& name, const std::string& path){
	this->name = name;
	this->path = path;
	const std::string filepath = fl::u8path(path + "/" + SETTINGS_FILE);
	std::ifstream file(filepath, std::ios::binary);
	if (!file) {
		logging::logwarning("Could not open and load settings file: %s", filepath.c_str());
		return;
	}
	std::string line;
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
			loadfile(Splitlines(line, " = ").second);
		}
	}

	loaded = true;
}

void Project::loadfile(const std::string& path) {
	if (path == "")
		return;
	activeFile.clear();
	activeFile.load(path);
	logging::loginfo("[Project::loadfile] New file selected:\n\
							Project:\t%s\n\
							File:\t\t%s", name.c_str(), path.c_str());
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
	mergeInfos.clear();
	activeMergeInfo.clear();
	loaded = false;
}

void Project::save() {
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
}

void MergeInfo::clear() {
	name.clear();
}

void SheetInfo::clear() {
	name.clear();
	data.clear();
	headers.clear();
	loaded = false;
}
