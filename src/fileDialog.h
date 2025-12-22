#pragma once

/*
* fileDialog's purpose is to open a FileDialog or DirectoryDialog
* It is single Header only using nfd
* Working in Windows only!
*/

#include "nfd.h"
#include <stdio.h>
#include <stdlib.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

// Opens Directory Browser in Windows and returns selected Path as std::string
inline std::string OpenDirectoryDialog() {
	namespace fs = std::filesystem;
	// Initializing NFD and values
	NFD_Init();
	nfdu8char_t* outPath;
	nfdpickfolderu8args_t args = { 0 };
	// Pick a folder
	nfdresult_t result = NFD_PickFolderU8_With(&outPath, &args);
	// Generate the output string
	std::string outStr;
	if (result == NFD_OKAY) {
		outStr = std::string(outPath);
		NFD_FreePathU8(outPath);
	}
	else if (result == NFD_CANCEL) {
		outStr = "";
	}
	else {
		outStr = "";
	}
	NFD_Quit();
	for (char& c : outStr) {
		if (c == '\\') {
			c = '/';
		}
	}
	fs::path u8path = fs::u8path(outStr);
	return u8path.string();
}

// Opens File Browser in Windows and returns selected Path as std::string
inline std::string OpenFileDialog(const std::string& filterName, const std::string& fileEndings) {
	namespace fs = std::filesystem;
	// Initialize NFD and variables
	NFD_Init();
	nfdu8char_t* outPath;
	nfdu8filteritem_t filters[1] = { {filterName.c_str(), fileEndings.c_str()} };
	nfdopendialogu8args_t args = { 0 };
	args.filterList = filters;
	args.filterCount = 1;
	// Retrieving picked file
	nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
	// Generating the outstring
	std::string outStr = "";
	if (result == NFD_OKAY) {
		outStr = std::string(outPath);
		NFD_FreePathU8(outPath);
	}
	else if (result == NFD_CANCEL) {
		puts("User pressed cancel.");
	}
	else {
		printf("Error: %s\n", NFD_GetError());
	}
	// Closing everything and free the path
	NFD_Quit();
	for (char& c : outStr) {
		if (c == '\\') {
			c = '/';
		}
	}
	fs::path u8path = fs::u8path(outStr);
	return u8path.string();
}

inline std::string SaveFileDialog(const std::string& filterName, const std::string& fileEndings) {
	namespace fs = std::filesystem;
	// Initialize NFD and variables
	NFD_Init();
	nfdu8char_t* outPath;
	nfdu8filteritem_t filters[1] = { {filterName.c_str(), fileEndings.c_str()} };
	nfdsavedialogu8args_t args = { 0 };
	args.filterList = filters;
	args.filterCount = 1;
	// Retrieving picked file
	nfdresult_t result = NFD_SaveDialogU8_With(&outPath, &args);
	// Generating the outstring
	std::string outStr = "";
	if (result == NFD_OKAY) {
		outStr = std::string(outPath);
		NFD_FreePathU8(outPath);
	}
	else if (result == NFD_CANCEL) {
		puts("User pressed cancel.");
	}
	else {
		printf("Error: %s\n", NFD_GetError());
	}
	// Closing everything and free the path
	NFD_Quit();
	for (char& c : outStr) {
		if (c == '\\') {
			c = '/';
		}
	}
	fs::path u8path = fs::u8path(outStr);
	return u8path.string();
}

