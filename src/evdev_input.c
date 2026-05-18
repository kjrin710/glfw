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
// This module reads keyboard and mouse events directly from the Linux
// evdev subsystem, bypassing the X11/Wayland input path. This provides
// lower input latency for applications like Minecraft.
//
// Usage:
//   Set GLFW_EVDEV_KEYBOARD=/dev/input/eventX to specify keyboard device
//   Set GLFW_EVDEV_MOUSE=/dev/input/eventY to specify mouse device
//   If unset, the first detected keyboard/mouse is used automatically.
//========================================================================

#include "internal.h"

#if defined(_GLFW_X11) && defined(__linux__)

#include <X11/Xlib.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Helper to test a bit in a bit array
#define isBitSet(bit, arr) ((arr)[(bit) / 8] & (1 << ((bit) % 8)))

// Modifier bit definitions
#define MOD_LSHIFT   (1u << 0)
#define MOD_RSHIFT   (1u << 1)
#define MOD_LCTRL    (1u << 2)
#define MOD_RCTRL    (1u << 3)
#define MOD_LALT     (1u << 4)
#define MOD_RALT     (1u << 5)
#define MOD_LSUPER   (1u << 6)
#define MOD_RSUPER   (1u << 7)
#define MOD_NUMLOCK  (1u << 8)
#define MOD_CAPSLOCK (1u << 9)

// Build GLFW mods from our internal modifier state
static int buildGLFWMods(unsigned int mods)
{
    int result = 0;
    if (mods & (MOD_LSHIFT | MOD_RSHIFT))  result |= GLFW_MOD_SHIFT;
    if (mods & (MOD_LCTRL | MOD_RCTRL))    result |= GLFW_MOD_CONTROL;
    if (mods & (MOD_LALT | MOD_RALT))      result |= GLFW_MOD_ALT;
    if (mods & (MOD_LSUPER | MOD_RSUPER))  result |= GLFW_MOD_SUPER;
    if (mods & MOD_CAPSLOCK)               result |= GLFW_MOD_CAPS_LOCK;
    if (mods & MOD_NUMLOCK)                result |= GLFW_MOD_NUM_LOCK;
    return result;
}

// Update modifier state from an evdev key event
static void updateModifiers(unsigned int* mods, int evdevCode, int pressed)
{
    unsigned int bit = 0;
    switch (evdevCode)
    {
        case KEY_LEFTSHIFT:    bit = MOD_LSHIFT;   break;
        case KEY_RIGHTSHIFT:   bit = MOD_RSHIFT;   break;
        case KEY_LEFTCTRL:     bit = MOD_LCTRL;    break;
        case KEY_RIGHTCTRL:    bit = MOD_RCTRL;    break;
        case KEY_LEFTALT:      bit = MOD_LALT;     break;
        case KEY_RIGHTALT:     bit = MOD_RALT;     break;
        case KEY_LEFTMETA:     bit = MOD_LSUPER;   break;
        case KEY_RIGHTMETA:    bit = MOD_RSUPER;   break;
        case KEY_NUMLOCK:      bit = MOD_NUMLOCK;  break;
        case KEY_CAPSLOCK:     bit = MOD_CAPSLOCK; break;
        default: return;
    }

    if (pressed)
        *mods |= bit;
    else
        *mods &= ~bit;
}

// Translate evdev key code to GLFW key code
//
// This uses the Linux input event codes which are standardized by the kernel
// and correspond to physical key positions (not layout-dependent characters).
// The mapping follows the USB HID usage table which evdev keycodes are directly
// derived from.
//
int _glfwEvdevKeyToGLFWKey(int evdevCode)
{
    // The evdev key codes are derived from USB HID usage tables
    // and are layout-independent (physical positions)
    switch (evdevCode)
    {
        // Letters
        case KEY_A:             return GLFW_KEY_A;
        case KEY_B:             return GLFW_KEY_B;
        case KEY_C:             return GLFW_KEY_C;
        case KEY_D:             return GLFW_KEY_D;
        case KEY_E:             return GLFW_KEY_E;
        case KEY_F:             return GLFW_KEY_F;
        case KEY_G:             return GLFW_KEY_G;
        case KEY_H:             return GLFW_KEY_H;
        case KEY_I:             return GLFW_KEY_I;
        case KEY_J:             return GLFW_KEY_J;
        case KEY_K:             return GLFW_KEY_K;
        case KEY_L:             return GLFW_KEY_L;
        case KEY_M:             return GLFW_KEY_M;
        case KEY_N:             return GLFW_KEY_N;
        case KEY_O:             return GLFW_KEY_O;
        case KEY_P:             return GLFW_KEY_P;
        case KEY_Q:             return GLFW_KEY_Q;
        case KEY_R:             return GLFW_KEY_R;
        case KEY_S:             return GLFW_KEY_S;
        case KEY_T:             return GLFW_KEY_T;
        case KEY_U:             return GLFW_KEY_U;
        case KEY_V:             return GLFW_KEY_V;
        case KEY_W:             return GLFW_KEY_W;
        case KEY_X:             return GLFW_KEY_X;
        case KEY_Y:             return GLFW_KEY_Y;
        case KEY_Z:             return GLFW_KEY_Z;

        // Numbers
        case KEY_1:             return GLFW_KEY_1;
        case KEY_2:             return GLFW_KEY_2;
        case KEY_3:             return GLFW_KEY_3;
        case KEY_4:             return GLFW_KEY_4;
        case KEY_5:             return GLFW_KEY_5;
        case KEY_6:             return GLFW_KEY_6;
        case KEY_7:             return GLFW_KEY_7;
        case KEY_8:             return GLFW_KEY_8;
        case KEY_9:             return GLFW_KEY_9;
        case KEY_0:             return GLFW_KEY_0;

        // Function keys
        case KEY_F1:            return GLFW_KEY_F1;
        case KEY_F2:            return GLFW_KEY_F2;
        case KEY_F3:            return GLFW_KEY_F3;
        case KEY_F4:            return GLFW_KEY_F4;
        case KEY_F5:            return GLFW_KEY_F5;
        case KEY_F6:            return GLFW_KEY_F6;
        case KEY_F7:            return GLFW_KEY_F7;
        case KEY_F8:            return GLFW_KEY_F8;
        case KEY_F9:            return GLFW_KEY_F9;
        case KEY_F10:           return GLFW_KEY_F10;
        case KEY_F11:           return GLFW_KEY_F11;
        case KEY_F12:           return GLFW_KEY_F12;
        case KEY_F13:           return GLFW_KEY_F13;
        case KEY_F14:           return GLFW_KEY_F14;
        case KEY_F15:           return GLFW_KEY_F15;
        case KEY_F16:           return GLFW_KEY_F16;
        case KEY_F17:           return GLFW_KEY_F17;
        case KEY_F18:           return GLFW_KEY_F18;
        case KEY_F19:           return GLFW_KEY_F19;
        case KEY_F20:           return GLFW_KEY_F20;
        case KEY_F21:           return GLFW_KEY_F21;
        case KEY_F22:           return GLFW_KEY_F22;
        case KEY_F23:           return GLFW_KEY_F23;
        case KEY_F24:           return GLFW_KEY_F24;
        // F25-F30 don't exist as standard evdev codes, they go to KEY_KPDOT etc.

        // Special keys
        case KEY_ESC:           return GLFW_KEY_ESCAPE;
        case KEY_ENTER:         return GLFW_KEY_ENTER;
        case KEY_TAB:           return GLFW_KEY_TAB;
        case KEY_BACKSPACE:     return GLFW_KEY_BACKSPACE;
        case KEY_INSERT:        return GLFW_KEY_INSERT;
        case KEY_DELETE:        return GLFW_KEY_DELETE;
        case KEY_RIGHT:         return GLFW_KEY_RIGHT;
        case KEY_LEFT:          return GLFW_KEY_LEFT;
        case KEY_DOWN:          return GLFW_KEY_DOWN;
        case KEY_UP:            return GLFW_KEY_UP;
        case KEY_PAGEUP:        return GLFW_KEY_PAGE_UP;
        case KEY_PAGEDOWN:      return GLFW_KEY_PAGE_DOWN;
        case KEY_HOME:          return GLFW_KEY_HOME;
        case KEY_END:           return GLFW_KEY_END;
        case KEY_CAPSLOCK:      return GLFW_KEY_CAPS_LOCK;
        case KEY_SCROLLLOCK:    return GLFW_KEY_SCROLL_LOCK;
        case KEY_NUMLOCK:       return GLFW_KEY_NUM_LOCK;
        case KEY_SYSRQ:         return GLFW_KEY_PRINT_SCREEN;
        case KEY_PAUSE:         return GLFW_KEY_PAUSE;

        // Modifiers
        case KEY_LEFTSHIFT:     return GLFW_KEY_LEFT_SHIFT;
        case KEY_RIGHTSHIFT:    return GLFW_KEY_RIGHT_SHIFT;
        case KEY_LEFTCTRL:      return GLFW_KEY_LEFT_CONTROL;
        case KEY_RIGHTCTRL:     return GLFW_KEY_RIGHT_CONTROL;
        case KEY_LEFTALT:       return GLFW_KEY_LEFT_ALT;
        case KEY_RIGHTALT:      return GLFW_KEY_RIGHT_ALT;
        case KEY_LEFTMETA:      return GLFW_KEY_LEFT_SUPER;
        case KEY_RIGHTMETA:     return GLFW_KEY_RIGHT_SUPER;
        case KEY_COMPOSE:       return GLFW_KEY_MENU; // Menu key often mapped to compose

        // Space
        case KEY_SPACE:         return GLFW_KEY_SPACE;

        // Punctuation / symbols (US layout positions, physical keys)
        case KEY_MINUS:         return GLFW_KEY_MINUS;
        case KEY_EQUAL:         return GLFW_KEY_EQUAL;
        case KEY_LEFTBRACE:     return GLFW_KEY_LEFT_BRACKET;
        case KEY_RIGHTBRACE:    return GLFW_KEY_RIGHT_BRACKET;
        case KEY_BACKSLASH:     return GLFW_KEY_BACKSLASH;
        case KEY_SEMICOLON:     return GLFW_KEY_SEMICOLON;
        case KEY_APOSTROPHE:    return GLFW_KEY_APOSTROPHE;
        case KEY_GRAVE:         return GLFW_KEY_GRAVE_ACCENT;
        case KEY_COMMA:         return GLFW_KEY_COMMA;
        case KEY_DOT:           return GLFW_KEY_PERIOD;
        case KEY_SLASH:         return GLFW_KEY_SLASH;

        // Non-US keys
        case KEY_102ND:         return GLFW_KEY_WORLD_1; // Extra key on 102-key keyboards

        // Keypad
        case KEY_KP0:           return GLFW_KEY_KP_0;
        case KEY_KP1:           return GLFW_KEY_KP_1;
        case KEY_KP2:           return GLFW_KEY_KP_2;
        case KEY_KP3:           return GLFW_KEY_KP_3;
        case KEY_KP4:           return GLFW_KEY_KP_4;
        case KEY_KP5:           return GLFW_KEY_KP_5;
        case KEY_KP6:           return GLFW_KEY_KP_6;
        case KEY_KP7:           return GLFW_KEY_KP_7;
        case KEY_KP8:           return GLFW_KEY_KP_8;
        case KEY_KP9:           return GLFW_KEY_KP_9;
        case KEY_KPDOT:         return GLFW_KEY_KP_DECIMAL;
        case KEY_KPSLASH:       return GLFW_KEY_KP_DIVIDE;
        case KEY_KPASTERISK:    return GLFW_KEY_KP_MULTIPLY;
        case KEY_KPMINUS:       return GLFW_KEY_KP_SUBTRACT;
        case KEY_KPPLUS:        return GLFW_KEY_KP_ADD;
        case KEY_KPENTER:       return GLFW_KEY_KP_ENTER;
        case KEY_KPEQUAL:       return GLFW_KEY_KP_EQUAL;

        default:
            return GLFW_KEY_UNKNOWN;
    }
}

// Check if a device appears to be a keyboard (has letter keys, not just
// consumer/function keys like power buttons or volume knobs)
//
static GLFWbool isKeyboardDevice(int fd)
{
    char evBits[(EV_CNT + 7) / 8] = {0};
    char keyBits[(KEY_CNT + 7) / 8] = {0};

    if (ioctl(fd, EVIOCGBIT(0, sizeof(evBits)), evBits) < 0)
        return GLFW_FALSE;
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keyBits)), keyBits) < 0)
        return GLFW_FALSE;

    // Must have EV_KEY capability
    if (!isBitSet(EV_KEY, evBits))
        return GLFW_FALSE;

    // Must have standard letter keys (A-Z), not just consumer/function buttons
    // Check for KEY_A through KEY_Z
    int letterCount = 0;
    for (int k = KEY_A; k <= KEY_Z; k++)
    {
        if (isBitSet(k, keyBits))
            letterCount++;
    }

    // Must have at least a few letter keys to be considered a keyboard
    // (some devices like power buttons have EV_KEY but no letter keys)
    return letterCount >= 10;
}

// Check if a device appears to be a mouse (has relative axes and mouse buttons)
//
static GLFWbool isMouseDevice(int fd)
{
    char evBits[(EV_CNT + 7) / 8] = {0};
    char relBits[(REL_CNT + 7) / 8] = {0};
    char keyBits[(KEY_CNT + 7) / 8] = {0};

    if (ioctl(fd, EVIOCGBIT(0, sizeof(evBits)), evBits) < 0)
        return GLFW_FALSE;
    if (ioctl(fd, EVIOCGBIT(EV_REL, sizeof(relBits)), relBits) < 0)
        return GLFW_FALSE;
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keyBits)), keyBits) < 0)
        return GLFW_FALSE;

    // Must have relative axes
    if (!isBitSet(EV_REL, evBits))
        return GLFW_FALSE;

    // Must have REL_X and REL_Y
    if (!isBitSet(REL_X, relBits) || !isBitSet(REL_Y, relBits))
        return GLFW_FALSE;

    // Must have mouse buttons (BTN_LEFT, BTN_RIGHT, BTN_MIDDLE at minimum)
    if (!isBitSet(BTN_LEFT, keyBits) && !isBitSet(BTN_RIGHT, keyBits))
        return GLFW_FALSE;

    return GLFW_TRUE;
}

// Try to auto-detect and open a device of the given type
// Returns the fd on success, -1 on failure
//
static int openDeviceByType(GLFWbool isKeyboard, char* pathOut, size_t pathSize)
{
    const char* dirname = "/dev/input";
    DIR* dir = opendir(dirname);
    if (!dir)
        return -1;

    int bestFd = -1;
    struct dirent* entry;

    while ((entry = readdir(dir)))
    {
        // Only look at event* devices
        if (strncmp(entry->d_name, "event", 5) != 0)
            continue;

        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", dirname, entry->d_name);

        int fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd < 0)
            continue;

        GLFWbool match;
        if (isKeyboard)
            match = isKeyboardDevice(fd);
        else
            match = isMouseDevice(fd);

        if (match)
        {
            // Found a matching device
            if (pathOut)
            {
                strncpy(pathOut, path, pathSize - 1);
                pathOut[pathSize - 1] = '\0';
            }
            bestFd = fd;
            break;
        }

        close(fd);
    }

    closedir(dir);
    return bestFd;
}

// Open a specific device by path (from env var or auto-detection)
//
static int openDevice(const char* path)
{
    if (!path || !path[0])
        return -1;

    return open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
}

// Check if the given window currently has focus (via X11)
//
static GLFWbool windowHasFocus(_GLFWwindow* window)
{
    // Use X11 to check if our window is the active (focused) window
    Window focused;
    int revertTo;
    XGetInputFocus(_glfw.x11.display, &focused, &revertTo);

    return (focused == window->x11.handle);
}

// Drain all pending evdev events and convert to GLFW input calls
//
static void drainKeyboardEvents(int fd, _GLFWwindow* window)
{
    struct input_event e;

    for (;;)
    {
        errno = 0;
        if (read(fd, &e, sizeof(e)) < (ssize_t)sizeof(e))
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            // ENODEV means device was disconnected - we'll handle gracefully
            if (errno == ENODEV)
                break;
            break;
        }

        // Skip non-keyboard events
        if (e.type != EV_KEY)
            continue;

        // Skip events from keys we don't map
        int key = _glfwEvdevKeyToGLFWKey(e.code);
        if (key == GLFW_KEY_UNKNOWN)
            continue;

        // Determine action based on evdev value
        // value 0 = release, 1 = press, 2 = repeat -> treat as press
        int action;
        if (e.value == 0)
            action = GLFW_RELEASE;
        else
            action = GLFW_PRESS;
            // (GLFW 3.5 does not accept GLFW_REPEAT in _glfwInputKey)

        // Update our modifier tracking
        updateModifiers(&_glfw.evdev.mods, e.code, e.value != 0);

        int mods = buildGLFWMods(_glfw.evdev.mods);

        // Pass evdev scancode directly as the GLFW scancode
        _glfwInputKey(window, key, e.code, action, mods);
    }
}

// Drain all pending evdev mouse events and convert to GLFW input calls
//
static void drainMouseEvents(int fd, _GLFWwindow* window)
{
    struct input_event e;

    for (;;)
    {
        errno = 0;
        if (read(fd, &e, sizeof(e)) < (ssize_t)sizeof(e))
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            if (errno == ENODEV)
                break;
            break;
        }

        switch (e.type)
        {
            case EV_REL:
            {
                switch (e.code)
                {
                    case REL_X:
                        _glfw.evdev.cursorX += e.value;
                        break;
                    case REL_Y:
                        _glfw.evdev.cursorY += e.value;
                        break;
                    case REL_WHEEL:
                        // Standard wheel: value is typically 1 or -1 per notch
                        // High-resolution: value is 120 per notch
                        if (e.value == 120 || e.value == -120)
                            _glfwInputScroll(window, 0.0, (double)e.value / 120.0);
                        else
                            _glfwInputScroll(window, 0.0, (double)e.value);
                        break;
                    case REL_HWHEEL:
                        if (e.value == 120 || e.value == -120)
                            _glfwInputScroll(window, (double)e.value / 120.0, 0.0);
                        else
                            _glfwInputScroll(window, (double)e.value, 0.0);
                        break;
                    default:
                        break;
                }
                break;
            }

            case EV_KEY:
            {
                // Mouse button events
                int button = -1;
                switch (e.code)
                {
                    case BTN_LEFT:    button = GLFW_MOUSE_BUTTON_LEFT;    break;
                    case BTN_RIGHT:   button = GLFW_MOUSE_BUTTON_RIGHT;   break;
                    case BTN_MIDDLE:  button = GLFW_MOUSE_BUTTON_MIDDLE;  break;
                    case BTN_SIDE:     button = GLFW_MOUSE_BUTTON_4;       break;
                    case BTN_EXTRA:   button = GLFW_MOUSE_BUTTON_5;       break;
                    case BTN_FORWARD: button = GLFW_MOUSE_BUTTON_5;       break;
                    case BTN_BACK:    button = GLFW_MOUSE_BUTTON_4;       break;
                    default: break;
                }

                if (button >= 0)
                {
                    int action = (e.value != 0) ? GLFW_PRESS : GLFW_RELEASE;
                    int mods = buildGLFWMods(_glfw.evdev.mods);
                    _glfwInputMouseClick(window, button, action, mods);
                }
                break;
            }

            case EV_SYN:
                // SYN_REPORT: after processing all queued REL/KEY events,
                // submit the accumulated cursor position
                if (e.code == SYN_REPORT)
                {
                    // Only clamp cursor when NOT in disabled mode (in-game camera)
                    if (window->cursorMode != GLFW_CURSOR_DISABLED)
                    {
                        if (_glfw.evdev.cursorX < 0)
                            _glfw.evdev.cursorX = 0;
                        if (_glfw.evdev.cursorY < 0)
                            _glfw.evdev.cursorY = 0;
                        if (_glfw.evdev.windowWidth > 0 && _glfw.evdev.cursorX >= _glfw.evdev.windowWidth)
                            _glfw.evdev.cursorX = _glfw.evdev.windowWidth - 1;
                        if (_glfw.evdev.windowHeight > 0 && _glfw.evdev.cursorY >= _glfw.evdev.windowHeight)
                            _glfw.evdev.cursorY = _glfw.evdev.windowHeight - 1;
                    }

                    _glfwInputCursorPos(window, _glfw.evdev.cursorX, _glfw.evdev.cursorY);
                }
                break;

            default:
                break;
        }
    }
}


//////////////////////////////////////////////////////////////////////////
//////                       GLFW platform API                      //////
//////////////////////////////////////////////////////////////////////////

// Initialize evdev keyboard and mouse input
//
GLFWbool _glfwInitEvdevInput(void)
{
    memset(&_glfw.evdev, 0, sizeof(_glfw.evdev));

    _glfw.evdev.keyboardFd = -1;
    _glfw.evdev.mouseFd = -1;

    // Try environment variable overrides first
    const char* kbPath = getenv("GLFW_EVDEV_KEYBOARD");
    const char* msPath = getenv("GLFW_EVDEV_MOUSE");

    if (kbPath)
    {
        _glfw.evdev.keyboardFd = openDevice(kbPath);
        if (_glfw.evdev.keyboardFd >= 0)
        {
            strncpy(_glfw.evdev.keyboardPath, kbPath,
                    sizeof(_glfw.evdev.keyboardPath) - 1);
        }
    }

    if (msPath)
    {
        _glfw.evdev.mouseFd = openDevice(msPath);
        if (_glfw.evdev.mouseFd >= 0)
        {
            strncpy(_glfw.evdev.mousePath, msPath,
                    sizeof(_glfw.evdev.mousePath) - 1);
        }
    }

    // Auto-detect if not provided
    if (_glfw.evdev.keyboardFd < 0)
    {
        _glfw.evdev.keyboardFd = openDeviceByType(GLFW_TRUE,
                                                    _glfw.evdev.keyboardPath,
                                                    sizeof(_glfw.evdev.keyboardPath));
        if (_glfw.evdev.keyboardFd < 0)
        {
            _glfwInputError(GLFW_PLATFORM_ERROR,
                            "Evdev: Failed to auto-detect keyboard device");
        }
    }

    if (_glfw.evdev.mouseFd < 0)
    {
        _glfw.evdev.mouseFd = openDeviceByType(GLFW_FALSE,
                                                _glfw.evdev.mousePath,
                                                sizeof(_glfw.evdev.mousePath));
        if (_glfw.evdev.mouseFd < 0)
        {
            _glfwInputError(GLFW_PLATFORM_ERROR,
                            "Evdev: Failed to auto-detect mouse device");
        }
    }

    // Evdev is active if at least one device is available
    // (keyboard or mouse alone is still useful)
    _glfw.evdev.active = (_glfw.evdev.keyboardFd >= 0 || _glfw.evdev.mouseFd >= 0);

    { FILE* f = fopen("/tmp/glfw_debug.log", "a"); if (f) { fprintf(f, "INIT keyboard=%s fd=%d mouse=%s fd=%d active=%d\n", _glfw.evdev.keyboardPath[0] ? _glfw.evdev.keyboardPath : "(none)", _glfw.evdev.keyboardFd, _glfw.evdev.mousePath[0] ? _glfw.evdev.mousePath : "(none)", _glfw.evdev.mouseFd, _glfw.evdev.active); fclose(f); } }

    // Initialize cursor position to center of window (will be set properly later)
    _glfw.evdev.cursorX = 0;
    _glfw.evdev.cursorY = 0;

    return GLFW_TRUE;
}

// Terminate evdev input, close all device fds
//
void _glfwTerminateEvdevInput(void)
{
    if (_glfw.evdev.keyboardFd >= 0)
    {
        close(_glfw.evdev.keyboardFd);
        _glfw.evdev.keyboardFd = -1;
    }

    if (_glfw.evdev.mouseFd >= 0)
    {
        close(_glfw.evdev.mouseFd);
        _glfw.evdev.mouseFd = -1;
    }

    _glfw.evdev.active = GLFW_FALSE;
    memset(&_glfw.evdev, 0, sizeof(_glfw.evdev));
}

// Poll all pending evdev events and deliver to the focused GLFW window
//
void _glfwPollEvdevInput(_GLFWwindow* window)
{
    static int pollCount = 0;
    if (!_glfw.evdev.active)
        return;

    if (pollCount < 5 || pollCount % 300 == 0) {
        FILE* f = fopen("/tmp/glfw_debug.log", "a");
        if (f) { fprintf(f, "POLL #%d window=%p\n", pollCount, (void*)window); fclose(f); }
    }
    pollCount++;

    // Deliver evdev events even when window lacks focus (bypass X11 focus check
    // to ensure low-latency input delivery regardless of WM focus state)
    if (!window)
    {
        // Drain events so the fd buffer doesn't fill up, but discard them
        struct input_event e;
        if (_glfw.evdev.keyboardFd >= 0)
        {
            while (read(_glfw.evdev.keyboardFd, &e, sizeof(e)) == sizeof(e)) {}
        }
        if (_glfw.evdev.mouseFd >= 0)
        {
            while (read(_glfw.evdev.mouseFd, &e, sizeof(e)) == sizeof(e)) {}
        }
        return;
    }

    // Update window size for cursor clamping
    if (window)
    {
        int width, height;
        _glfwGetWindowSizeX11(window, &width, &height);
        _glfw.evdev.windowWidth = width;
        _glfw.evdev.windowHeight = height;
    }

    // Sync evdev cursor with X11 cursor position when in NORMAL/HIDDEN mode
    // (evdev accumulates relative movements but X11 cursor may be elsewhere)
    int cursorMode = window->cursorMode;
    if (cursorMode != _glfw.evdev.prevCursorMode)
    {
        _glfw.evdev.cursorSynced = GLFW_FALSE;
        _glfw.evdev.prevCursorMode = cursorMode;
    }
    if (cursorMode != GLFW_CURSOR_DISABLED && !_glfw.evdev.cursorSynced)
    {
        Window root, child;
        int rootX, rootY, winX, winY;
        unsigned int mask;
        if (XQueryPointer(_glfw.x11.display, window->x11.handle,
                          &root, &child, &rootX, &rootY, &winX, &winY, &mask))
        {
            _glfw.evdev.cursorX = winX;
            _glfw.evdev.cursorY = winY;
            _glfw.evdev.cursorSynced = GLFW_TRUE;
        }
    }

    // Drain keyboard events
    if (_glfw.evdev.keyboardFd >= 0)
        drainKeyboardEvents(_glfw.evdev.keyboardFd, window);

    // Drain mouse events
    if (_glfw.evdev.mouseFd >= 0)
        drainMouseEvents(_glfw.evdev.mouseFd, window);
}

#endif // _GLFW_X11 && __linux__
