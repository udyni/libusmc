/*******************************************************
 * @file    libusmc.h
 * @date    May 2020
 * @author  Michele Devetta
 *
 * This is the libusmc public interface.
 *
 * LICENSE:
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *******************************************************/


#ifndef LIBUSMC_H
#define LIBUSMC_H

#include <stdint.h>
#include <string>

// LibUSMC error codes
#define ERR_SUCCESS             0
// Error codes from libusb
#define ERR_USB_IO             -1
#define ERR_USB_INVALID_PARAM  -2
#define ERR_USB_ACCESS         -3
#define ERR_USB_NO_DEVICE      -4
#define ERR_USB_NOT_FOUND      -5
#define ERR_USB_BUSY           -6
#define ERR_USB_TIMEOUT        -7
#define ERR_USB_OVERFLOW       -8
#define ERR_USB_PIPE           -9
#define ERR_USB_INTERRUPTED   -10
#define ERR_USB_NO_MEM        -11
#define ERR_USB_NOT_SUPPORTED -12
#define ERR_USB_OTHER         -99
// Other error codes
#define ERR_INVALID_ID        -40
#define ERR_INVALID_PARAM     -41
#define ERR_INVALID_VALUE     -42



typedef struct _USMC_EncoderState
{
    int EncoderPos; // Current position measured by encoder.
    int ECurPos;    // Current position (in Encoder Steps) - Synchronized with request call.
} USMC_EncoderState;

typedef struct _USMC_State
{
    int CurPos;        // Current position (in 1/8 steps).
    float Temp;        // Current temperature of the power driver.
    uint8_t SDivisor;  // Step is divided by this factor.
    bool Loft;         // Indicates backlash status.
    bool FullPower;    // Full power if TRUE.
    bool CW_CCW;       // Current direction of rotation (relatively to some direction √ dependent on step motor circuits connection and on its construction).
    bool Power;        // If TRUE then Step Motor power is ON.
    bool FullSpeed;    // If TRUE then full speed. Valid in "Slow Start" mode only.
    bool AReset;       // TRUE After Device reset, FALSE after "Set Position".
    bool RUN;          // TRUE if step motor is rotating.
    bool SyncIN;       // Logical state directly from input synchronization PIN (pulses treated as positive).
    bool SyncOUT;      // Logical state directly from output synchronization PIN (pulses are positive).
    bool RotTr;        // Indicates current rotary transducer logical press state.
    bool RotTrErr;     // Indicates rotary transducer error flag (reset by USMC_SetMode function with ResetRT bit √ TRUE).
    bool EmReset;      // Indicates state of emergency disable button (TRUE √ Step motor power off).
    bool Trailer1;     // Indicates trailer 1 logical press state.
    bool Trailer2;     // Indicates trailer 2 logical press state.
    float Voltage;     // Input power source voltage (6-39V) -= 24 version 0nly=-.
} USMC_State;

typedef struct _USMC_Mode
{                       //      Masked        Saved To Flash      Description:
    bool  PMode;        //        √            YES                Turn off buttons (TRUE - buttons disabled).
    bool  PReg;         //        √            YES                Current reduction regime (TRUE - regime is on).
    bool  ResetD;       //        √            YES                Turn power off and make a whole step (TRUE - apply).
    bool  EMReset;      //        √            √                  Quick power off.
    bool  Tr1T;         //        √            YES                Trailer 1 TRUE state (TRUE : +3/+5б; FALSE : 0б).
    bool  Tr2T;         //        √            YES                Trailer 2 TRUE state (TRUE : +3/+5б; FALSE : 0б).
    bool  RotTrT;       //        √            YES                Rotary Transducer TRUE state (TRUE : +3/+5б; FALSE : 0б).
    bool  TrSwap;       //        √            YES                If TRUE, Trailers are treated to be swapped.
    bool  Tr1En;        //        √            YES                If TRUE Trailer 1 Operation Enabled.
    bool  Tr2En;        //        √            YES                If TRUE Trailer 2 Operation Enabled.
    bool  RotTeEn;      //        √            YES                If TRUE Rotary Transducer Operation Enabled.
    bool  RotTrOp;      //        √            YES                Rotary Transducer Operation Select (stop on error if TRUE).
    bool  Butt1T;       //        √            YES                Button 1 TRUE state (TRUE : +3/+5б; FALSE : 0б).
    bool  Butt2T;       //        √            YES                Button 2 TRUE state (TRUE : +3/+5б; FALSE : 0б).
    bool  ResetRT;      //        YES            √                Reset Rotary Transducer Check Positions (need one full revolution before it can detect error).
    bool  SyncOUTEn;    //        √            YES                If TRUE output synchronization enabled.
    bool  SyncOUTR;     //        YES            √                If TRUE output synchronization counter will be reset.
    bool  SyncINOp;     //        √            YES                Synchronization input mode:
                        //                                          TRUE - Step motor will move one time to the DestPos
                        //                                          FALSE - Step motor will move multiple times by DestPos microsteps as distance.
    uint32_t SyncCount; //        √            YES                Number of steps after which synchronization output signal occurs.
    bool  SyncInvert;   //        √            YES                Set this bit to TRUE to invert output synchronization polarity.
    bool  EncoderEn;    //        √            YES                Enable Encoder on pins {SYNCIN,ROTTR} - disables Synchronization input and Rotary Transducer.
    bool  EncoderInv;   //        √            YES                Invert Encoder Counter Direction.
    bool  ResBEnc;      //        YES            √                Reset <EncoderPos> and <ECurPos> to 0.
    bool  ResEnc;       //        YES            √                Reset <ECurPos> to <EncoderPos>.
} USMC_Mode;


typedef struct _USMC_Parameters
{
    float AccelT;         // Acceleration time (in ms).
    float DecelT;         // Deceleration time (in ms).
    float PTimeout;       // Time (in ms) after which current will be reduced to 60% of normal.
    float BTimeout1;      // Time (in ms) after which speed of step motor rotation will be equal to the one specified at BTO1P field in this structure.
    float BTimeout2;      // Time (in ms) after which speed of step motor rotation will be equal to the one specified at BTO2P field in this structure.
    float BTimeout3;      // Time (in ms) after which speed of step motor rotation will be equal to the one specified at BTO3P field in this structure.
    float BTimeout4;      // Time (in ms) after which speed of step motor rotation will be equal to the one specified at BTO4P field in this structure.
    float BTimeoutR;      // Time (in ms) after which reset command will be performed.
    float BTimeoutD;      // This field is reserved for future use.
    float MinP;           // Speed (steps/sec) while performing reset operation.
    float BTO1P;          // Speed (steps/sec) after BTIMEOUT1 time has passed.
    float BTO2P;          // Speed (steps/sec) after BTIMEOUT2 time has passed.
    float BTO3P;          // Speed (steps/sec) after BTIMEOUT3 time has passed.
    float BTO4P;          // Speed (steps/sec) after BTIMEOUT4 time has passed.
    uint16_t  MaxLoft;    // Value in full steps that will be used performing backlash operation.
    uint32_t StartPos;    // Current Position saved to FLASH.
    uint16_t  RTDelta;    // Revolution distance √ number of full steps per one full revolution.
    uint16_t  RTMinError; // Number of full steps missed to raise the error flag.
    float MaxTemp;        // Maximum allowed temperature (centigrade degrees).
    uint8_t  SynOUTP;     // Duration of the output synchronization pulse.
    float LoftPeriod;     // Speed (steps/sec) of the last phase of the backlash operation.
    float EncMult;        // Encoder step multiplier. Should be <Encoder Steps per Evolution> / <SM Steps per Evolution> and should be integer multiplied by 0.25.
} USMC_Parameters;


typedef struct _USMC_StartParameters
{
    uint8_t SDivisor;  // Step is divided by this factor (1,2,4,8).
    bool DefDir;       // Direction for backlash operation (relative).
    bool LoftEn;       // Enable automatic backlash operation (works if slow start/stop mode is off).
    bool SlStart;      // If TRUE slow start/stop mode enabled.
    bool WSyncIN;      // If TRUE controller will wait for input synchronization signal to start.
    bool SyncOUTR;     // If TRUE output synchronization counter will be reset.
    bool ForceLoft;    // If TRUE and destination position is equal to the current position backlash operation will be performed.
} USMC_StartParameters;


/**
 * @class USMC
 * Public interface to USMC devices
 */
class USMC {
public:
    /**
     * Get instance of USMC (should be a singleton)
     * @see USMC_State
     * @return pointer to USMC instance
     */
    static USMC* getInstance();

    /**
     * Shutdown library
     */
    static void shutdown();

    /**
     * Probe available devices
     * @return 0 un success, negative error number on error
     */
    virtual int probeDevices() = 0;

    /**
     * Return the number of available devices
     * @return the number of available devices
     */
    virtual size_t countDevices()const = 0;

    /**
     * Get the device number, given the serial number
     * @param serial a string containing the serial number you're looking for
     * @return the device number ID, -1 if not found
     */
    virtual int getDeviceID(const std::string& serial)const = 0;

    /**
     * Configure debugging
     * @param en Enable flag
     */
    virtual void debug(bool en) = 0;

    /**
     * Setup the error logger
     * @param logger Pointer to a function taking a format string and variable number of parameters
     */
    virtual void set_error_logger(void (*logger)(const char*, ...)) = 0;

    /**
     * Setup the warning logger
     * @param logger Pointer to a function taking a format string and variable number of parameters
     */
    virtual void set_warn_logger(void (*logger)(const char*, ...)) = 0;

    /**
     * Setup the information logger
     * @param logger Pointer to a function taking a format string and variable number of parameters
     */
    virtual void set_info_logger(void (*logger)(const char*, ...)) = 0;

    /**
     * Setup the debug logger
     * @param logger Pointer to a function taking a format string and variable number of parameters
     */
    virtual void set_debug_logger(void (*logger)(const char*, ...)) = 0;

    /**
     * Get device serial number
     * @param device the index of the desired device.
     * @param serial a reference to a string to store the serial number
     * @return 0 on success, negative error number on error
     */
    virtual int getSerialNumber(int device, std::string& serial)const = 0;

    /**
     * Get device firmware version
     * @param device the index of the desired device.
     * @param version a reference to a uint32_t to store the firmware version
     * @return 0 on success, negative error number on error
     */
    virtual int getVersion(int device, uint32_t& version)const = 0;

    /**
     * Get device status
     * @param device the index of the desired device.
     * @param mode a pointer to a USMC_State structure.
     * @see USMC_State
     * @return 0 on success, negative error number on error
     */
    virtual int getState(int device, USMC_State *state) = 0;

    /**
     * Get device mode. The result is stored in the given structure
     * @param device the index of the desired device.
     * @param mode a pointer to a USMC_Mode structure.
     * @see USMC_Mode
     * @return 0 on success, negative error number on error
     */
    virtual int getMode(int device, USMC_Mode* mode)const = 0;

    /**
     * Set device mode
     * @param device the index of the desired device.
     * @param mode a pointer to a USMC_Mode structure.
     * @see USMC_Mode
     * @return 0 on success, negative error number on error
     */
    virtual int setMode(int device, const USMC_Mode* mode) = 0;

    /**
     * Get device parameters
     * @param device the index of the desired device.
     * @param mode a pointer to a USMC_Parameters structure.
     * @see USMC_Parameters
     * @return 0 on success, negative error number on error
     */
    virtual int getParameters(int device, USMC_Parameters* parameters)const = 0;

    /**
     * Set device parameters
     * @param device the index of the desired device.
     * @param mode a pointer to a USMC_Parameters structure.
     * @see USMC_Parameters
     * @return 0 on success, negative error number on error
     */
    virtual int setParameters(int device, const USMC_Parameters* parameters) = 0;

    /**
     * Get device move parameters
     * @param device the index of the desired device.
     * @param mode a pointer to a USMC_StartParameters structure.
     * @see USMC_StartParameters
     * @return 0 on success, negative error number on error
     */
    virtual int getStartParameters(int device, USMC_StartParameters* start_params)const = 0;

    /**
     * Set device move parameters
     * @param device the index of the desired device.
     * @param mode a pointer to a USMC_StartParameters structure.
     * @see USMC_StartParameters
     * @return 0 on success, negative error number on error
     */
    virtual int setStartParameters(int device, const USMC_StartParameters* start_params) = 0;

    /**
     * Get device speed
     * @param device the index of the desired device.
     * @param speed a reference to a float to store speed
     * @return 0 on success, negative error number on error
     */
    virtual int getSpeed(int device, float& speed)const = 0;

    /**
     * Set device speed
     * @param device the index of the desired device.
     * @param speed a float with the desired speed
     * @return 0 on success, negative error number on error
     */
    virtual int setSpeed(int device, float speed) = 0;

    /**
     * Start a move on device
     * @param device the index of the desired device.
     * @param destination the destination of the move in steps.
     * @return 0 on success, negative error number on error
     */
    virtual int moveTo(int device, int destination) = 0;

    /**
     * Stop the device
     * @param device the index of the desired device.
     * @return 0 on success, negative error number on error
     */
    virtual int stop(int device) = 0;

    /**
     * Set current position to a defined value
     * @param device the index of the desired device.
     * @param mode the desired position value in steps.
     * @return 0 on success, negative error number on error
     */
    virtual int setCurrentPosition(int device, int position) = 0;

    /**
     * Get the status of the encoder
     * @param device the index of the desired device.
     * @param mode a pointer to a USMC_EncoderState structure.
     * @see USMC_EncoderState
     * @return 0 on success, negative error number on error
     */
    virtual int getEncoderState(int device, USMC_EncoderState* state) = 0;

protected:
    // Constructor and destructor
    USMC();
    virtual ~USMC();

private:
    // Private copy constructor
    USMC(const USMC& obj);
    USMC& operator=(const USMC& obj);

    // Instance
    static USMC* _instance;
};


#endif
