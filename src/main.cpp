#include "logging.h"
#include "app.h"

int main(int argc, char *argv[]) {
#ifdef NDEBUG
  logging::startlogging("", "run.log"); // log to file in Release mode
#endif
  try {
	App app;
	app.init("NimbleAnalyzer");
	app.run();
  }
  catch (std::exception& e) {
    logging::logerror("%s", e.what());
  }
#ifdef NDEBUG
  logging::stoplogging();
#endif
  return 0;
}

// This is needed when building in Release with msvc
#ifdef _MSC_VER
#ifdef NDEBUG
#include <Windows.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
  return main(__argc, __argv);
}
#endif
#endif
