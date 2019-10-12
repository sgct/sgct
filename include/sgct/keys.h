/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__KEYS__H__
#define __SGCT__KEYS__H__

// abock(2019-10-02);  The values here are hardcoded as I don't want to pull in the
// entirety of GLFW in this header just for the definitions.  And they most likely will
// not change anytime soon anyway (famous last words)

namespace sgct::modifier {
    constexpr const int Shift = 0x0001; // = GLFW_MOD_SHIFT;
    constexpr const int Control = 0x0002; // = GLFW_MOD_CONTROL;
    constexpr const int Alt = 0x0004; // = GLFW_MOD_ALT;
    constexpr const int Super = 0x0008; // = GLFW_MOD_SUPER;
} // namespace sgct::modifier

namespace sgct::key {
    constexpr const int Unknown = -1; // = GLFW_KEY_UNKNOWN;
    constexpr const int Space = 32; // =  GLFW_KEY_SPACE;
    constexpr const int Apostrophe = 39; // =  GLFW_KEY_APOSTROPHE;
    constexpr const int Comma = 44; // = GLFW_KEY_COMMA;
    constexpr const int Minus = 45; // = GLFW_KEY_MINUS;
    constexpr const int Period = 46; // = GLFW_KEY_PERIOD;
    constexpr const int Slash = 47; // = GLFW_KEY_SLASH;
    constexpr const int Key0 = 48; // = GLFW_KEY_0;
    constexpr const int Key1 = 49; // = GLFW_KEY_1;
    constexpr const int Key2 = 50; // = GLFW_KEY_2;
    constexpr const int Key3 = 51; // = GLFW_KEY_3;
    constexpr const int Key4 = 52; // = GLFW_KEY_4;
    constexpr const int Key5 = 53; // = GLFW_KEY_5;
    constexpr const int Key6 = 54; // = GLFW_KEY_6;
    constexpr const int Key7 = 55; // = GLFW_KEY_7;
    constexpr const int Key8 = 56; // = GLFW_KEY_8;
    constexpr const int Key9 = 57; // = GLFW_KEY_9;
    constexpr const int Semicolon = 59; // = GLFW_KEY_SEMICOLON;
    constexpr const int Equal = 61; // = GLFW_KEY_EQUAL;
    constexpr const int A = 65; // = GLFW_KEY_A;
    constexpr const int B = 66; // = GLFW_KEY_B;
    constexpr const int C = 67; // = GLFW_KEY_C;
    constexpr const int D = 68; // = GLFW_KEY_D;
    constexpr const int E = 69; // = GLFW_KEY_E;
    constexpr const int F = 70; // = GLFW_KEY_F;
    constexpr const int G = 71; // = GLFW_KEY_G;
    constexpr const int H = 72; // = GLFW_KEY_H;
    constexpr const int I = 73; // = GLFW_KEY_I;
    constexpr const int J = 74; // = GLFW_KEY_J;
    constexpr const int K = 75; // = GLFW_KEY_K;
    constexpr const int L = 76; // = GLFW_KEY_L;
    constexpr const int M = 77; // = GLFW_KEY_M;
    constexpr const int N = 78; // = GLFW_KEY_N;
    constexpr const int O = 79; // = GLFW_KEY_O;
    constexpr const int P = 80; // = GLFW_KEY_P;
    constexpr const int Q = 81; // = GLFW_KEY_Q;
    constexpr const int R = 82; // = GLFW_KEY_R;
    constexpr const int S = 83; // = GLFW_KEY_S;
    constexpr const int T = 84; // = GLFW_KEY_T;
    constexpr const int U = 85; // = GLFW_KEY_U;
    constexpr const int V = 86; // = GLFW_KEY_V;
    constexpr const int W = 87; // = GLFW_KEY_W;
    constexpr const int X = 88; // = GLFW_KEY_X;
    constexpr const int Y = 89; // = GLFW_KEY_Y;
    constexpr const int Z = 90; // = GLFW_KEY_Z;
    constexpr const int LeftBracket = 91; // = GLFW_KEY_LEFT_BRACKET;
    constexpr const int Backslash = 92; // = GLFW_KEY_BACKSLASH;
    constexpr const int RightBracket = 93; // = GLFW_KEY_RIGHT_BRACKET;
    constexpr const int GraveAccent = 96; // = GLFW_KEY_GRAVE_ACCENT;
    constexpr const int World1 = 161; // = GLFW_KEY_WORLD_1;
    constexpr const int World2 = 162; // = GLFW_KEY_WORLD_2;
    constexpr const int Esc = 256; // = GLFW_KEY_ESCAPE;
    constexpr const int Escape = 256; // = GLFW_KEY_ESCAPE;
    constexpr const int Enter = 257; // = GLFW_KEY_ENTER;
    constexpr const int Tab = 258; // = GLFW_KEY_TAB;
    constexpr const int Backspace = 259; // = GLFW_KEY_BACKSPACE;
    constexpr const int Insert = 260; // = GLFW_KEY_INSERT;
    constexpr const int Del = 261; // = GLFW_KEY_DELETE;
    constexpr const int Delete = 261; // = GLFW_KEY_DELETE;
    constexpr const int Right = 262; // = GLFW_KEY_RIGHT;
    constexpr const int Left = 263; // = GLFW_KEY_LEFT;
    constexpr const int Down = 264; // = GLFW_KEY_DOWN;
    constexpr const int Up = 265; // = GLFW_KEY_UP;
    constexpr const int Pageup = 266; // = GLFW_KEY_PAGE_UP;
    constexpr const int Pagedown = 267; // = GLFW_KEY_PAGE_DOWN;
    constexpr const int PageUp = 266; // = GLFW_KEY_PAGE_UP;
    constexpr const int PageDown = 267; // = GLFW_KEY_PAGE_DOWN;
    constexpr const int Home = 268; // = GLFW_KEY_HOME;
    constexpr const int End = 269; // = GLFW_KEY_END;
    constexpr const int CapsLock = 280; // = GLFW_KEY_CAPS_LOCK;
    constexpr const int ScrollLock = 281; // = GLFW_KEY_SCROLL_LOCK;
    constexpr const int NumLock = 282; // = GLFW_KEY_NUM_LOCK;
    constexpr const int PrintScreen = 283; // = GLFW_KEY_PRINT_SCREEN;
    constexpr const int Pause = 284; // = GLFW_KEY_PAUSE;
    constexpr const int F1 = 290; // = GLFW_KEY_F1;
    constexpr const int F2 = 291; // = GLFW_KEY_F2;
    constexpr const int F3 = 292; // = GLFW_KEY_F3;
    constexpr const int F4 = 293; // = GLFW_KEY_F4;
    constexpr const int F5 = 294; // = GLFW_KEY_F5;
    constexpr const int F6 = 295; // = GLFW_KEY_F6;
    constexpr const int F7 = 296; // = GLFW_KEY_F7;
    constexpr const int F8 = 297; // = GLFW_KEY_F8;
    constexpr const int F9 = 298; // = GLFW_KEY_F9;
    constexpr const int F10 = 299; // = GLFW_KEY_F10;
    constexpr const int F11 = 300; // = GLFW_KEY_F11;
    constexpr const int F12 = 301; // = GLFW_KEY_F12;
    constexpr const int F13 = 302; // = GLFW_KEY_F13;
    constexpr const int F14 = 303; // = GLFW_KEY_F14;
    constexpr const int F15 = 304; // = GLFW_KEY_F15;
    constexpr const int F16 = 305; // = GLFW_KEY_F16;
    constexpr const int F17 = 306; // = GLFW_KEY_F17;
    constexpr const int F18 = 307; // = GLFW_KEY_F18;
    constexpr const int F19 = 308; // = GLFW_KEY_F19;
    constexpr const int F20 = 309; // = GLFW_KEY_F20;
    constexpr const int F21 = 310; // = GLFW_KEY_F21;
    constexpr const int F22 = 311; // = GLFW_KEY_F22;
    constexpr const int F23 = 312; // = GLFW_KEY_F23;
    constexpr const int F24 = 313; // = GLFW_KEY_F24;
    constexpr const int F25 = 314; // = GLFW_KEY_F25;
    constexpr const int Kp0 = 320; // = GLFW_KEY_KP_0;
    constexpr const int Kp1 = 321; // = GLFW_KEY_KP_1;
    constexpr const int Kp2 = 322; // = GLFW_KEY_KP_2;
    constexpr const int Kp3 = 323; // = GLFW_KEY_KP_3;
    constexpr const int Kp4 = 324; // = GLFW_KEY_KP_4;
    constexpr const int Kp5 = 325; // = GLFW_KEY_KP_5;
    constexpr const int Kp6 = 326; // = GLFW_KEY_KP_6;
    constexpr const int Kp7 = 327; // = GLFW_KEY_KP_7;
    constexpr const int Kp8 = 328; // = GLFW_KEY_KP_8;
    constexpr const int Kp9 = 329; // = GLFW_KEY_KP_9;
    constexpr const int KpDecimal = 330; // = GLFW_KEY_KP_DECIMAL;
    constexpr const int KpDivide = 331; // = GLFW_KEY_KP_DIVIDE;
    constexpr const int KpMultiply = 332; // = GLFW_KEY_KP_MULTIPLY;
    constexpr const int KpSubtract = 333; // = GLFW_KEY_KP_SUBTRACT;
    constexpr const int KpAdd = 334; // = GLFW_KEY_KP_ADD;
    constexpr const int KpEnter = 335; // = GLFW_KEY_KP_ENTER;
    constexpr const int KpEqual = 336; // = GLFW_KEY_KP_EQUAL;
    constexpr const int LeftShift = 340; // = GLFW_KEY_LEFT_SHIFT;
    constexpr const int LeftControl = 341; // = GLFW_KEY_LEFT_CONTROL;
    constexpr const int LeftAlt = 342; // = GLFW_KEY_LEFT_ALT;
    constexpr const int LeftSuper = 343; // = GLFW_KEY_LEFT_SUPER;
    constexpr const int RightShift = 344; // = GLFW_KEY_RIGHT_SHIFT;
    constexpr const int RightControl = 345; // = GLFW_KEY_RIGHT_CONTROL;
    constexpr const int RightAlt = 346; // = GLFW_KEY_RIGHT_ALT;
    constexpr const int RightSuper = 347; // = GLFW_KEY_RIGHT_SUPER;
    constexpr const int Menu = 348; // = GLFW_KEY_MENU;
    constexpr const int Last = Menu;
} // namespace sgct::key

#endif // __SGCT__KEYS__H__
