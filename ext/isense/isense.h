//////////////////////////////////////////////////////////////////////////////
//
//      File Name:      isense.h
//      Description:    Header File for InterSense library
//      Created:        12/4/98
//      Author:         Yury Altshuler
//
//      Copyright:      InterSense 2012 - All rights Reserved.
//
//      Comments:       This dynamic link library (DLL)/shared library is provided to simplify 
//                      communications with all models of InterSense tracking devices. 
//                      Currently, these include the InertiaCube series, other OEM sensors,
//						the IS-900, IS-1200, and NavChip.  Older sensors including the IS-300,
//						IS-600, and InterTrax are expected to work, but are no longer supported
//						by InterSense. It can detect, configure, and get data from up to 8 trackers.
//
//                      The library provides two methods of configuring the Precision Series trackers.
//                      You can use the provided function calls and hardcode the 
//                      settings in your application, or use the isenseX.ini files. 
//                      Second method is strongly recommended, as it provides you with the 
//                      ability to change the configuration without recompiling your 
//                      application.  In the isenseX.ini file name, X is a number, starting 
//                      at 1, identifying the tracking system in the order of 
//                      initialization. Use included the included isenseX.ini file as an example. 
//                      Only enter the settings you want the program to change.
//
//                      The InterTrax (unsupported) requires no configuration. All function calls and 
//                      settings in the isenseX.ini files are ignored.
//
//                       
//////////////////////////////////////////////////////////////////////////////
#ifndef _ISD_isenseh
#define _ISD_isenseh

#define ISLIB_VERSION "4.237"

#if defined(_WIN32) || defined(WIN32) || defined(__WIN32__)
    #include <windows.h>
    #include "types.h"
#else
    #ifndef UNIX
        #define UNIX
    #endif
    #include "types.h"
#endif

#ifndef TRUE
    #define TRUE    1
    #define FALSE   0
#endif

//////////////////////////////////////////////////////////////////////////////
//
// This defines the calling conventions of the entry points to the
// library. This varies depending upon the operating system.
//
//////////////////////////////////////////////////////////////////////////////

#if defined _LIB // Static library
    #define	DLLEXPORT 
    #define	DLLENTRY __cdecl
    typedef void (* DLL_EP)(void);
    #define	DLL_EP_PTR __cdecl *
#else

#if defined UNDER_RTSS
    #define	DLLEXPORT __declspec(dllexport)
    #define	DLLENTRY _stdcall
    typedef void (* DLL_EP)(void);
    #define	DLL_EP_PTR _stdcall *
#elif defined(_WIN32) || defined(WIN32) || defined(__WIN32__)
    #define	DLLEXPORT __declspec(dllexport)
    #define	DLLENTRY __cdecl
    typedef void (* DLL_EP)(void);
    #define	DLL_EP_PTR __cdecl *
#else
    #define	DLLEXPORT
    #define	DLLENTRY
    typedef void (* DLL_EP)(void);
    #define	DLL_EP_PTR *
#endif
#endif

//////////////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
extern "C"
{
#endif


// tracking system type
typedef enum
{
    ISD_NONE = 0,           // Not found, or unable to identify 
    ISD_PRECISION_SERIES,   // InertiaCubes, NavChip, IS-300, IS-600, IS-900 and IS-1200 
    ISD_INTERTRAX_SERIES    // InterTrax 
}
ISD_SYSTEM_TYPE;


// tracking system model 
typedef enum
{
    ISD_UNKNOWN = 0,          
    ISD_IS300,          // 3DOF system (unsupported)
    ISD_IS600,          // 6DOF system (unsupported) 
    ISD_IS900,          // 6DOF system   
    ISD_INTERTRAX,      // InterTrax (Serial) (unsupported)
    ISD_INTERTRAX_2,    // InterTrax (USB) (unsupported)
    ISD_INTERTRAX_LS,   // InterTraxLS, verification required (unsupported)
    ISD_INTERTRAX_LC,   // InterTraxLC (unsupported)
    ISD_ICUBE2,         // InertiaCube2 
    ISD_ICUBE2_PRO,     // InertiaCube2 Pro 
    ISD_IS1200,         // 6DOF system   
    ISD_ICUBE3,         // InertiaCube3 
    ISD_NAVCHIP,        // NavChip
    ISD_INTERTRAX_3,    // InterTrax3 (unsupported)
    ISD_IMUK,           // K-Sensor
    ISD_ICUBE2B_PRO,    // InertiaCube2B Pro
    ISD_ICUBE2_PLUS,    // InertiaCube2 Plus
    ISD_ICUBE_BT        // InertiaCube BT
}
ISD_SYSTEM_MODEL;


typedef enum
{
    ISD_INTERFACE_UNKNOWN = 0,
    ISD_INTERFACE_SERIAL,
    ISD_INTERFACE_USB,
    ISD_INTERFACE_ETHERNET_UDP,
    ISD_INTERFACE_ETHERNET_TCP,
    ISD_INTERFACE_IOCARD,
    ISD_INTERFACE_PCMCIA,
    ISD_INTERFACE_FILE,
    ISD_INTERFACE_PIPE
}
ISD_INTERFACE_TYPE;


#if defined ISENSE_LIMITED
#define ISD_MAX_TRACKERS        2	// Maximum stations (size of array for ISD_OpenAllTrackers()) 
#define ISD_MAX_STATIONS        4	// Maximum stations (for IS-900, IS-1200)
#else
#define ISD_MAX_TRACKERS        32  // Maximum trackers (size of array for ISD_OpenAllTrackers())
#define ISD_MAX_STATIONS        8	// Maximum stations (for IS-900, IS-1200)
#endif

// Orientation format (3DOF sensors report both regardless of setting; for IS-900, IS-1200)
#define ISD_EULER               1
#define ISD_QUATERNION          2

// Coordinate frame in which position and orientation data is reported 
#define ISD_DEFAULT_FRAME       1    // InterSense default 
#define ISD_VSET_FRAME          2    // Virtual set frame, use for camera tracker only 

// Number of supported stylus buttons 
#define ISD_MAX_BUTTONS         8

// Hardware is limited to 10 analog/digital input channels per station 
#define ISD_MAX_CHANNELS        10

// Maximum supported number of bytes for auxiliary input data
#define ISD_MAX_AUX_INPUTS      4

// Maximum supported number of bytes for auxiliary output data
#define ISD_MAX_AUX_OUTPUTS     4

typedef int ISD_TRACKER_HANDLE;


///////////////////////////////////////////////////////////////////////////////

typedef struct
{
    // Following item are for information only and should not be changed 
    float  LibVersion;     // InterSense Library version 

    DWORD  TrackerType;    // IS Precision series or InterTrax. 
                           // TrackerType can be: 
                           // ISD_PRECISION_SERIES for InertiaCube, NavChip, IS-300, IS-600, IS-900 and IS-1200 model trackers, 
                           // ISD_INTERTRAX_SERIES for InterTrax, or 
                           // ISD_NONE if tracker is not initialized 

    DWORD  TrackerModel;   // ISD_UNKNOWN, ISD_IS300, ISD_IS600, ISD_IS900, ISD_INTERTRAX 
    
    DWORD  Port;           // Number of the RS232 port. Starts with 1 (COM1/ttyS0). 

    // Communications statistics, for information only. 
    DWORD  RecordsPerSec;
    float  KBitsPerSec;    

    // Following items are used to configure the tracker and can be set in
    // the isenseX.ini file 

    DWORD  SyncState;   // 4 states: 0 - OFF, system is in free run 
                        //           1 - ON, hardware genlock frequency is automatically determined
                        //           2 - ON, hardware genlock frequency is specified by the user
                        //           3 - ON, no hardware signal, lock to the user specified frequency  

    float  SyncRate;    // Sync frequency - number of hardware sync signals per second, 
                        // or, if SyncState is 3 - data record output frequency 

    DWORD  SyncPhase;   // 0 to 100%    

    DWORD  Interface;   // Hardware interface, read-only 

    DWORD  UltTimeout;  // IS-900 only, ultrasonic timeout (sampling rate)
    DWORD  UltVolume;   // IS-900 only, ultrasonic speaker volume
    DWORD  dwReserved4;

    float  FirmwareRev; // Firmware revision 
    float  fReserved2;
    float  fReserved3;
    float  fReserved4;

    Bool   LedEnable;   // IS-900 only, enables flasing blue LEDs on SoniStrips/SoniDiscs if TRUE
    Bool   bReserved2;
    Bool   bReserved3;
    Bool   bReserved4;
}
ISD_TRACKER_INFO_TYPE;



///////////////////////////////////////////////////////////////////////////////

// ISD_STATION_INFO_TYPE can only be used with IS Precision Series tracking devices.
// If passed to ISD_SetStationConfig or ISD_GetStationConfig with InterTrax, FALSE is returned. 

typedef struct
{
    DWORD   ID;                 // Unique number identifying a station. It is the same as that 
                                // passed to the ISD_SetStationConfig and ISD_GetStationConfig   
                                // functions and can be 1 to ISD_MAX_STATIONS 

    Bool    State;              // TRUE if ON, FALSE if OFF 

    Bool    Compass;            // 0 (OFF) or 2 (ON).  Setting 1 is no longer used and will
                                // have the same result as 2 (ON).
                                // Compass setting is ignored if station is configured for 
                                // Fusion Mode operation (such as an IS-900 tracker). 

    LONG    InertiaCube;        // InertiaCube associated with this station (IS-600). If no InertiaCube is
                                // assigned, this number is -1. Otherwise, it is a positive number
                                // 1 to 4 

    DWORD   Enhancement;        // Perceptual enhancement; levels 0, 1, or 2 
    DWORD   Sensitivity;        // Sensitivity; levels 1 to 4 (only used with Enhancement = 1 or 2) 
    DWORD   Prediction;         // 0 to 50 msec
    DWORD   AngleFormat;        // ISD_EULER or ISD_QUATERNION (only needed for IS-900, IS-1200)
   
    Bool    TimeStamped;        // TRUE if time stamp is requested 
    Bool    GetInputs;          // TRUE if button and joystick data is requested 
    Bool    GetEncoderData;     // TRUE if raw encoder data is requested 

    // This setting controls how Magnetic Environment Calibration is applied. This calibration
    // calculates nominal field strength and dip angle for the environment in which the sensor
    // is used. Based on these values, the system can assign a weight to compass measurements,
    // allowing it to reject bad measurements. Values from 0 to 3 are accepted.
    //   If CompassCompensation is set to 0, the calibration is ignored and all compass data is
    // used. Higher values result in a tighter rejection threshold, resulting in more measurements 
    // being rejected. If the sensor is used in an environment with significant magnetic interference
    // this can result in drift due to insufficient compensation from the compass data. Default
    // setting is 2.
	//   Note that the sensor must be calibrated in the ISDemo Compass Calibration Tool for this 
	// setting to have any effect.
    // -----------------------------------------------------------------------------------------
    BYTE    CompassCompensation;     

    // This setting controls how the system deals with sharp changes in IMU data that can
    // be caused by shock or impact. Sensors may experience momentary rotation rates or
    // accelerations that are outside of the specified range, resulting in undesirable 
    // behavior. By turning on shock suppression you can have the system filter out
    // corrupted data. Values 0 (OFF) to 2 are accepted, with higher values resulting in
    // greater filtering.
    // -----------------------------------------------------------------------------------------
    BYTE    ImuShockSuppression;

    // This setting controls the rejection threshold for ultrasonic measurements. Currently, it is
    // implemented only for the IS-900 PCTracker. Default setting is 4, which results in measurements
    // with range errors greater than 4 times the average to be rejected. Please do not change
    // this setting without first consulting with InterSense technical support.
    // -----------------------------------------------------------------------------------------
    BYTE    UrmRejectionFactor;

    BYTE    bReserved2;

    DWORD   CoordFrame;         // Coordinate frame in which position and orientation data is reported  

    // AccelSensitivity is used for 3-DOF tracking with InertiaCube products only. It controls how 
    // fast tilt correction, using accelerometers, is applied. Valid values are 1 to 4, with 2 as default. 
    //    Level 1 reduces the amount of tilt correction during movement. While it will prevent any effect  
    // linear accelerations may have on pitch and roll, it will also reduce stability and dynamic accuracy. 
    // It should only be used in situations when sensor is not expected to experience a lot of movement.
    //    Level 2 (default) is best for head tracking in static environment, with user seated. 
	//    Level 3 allows for more aggressive tilt compensation, appropriate when sensor is moved a lot, 
    // for example, when the user is walking for long periods of time. 
    //    Level 4 allows for even greater tilt corrections. It will reduce orientation accuracy by 
    // allowing linear accelerations to effect orientation, but increase stability. This level 
    // is appropriate for when the user is running, or in other situations where the sensor experiences 
    // a great deal of movement. 
    // -----------------------------------------------------------------------------------------
    DWORD   AccelSensitivity; 

    float   fReserved1;    
    float   fReserved2;

    float   TipOffset[3];       // Coordinates in station frame (relative to tracker origin) of the point being tracked
    float   fReserved3;

    Bool    GetCameraData;      // TRUE to get computed FOV, aperture, etc  
    Bool    GetAuxInputs;     
    Bool    GetCovarianceData;
    Bool    GetExtendedData;    // Retrieving extended data will reduce update rate with even a single tracker
                                // when using serial communications; Ethernet is highly recommended when retrieving
                                // extended data
}
ISD_STATION_INFO_TYPE;


///////////////////////////////////////////////////////////////////////////////

typedef struct
{
    BYTE    TrackingStatus;     // Tracking status, represents "Tracking Quality" (0-255; 0 if lost)
    BYTE    NewData;            // TRUE if data changed since last call to ISD_GetTrackingData       
    BYTE    CommIntegrity;      // Communication integrity (percentage of packets received from tracker, 0-100) 
    BYTE    BatteryState;       // Wireless devices only 0=N/A, 1=Low, 2=OK

    float   Euler[3];           // Orientation in Euler angle format (Yaw, Pitch, Roll)
    float   Quaternion[4];      // Orientation in Quaternion format (W,X,Y,Z)
    float   Position[3];        // Always in meters 
    float   TimeStamp;          // Timestamp in seconds, reported only if requested 
    float   StillTime;          // InertiaCube and PC-Tracker products only, whether sensor is still
    float   BatteryLevel;       // Battery voltage, if available
    float   CompassYaw;         // Magnetometer heading, computed based on current orientation.
                                // Available for InertiaCube products only, such as IC2, IC3 and IC2+

    Bool    ButtonState[ISD_MAX_BUTTONS];    // Only if requested 

    // -----------------------------------------------------------------------------------------
    // Current hardware is limited to 10 channels, with only 2 being used. 
    // The only device using this is the IS-900 wand that has a built-in
    // analog joystick. Channel 1 is X-axis rotation, channel 2 is Y-axis
    // rotation 

    short   AnalogData[ISD_MAX_CHANNELS]; // Only if requested 

    BYTE    AuxInputs[ISD_MAX_AUX_INPUTS];


    float   AngularVelBodyFrame[3]; // rad/sec, in sensor body coordinate frame. 
                                    // Reported as rates about X, Y and Z axes, corresponding
                                    // to Roll, Pitch, Yaw order.
                                    // This is the processed angular rate, with current biases 
                                    // removed. This is the angular rate used to produce 
                                    // orientation updates.

    float   AngularVelNavFrame[3];  // rad/sec, in world coordinate frame, with boresight and other
                                    // transformations applied. 
                                    // Reported as rates about X, Y and Z axes, corresponding
                                    // to Roll, Pitch, Yaw order.
    
    float   AccelBodyFrame[3];      // meter/sec^2, in sensor body coordinate frame. These are 
                                    // the accelerometer measurements in the sensor body coordinate 
                                    // frame. Only factory calibration is applied to this data, 
                                    // gravity component is not removed.    
                                    // Reported as accelerations along X, Y and Z axes.

    float   AccelNavFrame[3];       // meters/sec^2, in the navigation (earth) coordinate frame. 
                                    // This is the accelerometer measurements with calibration,  
                                    // current sensor orientation applied, and gravity  
                                    // subtracted. This is the best available estimate of
                                    // tracker acceleration.
                                    // Reported as accelerations along X, Y and Z axes.

    float   VelocityNavFrame[3];    // meters/sec, 6-DOF systems only.  
                                    // Reported as velocity along X, Y and Z axes.

    float   AngularVelRaw[3];       // Raw gyro output, only factory calibration is applied. 
                                    // Some errors due to temperature dependant gyro bias 
                                    // drift will remain.

    BYTE    MeasQuality;            // Ultrasonic Measurement Quality (IS-900 only, firmware >= 4.26)
    BYTE    bReserved2;
    BYTE    bReserved3;
    BYTE    bReserved4;
    
    DWORD   TimeStampSeconds;       // Time Stamp in whole seconds.
    DWORD   TimeStampMicroSec;      // Fractional part of the Time Stamp in micro-seconds.

    DWORD   OSTimeStampSeconds;     // Data record arrival time stamp based on OS time,
    DWORD   OSTimeStampMicroSec;    // reserved for future use, not implemented.

    float   Reserved[56]; 
    float   MagBodyFrame[3];		// 3DOF sensors only
 									// Magnetometer data along the X, Y, and Z axes
									// Units are nominally in Gauss, and factory calibration 
									// is applied.  Note, however, that most sensors are not 
                                    // calibrated precisely since the exact field strength is 
                                    // not necessary to for tracking purposes.  Relative 
                                    // magnitudes should be accurate, however.  Fixed metal 
                                    // compass calibration may rescale the values, as well       
}
ISD_STATION_DATA_TYPE;


///////////////////////////////////////////////////////////////////////////////

typedef struct
{
    BYTE    TrackingStatus;     // Tracking status byte 
    BYTE    bReserved1;         // Pack to 4 byte boundary 
    BYTE    bReserved2;
    BYTE    bReserved3;

    DWORD   Timecode;           // Timecode, not implemented yet 
    LONG    ApertureEncoder;    // Aperture encoder counts, relative to last reset or power up 
    LONG    FocusEncoder;       // Focus encoder counts 
    LONG    ZoomEncoder;        // Zoom encoded counts 
    DWORD   TimecodeUserBits;   // Time code user bits, not implemented yet 

    float   Aperture;           // Computed aperture value 
    float   Focus;              // Computed focus value (mm), not implemented yet 
    float   FOV;                // Computed vertical FOV value (degrees) 
    float   NodalPoint;         // Nodal point offset due to zoom and focus (mm) 

    float   CovarianceOrientation[3];     // Available only for IS-1200
    float   CovariancePosition[3];

    DWORD   dwReserved1;
    DWORD   dwReserved2;

    float   fReserved1;
    float   fReserved2;
    float   fReserved3;
    float   fReserved4;
}
ISD_CAMERA_ENCODER_DATA_TYPE;


typedef struct
{
    ISD_STATION_DATA_TYPE Station[ISD_MAX_STATIONS];
}
ISD_TRACKING_DATA_TYPE;


typedef struct
{
    ISD_CAMERA_ENCODER_DATA_TYPE Camera[ISD_MAX_STATIONS];
}
ISD_CAMERA_DATA_TYPE;


typedef enum
{
    ISD_AUX_SYSTEM_NONE = 0,
    ISD_AUX_SYSTEM_ULTRASONIC,
    ISD_AUX_SYSTEM_OPTICAL,
    ISD_AUX_SYSTEM_MAGNETIC,
    ISD_AUX_SYSTEM_RF,
    ISD_AUX_SYSTEM_GPS
}
ISD_AUX_SYSTEM_TYPE;


typedef struct
{
    Bool    Valid;            // Set to TRUE if ISD_GetSystemHardwareInfo succeeded

    DWORD   TrackerType;      // See ISD_SYSTEM_TYPE
    DWORD   TrackerModel;     // See ISD_SYSTEM_MODEL
    DWORD   Port;             // Hardware port number (1 for COM1/ttyS0, etc.)
    DWORD   Interface;        // Hardware interface (RS232, USB, etc.)
    Bool    OnHost;           // TRUE if tracking algorithms are executed in the library
    DWORD   AuxSystem;        // Position tracking hardware, see ISD_AUX_SYSTEM_TYPE
    float   FirmwareRev;      // Firmware revision 
    
    char    ModelName[128];

    struct
    {
        Bool    Position;     // Can track position
        Bool    Orientation;  // Can track orientation
        Bool    Encoders;     // Can support lens encoders
        Bool    Prediction;   // Predictive algorithms are available
        Bool    Enhancement;  // Enhancement level can be changed
        Bool    Compass;      // Compass setting can be changed
        Bool    SelfTest;     // Has the self-test capability
        Bool    ErrorLog;     // Can keep error log

        Bool    UltVolume;    // Can control ultrasonic volume via software
        Bool    UltGain;      // Can control microphone sensitivity by software
        Bool    UltTimeout;   // Can change ultrasonic sampling frequency
        Bool    PhotoDiode;   // SoniDiscs support photodiode

        DWORD   MaxStations;  // Number of supported stations
        DWORD   MaxImus;      // Number of supported IMUs
        DWORD   MaxFPses;     // Maximum number of Fixed Position Sensing Elements (constellation/galaxy)
        DWORD   MaxChannels;  // Maximum number of analog channels supported per station
        DWORD   MaxButtons;   // Maximum number of digital button inputs per station

        Bool    MeasData;     // Can provide measurement data
        Bool    DiagData;     // Can provide diagnostic data
        Bool    PseConfig;    // Supports PSE configuration/reporting tools
        Bool    ConfigLock;   // Supports configuration locking     
        
        float   UltMaxRange;  // Maximum ultrasonic range  
        float   fReserved2;
        float   fReserved3;
        float   fReserved4;
        
        Bool    CompassCal;   // Supports dynamic compass calibration     
        Bool    bReserved2;
        Bool    bReserved3;     
        Bool    bReserved4;

        DWORD   dwReserved1;        
        DWORD   dwReserved2;       
        DWORD   dwReserved3;     
        DWORD   dwReserved4;
    }
    Capability;
        
    Bool    bReserved1;
    Bool    bReserved2;
    Bool    bReserved3;     
    Bool    bReserved4;

    DWORD   BaudRate;           // Serial port baud rate      
    DWORD   NumTestLevels;      // Number of self test levels       
    DWORD   dwReserved3;     
    DWORD   dwReserved4;

    float   fReserved1;  
    float   fReserved2;
    float   fReserved3;
    float   fReserved4;

    char    cReserved1[128];    
    char    cReserved2[128];    
    char    cReserved3[128];    
    char    cReserved4[128];    
}
ISD_HARDWARE_INFO_TYPE;


///////////////////////////////////////////////////////////////////////////////

// Station hardware information.
// This structure provides detailed information on station hardware and
// its capabilities.

typedef struct
{
    Bool    Valid;             // Set to TRUE if ISD_GetStationHardwareInfo succeeded

    DWORD   ID;                // Unique number identifying a station. It is the same as that 
                               // passed to the ISD_SetStationConfig and ISD_GetStationConfig   
                               // functions and can be 1 to ISD_MAX_STATIONS 

    char    DescVersion[20];   // Station Descriptor version 

    float   FirmwareRev;       // Station firmware revision
    DWORD   SerialNum;         // Station serial number 
    char    CalDate[20];       // Last factory calibration date (mm/dd/yyyy)
    DWORD   Port;              // Hardware port number 
    
    struct
    {
        Bool    Position;      // TRUE if station can track position
        Bool    Orientation;   // TRUE if station can track orientation
        DWORD   Encoders;      // Number of lens encoders, if 0 then none are available
        DWORD   NumChannels;   // Number of analog channels supported by this station, wand has 2 (joystick axes)
        DWORD   NumButtons;    // Number of digital button inputs supported by this station
        DWORD   AuxInputs;     // Number of auxiliary input channels (OEM products)
        DWORD   AuxOutputs;    // Number of auxiliary output channels (OEM products)
        Bool    Compass;       // TRUE if station has a compass

        Bool    bReserved1;     
        Bool    bReserved2;
        Bool    bReserved3;     
        Bool    bReserved4;

        DWORD   dwReserved1;        
        DWORD   dwReserved2;       
        DWORD   dwReserved3;     
        DWORD   dwReserved4;
    }
    Capability;

    Bool    bReserved1;
    Bool    bReserved2;
    Bool    bReserved3;     
    Bool    bReserved4;

    DWORD   Type;           // Station type        
    DWORD   DeviceID;       
    DWORD   dwReserved3;     
    DWORD   dwReserved4;

    float   fReserved1;  
    float   fReserved2;
    float   fReserved3;
    float   fReserved4;

    char    cReserved1[128];    
    char    cReserved2[128];    
    char    cReserved3[128];    
    char    cReserved4[128];    
}
ISD_STATION_HARDWARE_INFO_TYPE;


typedef struct
{
    Bool valid;

    LONG status;
    Bool wireless;
    DWORD channel;
    DWORD id[4];
    DWORD radioVersion;

    DWORD    dReserved1;
    DWORD    dReserved2;
    DWORD    dReserved3;     
    DWORD    dReserved4;
}
ISD_PORT_WIRELESS_INFO_TYPE;


// Returns -1 on failure. To detect one tracker automatically, specify 0 for commPort.
// hParent parameter to ISD_OpenTracker is optional and should only be used if 
// information screen or tracker configuration tools are to be used when available 
// in a future releases. If you would like a tracker initialization window to be 
// displayed, specify TRUE for the infoScreen parameter (not implemented in
// this release).  Call multiple times to detect multiple trackers (but also please
// see comments for ISD_OpenAllTrackers below).  
//   Note that wireless 3DOF sensors such as the wireless IC3 are considered trackers,
// not stations (even if using a shared receiver), so each tracker must have a 
// separate handle associated with it.  A station is a concept associated with the 
// IS-900 and IS-1200 systems.
// ----------------------------------------------------------------------------
DLLEXPORT ISD_TRACKER_HANDLE DLLENTRY ISD_OpenTracker( 
                                                      Hwnd hParent, 
                                                      DWORD commPort, 
                                                      Bool infoScreen, 
                                                      Bool verbose 
                                                      );

// Returns -1 on failure.  This is the recommended method for opening multiple 
// trackers.  The handle pointer will be populated with handles for all detected
// trackers, and should point to an ISD_TRACKER_HANDLE array of size ISD_MAX_TRACKERS
DLLEXPORT DWORD DLLENTRY ISD_OpenAllTrackers( 
                                             Hwnd hParent, 
                                             ISD_TRACKER_HANDLE *handle, 
                                             Bool infoScreen, 
                                             Bool verbose 
                                             );

// This function call uninitializes a tracker, closes communications port and 
// frees the resources associated with the tracker handle. If 0 is passed, all currently
// open trackers are closed. When the last tracker is closed, program frees the library. 
// ----------------------------------------------------------------------------
DLLEXPORT Bool DLLENTRY ISD_CloseTracker( ISD_TRACKER_HANDLE handle );


// Get general tracker information, such as type, model, port, etc.
// Also retrieves genlock synchronization configuration, if available. 
// See the ISD_TRACKER_INFO_TYPE structure definition above for a complete list
// ----------------------------------------------------------------------------
DLLEXPORT Bool DLLENTRY ISD_GetTrackerConfig( 
                                             ISD_TRACKER_HANDLE handle, 
                                             ISD_TRACKER_INFO_TYPE *Tracker, 
                                             Bool verbose 
                                             );


// When used with IS Precision Series (IS-300, IS-600, IS-900, IS-1200) tracking devices 
// this function call will set genlock synchronization parameters; all other fields 
// in the ISD_TRACKER_INFO_TYPE structure are for information purposes only 
// ----------------------------------------------------------------------------
DLLEXPORT Bool DLLENTRY ISD_SetTrackerConfig(
                                             ISD_TRACKER_HANDLE handle, 
                                             ISD_TRACKER_INFO_TYPE *Tracker, 
                                             Bool verbose
                                             );


// Get RecordsPerSec and KBitsPerSec without requesting genlock settings from the tracker.
// Use this instead of ISD_GetTrackerConfig to prevent your program from stalling while
// waiting for the tracker response. 
// ----------------------------------------------------------------------------
DLLEXPORT Bool DLLENTRY ISD_GetCommInfo( 
                                        ISD_TRACKER_HANDLE handle, 
                                        ISD_TRACKER_INFO_TYPE *Tracker
                                        );


// Configure station as specified in the ISD_STATION_INFO_TYPE structure. Before 
// this function is called, all elements of the structure must be assigned a value. 
// stationID is a number from 1 to ISD_MAX_STATIONS. Should only be used with
// IS Precision Series tracking devices, not valid for InterTrax.  
// ----------------------------------------------------------------------------
DLLEXPORT Bool DLLENTRY ISD_SetStationConfig( 
                                             ISD_TRACKER_HANDLE handle, 
                                             ISD_STATION_INFO_TYPE *Station, 
                                             WORD stationID,
                                             Bool verbose 
                                             );


// Fills the ISD_STATION_INFO_TYPE structure with current settings. Function
// requests configuration records from the tracker and waits for the response.
// If communications are interrupted, it will stall for several seconds while 
// attempting to recover the settings. Should only be used with IS Precision Series 
// tracking devices, not valid for InterTrax.
// stationID is a number from 1 to ISD_MAX_STATIONS 
// ----------------------------------------------------------------------------
DLLEXPORT Bool DLLENTRY ISD_GetStationConfig(
                                             ISD_TRACKER_HANDLE handle, 
                                             ISD_STATION_INFO_TYPE *Station,
                                             WORD stationID, 
                                             Bool verbose 
                                             );

// When a tracker is first opened, library automatically looks for a configuration
// file in current directory of the application. File name convention is
// isenseX.ini where X is a number, starting at 1, identifying the first tracking 
// system in the order of initialization. This function provides for a way to
// manually configure the tracker using an arbitrary configuration file instead.
// ----------------------------------------------------------------------------
DLLEXPORT Bool ISD_ConfigureFromFile(
                                     ISD_TRACKER_HANDLE handle, 
                                     char *path, 
                                     Bool verbose 
                                     );


// Save tracker configuration. For devices with on-host processing,
// like the IS-900 PCTracker, this will write to the isenseX.cfg file. 
// Serial port devices like IS-300, IS-600 and IS-900 save configuration 
// in the base unit, and this call will just send a command to commit the
// changes to permanent storage.

DLLEXPORT Bool ISD_ConfigSave( ISD_TRACKER_HANDLE handle );


// Get data from all configured stations. Data is placed in the ISD_TRACKING_DATA_TYPE
// structure. 
// ----------------------------------------------------------------------------
DLLEXPORT Bool DLLENTRY ISD_GetTrackingData( 
                                    ISD_TRACKER_HANDLE handle, 
                                    ISD_TRACKING_DATA_TYPE *Data 
                                    );


// Get data from all configured stations. Data is placed in the ISD_TRACKING_DATA_TYPE
// structure. 
// ----------------------------------------------------------------------------
DLLEXPORT Bool DLLENTRY ISD_GetTrackingDataAtTime( 
                                    ISD_TRACKER_HANDLE handle, 
                                    ISD_TRACKING_DATA_TYPE *Data,
                                    double atTime,
                                    double maxSyncWait
                                    );

// Get camera encode and other data for all configured stations. Data is placed 
// in the ISD_CAMERA_DATA_TYPE structure. This function does not service the serial 
// port, so ISD_GetTrackingData must be called prior to this. 
// ----------------------------------------------------------------------------
DLLEXPORT Bool DLLENTRY ISD_GetCameraData( 
                                          ISD_TRACKER_HANDLE handle, 
                                          ISD_CAMERA_DATA_TYPE *Data 
                                          );


// By default, ISD_GetTrackingData processes all records available from the tracker
// and only returns the latest data. As the result, data samples can be lost.
// If all the data samples are required, you can use a ring buffer to store them.
// ISD_RingBufferSetup accepts a pointer to the ring buffer, and its size.
// Once activated, all processed data samples are stored in the buffer for use
// by the application. 
//
// ISD_GetTrackingData can still be used to read the data, but will return the
// oldest saved data sample, then remove it from the buffer (first in - first out). 
// By repeatedly calling ISD_GetTrackingData, all samples are retrieved, the latest
// coming last. All consecutive calls to ISD_GetTrackingData will return the last
// sample, but the NewData flag will be FALSE to indicate that the buffer has
// been emptied.
// ----------------------------------------------------------------------------
DLLEXPORT Bool ISD_RingBufferSetup( 
                                   ISD_TRACKER_HANDLE handle, 
                                   WORD stationID, 
                                   ISD_STATION_DATA_TYPE *dataBuffer, 
                                   DWORD samples 
                                   );

// Activate the ring buffer. While active, all data samples are stored in the 
// buffer. Because this is a ring buffer, it will only store the number of samples
// specified in the call to ISD_RingBufferSetup, so the oldest samples can be 
// overwritten.
// ----------------------------------------------------------------------------
DLLEXPORT Bool ISD_RingBufferStart(
                                   ISD_TRACKER_HANDLE handle,
                                   WORD stationID
                                   );

// Stop collection. The library will continue to process data, but the contents of
// the ring buffer will not be altered.
// ----------------------------------------------------------------------------
DLLEXPORT Bool ISD_RingBufferStop(
                                  ISD_TRACKER_HANDLE handle,
                                  WORD stationID
                                  );


// Queries the library for the latest data without removing it from the buffer or 
// affecting the NewData flag. It also returns the indexes of the newest and the
// oldest samples in the buffer. These can then be used to parse the buffer.
// ----------------------------------------------------------------------------
DLLEXPORT Bool ISD_RingBufferQuery( 
                                   ISD_TRACKER_HANDLE handle, 
                                   WORD stationID,
                                   ISD_STATION_DATA_TYPE *currentData,
                                   DWORD *head,
                                   DWORD *tail
                                   );

// Reset heading (yaw) to zero 
// ----------------------------------------------------------------------------
DLLEXPORT Bool DLLENTRY ISD_ResetHeading( 
                                         ISD_TRACKER_HANDLE handle, 
                                         WORD stationID 
                                         );


// Works with all IS-X00 series products, InterTraxLC and InertiaCube products.
// For InterTrax30 and InterTrax2, it behaves like ISD_ResetHeading.
// Boresight a station using specific reference angles. This is useful when
// you need to apply a specific offset to system output. For example, if
// a sensor is mounted at 40 degrees pitch relative to an HMD, you can 
// pass in 0, 40, 0 to make the system output (0,0,0) when the HMD is horizontal.
// ----------------------------------------------------------------------------
DLLEXPORT Bool DLLENTRY ISD_BoresightReferenced( 
                                                ISD_TRACKER_HANDLE handle, 
                                                WORD stationID, 
                                                float yaw,
                                                float pitch, 
                                                float roll 
                                                );

// Works with all IS-X00 series products, InterTraxLC and InertiaCube products.
// For InterTrax30 and InterTrax2, it behaves like ISD_ResetHeading.
// Boresight or unboresight a station. If 'set' is TRUE, all angles
// are reset to zero. Otherwise, all boresight settings are cleared,
// including those set by ISD_ResetHeading and ISD_BoresightReferenced.
//   Note that the angles are reset relative to the current yaw; if the station
// is at 90 degrees yaw and 0 degrees pitch/roll when this function is called, 
// rolling the sensor (relative to its current heading) will be considered pitch,
// and pitch (relative to its current heading) will be considered roll; it does
// not perform a boresight 'relative' to the current orientation vector.
// ----------------------------------------------------------------------------
DLLEXPORT Bool DLLENTRY ISD_Boresight( 
                                      ISD_TRACKER_HANDLE handle, 
                                      WORD stationID,
                                      Bool set
                                      );



// Send a configuration script to the tracker. Script must consist of valid 
// commands as described in the interface protocol. Commands in the script 
// should be terminated by the newline character '\n'.  The linefeed character '\r' 
// is added by the function, and is not required. 
//   Note that this may not be supported when using the shared memory interface,
// such as with sfServer, and is primarily intended for the IS-300/IS-600/IS-900 system.
// ----------------------------------------------------------------------------
DLLEXPORT Bool DLLENTRY ISD_SendScript( 
                                       ISD_TRACKER_HANDLE handle, 
                                       char *script 
                                       );


// Sends up to 4 output bytes to the auxiliary interface of the station  
// specified. The number of bytes should match the number the auxiliary outputs
// the interface is configured to expect. If too many are specified, extra bytes 
// are ignored. 
// ----------------------------------------------------------------------------
DLLEXPORT Bool DLLENTRY ISD_AuxOutput( 
                                       ISD_TRACKER_HANDLE handle, 
                                       WORD stationID,
                                       BYTE *AuxOutput, 
                                       WORD length 
                                       );

// Number of currently opened trackers is stored in the parameter passed to this
// functions 
// ----------------------------------------------------------------------------
DLLEXPORT Bool DLLENTRY ISD_NumOpenTrackers( WORD *num );


// Platform independent time function
// ----------------------------------------------------------------------------
DLLEXPORT float DLLENTRY ISD_GetTime( void );


// Broadcast tracker data over the network using UDP broadcast
// ----------------------------------------------------------------------------
DLLEXPORT Bool DLLENTRY ISD_UdpDataBroadcast( 
                                             ISD_TRACKER_HANDLE handle, 
                                             DWORD port,
                                             ISD_TRACKING_DATA_TYPE *trackingData,
                                             ISD_CAMERA_DATA_TYPE *cameraData
                                             );

// Retrieve system hardware information
// ----------------------------------------------------------------------------
DLLEXPORT Bool DLLENTRY ISD_GetSystemHardwareInfo( 
                                                  ISD_TRACKER_HANDLE handle, 
                                                  ISD_HARDWARE_INFO_TYPE *hwInfo
                                                  );


// Retrieve station hardware information
// ----------------------------------------------------------------------------
DLLEXPORT Bool DLLENTRY ISD_GetStationHardwareInfo( 
                                                   ISD_TRACKER_HANDLE handle, 
                                                   ISD_STATION_HARDWARE_INFO_TYPE *info, 
                                                   WORD stationID 
                                                   );

DLLEXPORT Bool DLLENTRY ISD_EnterHeading( ISD_TRACKER_HANDLE handle,  WORD stationID, float yaw );


// Retrieve wireless configuration information
// ----------------------------------------------------------------------------
DLLEXPORT Bool DLLENTRY ISD_GetPortWirelessInfo( 
                                                ISD_TRACKER_HANDLE handle, 
                                                WORD port, 
                                                ISD_PORT_WIRELESS_INFO_TYPE *info 
                                                );

#ifdef __cplusplus
}
#endif

#endif //_ISD_isenseh

