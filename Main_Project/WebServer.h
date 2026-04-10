// Passthrough to ESP32 WebServer library.
// ESP32 core 3.x declares FS inside namespace fs — bring it to global scope
// before WebServer.h tries to use it as plain 'FS'.
#include <FS.h>
using fs::FS;
#include_next <WebServer.h>
