
/* framelock_demo.c - WGL_swap_group example */

/* Copyright NVIDIA Corporation, 2003. */

/* Summary: This example demonstrates the use of the extensions WGL_NV_swap_group
as well as WGL_I3D_swap_frame_lock/WGL_I3D_genlock. It queries the capabilities
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

/* Usage: framelock_demo [-master|-slave][-i3d][-barrier][-framecount][-stereo]
-master initializes the framelock on a master device 
-slave joins the framelock system as a slave
-i3d uses WGL_I3D_genlock to enable/disable genlock sync
-barrier joins the swap barrier immediately
-framecount enables frame counter query and display per default
-stereo enables stereo window and displays different bars for left/right 
If no parameter is specified, the application doesn't initialize framelock
and just renders continuosly.
*/ 

/* Windows command line compile instructions:

cl framelock_demo.c glut32.lib

*/



/*
Modified from  Michael.Tirtasana@science-computing.de
- Added Borderless Windows
- Added TCP FrameCounter
Options added:

-X <value> Window size
-Y <value> Window size
-XStart <value>Window start pos
-YStart <value> Window start pos
-turbo  Speed up 16x

-useIPFrameCount     uses port 4343 (check Firewall)
-slaveHosts <number of Sync Slaves> <hostname1> <hostname2> ....



*/


////////////////////////////////////
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib,"Ws2_32.lib")
#pragma warning(disable : 4996)
#pragma warning(disable : 4995)

SOCKET server_socket;
SOCKET client_socket[16];

//A buffer to get error string
char localBuffer[1025];


//Return the error string from the error code
char* getLastErrorMessage(char* buffer, DWORD size, DWORD errorcode)
{
	memset(buffer, 0, size);
	if(FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		errorcode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)buffer,
		size, NULL) == 0){

			//failed in format message, let just do error code then
			sprintf(buffer,"Error code is %d", errorcode);
	}
	return buffer;
}
//Return the sockaddr into a readable string
//You can certainly use WSAAddressToString too
char* getSockAddrAsString(char* buffer, DWORD size, struct sockaddr* saddr){
	memset(buffer, 0, size);
	if(saddr != NULL){
		switch(saddr->sa_family){
   case AF_INET:
	   sprintf(buffer,"AF_INET, Port=%d, IP=%s",
		   //note, this place really need to convert network order to host order
		   ntohs(((struct sockaddr_in*)saddr)->sin_port),
		   inet_ntoa(((struct sockaddr_in*)saddr)->sin_addr));
	   break;
   default:
	   break;
		}
	}
	return buffer;
} 


//http://www.tenouk.com/Winsock/Winsock2example4.html




int initWinsock()
{
	//initialize the winsock 2.2
	WSAData  wsadata;
	if(WSAStartup(MAKEWORD(2,2), &wsadata)){
		printf("Failed to Startup Winsock\n");
		return -1; 
	}
	else
	{
		printf("Winsock started\n");
		return 1; 
	}

	;
}


int startServer(unsigned int serverport)
{
	char serverportstr[64];
	sprintf(serverportstr,"%u",serverport);


	//sockaddr_in service;
	//memset(&service,0,sizeof(sockaddr_in));
	//service.sin_family = AF_INET;
	//service.sin_addr.s_addr = ADDR_ANY;
	//service.sin_port = htons(serverport);





	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_INET;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	struct addrinfo* addrs;
	if(getaddrinfo(NULL,(PCSTR) serverportstr, &hints, &addrs))
	{
		printf("Client: Error getaddrinfo %s\n", getLastErrorMessage(localBuffer, 1024, WSAGetLastError()));
		return 1;
	}

	//server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	server_socket = socket(addrs->ai_family, addrs->ai_socktype, addrs->ai_protocol);

	if (server_socket == INVALID_SOCKET)

	{
		printf("Server: Error socket(): %s\n", getLastErrorMessage(localBuffer, sizeof(localBuffer), WSAGetLastError()));
		WSACleanup();
		return -1;

	}
	else
	{
		printf("Server: socket() is OK!\n");
	}



	//if (bind(server_socket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR)
	if (bind(server_socket, addrs->ai_addr, (int)addrs->ai_addrlen) == SOCKET_ERROR)
	{
		printf("Server:  bind() failed: %s\n", getLastErrorMessage(localBuffer, sizeof(localBuffer), WSAGetLastError()));
		closesocket(server_socket);
		return -1;
	}
	else
	{
		printf("Server: bind() is OK!\n");
		printf("Server: Server IP: %s PORT: %u\n", inet_ntoa(((struct sockaddr_in*)addrs->ai_addr)->sin_addr),ntohs(((struct sockaddr_in*)addrs->ai_addr)->sin_port));
	}

	// Call the listen function, passing the created socket and the maximum number of allowed
	// connections to accept as parameters. Check for general errors.
	if (listen(server_socket, 10) == SOCKET_ERROR)
	{
		printf("Server: listen(): Error listening on socket  %s\n", getLastErrorMessage(localBuffer, sizeof(localBuffer), WSAGetLastError()));
		return -1;
	}
	else
	{
		printf("Server: listen() is OK, I'm waiting for connections...\n");
	}

	// Create a temporary SOCKET object called AcceptSocket for accepting connections.
	SOCKET AcceptSocket;
	// Create a continuous loop that checks for connections requests. If a connection
	// request occurs, call the accept function to handle the request.
	printf("Server: Waiting for a client to connect...\n" );
	// Do some verification...
	while (1)
	{
		AcceptSocket = SOCKET_ERROR;
		while (AcceptSocket == SOCKET_ERROR)
		{
			AcceptSocket = accept(server_socket, NULL, NULL);
		}
		// else, accept the connection...
		// When the client connection has been accepted, transfer control from the
		// temporary socket to the original socket and stop checking for new connections.
		printf("Server: Client Connected!\n");
		server_socket = AcceptSocket;
		break;
	}

	int tcpnodelay_flag = 1;
	/* set TCP_NODELAY to turn off Nagle's algorithm */
	if(setsockopt(server_socket, IPPROTO_TCP, TCP_NODELAY, (char *) &tcpnodelay_flag, sizeof(int)) == -1)
	{
		printf("Error setsockopt");
	}

	return 1;
}



int startClient(int numclient, char *client,unsigned int clientport)
{
	char clientportstr[64];
	sprintf(clientportstr,"%u",clientport);


	//try to resolve the IP from hostname
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_INET;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_socktype = SOCK_STREAM;
	struct addrinfo* addrs;
	if(getaddrinfo(client,(PCSTR) clientportstr, &hints, &addrs))
	{
		printf("Client: Error getaddrinfo %s\n", getLastErrorMessage(localBuffer, 1024, WSAGetLastError()));
		return 1;
	}
	else
	{
		struct addrinfo* paddr= addrs;
		while(paddr){
			printf("Client: Server IP: %s PORT: %u\n", inet_ntoa(((struct sockaddr_in*)paddr->ai_addr)->sin_addr),ntohs(((struct sockaddr_in*)paddr->ai_addr)->sin_port));
			paddr = paddr->ai_next;
		}
		printf("\n");
	}


	// Create a SOCKET for connecting to server
	client_socket[numclient] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client_socket[numclient] == INVALID_SOCKET)
	{
		printf("Client: Error  socket():  %s\n", getLastErrorMessage(localBuffer, sizeof(localBuffer), WSAGetLastError()));
		WSACleanup();
		return -1;

	}
	else
	{
		printf("Client: socket() is OK.\n");
	}

	// Connect to server.
	if(connect(client_socket[numclient], addrs->ai_addr, sizeof(*(addrs->ai_addr))) == SOCKET_ERROR )
	{
		printf("Client: Error  connect():  %s\n", getLastErrorMessage(localBuffer, sizeof(localBuffer), WSAGetLastError()));
		WSACleanup();
		return -1;

	}
	else
	{
		printf("Client: connect() is OK.\n");
	}



	int tcpnodelay_flag = 1;
	/* set TCP_NODELAY to turn off Nagle's algorithm */
	if(setsockopt(client_socket[numclient], IPPROTO_TCP, TCP_NODELAY, (char *) &tcpnodelay_flag, sizeof(int)) == -1)
	{
		printf("Error setsockopt");
	}




	return 1;
}


int sendToClientUint(int numclient,unsigned int data)
{
	if(send(client_socket[numclient] , (const char*) &data, sizeof(unsigned int), 0) == SOCKET_ERROR)
	{
		printf("Error sendToClientUint %d  reason %s\n", numclient ,getLastErrorMessage(localBuffer, sizeof(localBuffer), WSAGetLastError()));
		Sleep(1000);
		return -1;
	}
	//printf("Client %d send() OK\n",numclient);

	return 1;
}
int recvFromClientUint(int numclient,unsigned int *data)
{
	if(recv(client_socket[numclient] , (char*) data, sizeof(unsigned int), 0) == SOCKET_ERROR)
	{
		printf("Error recvFromClientUint %d  reason %s\n", numclient ,getLastErrorMessage(localBuffer, sizeof(localBuffer), WSAGetLastError()));
		Sleep(1000);
		return -1;
	}
	return 1;
}

int sendToServerUInt(unsigned int data)
{
	if(send(server_socket, (const char*) &data, sizeof(unsigned int), 0) == SOCKET_ERROR)
	{
		printf("Error sendToServerUInt reason %s\n", getLastErrorMessage(localBuffer, sizeof(localBuffer), WSAGetLastError()));
		Sleep(1000);
		return -1;
	}
	return 1;
}

int recvFromServerUInt(unsigned int *data)
{
	if(recv(server_socket, ( char*) data, sizeof(unsigned int), 0) == SOCKET_ERROR)
	{
		printf("Error recvFromServerUInt  reason %s\n", getLastErrorMessage(localBuffer, sizeof(localBuffer), WSAGetLastError()));
		Sleep(1000);
		return -1;
	}
	//printf("Client recv() OK\n");

	return 1;
}






////////////////////////////////////



#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "./glut/glut.h"
#include "framelock_demo.h"


#pragma warning(disable : 4996)
#pragma comment(lib,"./glut/glut32.lib")








/* WGL function pointers */ 
PFNWGLENABLEFRAMELOCKI3DPROC wglEnableFrameLockI3D;
PFNWGLDISABLEFRAMELOCKI3DPROC wglDisableFrameLockI3D;
PFNWGLISENABLEDFRAMELOCKI3DPROC wglIsEnabledFrameLockI3D;
PFNWGLQUERYFRAMELOCKMASTERI3DPROC wglQueryFrameLockMasterI3D;

PFNWGLENABLEGENLOCKI3DPROC			wglEnableGenlockI3D;
PFNWGLDISABLEGENLOCKI3DPROC			wglDisableGenlockI3D;
PFNWGLISENABLEDGENLOCKI3DPROC		wglIsEnabledGenlockI3D;
PFNWGLGENLOCKSOURCEI3DPROC			wglGenlockSourceI3D;
PFNWGLGETGENLOCKSOURCEI3DPROC		wglGetGenlockSourceI3D;
PFNWGLGENLOCKSOURCEEDGEI3DPROC		wglGenlockSourceEdgeI3D;
PFNWGLGETGENLOCKSOURCEEDGEI3DPROC	wglGetGenlockSourceEdgeI3D;
PFNWGLGENLOCKSAMPLERATEI3DPROC		wglGenlockSampleRateI3D;
PFNWGLGETGENLOCKSAMPLERATEI3DPROC	wglGetGenlockSampleRateI3D;
PFNWGLGENLOCKSOURCEDELAYI3DPROC		wglGenlockSourceDelayI3D;
PFNWGLGETGENLOCKSOURCEDELAYI3DPROC	wglGetGenlockSourceDelayI3D;
PFNWGLQUERYGENLOCKMAXSOURCEDELAYI3DPROC wglQueryGenlockMaxSourceDelayI3D;

PFNWGLJOINSWAPGROUPNVPROC       wglJoinSwapGroupNV;
PFNWGLBINDSWAPBARRIERNVPROC     wglBindSwapBarrierNV;
PFNWGLQUERYSWAPGROUPNVPROC      wglQuerySwapGroupNV;
PFNWGLQUERYMAXSWAPGROUPSNVPROC  wglQueryMaxSwapGroupNV;
PFNWGLQUERYFRAMECOUNTNVPROC     wglQueryFrameCountNV;
PFNWGLRESETFRAMECOUNTNVPROC     wglResetFrameCountNV;

PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB=NULL;

ApplicationMode appMode = FRAMELOCK_SINGLE;

int nvWGLExtensionSupported(const char *extension);
GLboolean initSwapBarrierNV();
void initWGLExtensions();
void exitWGLExtensions();

#define FRAME_COUNT_SCALE  640

/* window state */
GLint windowWidth = 6400;
GLint windowHeight = 4800;
HDC hDC;
HWND hWndMain = 0;

/* framelock state */
GLboolean useI3D       = GL_FALSE;
GLboolean hasGenLockI3D       = GL_FALSE;
GLboolean hasSwapGroupNV      = GL_FALSE;
GLboolean hasSwapBarrierNV    = GL_FALSE;
GLboolean hasFrameCountNV     = GL_FALSE;
BOOL      isMasterDevice         = GL_FALSE;
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

//mic
LONG WINAPI winstyle;
int xres=6400;
int yres=4800;
int xpos=0;
int ypos=0;

int useIPFrameCount=0;
unsigned int IPFrameCounter=0;

unsigned int turbo=1;
unsigned int numSlave=0;


unsigned int startTime=0;
unsigned int startFrame=0;
char slaveHost[16][256];



/* Help text */
char *pszHelp = "Keyboard commands:\r\n\r\nB:\ttoggle swap barrier on/off\r\nC:\ttoggle display of frame counter on/off\r\nG:\ttoggle swap group on/off\r\nH:\tdisplay this help window\r\nP:\tpause/continue rendering\r\nQ:\tquit\r\nR:\treset frame counter\r\nS:\tdisplay framelock state";

#define ENDOF(pszBuffer) (pszBuffer+strlen(pszBuffer))

/* GLUT text drawing */
static void *font = GLUT_BITMAP_HELVETICA_18;

static void
printStr(int x, int y, const char *string)
{
	int len, i;

	glRasterPos2f(x, y);
	len = (int) strlen(string);
	for (i = 0; i < len; i++) {
		glutBitmapCharacter(font, string[i]);
	}
}

/* application subroutines */

static GLuint updateFrameCounter()
{
	if(useIPFrameCount==1)
	{
		if(appMode == FRAMELOCK_MASTER)
		{
			//printf("sendToClientUint: %u\n",IPFrameCounter);
			for (unsigned int i=0; i<numSlave;i++)
			{
				sendToClientUint(i,IPFrameCounter);
				unsigned in;
				recvFromClientUint(i,&in);
				if(IPFrameCounter!=in)
					printf("IPFrameCoutner Error: %u %u\n",IPFrameCounter,in);

			}


		}
		else
		{
			recvFromServerUInt(&IPFrameCounter);
			sendToServerUInt(IPFrameCounter);
			//printf("recvFromServerUInt: %u\n",IPFrameCounter);
		}
		globalFrameCounter=IPFrameCounter;
	}
	else
	{


		if (hasFrameCountNV && displayCounter) 
		{
			wglQueryFrameCountNV(hDC, &globalFrameCounter);

		}
		else 
		{
			//printf("No Global FrameCounter: %u\n",globalFrameCounter);
			if(pauseRendering==FALSE) globalFrameCounter++;
		}
	}

	unsigned int endTime=timeGetTime();
	if(startTime+1000<timeGetTime())
	{
		if(useIPFrameCount==1)
		{
			printf("\nIPFrameCount FPS: %u\n",IPFrameCounter-startFrame);
		}
		else
		{
			printf("\nGSync FPS: %u\n",IPFrameCounter-startFrame);
		}

		startFrame=IPFrameCounter;
		startTime=endTime;

	}

	//printf("%u   ",IPFrameCounter);

	
	if(pauseRendering==FALSE) IPFrameCounter=IPFrameCounter+2;

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

static void toggleSwapGroup()
{
	if (hasSwapGroupNV)
	{
		swapGroup = (swapGroup == 0) ? 1 :0;
		wglJoinSwapGroupNV(hDC, swapGroup);
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

void displayHelp(HWND hwnd)
{
	char szTitle[] = "NVIDIA FrameLock - Help";

	MessageBox(hwnd, pszHelp, szTitle, MB_APPLMODAL|MB_ICONINFORMATION);
}

void displayAbout(HWND hwnd)
{
	char szTitle[] = "About NVIDIA Framelock";
	char szAbout[] = "About NVIDIA Framelock:\r\n\r\nCopyright NVIDIA Corporation, 2003\nwww.nvidia.com";

	MessageBox(hwnd, szAbout, szTitle, MB_APPLMODAL|MB_ICONINFORMATION);
}

void displayState(HDC hDC, HWND hwnd)
{
	char szTitle[] = "NVIDIA FrameLock State";
	char *pszState = (char *) calloc(1, 32000);

	if (!pszState) {
		return;
	}

	if (appMode == FRAMELOCK_SINGLE) {
		sprintf(ENDOF(pszState), "\nStandalone mode, framelock not active");
	} else {
		if (hasGenLockI3D) {
			BOOL isEnabled = FALSE;
			GLuint source, rate, delay, edge;
			wglIsEnabledGenlockI3D(hDC, &isEnabled);
			sprintf(ENDOF(pszState), "\r\nWGL_I3D_genlock initialized\n");
			sprintf(ENDOF(pszState), "  Genlock: %s\n", 
				isEnabled ? "Enabled" : "Disabled");
			if (isEnabled) {
				if (wglGetGenlockSourceI3D(hDC, &source)) {
					switch (source) {
					case WGL_GENLOCK_SOURCE_MULTIVIEW_I3D:     
					case WGL_GENLOCK_SOURCE_DIGITAL_SYNC_I3D: 
					case WGL_GENLOCK_SOURCE_DIGITAL_FIELD_I3D:
						sprintf(ENDOF(pszState), "  Sync source: internal\n");
						break;

					case WGL_GENLOCK_SOURCE_I3DERNAL_SYNC_I3D: 
					case WGL_GENLOCK_SOURCE_I3DERNAL_FIELD_I3D:
					case WGL_GENLOCK_SOURCE_I3DERNAL_TTL_I3D: 
						sprintf(ENDOF(pszState), "  Sync source: external\n");
						break;
					default:
						break;
					}   
				}
				if (wglGetGenlockSampleRateI3D(hDC, &rate))
					sprintf(ENDOF(pszState), "  Sample Rate: trigger every %d. pulse\n", rate);
				if (wglGetGenlockSourceDelayI3D(hDC, &delay))
					sprintf(ENDOF(pszState), "  SourceDelay: %d\n", delay);
				if (wglGetGenlockSourceEdgeI3D(hDC, &edge)) {
					switch (edge) {
					case WGL_GENLOCK_SOURCE_EDGE_FALLING_I3D:
						sprintf(ENDOF(pszState), "  Source edge: Falling\n");
						break;
					case WGL_GENLOCK_SOURCE_EDGE_RISING_I3D:
						sprintf(ENDOF(pszState), "  Source edge: Leading\n");
						break;
					case WGL_GENLOCK_SOURCE_EDGE_BOTH_I3D:
						sprintf(ENDOF(pszState), "  Source edge: Both\n");
						break;
					default:
						break;
					}
				}
			}
		} else {
			sprintf(ENDOF(pszState), "\r\nWGL_I3D_genlock not initialized\n");
		}

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

void
display(void)
{
	GLint horiz_left, vert_top;
	GLint horiz_width, vert_width;
	GLint frameCountPosition;
	GLfloat frameCountScale;
	GLuint frameCount;

	/* refresh the current frame counter */
	frameCount = updateFrameCounter()*turbo;

	/* skip current frame while in pause mode */
	//if (pauseRendering) {
	//	return;
	//}

	/* adjust the width of the bars */
	horiz_width = (windowWidth / 12) + 1;
	vert_width  = (windowHeight / 9) + 1;

	/* determine the current position of the bars depending on 
	the frame counter */
#if 0
	horiz_left = frameCount % (windowWidth - horiz_width);
	vert_top   = frameCount % (windowHeight - vert_width);
#endif
	// Dat's code
	frameCountPosition = frameCount % FRAME_COUNT_SCALE;
	frameCountScale    = (GLfloat)frameCountPosition / (GLfloat)FRAME_COUNT_SCALE;

	horiz_left = frameCountScale * (windowWidth - horiz_width);
	vert_top   = frameCountScale * (windowHeight - vert_width);

	glClear(GL_COLOR_BUFFER_BIT);

	/* draw the vertical and horizontal bars as rectangles */
	glDrawBuffer(GL_BACK_LEFT);
	glColor3f(1, 0, 0);
	glRecti(horiz_left, windowHeight, horiz_left + horiz_width, 0);
	glColor3f(0, 1, 0);
	glRecti(0, vert_top, windowWidth, vert_top + vert_width);

	/* draw another pair of bars for stereo display */
	if (useStereo) {
		glDrawBuffer(GL_BACK_RIGHT);
		glColor3f(1, 0, 1);
		glRecti(horiz_left + horiz_width, windowHeight, horiz_left + 2*horiz_width, 0);
		glColor3f(0, 1, 1);
		glRecti(0, vert_top + vert_width, windowWidth, vert_top + 2*vert_width);
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

void
reshape(int w, int h)
{
	windowWidth = w;
	windowHeight = h;
	glViewport(0, 0, windowWidth, windowHeight);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, windowWidth, 0.0, windowHeight, 1.0, -1.0);
}

void
keyboard(unsigned char c, int x, int y)
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
	case 'g':
	case 'G':
		toggleSwapGroup();
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
	default:
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
	case M_DISPLAY_STATE:
		displayState(hDC, hWndMain);
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

GLboolean initGenLockI3D() 
{
	if(nvWGLExtensionSupported("WGL_I3D_genlock")) 
	{
		wglEnableGenlockI3D				= (PFNWGLENABLEGENLOCKI3DPROC)					wglGetProcAddress("wglEnableGenlockI3D");
		wglDisableGenlockI3D				= (PFNWGLDISABLEGENLOCKI3DPROC)					wglGetProcAddress("wglDisableGenlockI3D");
		wglIsEnabledGenlockI3D				= (PFNWGLISENABLEDGENLOCKI3DPROC)				wglGetProcAddress("wglIsEnabledGenlockI3D");
		wglGenlockSourceI3D				= (PFNWGLGENLOCKSOURCEI3DPROC)					wglGetProcAddress("wglGenlockSourceI3D");
		wglGetGenlockSourceI3D				= (PFNWGLGETGENLOCKSOURCEI3DPROC)				wglGetProcAddress("wglGetGenlockSourceI3D");		
		wglGenlockSourceEdgeI3D			= (PFNWGLGENLOCKSOURCEEDGEI3DPROC)				wglGetProcAddress("wglGenlockSourceEdgeI3D");		
		wglGetGenlockSourceEdgeI3D			= (PFNWGLGETGENLOCKSOURCEEDGEI3DPROC)			wglGetProcAddress("wglGetGenlockSourceEdgeI3D");
		wglGenlockSampleRateI3D			= (PFNWGLGENLOCKSAMPLERATEI3DPROC)				wglGetProcAddress("wglGenlockSampleRateI3D");		
		wglGetGenlockSampleRateI3D			= (PFNWGLGETGENLOCKSAMPLERATEI3DPROC)			wglGetProcAddress("wglGetGenlockSampleRateI3D");		
		wglGenlockSourceDelayI3D			= (PFNWGLGENLOCKSOURCEDELAYI3DPROC)				wglGetProcAddress("wglGenlockSourceDelayI3D");		
		wglGetGenlockSourceDelayI3D		= (PFNWGLGETGENLOCKSOURCEDELAYI3DPROC)			wglGetProcAddress("wglGetGenlockSourceDelayI3D");	
		wglQueryGenlockMaxSourceDelayI3D	= (PFNWGLQUERYGENLOCKMAXSOURCEDELAYI3DPROC)		wglGetProcAddress("wglQueryGenlockMaxSourceDelayI3D");

		if (wglEnableGenlockI3D &&		
			wglDisableGenlockI3D			 &&
			wglIsEnabledGenlockI3D			 &&
			wglGenlockSourceI3D			 &&
			wglGetGenlockSourceI3D			 &&
			wglGenlockSourceEdgeI3D		 &&
			wglGetGenlockSourceEdgeI3D		 &&
			wglGenlockSampleRateI3D		 &&
			wglGetGenlockSampleRateI3D		 &&
			wglGenlockSourceDelayI3D		 &&
			wglGetGenlockSourceDelayI3D	 &&
			wglQueryGenlockMaxSourceDelayI3D) {
				GLuint framelockEnabled = 0;

				if (wglEnableGenlockI3D(hDC)) {
					if (wglIsEnabledGenlockI3D(hDC, (BOOL *) &framelockEnabled)) {
						if (framelockEnabled) {
							return GL_TRUE;
						}
					}
				}
		}
		printf("framelock_demo: error while initializing WGL_I3D_genlock.\n");
	} else {
		printf("framelock_demo: WGL_I3D_genlock not exported in this OpenGL implementation.\n");
	}
	return GL_FALSE;
}

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

		if (  wglJoinSwapGroupNV     &&
			wglBindSwapBarrierNV   &&
			wglQuerySwapGroupNV    &&
			wglQueryMaxSwapGroupNV &&
			wglQueryFrameCountNV   &&
			wglResetFrameCountNV      ) {

				if (wglQueryMaxSwapGroupNV(hDC, &maxSwapGroups, &maxSwapBarriers)) {
					if (maxSwapGroups >= 1) {
						if(!wglJoinSwapGroupNV(hDC, 1)) {
							printf("framelock_demo: error in wglJoinSwapGroupNV.\n");
							return FALSE;
						}
						swapGroup = 1;
					}
					return GL_TRUE;
				}
		}
		printf("framelock_demo: error while initializing WGL_NV_swap_group.\n");
	} else {
		printf("framelock_demo: WGL_NV_swap_group not exported in this OpenGL implementation.\n");
	}

	return GL_FALSE;
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
			isMasterDevice = TRUE;
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

void initWGLExtensions() 
{
	wglGetExtensionsStringARB=(PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");
	if (!wglGetExtensionsStringARB) {
		exit(-1);
	}

	if (appMode != FRAMELOCK_SINGLE) {
		if (useI3D) {
			hasGenLockI3D       = initGenLockI3D();
		}
		hasSwapGroupNV      = initSwapGroupNV();
		/* swap barrier is disabled by default, may be enabled per cmdline */
		if (paramSwapBarrier) {
			hasSwapBarrierNV    = initSwapBarrierNV();
		}
		hasFrameCountNV     = initFrameCountNV();
	}
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
	/* disable genlock state if initialized */
	if (hasGenLockI3D) {
		wglDisableGenlockI3D(hDC);
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




int
main(int argc, char **argv)
{
	int i;
	char windowTitle[80] = "";
	for (i=1; i<argc; i++) {
		if (!strcmp(argv[i], "-master"))
		{
			appMode = FRAMELOCK_MASTER;


		}
		if (!strcmp(argv[i], "-slave"))
		{
			// don't override the master
			if (!appMode == FRAMELOCK_MASTER) 
			{
				appMode = FRAMELOCK_SLAVE;
			}
		}
		if (!strcmp(argv[i], "-barrier")) {
			paramSwapBarrier = GL_TRUE;
		}
		if (!strcmp(argv[i], "-i3d")) {
			useI3D = GL_TRUE;
		}
		if (!strcmp(argv[i], "-framecount")) {
			displayCounter = GL_TRUE;
		}
		if (!strcmp(argv[i], "-stereo")) {
			useStereo = GL_TRUE;
		}

		if (!strcmp(argv[i], "-X")) {
			xres=atoi(argv[i+1]);
		}
		if (!strcmp(argv[i], "-Y")) {
			yres=atoi(argv[i+1]);
		}
		if (!strcmp(argv[i], "-XStart")) {
			xpos=atoi(argv[i+1]);
		}
		if (!strcmp(argv[i], "-YStart")) {
			ypos=atoi(argv[i+1]);
		}
		if (!strcmp(argv[i], "-turbo")) 
		{
			printf("Turbo Speed up 16x\n");
			turbo=16;
		}
		if (!strcmp(argv[i], "-useIPFrameCount")) {
			useIPFrameCount=1;
		}
		if (!strcmp(argv[i], "-YStart")) {
			ypos=atoi(argv[i+1]);
		}
		if (!strcmp(argv[i], "-slaveHosts")) 
		{
			numSlave=atoi(argv[i+1]);
			printf("Nuber of Slaves %d\n",numSlave);

			for (unsigned int n=0; n<numSlave;n++)
			{
				sprintf(slaveHost[n],"%s",argv[i+2+n]);
				printf("Slave %d: %s\n",n, slaveHost[n]);

			}
		}


	}

	if(useIPFrameCount==1)
	{
		if (appMode == FRAMELOCK_MASTER) 
		{
			initWinsock();
			for (unsigned int i=0; i<numSlave;i++)
			{
				startClient(i,slaveHost[i],4343);
				sendToClientUint(i,i+3);
			}




		}
		else
		{
			initWinsock();
			startServer(4343);
			unsigned int tint;
			recvFromServerUInt(&tint);
			printf("Got MasterStartData %d\n", tint);
		}
	}




	windowWidth=xres;
	windowHeight=yres;
	glutInitWindowSize(windowWidth, windowHeight);
	glutInit(&argc, argv);

	if (useStereo) {
		glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_STEREO);
	} else {
		glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
	}
	switch (appMode) {
	case FRAMELOCK_SINGLE:
		sprintf(windowTitle, "NVIDIA FrameLock sample v1.0 (standalone process)");
		break;
	case FRAMELOCK_MASTER:
		sprintf(windowTitle, "NVIDIA FrameLock sample v1.0 (master process)");
		break;
	case FRAMELOCK_SLAVE:
		sprintf(windowTitle, "NVIDIA FrameLock sample v1.0 (slave process)");
		break;
	}




	glutCreateWindow("NVIDIA FrameLock");
	glutSetIconTitle(windowTitle);
	hWndMain = FindWindow(NULL, windowTitle);

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);

	hDC = GetDC(hWndMain);
	initWGLExtensions();

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
	glutAddMenuEntry("Display framelock state",           M_DISPLAY_STATE);
	glutAddMenuEntry("Display help on keyboard commands", M_DISPLAY_HELP);
	glutAddMenuEntry("About NVIDIA FrameLock",            M_ABOUT);
	glutAttachMenu(GLUT_RIGHT_BUTTON);


	//MIC
	winstyle = GetWindowLong( hWndMain, GWL_STYLE );
	winstyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
	SetWindowLong(hWndMain, GWL_STYLE, winstyle);

	//winstyle = GetWindowLong( hWndMain, GWL_EXSTYLE );
	//winstyle |= WS_EX_TOPMOST;
	//SetWindowLong(hWndMain, GWL_EXSTYLE, winstyle);

	SetForegroundWindow(hWndMain);




	SetWindowPos(hWndMain, NULL, xpos, ypos, xres, yres, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	glutIdleFunc(animate);

	glutMainLoop();

	exitWGLExtensions();

	return 0;
}
