#ifndef GLAPI
# define GLAPI __stdcall
# define __DEFINED_GLAPI
#endif

/* WGL_NV_swap_group */
typedef BOOL (GLAPI * PFNWGLJOINSWAPGROUPNVPROC) (HDC hDC, GLuint group);
typedef BOOL (GLAPI * PFNWGLBINDSWAPBARRIERNVPROC) (GLuint group, GLuint barrier);
typedef BOOL (GLAPI * PFNWGLQUERYSWAPGROUPNVPROC) (HDC hDC, GLuint * group, GLuint * barrier);
typedef BOOL (GLAPI * PFNWGLQUERYMAXSWAPGROUPSNVPROC) (HDC hDC, GLuint * maxGroups, GLuint * maxBarriers);
typedef BOOL (GLAPI * PFNWGLQUERYFRAMECOUNTNVPROC) (HDC hDC, GLuint * count);
typedef BOOL (GLAPI * PFNWGLRESETFRAMECOUNTNVPROC) (HDC hDC);

typedef BOOL (GLAPI * PFNWGLSWAPINTERVALEXTPROC) (int interval);


/* WGL_ARB_extensions_string */
typedef const char * (GLAPI * PFNWGLGETEXTENSIONSSTRINGARBPROC) (HDC hdc);

typedef enum {
    FRAMELOCK_SINGLE,   /* single standalone application */
    FRAMELOCK_MASTER,   /* master application, controls timing */
    FRAMELOCK_SLAVE     /* slave application, syncs to master timing */
} ApplicationMode;

/* Menu items. */
enum {
    M_RESET_FRAME_COUNT,
    M_PAUSE_RENDER,
    M_TOGGLE_SWAP_GROUP,
    M_TOGGLE_SWAP_BARRIER,
    M_TOGGLE_COUNTER_DISPLAY,
    M_TOGGLE_THIN_LINES,
    M_DISPLAY_STATE, 
    M_TOGGLE_FULLSCREEN, 
    M_DISPLAY_HELP, 
    M_ABOUT
};
