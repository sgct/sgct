
/* framelock_demo.c - WGL_swap_group example */

/* Copyright NVIDIA Corporation, 2003. */

/* Summary: This example demonstrates the use of the extensions WGL_NV_swap_group
   extension.  It queries the capabilities
   of the OpenGL implementation and depending on these capabilities it enables
   framelocking on the current display, joins a swap group and a barrier if available.

   Multiple instances of the application can be run to demonstrate 
   swap synchronization using swap groups and barriers.

   The application also demonstrates the universal frame counter control. The frame
   counter can be displayed and reset.

   In the application window, the application draws a horizontal moving red bar and a 
   vertically moving green bar whose positions are computed depending on the current
   frame counter. As the frame counter increases, the bars continue moving. The 
   master device can reset the frame counter, which also resets the bars to 
   their original position. 
*/


/* Windows command line compile instructions:

cl framelock_demo.c glut32.lib
  
*/

#include <framelock_ip.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <GL/glut.h>
#include <framelock_demo.h>

#define _VERSION 2.3

/* WGL function pointers */ 

PFNWGLJOINSWAPGROUPNVPROC       wglJoinSwapGroupNV;
PFNWGLBINDSWAPBARRIERNVPROC     wglBindSwapBarrierNV;
PFNWGLQUERYSWAPGROUPNVPROC      wglQuerySwapGroupNV;
PFNWGLQUERYMAXSWAPGROUPSNVPROC  wglQueryMaxSwapGroupNV;
PFNWGLQUERYFRAMECOUNTNVPROC     wglQueryFrameCountNV;
PFNWGLRESETFRAMECOUNTNVPROC     wglResetFrameCountNV;

PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB=NULL;

PFNWGLSWAPINTERVALEXTPROC wglGetSwapIntervalext=NULL;

ApplicationMode appMode = FRAMELOCK_SINGLE;

int nvWGLExtensionSupported(const char *extension);
GLboolean initSwapBarrierNV();
int initWGLExtensions();
void exitWGLExtensions();
void SetFullScreen(GLboolean fullscreen);
void SetWindowSize(HWND hWnd, int fStandardWindow);

#define FRAME_COUNT_SCALE  640

/* window state */
#define INITIAL_WINDOW_WIDTH  640
#define INITIAL_WINDOW_HEIGHT 480
GLint windowWidth = INITIAL_WINDOW_WIDTH;
GLint windowHeight = INITIAL_WINDOW_HEIGHT;
HDC hDC;
HWND hWndMain = 0;
RECT prevLoc = {100,100,640+100,480+100};

/* framelock state */
GLboolean hasSwapGroupNV      = GL_FALSE;
GLboolean hasSwapBarrierNV    = GL_FALSE;
GLboolean hasFrameCountNV     = GL_FALSE;
GLboolean useThinLines        = GL_FALSE;
GLboolean useFullScreen       = GL_FALSE;
GLboolean useForceVsync       = GL_FALSE;
GLboolean isMasterDevice      = GL_FALSE;
GLuint maxSwapGroups = 0;
GLuint maxSwapBarriers = 0;
GLuint swapGroup    = 0;
GLuint swapBarrier  = 0;

/* app state */
GLuint globalFrameCounter = 0;
GLboolean pauseRendering = GL_FALSE;
GLboolean displayCounter = GL_FALSE;
GLboolean paramSwapBarrier = GL_FALSE;
GLboolean useStereo = GL_FALSE;

/* IPFramecount settings */
GLboolean useIPFrameCount     = GL_FALSE;
unsigned int IPFrameCounter=0;
unsigned int IPport = 4343;    // Default port when using -useIPFrameCount -- make sure firewall is open

unsigned int numSlave=0;

unsigned int startTime=0;
unsigned int startFrame=0;
char slaveHost[16][256];



/* Help text */
TCHAR *pszHelp = "Keyboard commands:\r\n\r\n"
                 "B:\ttoggle swap barrier on/off\r\n"
                 "C:\ttoggle display of frame counter on/off\r\n"
                 "G:\ttoggle swap group on/off\r\n"
                 "F:\ttoggle full screen on/off\r\n"
                 "T:\ttoggle thin lines on/off\r\n"
                 "H:\tdisplay this help window\r\n"
                 "P:\tpause/continue rendering\r\n"
                 "Q:\tquit\r\n"
                 "R:\treset frame counter\r\n"
                 "S:\tdisplay framelock state";

#define ENDOF(pszBuffer) (pszBuffer+strlen(pszBuffer))

/* GLUT text drawing */
static void *font = GLUT_BITMAP_HELVETICA_18;

void Usage()
{
    printf("\n");
    printf("  Nvidia framelock_demo version %.1f\n",_VERSION);
    printf("\n");
    printf("  Usage: framelock_demo [-master|-slave] [-barrier] [-framecount] [-stereo] \n");
    printf("                        [-fullscreen] [-useIPFrameCount] [slaveHosts N IP]\n");
    printf("     -master initializes the framelock on a master device \n");
    printf("     -slave joins the framelock system as a slave\n");
    printf("     -barrier joins the swap barrier immediately\n");
    printf("     -framecount enables frame counter query and display per default\n");
    printf("     -stereo enables stereo window and displays different bars for left/right \n");
    printf("     -fullscreen creates the window fullscreen\n");
    printf("     -useIPFrameCount uses TCP/IP to send the framecounter information to \n");
    printf("                        the slave(s)\n");
    printf("     -slaveHosts specifies the slaves to send the framecounter to (only \n");
    printf("                        when using -useIPFrameCount)\n");
    printf(" \n");
    printf("     If no parameter is specified, the application doesn't initialize framelock\n");
    printf("     and just renders continuosly.\n");
    printf(" \n");
    printf("     Example use cases:\n");
    printf("     framelock_demo.exe -master -barrier -fullscreen\n");
    printf("     framelock_demo.exe -slave -barrier -fullscreen\n");
    printf("    \n");
    printf("     framelock_demo.exe -slave -barrier -useIPFrameCount (must start \n");
    printf("                        before master)\n");
    printf("     framelock_demo.exe -master -barrier -useIPFrameCount \n");
    printf("                        -slaveHosts 1 172.16.205.233\n");
    printf(" \n");
}

static void
printStr(int x, int y, const char *string)
{
    int len, i;

    glRasterPos2f((GLfloat) x, (GLfloat) y);
    len = (int) strlen(string);
    for (i = 0; i < len; i++) {
        glutBitmapCharacter(font, string[i]);
    }
}

/* application subroutines */

// helper function to update the window and icon title.
// Should be called after each swapgroup change as this is printed in the title.
static void setWindowAndIconTitle(void)
{
    char windowTitle[80] = "";

    switch (appMode) {
        case FRAMELOCK_SINGLE:
            sprintf(windowTitle, "NVIDIA FrameLock sample v%.1f (standalone process) group %d", _VERSION, swapGroup);
            break;
        case FRAMELOCK_MASTER:
            sprintf(windowTitle, "NVIDIA FrameLock sample v%.1f (master process) group %d", _VERSION, swapGroup);
            break;
        case FRAMELOCK_SLAVE:
            sprintf(windowTitle, "NVIDIA FrameLock sample v%.1f (slave process) group %d", _VERSION, swapGroup);
            break;
    }

    glutSetWindowTitle(windowTitle);
    glutSetIconTitle(windowTitle);
}


static GLuint updateFrameCounter()
{
    unsigned int i;
    unsigned int endTime=timeGetTime();

    if (useIPFrameCount) {
        if (appMode == FRAMELOCK_MASTER) {
            //printf("sendToClientUint: %u\n",IPFrameCounter);
            for (i=0; i<numSlave;i++) {
                unsigned int in;
                sendToClientUint(i,IPFrameCounter);
                recvFromClientUint(i,&in);
                if (IPFrameCounter!=in)
                    printf("IPFrameCoutner Error: %u %u\n",IPFrameCounter,in);

            }

        } else {
            recvFromServerUInt(&IPFrameCounter);
            sendToServerUInt(IPFrameCounter);
            //printf("recvFromServerUInt: %u\n",IPFrameCounter);
        }
        globalFrameCounter=IPFrameCounter;

    } else {

        if (hasFrameCountNV && displayCounter) {
            wglQueryFrameCountNV(hDC, &globalFrameCounter);

        } else {
            //printf("No Global FrameCounter: %u\n",globalFrameCounter);
            if (pauseRendering==GL_FALSE) globalFrameCounter++;
        }
    }
#if 0
    if (startTime+1000<timeGetTime()) {
        if (useIPFrameCount) {
            printf("\nIPFrameCount FPS: %u\n",IPFrameCounter-startFrame);
        } else {
            printf("\nGSync FPS: %u\n",IPFrameCounter-startFrame);
        }

        startFrame=IPFrameCounter;
        startTime=endTime;
    }
#endif

    if (pauseRendering==GL_FALSE) IPFrameCounter=IPFrameCounter+1;

    return globalFrameCounter;
}


static void resetFrameCount()
{
    globalFrameCounter = 0;
    if (hasFrameCountNV && isMasterDevice) {
        wglResetFrameCountNV(hDC);
        glutPostRedisplay();
    }
}

static void pauseRender()
{
    pauseRendering = pauseRendering ? GL_FALSE : GL_TRUE;
}

static void toggleFullScreen()
{
    static int oldWidth  = INITIAL_WINDOW_WIDTH;
    static int oldHeight = INITIAL_WINDOW_HEIGHT;
    // Toggle fullscreen mode

    useFullScreen = (useFullScreen == GL_FALSE) ? GL_TRUE : GL_FALSE;


    if (useFullScreen) {
        // Remember old window size before going fullscreen.
        oldWidth = windowWidth;
        oldHeight = windowHeight;

        SetFullScreen(GL_TRUE);
        glutPostRedisplay();
    } else {
        // Set back pre-fullscreen window size.
        // Position is handled automatically.
        SetFullScreen(GL_FALSE);
        glutPostRedisplay();

        glutReshapeWindow(oldWidth, oldHeight);
    }
}

static void toggleSwapGroup()
{
    if (hasSwapGroupNV)
    {
        swapGroup = (swapGroup == 0) ? 1 :0;
        wglJoinSwapGroupNV(hDC, swapGroup);
        // Reflect that change in the title bar also.
        setWindowAndIconTitle();
        glutPostRedisplay();
    }
}

static void toggleSwapBarrier()
{
    if (hasSwapGroupNV && swapGroup > 0)
    {
        if (swapBarrier > 0) {
            swapBarrier = 0;
            wglBindSwapBarrierNV(swapGroup, swapBarrier);
        } else {
            hasSwapBarrierNV = initSwapBarrierNV();
        }
        glutPostRedisplay();
    }
}

static void toggleDisplayCounter()
{
    displayCounter = (displayCounter == GL_FALSE) ? GL_TRUE : GL_FALSE;
}

static void toggleThinLines()
{
    useThinLines = (useThinLines == GL_FALSE) ? GL_TRUE : GL_FALSE;
}

static void toggleSwapInterval()
{
    useForceVsync = (useForceVsync == GL_FALSE) ? GL_TRUE : GL_FALSE;

    if (useForceVsync) {

//       wglSwapIntervalext(1);

    } else {

//       wglSwapIntervalext(0);

    }

}

void displayHelp(HWND hwnd)
{
    TCHAR szTitle[] = "NVIDIA FrameLock - Help";

    MessageBox(hwnd, pszHelp, szTitle, MB_APPLMODAL|MB_ICONINFORMATION);
}

void displayAbout(HWND hwnd)
{
    TCHAR szTitle[] = "About NVIDIA Framelock";
    TCHAR szAbout[] = "About NVIDIA Framelock:\r\n\r\nCopyright NVIDIA Corporation, 2003\nwww.nvidia.com";

    MessageBox(hwnd, szAbout, szTitle, MB_APPLMODAL|MB_ICONINFORMATION);
}

void displayState(HDC hDC, HWND hwnd)
{
    TCHAR szTitle[] = "NVIDIA FrameLock State";
    TCHAR *pszState = calloc(1, 32000);
    
    if (!pszState) {
        return;
    }

    if (appMode == FRAMELOCK_SINGLE) {
        sprintf(ENDOF(pszState), "\nStandalone mode, framelock not active");
    } else {
        if (hasSwapGroupNV) {
            sprintf(ENDOF(pszState), "\r\nWGL_NV_swap_group initialized\n");
            sprintf(ENDOF(pszState), "  Swap Group: %d\n", swapGroup);
            sprintf(ENDOF(pszState), "  Swap Barrier: %d [%s]\n", swapBarrier, 
                    (hasSwapBarrierNV && swapBarrier) ? "enabled" :"disabled");
            sprintf(ENDOF(pszState), "  Universal Frame Counter: %s\n", 
                    hasFrameCountNV  ? "initialized" : "disabled");
        } else {
            sprintf(ENDOF(pszState), "\r\nWGL_NV_swap_group not initialized\n");
        }
    }
    MessageBox(hwnd, pszState, szTitle, MB_APPLMODAL|MB_ICONINFORMATION);
    free(pszState);
}

void display(void)
{
    GLint horiz_left, vert_top;
    GLint horiz_width, vert_width;
    GLuint frameCount;

    /* skip current frame while in pause mode */
    if (pauseRendering) {
        return;
    }

    /* refresh the current frame counter */
    frameCount = updateFrameCounter();


    /* adjust the width of the bars */
    horiz_width = (windowWidth / 12) + 1;
    vert_width  = (windowHeight / 9) + 1;

    /* determine the current position of the bars depending on 
       the frame counter */
#if 0
    horiz_left = frameCount % (windowWidth - horiz_width);
    vert_top   = frameCount % (windowHeight - vert_width);
#endif

    // IMW change to increment bar by one pixel regardless of width

    if (useThinLines) {
        horiz_left = (frameCount % windowWidth);
        vert_top   = (frameCount % windowHeight);
    } else {
        horiz_left = (frameCount % windowWidth) - horiz_width;
        vert_top   = (frameCount % windowHeight) - vert_width;
    }

    glClear(GL_COLOR_BUFFER_BIT);

    /* draw the vertical and horizontal bars as rectangles */
    glDrawBuffer(useStereo?GL_BACK_LEFT:GL_BACK);
    glColor3f(1, 0, 0);
    glRecti(horiz_left, windowHeight, 
            horiz_left + (useThinLines?1:horiz_width), 0);
    glColor3f(0, 1, 0);
    glRecti(0, vert_top, 
            windowWidth, vert_top + (useThinLines?1:vert_width) );

    /* draw another pair of bars for stereo display */
    if (useStereo) {
        glDrawBuffer(GL_BACK_RIGHT);
        glColor3f(1, 0, 1);
        glRecti(horiz_left + horiz_width, windowHeight, horiz_left + (useThinLines?(horiz_width+1):2*horiz_width), 0);
        glColor3f(0, 1, 1);
        glRecti(0, vert_top + vert_width, windowWidth, vert_top + (useThinLines?(vert_width+1):2*vert_width) );
    }

    glDrawBuffer(GL_BACK);

    /* display frame counter if enabled */
    if (displayCounter) {
        char counterString[16] = "";
        sprintf(counterString, "%d", frameCount);

        glColor3f(1, 1, 0);
        printStr(10, 10, counterString);
    }

    /* swap buffers to display current frame */
    glutSwapBuffers();
}

void reshape(int w, int h)
{
    windowWidth = w;
    windowHeight = h;
    glViewport(0, 0, windowWidth, windowHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, windowWidth, 0.0, windowHeight, 1.0, -1.0);
}

void keyboard(unsigned char c, int x, int y)
{
    switch (c) {
        case 27:
        case 'q':
        case 'Q':
            exitWGLExtensions();
            exit(0);
            break;
        case 'b':
        case 'B':
            toggleSwapBarrier();
            break;
        case 'c':
        case 'C':
            toggleDisplayCounter();
            break;
        case 'f':
        case 'F':
            toggleFullScreen();
            break;
        case 'g':
        case 'G':
            toggleSwapGroup();
            break;
        case 't':
        case 'T':
            toggleThinLines();
            break;
        case 'h':
        case 'H':
            displayHelp(hWndMain);
            break;
        case 'p':
        case 'P':
            pauseRender();
            break;
        case 'r':
        case 'R':
            resetFrameCount();
            break;
        case 's':
        case 'S':
            displayState(hDC, hWndMain);
            break;
        case 'v':
        case 'V':
            toggleSwapInterval();
            break;
        default:
            printf("unknown keycommand: %c\n",c);
            break;
    }
}

void specialkbd(int c, int x, int y)
{

    switch (c) {

        default:
//           printf("unknown keycommand: %d\n",c);
           break;
    }
}

void
animate(void)
{
    glutPostRedisplay();
}

void
menu(int item)
{
    switch (item) {
        case M_RESET_FRAME_COUNT:
            resetFrameCount();
            break;
        case M_PAUSE_RENDER:
            pauseRender();
            break;
        case M_TOGGLE_SWAP_GROUP:
            toggleSwapGroup();
            break;
        case M_TOGGLE_SWAP_BARRIER:
            toggleSwapBarrier();
            break;
        case M_TOGGLE_COUNTER_DISPLAY:
            toggleDisplayCounter();
            break;
        case M_TOGGLE_THIN_LINES:
            toggleThinLines();
            break;
        case M_DISPLAY_STATE:
            displayState(hDC, hWndMain);
            break;
        case M_TOGGLE_FULLSCREEN:
            toggleFullScreen();
            break;
        case M_DISPLAY_HELP:
            displayHelp(hWndMain);
            break;
        case M_ABOUT:
            displayAbout(hWndMain);
            break;
        default:
            assert(0);
    }
    glutPostRedisplay();
}

/* WGL extension initialization: looks for the extensions in the WGL extension 
   string and gets the function pointers for the supported extensions. Then 
   calls the extensions to initialize framelock, frame counter, swap group
   and swap barrier. For each feature, check return values of the calls to 
   get an initial state.
   */

GLboolean initSwapGroupNV() 
{
    if(nvWGLExtensionSupported("WGL_NV_swap_group")) 
    {
        wglJoinSwapGroupNV      = (PFNWGLJOINSWAPGROUPNVPROC)      wglGetProcAddress("wglJoinSwapGroupNV");
        wglBindSwapBarrierNV      = (PFNWGLBINDSWAPBARRIERNVPROC)    wglGetProcAddress("wglBindSwapBarrierNV");
        wglQuerySwapGroupNV       = (PFNWGLQUERYSWAPGROUPNVPROC)     wglGetProcAddress("wglQuerySwapGroupNV");
        wglQueryMaxSwapGroupNV    = (PFNWGLQUERYMAXSWAPGROUPSNVPROC) wglGetProcAddress("wglQueryMaxSwapGroupsNV");
        wglQueryFrameCountNV      = (PFNWGLQUERYFRAMECOUNTNVPROC)    wglGetProcAddress("wglQueryFrameCountNV");
        wglResetFrameCountNV      = (PFNWGLRESETFRAMECOUNTNVPROC)    wglGetProcAddress("wglResetFrameCountNV");
        if (!wglJoinSwapGroupNV     ||
                !wglBindSwapBarrierNV   ||
                !wglQuerySwapGroupNV    ||
                !wglQueryMaxSwapGroupNV ||
                !wglQueryFrameCountNV   ||
                !wglResetFrameCountNV      ) {
            printf("framelock_demo: error while initializing WGL_NV_swap_group.\n");
            return GL_FALSE;
        }
    } else {
        printf("framelock_demo: WGL_NV_swap_group not exported in this OpenGL implementation.\n");
        return GL_FALSE;
    }

    if (appMode != FRAMELOCK_SINGLE) {
        if (wglQueryMaxSwapGroupNV(hDC, &maxSwapGroups, &maxSwapBarriers)) {
            if (maxSwapGroups >= 1) {
                if(!wglJoinSwapGroupNV(hDC, 1)) {
                    printf("framelock_demo: error in wglJoinSwapGroupNV.\n");
                    return GL_FALSE;
                }
                swapGroup = 1;
                // Reflect that change in the title bar also.
                setWindowAndIconTitle();
            }
        } else {
            printf("framelock_demo: Error querying max swapgroups");
            return GL_FALSE;
        }
    }

    return GL_TRUE;
}

GLboolean initSwapBarrierNV() 
{
    if (hasSwapGroupNV && 
            maxSwapBarriers >= 1) {
        if (wglBindSwapBarrierNV(swapGroup, 1)) {
            swapBarrier = 1;
            return GL_TRUE;
        }
    }

    return GL_FALSE;
}

GLboolean initFrameCountNV() 
{
    // need the WGL_NV_swap_group extension
    if (hasSwapGroupNV) 
    {
        if (wglResetFrameCountNV(hDC)) {
            // if a reset of the framecount succeeeds, we are running 
            // on a master device
            isMasterDevice = GL_TRUE;
        }

        // a successful update of the framecounter confirms that we
        // can use the framelock device's universal framecount.
        if (!wglQueryFrameCountNV(hDC, &globalFrameCounter)) {
            printf("framelock_demo: error in wglQueryFrameCountNV.\n");
            return GL_FALSE;
        }
        return GL_TRUE;
    }

    return GL_FALSE;
}

int initWGLExtensions() 
{
    wglGetExtensionsStringARB=(PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
    if (!wglGetExtensionsStringARB) {
        return 0;
    }

//    wglSwapIntervalext      = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalext");

    /* Always call initSwapGroupNV to get the OpenGL extensions */
    hasSwapGroupNV      = initSwapGroupNV();

    if (appMode != FRAMELOCK_SINGLE) {
        /* swap barrier is disabled by default, may be enabled per cmdline */
        if (paramSwapBarrier) {
            hasSwapBarrierNV    = initSwapBarrierNV();
        }
        hasFrameCountNV     = initFrameCountNV();
    }

    return 1;
}

void exitWGLExtensions() 
{
    /* release swap group from swap barrier by binding it to 0 */
    if (hasSwapBarrierNV) {
        wglBindSwapBarrierNV(swapGroup, 0);
    }
    /* release window from swap group by binding it to 0 */
    if (hasSwapGroupNV) {
        wglJoinSwapGroupNV(hDC, 0);
    }
}

/*****************************************************************
* nvExtensionSupported() - is an OpenGL extension supported?
*****************************************************************/
int nvExtensionSupported(const char *extension)
{
    static const GLubyte *extensions = NULL;
    const GLubyte *start;
    GLubyte *where, *terminator;

    /* Extension names should not have spaces. */
    where = (GLubyte *) strchr(extension, ' ');
    if (where || *extension == '\0')
        return 0;

    if (!extensions) {
#if defined(macintosh)
        extensions = glGetString(GL_ALL_EXTENSIONS_NV);
#else
        extensions = glGetString(GL_EXTENSIONS);
#endif
    }
    /* It takes a bit of care to be fool-proof about parsing the
       OpenGL extensions string.  Don't be fooled by sub-strings,
       etc. */
    start = extensions;
    for (;;) {
        /* If your application crashes in the strstr routine below,
           you are probably calling glutExtensionSupported without
           having a current window.  Calling glGetString without
           a current OpenGL context has unpredictable results. */
        where = (GLubyte *) strstr((const char *) start, extension);
        if (!where)
            break;
        terminator = where + strlen(extension);
        if (where == start || *(where - 1) == ' ') {
            if (*terminator == ' ' || *terminator == '\0') {
                return 1;
            }
        }
        start = terminator;
    }
    return 0;
}

int nvWGLExtensionSupported(const char *extension)
{
    if (wglGetExtensionsStringARB != NULL) {
        static const GLubyte *extensions = NULL;
        const GLubyte *start;
        GLubyte *where, *terminator;

        /* Extension names should not have spaces. */
        where = (GLubyte *) strchr(extension, ' ');
        if (where || *extension == '\0')
            return 0;

        if (!extensions) {
            extensions = (const GLubyte *) wglGetExtensionsStringARB(hDC);
        }
        /* It takes a bit of care to be fool-proof about parsing the
           OpenGL extensions string.  Don't be fooled by sub-strings,
           etc. */
        start = extensions;
        for (;;) {
            /* If your application crashes in the strstr routine below,
               you are probably calling glutExtensionSupported without
               having a current window.  Calling glGetString without
               a current OpenGL context has unpredictable results.
               Please fix your program. */
            where = (GLubyte *) strstr((const char *) start, extension);
            if (!where)
                break;
            terminator = where + strlen(extension);
            if (where == start || *(where - 1) == ' ') {
                if (*terminator == ' ' || *terminator == '\0') {
                    return 1;
                }
            }
            start = terminator;
        }
    } 

    /* Old WGL extensions can still be found in the GL extensions
       string, so check for them there if all else fails. */
    return nvExtensionSupported(extension);
}

void SetFullScreen(GLboolean fullscreen)
{
    RECT screen;
    static HDC   hdc   = NULL;
    HMONITOR hMonitor;
    MONITORINFO mi;
    HWND hWnd;

    hdc = wglGetCurrentDC();
    hWnd = WindowFromDC(hdc);

    if (fullscreen) {

        // Save off previous position.
        GetWindowRect(hWnd, &prevLoc);

        hMonitor = MonitorFromRect(&prevLoc, MONITOR_DEFAULTTONEAREST);

        mi.cbSize = sizeof(mi);
        GetMonitorInfo(hMonitor, &mi);

        screen = mi.rcMonitor;

        // Remove the window decorations and menu.
        SetWindowLong(hWnd, GWL_STYLE, WS_VISIBLE);
        SetMenu(hWnd, NULL);
        // Make the window fullscreen.
        SetWindowPos(hWnd, HWND_TOPMOST, screen.left, screen.top,
                screen.right - screen.left, screen.bottom - screen.top, 0);
    } else {
        // Restore the window decorations and menu.
        SetWindowLong(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
        // Restore the window position.
        SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetWindowSize(hWnd, 0);
    }
} // SetFullScreen

void SetWindowSize(HWND hWnd, int fStandardWindow)
{
    if (fStandardWindow) {
        int xCenter, yCenter, dx, dy;
        xCenter = (prevLoc.right + prevLoc.left)/2;
        yCenter = (prevLoc.bottom + prevLoc.top)/2;
        dx = GetSystemMetrics(SM_CXSIZEFRAME)*2;
        dy = GetSystemMetrics(SM_CYSIZEFRAME)*2 + GetSystemMetrics(SM_CYMENU) +
            GetSystemMetrics(SM_CYCAPTION);
        MoveWindow(hWnd, xCenter - 128, yCenter - 128, 256 + dx, 256 + dy, 1);
    } else {
        MoveWindow(hWnd, prevLoc.left, prevLoc.top,
                prevLoc.right - prevLoc.left,
                prevLoc.bottom - prevLoc.top, 1);
    }
} // SetWindowSize

// return time in milliseconds
float QPC2(int i) 
{
    static LARGE_INTEGER freq  = {0};
    static LARGE_INTEGER start = {0};
    static LARGE_INTEGER zeiten[2] = {0,0};
    LARGE_INTEGER count;

    QueryPerformanceCounter(&count);

    if (0==freq.QuadPart) {
        QueryPerformanceFrequency(&freq);
        start = count;
    }

    if (0==i){
        zeiten[0].QuadPart = (count.QuadPart - start.QuadPart);
    } else {
        zeiten[1].QuadPart = (count.QuadPart - start.QuadPart);
    }

    // when 1 is given, return counted time
    if (1 == i) {
        float f1,fr;
        // ms -> adjust frequency reference by 1000.
        fr = (float)freq.QuadPart / 1000.0f;

        f1 = (float)zeiten[1].QuadPart - (float)zeiten[0].QuadPart;
        f1 = f1 / fr;

        return f1;
    }
    return 0.0f;
}

int
main(int argc, char **argv)
{
    int i;

    glutInitWindowSize(windowWidth, windowHeight);
    glutInit(&argc, argv);
    for (i=1; i<argc; i++) {
        if (!strcmp(argv[i], "-help")) {
            Usage();
            exit(0);
        } else if (!strcmp(argv[i], "-master")) {
            appMode = FRAMELOCK_MASTER;
        } else if (!strcmp(argv[i], "-slave")) {
            // don't override the master
            if (!appMode == FRAMELOCK_MASTER) {
                appMode = FRAMELOCK_SLAVE;
            }
        } else if (!strcmp(argv[i], "-barrier")) {
            paramSwapBarrier = GL_TRUE;
        } else if (!strcmp(argv[i], "-framecount")) {
            displayCounter = GL_TRUE;
        } else if (!strcmp(argv[i], "-stereo")) {
            useStereo = GL_TRUE;
        } else if (!strcmp(argv[i], "-fullscreen")) {
            useFullScreen = GL_TRUE;
        } else if (!strcmp(argv[i], "-useIPFrameCount")) {
            useIPFrameCount = GL_TRUE;
        } else if (!strcmp(argv[i], "-slaveHosts")) {
            unsigned int n;
            numSlave=atoi(argv[i+1]);
            printf("Number of Slaves %d\n",numSlave);

            for (n=0; n<numSlave;n++) {
                sprintf(slaveHost[n],"%s",argv[i+2+n]);
                printf("Slave %d: %s\n",n, slaveHost[n]);
            }
            i = i+2+numSlave;
        } else if (!strcmp(argv[i], "-IPport")) {
            IPport=atoi(argv[i+1]);
            i++;
        } else {
            printf("Unrecognized command: %s\n",argv[i]);
            Usage();
        }

    }

    if(useIPFrameCount) {
        if (appMode == FRAMELOCK_MASTER) {
            unsigned int i;
            initWinsock();
            for (i=0; i<numSlave;i++) {
                startClient(i,slaveHost[i],IPport);
                sendToClientUint(i,i+3);
            } 
        } else {
            unsigned int tint;
            initWinsock();
            startServer(IPport);
            recvFromServerUInt(&tint);
            printf("Got MasterStartData %d\n", tint);
        }
    }

    if (useStereo) {
        glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_STEREO);
    } else {
        glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    }
    glutCreateWindow("Dummy Window");

    // For the wgl extensions we need the right windowhandle.
    // The windowTitle is not unique, so use the threadid for that.
    {
        char cThreadId[20];
        sprintf(cThreadId ,"0x%x",GetCurrentThreadId());
        glutSetWindowTitle(cThreadId);
        hWndMain = FindWindow(NULL, cThreadId);
    }
    // Set the real window title.
    setWindowAndIconTitle();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialkbd);

    hDC = GetDC(hWndMain);

    // Check for extensions 1st. If we get none we're most likely running on MicroSoft OpenGL.
    if (0 == initWGLExtensions()) {
        printf("wglGetExtensionsStringARB is not exported from this OpenGL version.\n");
        printf("Vendor is %s.\n", glGetString(GL_VENDOR));
        printf("Exit.\n");
        return 0;
    }

    // set window to fullscreen if desired.
    if (useFullScreen) {
        SetFullScreen(GL_TRUE);
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0,windowWidth,0.0,windowHeight,1.0,-1.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);

    glutCreateMenu(menu);
    if (hasFrameCountNV && isMasterDevice) {
        glutAddMenuEntry("Reset frame counter", M_RESET_FRAME_COUNT);
    }
    if (hasSwapGroupNV && swapGroup) {
        glutAddMenuEntry("Pause/continue rendering",   M_PAUSE_RENDER);
        glutAddMenuEntry("Toggle swap group",          M_TOGGLE_SWAP_GROUP);
        if (maxSwapBarriers > 0) {
            glutAddMenuEntry("Toggle swap barrier",    M_TOGGLE_SWAP_BARRIER);
        }
    }
    glutAddMenuEntry("Display/hide frame counter",        M_TOGGLE_COUNTER_DISPLAY);
    glutAddMenuEntry("Toggle thin lines",                 M_TOGGLE_THIN_LINES);
    glutAddMenuEntry("Toggle fullscreen",                 M_TOGGLE_FULLSCREEN);
    glutAddMenuEntry("Display framelock state",           M_DISPLAY_STATE);
    glutAddMenuEntry("Display help on keyboard commands", M_DISPLAY_HELP);
    glutAddMenuEntry("About NVIDIA FrameLock",            M_ABOUT);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    glutIdleFunc(animate);

    glutMainLoop();

    exitWGLExtensions();

    return 0;
}
