
# Nimble Analyzer
**Nimble Analyzer** is a tool to read, merge and process excel filedata (xlsx).

## Technical
It is written in C++ using [Raylib](https://www.raylib.com/), [ImGui](https://github.com/ocornut/imgui), [rlImGui](https://github.com/raylib-extras/rlImGui), [xlnt](https://github.com/xlnt-community/xlnt) and [nfd](https://github.com/btzy/nativefiledialog-extended).

## Cloning the repo:
```sh
git clone --recurse-submodules https://github.com/BigAgg/NimbleAnalyzer2.git
cd NimbleAnalyzer2
git submodule update --init --recursive
```

## Building the Analyzer
### Requirements:
- [CMake](https://cmake.org/) Version 3.8 or higher
- Any c++ Compiler that supports c++20 or higher like gcc
### Windows:
#### Preparing the Project:
Simply run the *setup.bat* file. It installs all Dependencies and sets up the Project.\
If you are using **Visual Studio**, it generates a *.slnx* project inside the build directory that is ready to use.

#### Compiling and Building using Visual Studio:
Enter the *build* directory and type following command:
```sh
cmake --build . --config Release
```
The executable can now be found inside the *Release* directory.


## Resources
[Raylib](https://www.raylib.com/)\
[ImGui](https://github.com/ocornut/imgui)\
[rlImGui](https://github.com/raylib-extras/rlImGui)\
[xlnt](https://github.com/xlnt-community/xlnt)\
[nfd](https://github.com/btzy/nativefiledialog-extended)

## License
[MIT](https://mit-license.org/)
