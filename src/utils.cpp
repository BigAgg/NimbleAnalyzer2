#include "utils.h"
#include <codecvt>
#include <Windows.h>
#include <fstream>


bool s_CanDecodeAsCodePage(const std::vector<unsigned char>& bytes, UINT codePage);

std::string GetLastWriteTime(const std::filesystem::path& path) {
	using namespace std::chrono;

	auto ftime = std::filesystem::last_write_time(path);

	// Convert to system_clock time_point
	auto sctp = time_point_cast<system_clock::duration>(
		ftime - decltype(ftime)::clock::now() + system_clock::now());

	std::time_t cftime = system_clock::to_time_t(sctp);

	std::stringstream ss;
	ss << std::put_time(std::localtime(&cftime), "%F %T");  // e.g., 2025-05-23 13:45:00
	return ss.str();
}


bool StrContains(const std::string& input, const std::string& substring) {
	return (input.find(substring) != std::string::npos);
}

bool StrStartswith(const std::string& input, const std::string& start){
	return input.rfind(start, 0) == 0;
}

bool StrEndswith(const std::string& input, const std::string& ending){
	if (ending.size() > input.size()) return false;
	return std::equal(ending.rbegin(), ending.rend(), input.rbegin());
}

void RemoveAllSubstrings(std::string& input, const std::string& toRemove) {
	size_t pos;
	while ((pos = input.find(toRemove)) != std::string::npos) {
		input.erase(pos, toRemove.length());
	}
}

void ReplaceAllSubstrings(std::string& input, const std::string& from, const std::string& to){
	if (from.empty()) return;

	std::size_t start_pos = 0;
	while ((start_pos = input.find(from, start_pos)) != std::string::npos) {
		input.replace(start_pos, from.length(), to);
		start_pos += to.length(); // advance past replacement
	}
}

bool IsValidUTF8(const std::string& str){
	int c, i, ix, n, j;
	for (i = 0, ix = str.length(); i < ix; i++) {
		c = (unsigned char)str[i];
		if (c >= 0 && c <= 127) n = 0;
		else if ((c & 0xE0) == 0xC0) n = 1;
		else if ((c & 0xF0) == 0xE0) n = 2;
		else if ((c & 0xF8) == 0xF0) n = 3;
		else return false;
		for (j = 0; j < n && i < ix; j++) {
			if ((++i == ix) || (((unsigned char)str[i] & 0xC0) != 0x80)) return false;
		}
	}
	return true;
}

std::string Convert1252ToUTF8(const std::string& input){
	// Convert Windows-1252 to UTF-16
	int wideLen = MultiByteToWideChar(1252, 0, input.c_str(), -1, NULL, 0);
	std::wstring wideStr(wideLen, 0);
	MultiByteToWideChar(1252, 0, input.c_str(), -1, &wideStr[0], wideLen);

	// Convert UTF-16 to UTF-8
	int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, NULL, 0, NULL, NULL);
	std::string utf8Str(utf8Len, 0);
	WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &utf8Str[0], utf8Len, NULL, NULL);

	// Remove trailing null terminator
	utf8Str.pop_back();

	return utf8Str;
}

std::string ConvertUTF8To1252(const std::string& input){
	int wideLen = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, NULL, 0);
	if (wideLen == 0) return "";

	std::wstring wideStr(wideLen, 0);
	MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, &wideStr[0], wideLen);

	int ansiLen = WideCharToMultiByte(1252, 0, wideStr.c_str(), -1, NULL, 0, NULL, NULL);
	if (ansiLen == 0) return "";

	std::string ansiStr(ansiLen, 0);
	WideCharToMultiByte(1252, 0, wideStr.c_str(), -1, &ansiStr[0], ansiLen, NULL, NULL);

	ansiStr.pop_back(); // remove null terminator
	return ansiStr;
}

std::string AnsiToUtf8(const std::string& ansi){
	if (ansi.empty()) return {};

	// ANSI -> UTF-16
	int wlen = MultiByteToWideChar(CP_ACP, 0, ansi.data(), (int)ansi.size(),
		nullptr, 0);
	std::wstring wstr(wlen, L'\0');
	MultiByteToWideChar(CP_ACP, 0,
		ansi.data(), (int)ansi.size(),
		&wstr[0], wlen);

	// UTF-16 -> UTF-8
	int u8len = WideCharToMultiByte(CP_UTF8, 0,
		wstr.data(), (int)wstr.size(),
		nullptr, 0, nullptr, nullptr);
	std::string u8(u8len, '\0');
	WideCharToMultiByte(CP_UTF8, 0,
		wstr.data(), (int)wstr.size(),
		&u8[0], u8len, nullptr, nullptr);
	return u8;
}

std::string StrToWstr(const std::string& input){
	std::wstring wstr;
	try {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		wstr = converter.from_bytes(input);
	}
	catch (std::range_error& e) {
		size_t length = input.length();
		std::wstring result;
		result.reserve(length);
		for (size_t i = 0; i < length; i++) {
			result.push_back(input[i] & 0xFF);
		}
		wstr = result;
	}
	size_t len = std::wcstombs(nullptr, wstr.c_str(), 0) + 1;
	char* buffer = new char[len];
	std::wcstombs(buffer, wstr.c_str(), len);
	std::string output(buffer);
	delete[] buffer;

	return output;
}

std::wstring GetWstring(const std::string& input){
	try {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		return converter.from_bytes(input);
	}
	catch (std::range_error& e) {
		size_t length = input.length();
		std::wstring result;
		result.reserve(length);
		for (size_t i = 0; i < length; i++) {
			result.push_back(input[i] & 0xFF);
		}
		return result;
	}
}

std::pair<std::string, std::string> Splitlines(const std::string& input, const std::string& splitat) {
	size_t pos = input.find(splitat);
	if (pos == std::string::npos) {
		return { input, "" }; // No delimiter found
	}

	std::string left = input.substr(0, pos);
	std::string right = input.substr(pos + splitat.length());

	return { left, right };
}

bool IsNumber(const std::string& input) {
	try{
		if (input.size() == 0) return false;
		std::string str = input;
		std::replace(str.begin(), str.end(), ',', '.');
		// Checking for prefix
		size_t start = 0;
		if (input[0] == '-' || input[0] == '+') {
			if (input.size() == 1) return false;
			start = 1;
		}
		// Checking for any characters
		int dotcount = 0;
		for (size_t i = start; i < input.size(); ++i) {
			if (input[i] > 255 || input[i] < 0)
				return false;
			if (std::isdigit(input[i])) {
				continue;
			}
			else if (input[i] == '.' || input[i] == ',') {
				dotcount++;
				if (dotcount > 1)
					return false;
				continue;
			}
			else {
				return false;
			}
		}
		if (dotcount == 1)
			return true;
	}
	catch (const std::exception& e){
		return false;
	}
	return true;
}

bool IsInteger(const std::string& input) {
	if (input.size() == 0) return false;
	// First make sure there are no invalid characters
	size_t start = 0;
	if (input[0] == '+' || input[0] == '-')
		start = 1;
	for (size_t i = start; i < input.size(); i++) {
		if (input[0] > 255 || input[0] < 0)
			return false;
		if (!std::isdigit(input[i]))
			return false;
	}
	return true;
}

std::string ExcelSerialToDate(int serial) {
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

Encoding DetectEncoding(const std::wstring& path){
	std::ifstream file(path, std::ios::binary);
	if (!file) return Encoding::BINARY_OR_UNKNOWN;

	// Read first few bytes for BOM
	unsigned char bom[3] = { 0, 0, 0 };
	file.read(reinterpret_cast<char*>(bom), 3);
	std::streamsize readBytes = file.gcount();

	if (readBytes > 3 && bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF) {
		return Encoding::UTF8_BOM;
	}
	if (readBytes >= 2) {
		if (bom[0] == 0xFF && bom[1] == 0xFE) return Encoding::UTF16_LE;
		if (bom[0] == 0xFE && bom[1] == 0xFF) return Encoding::UTF16_BE;
	}

	// No BOM - rewind and read whole file for heuristics
	file.clear();
	file.seekg(0, std::ios::beg);
	
	std::vector<unsigned char> data((std::istreambuf_iterator<char>(file)),
		std::istreambuf_iterator<char>());
	if (data.empty()) {
		// Empty file, arbitrarily call it UTF8_NO_BOM
		return Encoding::UTF8_NO_BOM;
	}


	// If data looks like valid UTF-8, treat as UTF8_NO_BOM
	auto isValidUtf8 = [](const std::vector<unsigned char>& bytes) -> bool
		{
			size_t i = 0;
			while (i < bytes.size()) {
				unsigned char c = bytes[i];
				if (c <= 0x7F) { // ASCII
					++i;
				}
				else if ((c & 0xE0) == 0xC0) { // 2 bytes
					if (i + 1 >= bytes.size()) return false;
					unsigned char c1 = bytes[i + 1];
					if ((c1 & 0xC0) != 0x80) return false;
					// Overlong check
					if ((c & 0xFE) == 0xC0) return false;
					i += 2;
				}
				else if ((c & 0xF0) == 0xE0) { // 3 bytes
					if (i + 2 >= bytes.size()) return false;
					unsigned char c1 = bytes[i + 1];
					unsigned char c2 = bytes[i + 2];
					if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80) return false;
					i += 3;
				}
				else if ((c & 0xF8) == 0xF0) { // 4 bytes
					if (i + 3 >= bytes.size()) return false;
					unsigned char c1 = bytes[i + 1];
					unsigned char c2 = bytes[i + 2];
					unsigned char c3 = bytes[i + 3];
					if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80) return false;
					i += 4;
				}
				else {
					return false;
				}
			}
			return true;
		};

	if (isValidUtf8(data)) {
		return Encoding::UTF8_NO_BOM;
	}
	// At this point we *guess* ANSI (system code page)
	return Encoding::ANSI;
}

void convertContentToUTF8(std::string* content){
	std::vector<unsigned char> bytes(content->begin(), content->end());
	// Alreade is UTF8?
	if (s_CanDecodeAsCodePage(bytes, CP_UTF8)) {
		return;
	}
	// Is System ANSI?
	if (s_CanDecodeAsCodePage(bytes, GetACP())) {
		*content = AnsiToUtf8(*content);
	}
}

bool s_CanDecodeAsCodePage(const std::vector<unsigned char>& bytes, UINT codePage) {
	if (bytes.empty()) return true;

	int result = MultiByteToWideChar(
		codePage,
		MB_ERR_INVALID_CHARS,                      // fail on invalid sequences
		reinterpret_cast<LPCCH>(bytes.data()),
		static_cast<int>(bytes.size()),
		nullptr,
		0
	);
	return result > 0;   // 0 => failure (invalid char sequence)
}


