#include "fileloader.h"
#include <fstream>
#include <filesystem>
#include "timer.h"
#include "logging.h"
#include <chrono>
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
	return fs::exists(u8topath(filename));
}

std::vector<std::string> fileloader::iteratePath(const std::string& path, bool includeDirs, bool includeFiles)
{
	std::vector<std::string> content;
	const fs::path dir = u8topath(path);
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

std::string fileloader::u8topath(const std::string& path){
	fs::path p = fs::u8path(reinterpret_cast<const char*>(path.data()));
	return p.string();
}

std::string fileloader::GetLastWriteTime(const std::string& path){
	using namespace std::chrono;
	auto ftime = std::filesystem::last_write_time(u8topath(path));
	auto sctp = time_point_cast<system_clock::duration>(
		ftime - decltype(ftime)::clock::now() + system_clock::now());

	std::time_t cftime = system_clock::to_time_t(sctp);
	std::stringstream ss;
	ss << std::put_time(std::localtime(&cftime), "%F %T");
	return ss.str();
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

bool fileloader::csv::read_csv_record(std::ifstream& in, std::string& out_record){
	out_record.clear();
	std::string line;

	if (!std::getline(in, line))
		return false;

	out_record = line;

	// If quotes are unbalanced, keep reading lines (embedded newlines in quoted fields)
	auto count_quotes = [](const std::string& s) {
		size_t n = 0;
		for (char c : s) if (c == '"') ++n;
		return n;
		};

	while ((count_quotes(out_record) % 2) == 1)
	{
		if (!std::getline(in, line)) break;
		out_record.push_back('\n');
		out_record += line;
	}
	return true;

}

bool fileloader::csv::is_line_empty_or_ws(const std::string& s){
	for (unsigned char ch : s) if (!std::isspace(ch)) return false;
	return true;
}

std::string fileloader::csv::trim_ws(std::string s){
	size_t a = 0, b = s.size();
	while (a < b && std::isspace((unsigned char)s[a])) ++a;
	while (b > a && std::isspace((unsigned char)s[b - 1])) --b;
	return s.substr(a, b - a);
}

std::vector<std::string> fileloader::csv::split_csv_fields(const std::string& record, char delim) {
	std::vector<std::string> fields;
	std::string cur;
	cur.reserve(64);

	bool inQuotes = false;
	for (size_t i = 0; i < record.size(); ++i)
	{
		char ch = record[i];

		if (inQuotes)
		{
			if (ch == '"')
			{
				// escaped quote?
				if (i + 1 < record.size() && record[i + 1] == '"')
				{
					cur.push_back('"');
					++i;
				}
				else
				{
					inQuotes = false;
				}
			}
			else
			{
				cur.push_back(ch);
			}
		}
		else
		{
			if (ch == '"')
			{
				inQuotes = true;
			}
			else if (ch == delim)
			{
				fields.push_back(cur);
				cur.clear();
			}
			else
			{
				cur.push_back(ch);
			}
		}
	}
	fields.push_back(cur);
	return fields;
}

char fileloader::csv::sniff_delimiter(const std::string& headerLine){
	// Common: ';' in EU locales, ',' in US, '\t' for TSV
	int commas = 0, semis = 0, tabs = 0;
	bool inQuotes = false;

	for (size_t i = 0; i < headerLine.size(); ++i)
	{
		char ch = headerLine[i];
		if (ch == '"') inQuotes = !inQuotes;
		if (inQuotes) continue;
		if (ch == ',') ++commas;
		else if (ch == ';') ++semis;
		else if (ch == '\t') ++tabs;
	}

	if (tabs >= semis && tabs >= commas) return '\t';
	if (semis >= commas) return ';';
	return ',';
}
