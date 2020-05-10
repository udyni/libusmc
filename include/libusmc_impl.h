/***************************************************//**
 * @file    libusmc_impl.h
 * @date    May 2020
 * @author  Michele Devetta
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


#ifndef LIBUSMC_IMPL_H
#define LIBUSMC_IMPL_H

#include <string>
#include <vector>
#include <libusb.h>
#include <libusmc.h>
#include <usmctypes.h>
#include <usmc_mutex.h>



// USMC implementation
class USMC_impl : public USMC {
public:
    // Probe devices
    virtual int probeDevices();

    // Count devices
    virtual size_t countDevices()const;

    // Get device ID by serial number
    virtual int getDeviceID(const std::string& serial)const;

    // Enable debug
    virtual void debug(bool en);

    // Confiugre error logger
    virtual void set_error_logger(void (*logger)(const char*, ...));

    // Confiugre warning logger
    virtual void set_warn_logger(void (*logger)(const char*, ...));

    // Confiugre info logger
    virtual void set_info_logger(void (*logger)(const char*, ...));

    // Confiugre debug logger
    virtual void set_debug_logger(void (*logger)(const char*, ...));

    // Get serial number
    virtual int getSerialNumber(int device, std::string& serial)const;

    // Get firmware version
    virtual int getVersion(int device, uint32_t& version)const;

    // Get device state
    virtual int getState(int device, USMC_State *state);

    // Get device mode
    virtual int getMode(int device, USMC_Mode* mode)const;

    // Set device mode
    virtual int setMode(int device, const USMC_Mode* mode);

    // Get device parameters
    virtual int getParameters(int device, USMC_Parameters* parameters)const;

    // Set device parameters
    virtual int setParameters(int device, const USMC_Parameters* parameters);

    // Get move parameters
    virtual int getStartParameters(int device, USMC_StartParameters* start_params)const;

    // Set move parameters
    virtual int setStartParameters(int device, const USMC_StartParameters* start_params);

    // Get speed
    virtual int getSpeed(int device, float& speed)const;

    // Set speed
    virtual int setSpeed(int device, float speed);

    // Move device to position
    virtual int moveTo(int device, int destination);

    // Stop device
    virtual int stop(int device);

    // Set current position
    virtual int setCurrentPosition(int device, int position);

    // Get encoder state
    virtual int getEncoderState(int device, USMC_EncoderState* state);

public:
    // Destructor
    virtual ~USMC_impl();

protected:
    // Constructor
    USMC_impl();

    // Clamp values
    int clamp ( int val, int min, int max ) { return val > max ? max : ( val < min ? min : val ); }
    float clamp ( float val, float min, float max ) { return val > max ? max : ( val < min ? min : val ); }

    // Initialize structures to default values
    void initDefaults(int id);

    // Check if the device ID is valid
    bool checkDevice(int device)const;

    // USB communication methods
    int usmc_get_version(int id, uint32_t& version);
    int usmc_get_serial(int id, char* serial, size_t len);
    int usmc_get_encoder_state(int id, USMC_EncoderState& state);
    int usmc_get_state(int id, USMC_State& state);
    int usmc_goto(int id, int position, float speed, const USMC_StartParameters& params);
    int usmc_set_mode(int id, const USMC_Mode& mode);
    int usmc_set_parameters(int id, const USMC_Parameters& params);
//  int usmc_set_serial(int id);  // NOT IMPLEMENTED
    int usmc_set_current_position(int id, int32_t position);
//  int usmc_download(int id);    // NOT IMPLEMENTED
    int usmc_stop(int id);
//  int usmc_emulate(int id);     // NOT IMPLEMENTED
    int usmc_save(int id);

private:
    // Private copy constructor
    USMC_impl(const USMC_impl& obj);
    USMC_impl& operator=(const USMC_impl& obj);

    // Libusb context
    libusb_context* _usb_ctx;

    // Logger functions
    void (*_error_logger)(const char*, ...);
    void (*_warn_logger)(const char*, ...);
    void (*_info_logger)(const char*, ...);
    void (*_debug_logger)(const char*, ...);

    // Enable debug
    bool _debug;

    // USB timeout
    int _timeout;

    // Device handle
    std::vector<libusb_device_handle*> _dev;

    // Device mutexes
    std::vector<USMC_mutex*> _locks;

    // Firmware version
    std::vector<uint32_t> _version;

    // Serial number
    std::vector<std::string> _serial;

    // Speeds
    std::vector<float> _speed;

    // Device parameters structures
    std::vector<USMC_Parameters*> _params;
    std::vector<USMC_Mode*> _mode;
    std::vector<USMC_StartParameters*> _start_params;

    friend class USMC;
};


#endif
