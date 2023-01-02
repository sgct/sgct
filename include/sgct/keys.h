/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__KEYS__H__
#define __SGCT__KEYS__H__

// abock(2019-10-02);  The values here are hardcoded as I don't want to pull in the
// entirety of GLFW in this header just for the definitions.  And they most likely will
// not change anytime soon anyway (famous last words)

namespace sgct {

enum class Key {
    Unknown = -1, // = GLFW_KEY_UNKNOWN;
    Space = 32, // =  GLFW_KEY_SPACE;
    Apostrophe = 39, // =  GLFW_KEY_APOSTROPHE;
    Comma = 44, // = GLFW_KEY_COMMA;
    Minus = 45, // = GLFW_KEY_MINUS;
    Period = 46, // = GLFW_KEY_PERIOD;
    Slash = 47, // = GLFW_KEY_SLASH;
    Key0 = 48, // = GLFW_KEY_0;
    Key1 = 49, // = GLFW_KEY_1;
    Key2 = 50, // = GLFW_KEY_2;
    Key3 = 51, // = GLFW_KEY_3;
    Key4 = 52, // = GLFW_KEY_4;
    Key5 = 53, // = GLFW_KEY_5;
    Key6 = 54, // = GLFW_KEY_6;
    Key7 = 55, // = GLFW_KEY_7;
    Key8 = 56, // = GLFW_KEY_8;
    Key9 = 57, // = GLFW_KEY_9;
    Semicolon = 59, // = GLFW_KEY_SEMICOLON;
    Equal = 61, // = GLFW_KEY_EQUAL;
    A = 65, // = GLFW_KEY_A;
    B = 66, // = GLFW_KEY_B;
    C = 67, // = GLFW_KEY_C;
    D = 68, // = GLFW_KEY_D;
    E = 69, // = GLFW_KEY_E;
    F = 70, // = GLFW_KEY_F;
    G = 71, // = GLFW_KEY_G;
    H = 72, // = GLFW_KEY_H;
    I = 73, // = GLFW_KEY_I;
    J = 74, // = GLFW_KEY_J;
    K = 75, // = GLFW_KEY_K;
    L = 76, // = GLFW_KEY_L;
    M = 77, // = GLFW_KEY_M;
    N = 78, // = GLFW_KEY_N;
    O = 79, // = GLFW_KEY_O;
    P = 80, // = GLFW_KEY_P;
    Q = 81, // = GLFW_KEY_Q;
    R = 82, // = GLFW_KEY_R;
    S = 83, // = GLFW_KEY_S;
    T = 84, // = GLFW_KEY_T;
    U = 85, // = GLFW_KEY_U;
    V = 86, // = GLFW_KEY_V;
    W = 87, // = GLFW_KEY_W;
    X = 88, // = GLFW_KEY_X;
    Y = 89, // = GLFW_KEY_Y;
    Z = 90, // = GLFW_KEY_Z;
    LeftBracket = 91, // = GLFW_KEY_LEFT_BRACKET;
    Backslash = 92, // = GLFW_KEY_BACKSLASH;
    RightBracket = 93, // = GLFW_KEY_RIGHT_BRACKET;
    GraveAccent = 96, // = GLFW_KEY_GRAVE_ACCENT;
    World1 = 161, // = GLFW_KEY_WORLD_1;
    World2 = 162, // = GLFW_KEY_WORLD_2;
    Esc = 256, // = GLFW_KEY_ESCAPE;
    Escape = 256, // = GLFW_KEY_ESCAPE;
    Enter = 257, // = GLFW_KEY_ENTER;
    Tab = 258, // = GLFW_KEY_TAB;
    Backspace = 259, // = GLFW_KEY_BACKSPACE;
    Insert = 260, // = GLFW_KEY_INSERT;
    Del = 261, // = GLFW_KEY_DELETE;
    Delete = 261, // = GLFW_KEY_DELETE;
    Right = 262, // = GLFW_KEY_RIGHT;
    Left = 263, // = GLFW_KEY_LEFT;
    Down = 264, // = GLFW_KEY_DOWN;
    Up = 265, // = GLFW_KEY_UP;
    Pageup = 266, // = GLFW_KEY_PAGE_UP;
    Pagedown = 267, // = GLFW_KEY_PAGE_DOWN;
    PageUp = 266, // = GLFW_KEY_PAGE_UP;
    PageDown = 267, // = GLFW_KEY_PAGE_DOWN;
    Home = 268, // = GLFW_KEY_HOME;
    End = 269, // = GLFW_KEY_END;
    CapsLock = 280, // = GLFW_KEY_CAPS_LOCK;
    ScrollLock = 281, // = GLFW_KEY_SCROLL_LOCK;
    NumLock = 282, // = GLFW_KEY_NUM_LOCK;
    PrintScreen = 283, // = GLFW_KEY_PRINT_SCREEN;
    Pause = 284, // = GLFW_KEY_PAUSE;
    F1 = 290, // = GLFW_KEY_F1;
    F2 = 291, // = GLFW_KEY_F2;
    F3 = 292, // = GLFW_KEY_F3;
    F4 = 293, // = GLFW_KEY_F4;
    F5 = 294, // = GLFW_KEY_F5;
    F6 = 295, // = GLFW_KEY_F6;
    F7 = 296, // = GLFW_KEY_F7;
    F8 = 297, // = GLFW_KEY_F8;
    F9 = 298, // = GLFW_KEY_F9;
    F10 = 299, // = GLFW_KEY_F10;
    F11 = 300, // = GLFW_KEY_F11;
    F12 = 301, // = GLFW_KEY_F12;
    F13 = 302, // = GLFW_KEY_F13;
    F14 = 303, // = GLFW_KEY_F14;
    F15 = 304, // = GLFW_KEY_F15;
    F16 = 305, // = GLFW_KEY_F16;
    F17 = 306, // = GLFW_KEY_F17;
    F18 = 307, // = GLFW_KEY_F18;
    F19 = 308, // = GLFW_KEY_F19;
    F20 = 309, // = GLFW_KEY_F20;
    F21 = 310, // = GLFW_KEY_F21;
    F22 = 311, // = GLFW_KEY_F22;
    F23 = 312, // = GLFW_KEY_F23;
    F24 = 313, // = GLFW_KEY_F24;
    F25 = 314, // = GLFW_KEY_F25;
    Kp0 = 320, // = GLFW_KEY_KP_0;
    Kp1 = 321, // = GLFW_KEY_KP_1;
    Kp2 = 322, // = GLFW_KEY_KP_2;
    Kp3 = 323, // = GLFW_KEY_KP_3;
    Kp4 = 324, // = GLFW_KEY_KP_4;
    Kp5 = 325, // = GLFW_KEY_KP_5;
    Kp6 = 326, // = GLFW_KEY_KP_6;
    Kp7 = 327, // = GLFW_KEY_KP_7;
    Kp8 = 328, // = GLFW_KEY_KP_8;
    Kp9 = 329, // = GLFW_KEY_KP_9;
    KpDecimal = 330, // = GLFW_KEY_KP_DECIMAL;
    KpDivide = 331, // = GLFW_KEY_KP_DIVIDE;
    KpMultiply = 332, // = GLFW_KEY_KP_MULTIPLY;
    KpSubtract = 333, // = GLFW_KEY_KP_SUBTRACT;
    KpAdd = 334, // = GLFW_KEY_KP_ADD;
    KpEnter = 335, // = GLFW_KEY_KP_ENTER;
    KpEqual = 336, // = GLFW_KEY_KP_EQUAL;
    LeftShift = 340, // = GLFW_KEY_LEFT_SHIFT;
    LeftControl = 341, // = GLFW_KEY_LEFT_CONTROL;
    LeftAlt = 342, // = GLFW_KEY_LEFT_ALT;
    LeftSuper = 343, // = GLFW_KEY_LEFT_SUPER;
    RightShift = 344, // = GLFW_KEY_RIGHT_SHIFT;
    RightControl = 345, // = GLFW_KEY_RIGHT_CONTROL;
    RightAlt = 346, // = GLFW_KEY_RIGHT_ALT;
    RightSuper = 347, // = GLFW_KEY_RIGHT_SUPER;
    Menu = 348 // = GLFW_KEY_MENU;
};

} // namespace sgct

#endif // __SGCT__KEYS__H__
