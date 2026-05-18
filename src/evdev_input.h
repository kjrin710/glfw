//========================================================================
// GLFW 3.5 Linux evdev input - www.glfw.org
//------------------------------------------------------------------------
// Copyright (c) 2024 GLFW contributors
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would
//    be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.
//
//========================================================================
//
// This header declares the evdev keyboard/mouse input backend for Linux.
// It reads events directly from /dev/input/event* devices, bypassing the
// display server input path for lower latency.
//========================================================================

// evdev state for the library
//
typedef struct _GLFWlibraryEvdev
{
    // Keyboard device
    int             keyboardFd;
    char            keyboardPath[256];

    // Mouse device
    int             mouseFd;
    char            mousePath[256];

    // Track modifier key state ourselves (evdev doesn't give a state mask)
    // bit 0: left shift, bit 1: right shift
    // bit 2: left ctrl,  bit 3: right ctrl
    // bit 4: left alt,   bit 5: right alt
    // bit 6: left super, bit 7: right super
    // bit 8: num lock,   bit 9: caps lock
    unsigned int    mods;

    // Virtual cursor position (accumulated from REL_X/REL_Y)
    double          cursorX;
    double          cursorY;

    // Last known window size for cursor clamping
    int             windowWidth;
    int             windowHeight;

    // Whether evdev input is active
    GLFWbool        active;

    // Cursor position sync with X11
    GLFWbool        cursorSynced;
    int             prevCursorMode;
} _GLFWlibraryEvdev;

// evdev state macros for platform.h
#define GLFW_EVDEV_STATE         _GLFWlibraryEvdev evdev;
#define GLFW_EVDEV_LIBRARY_STATE _GLFWlibraryEvdev evdev;


// Function declarations
GLFWbool _glfwInitEvdevInput(void);
void _glfwTerminateEvdevInput(void);
void _glfwPollEvdevInput(_GLFWwindow* window);

// Key code translation
int _glfwEvdevKeyToGLFWKey(int evdevCode);
