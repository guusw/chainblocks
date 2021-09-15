#ifdef __EMSCRIPTEN__
// Installed path (sdl port)
#include "SDL2/SDL.h"
#else
// Source tree path (deps)
#include "SDL.h"
#endif