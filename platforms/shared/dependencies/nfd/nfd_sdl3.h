/*
  Native File Dialog Extended
  Repository: https://github.com/btzy/nativefiledialog-extended
  License: Zlib

  This header contains a function to convert an SDL3 window handle to a native window handle for
  passing to NFDe.
 */

#ifndef _NFD_SDL3_H
#define _NFD_SDL3_H

#include <SDL3/SDL.h>
#include <stdbool.h>
#include "nfd.h"

#ifdef __cplusplus
extern "C" {
#define NFD_INLINE inline
#else
#define NFD_INLINE static inline
#endif  // __cplusplus

/**
 *  Converts an SDL3 window handle to a native window handle that can be passed to NFDe.
 *  @param sdlWindow The SDL window handle.
 *  @param[out] nativeWindow The output native window handle, populated if and only if this function
 *  returns true.
 *  @return Either true to indicate success, or false to indicate failure. */
NFD_INLINE bool NFD_GetNativeWindowFromSDLWindow(SDL_Window* sdlWindow,
                                                 nfdwindowhandle_t* nativeWindow) {
    SDL_PropertiesID props = SDL_GetWindowProperties(sdlWindow);
    if (!props) {
        return false;
    }

#if defined(__APPLE__)
    {
        void* nswindow = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL);
        if (nswindow) {
            nativeWindow->type = NFD_WINDOW_HANDLE_TYPE_COCOA;
            nativeWindow->handle = nswindow;
            return true;
        }
    }
#elif defined(_WIN32)
    {
        void* hwnd = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
        if (hwnd) {
            nativeWindow->type = NFD_WINDOW_HANDLE_TYPE_WINDOWS;
            nativeWindow->handle = hwnd;
            return true;
        }
    }
#else
    {
        unsigned long xwindow = (unsigned long)SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
        if (xwindow) {
            nativeWindow->type = NFD_WINDOW_HANDLE_TYPE_X11;
            nativeWindow->handle = (void*)xwindow;
            return true;
        }
    }
#endif

    SDL_SetError("Unsupported native window type for NFD.");
    return false;
}

#undef NFD_INLINE
#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // _NFD_SDL3_H
