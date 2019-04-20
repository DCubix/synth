#ifndef SY_SDL_H
#define SY_SDL_H

#if __has_include("SDL2/SDL.h")
#include "SDL2/SDL.h"
#include "SDL2/SDL_audio.h"
#elif __has_include("SDL.h")
#include "SDL.h"
#include "SDL_audio.h"
#endif

#endif // SY_SDL_H