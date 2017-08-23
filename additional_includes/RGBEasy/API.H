/*******************************************************************************

Copyright Datapath Ltd. 1997, 2006.

File:    api.h

Purpose: Declarations of functions to help with loading DLLs and obtaining
         pointers to functions within those DLLs.

History:
         20 MAY 97    JH   Created.
         30 MAR 98    JH   Made C++ compatible.
         05 DEC 01    JH   Added APILoadLibrary, APIFreeLibrary and
                           APILoadInterface.
         06 MAY 06    JH   Ported to Unicode.

*******************************************************************************/

#define API_ERROR_BASE                                   0x00050000
#define API_ERROR_UNABLE_TO_OPEN_KEY                     ( API_ERROR_BASE + 0 )
#define API_ERROR_UNABLE_TO_READ_VALUE                   ( API_ERROR_BASE + 1 )
#define API_ERROR_UNABLE_TO_LOAD_DLL                     ( API_ERROR_BASE + 2 )
#define API_ERROR_INCOMPATIBLE_API                       ( API_ERROR_BASE + 3 )

/******************************************************************************/

#ifndef FAR
#define FAR
#endif

#if ( defined linux )
#elif ( defined WIN32 )
   #if (( defined KERNEL_MODE ) || ( defined _UNICODE ) || ( defined UNICODE ))
      typedef wchar_t   APICHAR;
   #else
      typedef char      APICHAR;
   #endif
#else /* WIN16 */
   typedef char         APICHAR;
#endif
typedef APICHAR   FAR *LPAPICHAR;
typedef APICHAR   const FAR *LPCAPICHAR;

typedef void (APIFN) ( void );

typedef struct
{
#if 1
   FARPROC  *PFnPtr;
   LPCSTR   FnName;
#else
   APIFN    *PFnPtr;
   char     *FnName;
#endif
}  APIFNENTRY, FAR *LPAPIFNENTRY, FAR *LPAPIFNLIST;

typedef void *HAPILIB;

#ifdef __cplusplus
extern "C"
{
#endif

#if 1
extern unsigned long
APILoadLibrary (
   LPCAPICHAR  pDriverName,
   HINSTANCE   FAR *pHInstance );

extern unsigned long
APIFreeLibrary (
   HINSTANCE   hInstance );

extern int
APILoadFunctions (
   HINSTANCE   hInstance,
   LPAPIFNLIST lpFnList,
   void        *pReserved );

extern int
APILoadFunctionsEx (
   HINSTANCE   hInstance,
   LPAPIFNLIST lpFnList,
   BOOL        bFailOnLoad );

extern unsigned long
APILoadInterface (
   LPCAPICHAR  lpSectionName,
   LPCAPICHAR  lpValueName,
   LPAPIFNLIST lpFnList,
   HINSTANCE   *lpHInstance );
#else
APILoadLibrary (
   LPCAPICHAR  pDriverName,
   HAPILIB     FAR *pHInstance );

extern unsigned long
APIFreeLibrary (
   HAPILIB     hInstance );

extern int
APILoadFunctions (
   HAPILIB     hInstance,
   LPAPIFNLIST lpFnList,
   void        *pReserved );

extern unsigned long
APILoadInterface (
   LPCAPICHAR  lpSectionName,
   LPCAPICHAR  lpValueName,
   LPAPIFNLIST lpFnList,
   HAPILIB     *lpHInstance );
#endif

#ifdef __cplusplus
}
#endif

/******************************************************************************/
