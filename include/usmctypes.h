/************************************************************************\
##                                                                      ##
##  Creation Date: 18 Mar 2007                                          ##
##  Last Update:   18 Mar 2007                                          ##
##  Author:       XInstruments                                          ##
##                                                                      ##
##  Modified: 4 Nov 2019 for user space libusb                          ##
##  Author: Michele Devetta                                             ##
##                                                                      ##
##  Desc:   Setup packet-building functions.                            ##
##                                                                      ##
\************************************************************************/


#ifndef _USMCTYPES_H
#define _USMCTYPES_H

#include <stdint.h>

// Redefine standard integers for compatibility with original kernel declaration
typedef uint8_t __u8;
typedef uint16_t __u16;
typedef uint32_t __u32;

// Types of GetDescriptor SetupPacket:
#define GET_DESCRIPTOR_CONFIGURATION    1
#define GET_DESCRIPTOR_DEVICE           2
#define GET_DESCRIPTOR_STRING           3

// Types of GetStatus SetupPacket:
#define GET_STATUS_DEVICE       1
#define GET_STATUS_ENDPOINT     2
#define GET_STATUS_INTERFACE    3

#pragma pack ( push, 1 )

typedef struct _STATE_PACKET // 11 bytes;
{
    __u32  CurPos;           // Current Position (byte 0 is lowest byte 3 - highest) - CP.
    __u16  Temp;             // Current Temperature of Driver.
    // Byte 6:
    __u8   M1        : 1;    // | Step size is 2^(-M1-2*M2), where M1,M2 = 0,1. May be otherwise 1<->2.
    __u8   M2        : 1;    // |
    __u8   LOFT      : 1;    // Indicates "Loft State".
    __u8   REFIN     : 1;    // If TRUE then full power.
    __u8   CW_CCW    : 1;    // Current direction. Relatively!
    __u8   RESET     : 1;    // If TRUE then Step Motor is ON.
    __u8   FULLSPEED : 1;    // If TRUE then full speed. Valid in "Slow Start" mode.
    __u8   AFTRESET  : 1;    // TRUE After Device reset, FALSE after "Set Position".
    // Byte 7:
    __u8   RUN       : 1;    // TRUE if step motor is rotating.
    __u8   SYNCIN    : 1;    // Logical state directly from input synchronization PIN (pulses treated as positive).
    __u8   SYNCOUT   : 1;    // Logical state directly from output synchronization PIN (pulses are positive).
    __u8   ROTTR     : 1;    // Indicates current rotary transducer logical press state.
    __u8   ROTTRERR  : 1;    // Indicates rotary transducer error flag (reset by USMC_SetMode function with ResetRT bit √ TRUE).
    __u8   EMRESET   : 1;    // Indicates state of emergency disable button (TRUE √ Step motor power off).
    __u8   TRAILER1  : 1;    // Indicates trailer 1 logical press state.
    __u8   TRAILER2  : 1;    // Indicates trailer 2 logical press state.
    // Byte 8:
    __u8   USBPOW    : 1;    // USB Powered.
    __u8   UNKNOWN   : 6;
    __u8   Working   : 1;    // This bit must be always TRUE (to chek functionality).
    __u16  Voltage;        // Voltage of +40V Power input.
} STATE_PACKET, * PSTATE_PACKET, * LPSTATE_PACKET;


typedef struct _ENCODER_STATE_PACKET // 8 bytes;
{
    __u32 ECurPos;   // Current Position in Encoder Units.
    __u32 EncPos;    // Encoder Current Position.
} ENCODER_STATE_PACKET, * PENCODER_STATE_PACKET, * LPENCODER_STATE_PACKET;


typedef struct _GO_TO_PACKET // 7 bytes;
{
    __u32 DestPos;          // Destination Position.
    __u16 TimerPeriod;      // Period between steps is 12*(65536-[TimerPeriod])/[SysClk] in seconds, where SysClk = 24000000 Hz.
    // Byte 7:
    __u8  M1        : 1;    // | Step size is 2^(-M1-2*M2), where M1,M2 = 0,1. May be otherwise 1<->2.
    __u8  M2        : 1;    // |
    __u8  DEFDIR    : 1;    // Default direction. For "Anti Loft" operation.
    __u8  LOFTEN    : 1;    // Enable automatic "Anti Loft" operation.
    __u8  SLSTRT    : 1;    // Slow Start(and Stop) mode.
    __u8  WSYNCIN   : 1;    // Wait for input synchronization signal to start.
    __u8  SYNCOUTR  : 1;    // Reset output synchronization counter.
    __u8  FORCELOFT : 1;    // Force driver automatic "Anti Loft" mechanism to reset "Loft State".
} GO_TO_PACKET, * PGO_TO_PACKET, * LPGO_TO_PACKET;


typedef struct _MODE_PACKET // 7 bytes;
{
    // Byte 0:
    __u8  PMODE    : 1;    // Turn off buttons (TRUE - buttons disabled).
    __u8  REFINEN  : 1;    // Current reduction regime (TRUE - regime is on).
    __u8  RESETD   : 1;    // Turn power off and make a whole step (TRUE - apply).
    __u8  EMRESET  : 1;    // Quick power off.
    __u8  TR1T     : 1;    // Trailer 1 TRUE state.
    __u8  TR2T     : 1;    // Trailer 2 TRUE state.
    __u8  ROTTRT   : 1;    // Rotary Transducer TRUE state.
    __u8  TRSWAP   : 1;    // If TRUE, Trailers are Swapped (Swapping After Reading Logical State).
    // Byte 1:
    __u8  TR1EN    : 1;    // Trailer 1 Operation Enabled.
    __u8  TR2EN    : 1;    // Trailer 2 Operation Enabled.
    __u8  ROTTREN  : 1;    // Rotary Transducer Operation Enabled.
    __u8  ROTTROP  : 1;    // Rotary Transducer Operation Select (stop on error for TRUE).
    __u8  BUTT1T   : 1;    // Button 1 TRUE state.
    __u8  BUTT2T   : 1;    // Button 2 TRUE state.
    __u8  BUTSWAP  : 1;    // If TRUE, Buttons are Swapped (Swapping After Reading Logical State).
    __u8  RESETRT  : 1;    // Reset Rotary Transducer Check Positions (need one full revolution before it can detect error).
    // Byte 2:
    __u8  SNCOUTEN : 1;    // Output Syncronization Enabled.
    __u8  SYNCOUTR : 1;    // Reset output synchronization counter.
    __u8  SYNCINOP : 1;    // Synchronization input mode: TRUE - Step motor will move one time to the DestPos FALSE - Step motor will move multiple times by DestPos microsteps as distance.
    __u8  SYNCOPOL : 1;    // Output Syncronization Pin Polarity.
    __u8  ENCODER  : 1;    // Encoder is used on pins {SYNCIN,ROTTR} - disables Syncronization input and Rotary Tranducer.
    __u8  INVENC   : 1;    // Invert Encoder Counter Direction.
    __u8  RESBENC  : 1;    // Reset <Encoder Position> and <SM Position in Encoder units> to 0.
    __u8  RESENC   : 1;    // Reset <SM Position in Encoder units> to <Encoder Position>.
    __u16 SYNCCOUNT;    // Number of steps after which synchronization output signal occurs.
} MODE_PACKET, * PMODE_PACKET, * LPMODE_PACKET;


typedef struct _PARAMETERS_PACKET // 57 bytes;
{
    __u8  DELAY1;        // Acceleration time multiplier.
    __u8  DELAY2;        // Deceleration time multiplier.
    __u16 RefINTimeout;  // Timeout for RefIN reseting.
    __u16 BTIMEOUT1;     // | Buttons Timeouts (4 stages).
    __u16 BTIMEOUT2;     // |
    __u16 BTIMEOUT3;     // |
    __u16 BTIMEOUT4;     // |
    __u16 BTIMEOUTR;     // Timeout for RESET command.
    __u16 BTIMEOUTD;     // Timeout for Double Click.
    __u16 MINPERIOD;     // Standart Timer Period.
    __u16 BTO1P;         // | Timer Periods for button rotation.
    __u16 BTO2P;         // |
    __u16 BTO3P;         // |
    __u16 BTO4P;         // |
    __u16 MAX_LOFT;      // Max Loft Value.
    __u32 STARTPOS;      // Start Position.
    __u16 RTDelta;       // Revolution Distance.
    __u16 RTMinError;    // Minimal value of Rotatory Tranduser Error.
    __u16 MaxTemp;       // Working Temperature Limit.
    __u8  SynOUTP;       // Syncronizaion OUT pulse duration( T = (sopd-1/2)*StepPeriod ).
    __u16 LoftPeriod;    // Loft last phase speed.
    __u8  EncVSCP;       // 4x Number of Encoder steps per one full SM step.
    __u8  Reserved [15]; // Reserved.
} PARAMETERS_PACKET, * PPARAMETERS_PACKET, * LPPARAMETERS_PACKET;


typedef struct _DOWNLOAD_PACKET // 65 bytes;
{
    __u8 Page;        // Page number ( 0 - 119 ). 0 - first, 119 - last.
    __u8 Data [64];   // Data.
} DOWNLOAD_PACKET, * PDOWNLOAD_PACKET, * LPDOWNLOAD_PACKET;


typedef struct _SERIAL_PACKET // 32 bytes;
{
    __u8 Password     [16];
    __u8 SerialNumber [16];
} SERIAL_PACKET, * PSERIAL_PACKET, * LPSERIAL_PACKET;

#pragma pack ( pop )


typedef GO_TO_PACKET const * PCGO_TO_PACKET;
typedef GO_TO_PACKET const * LPCGO_TO_PACKET;
typedef MODE_PACKET const * PCMODE_PACKET;
typedef MODE_PACKET const * LPCMODE_PACKET;
typedef PARAMETERS_PACKET const * PCPARAMETERS_PACKET;
typedef PARAMETERS_PACKET const * LPCPARAMETERS_PACKET;
typedef DOWNLOAD_PACKET const * PCDOWNLOAD_PACKET;
typedef DOWNLOAD_PACKET const * LPCDOWNLOAD_PACKET;
typedef SERIAL_PACKET const * PCSERIAL_PACKET;
typedef SERIAL_PACKET const * LPCSERIAL_PACKET;
typedef STATE_PACKET const * PCSTATE_PACKET;
typedef STATE_PACKET const * LPCSTATE_PACKET;
typedef ENCODER_STATE_PACKET const * PCENCODER_STATE_PACKET;
typedef ENCODER_STATE_PACKET const * LPCENCODER_STATE_PACKET;

#endif    // _USMCTYPES_H
