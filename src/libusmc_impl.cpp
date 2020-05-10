// kate: replace-tabs on; indent-width 4; indent-mode cstyle;
/***************************************************//**
 * @file    libusmc_impl.cpp
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

#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cmath>
#include <libusmc.h>
#include <libusmc_impl.h>


// Device vendor and product IDs
#define USMC_VENDOR_ID     0x10c4
#define USMC_PRODUCT_ID    0x0230

// Byte swapping and extraction functions
#define HIBYTE(w)                         ((w&0xff00)>>8)
#define LOBYTE(w)                         (w&0x00ff)
#define BYTE_I(i)                         (*(((__u8 * )pPacketData)+i))
#define FIRST_BYTE(pPacketData)           (*((__u8 * )pPacketData))
#define SECOND_BYTE(pPacketData)          (*(((__u8 * )pPacketData)+1))
#define THIRD_BYTE(pPacketData)           (*(((__u8 * )pPacketData)+2))
#define FOURTH_BYTE(pPacketData)          (*(((__u8 * )pPacketData)+3))
#define FIRST_WORD(pPacketData)           (*((__u16 * )pPacketData))
#define SECOND_WORD(pPacketData)          (*(((__u16 * )pPacketData)+1))
#define FIRST_WORD_SWAPPED(pPacketData)   ((FIRST_BYTE(pPacketData)<<8)|SECOND_BYTE(pPacketData))
#define SECOND_WORD_SWAPPED(pPacketData)  ((THIRD_BYTE(pPacketData)<<8)|FOURTH_BYTE(pPacketData))
#define PACK_WORD(w)                      (HIBYTE(w)|(LOBYTE(w)<<8))
#define REST_DATA(pPacketData)            ((void *)(((__u16 * )pPacketData)+2))
#define HIWORD(dw)                        ((dw&0xffff0000)>>16)
#define LOWORD(dw)                        (dw&0x0000ffff)
#define PACK_DWORD(w)                     (HIBYTE(HIWORD(w))| \
                                          (LOBYTE(HIWORD(w))<<8)| \
                                          (HIBYTE(LOWORD(w))<<16)| \
                                          (LOBYTE(LOWORD(w))<<24))


// Default logging functions
void usmc_log_error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    std::stringstream format;
    format << "[ERROR] " << fmt << std::endl;
    vprintf(format.str().c_str(), args);
    va_end(args);
}

void usmc_log_warn(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    std::stringstream format;
    format << "[WARN] " << fmt << std::endl;
    vprintf(format.str().c_str(), args);
    va_end(args);
}

void usmc_log_info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    std::stringstream format;
    format << "[INFO] " << fmt << std::endl;
    vprintf(format.str().c_str(), args);
    va_end(args);
}

void usmc_log_debug(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    std::stringstream format;
    format << "[DEBUG] " << fmt << std::endl;
    vprintf(format.str().c_str(), args);
    va_end(args);
}


// Implementation constructor
USMC_impl::USMC_impl() : _usb_ctx(NULL), _debug(false), _timeout(10000) {
    // Set default loggers
    _error_logger = usmc_log_error;
    _warn_logger = usmc_log_warn;
    _info_logger = usmc_log_info;
    _debug_logger = usmc_log_debug;

    // Initialize libusb
    int ret = libusb_init(&_usb_ctx);
    if(ret) {
        // Error
        _usb_ctx = NULL;
        _error_logger("Failed to initialize libusb. Error: %s", libusb_strerror(static_cast<libusb_error>(ret)));
        throw std::runtime_error("Failed to initialize libusb");
    }
}


// Implementation destructor
USMC_impl::~USMC_impl() {
    // Close device if is open
    for(size_t i = 0; i < _dev.size(); i++) {
        // Close device
        if(_dev[i]) {
            libusb_close(_dev[i]);
            _dev[i] = NULL;
        }
        // Deallocate structures
        if(_locks[i])
            delete _locks[i];
        if(_params[i])
            delete _params[i];
        if(_mode[i])
            delete _mode[i];
        if(_start_params[i])
            delete _start_params[i];
    }
    _dev.clear();
    _locks.clear();
    _params.clear();
    _mode.clear();
    _start_params.clear();
    _serial.clear();
    _version.clear();
    _speed.clear();

    // Close libusb
    if(_usb_ctx) {
        libusb_exit(_usb_ctx);
        _usb_ctx = NULL;
    }
}


// Enable debug
void USMC_impl::debug(bool en) {
    _debug = en;
}


// Configure loggers
void USMC_impl::set_error_logger(void (*logger)(const char*, ...)) {
    _error_logger = logger;
}
void USMC_impl::set_warn_logger(void (*logger)(const char*, ...)) {
    _warn_logger = logger;
}
void USMC_impl::set_info_logger(void (*logger)(const char*, ...)) {
    _info_logger = logger;
}
void USMC_impl::set_debug_logger(void (*logger)(const char*, ...)) {
    _debug_logger = logger;
}


// Probe and open available devices
int USMC_impl::probeDevices() {

    int count = 0;

    // Get device list
    libusb_device **devs;
    ssize_t cnt = libusb_get_device_list(NULL, &devs);
    if (cnt < 0){
        // Failed to get device list
        _error_logger("Failed to get device list. Error: %s", libusb_strerror(static_cast<libusb_error>(cnt)));
        return cnt;
    }

    // Traverse device list
    libusb_device *dev = NULL;
    int i = 0;
    while ((dev = devs[i++]) != NULL) {

        // Get device descriptor
        struct libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0) {
            _warn_logger("Failed to get device descriptor. Error: %s", libusb_strerror(static_cast<libusb_error>(r)));
            continue;
        }

        if(desc.idVendor == USMC_VENDOR_ID && desc.idProduct == USMC_PRODUCT_ID) {
            // Found an USMC device, try to open it!
            libusb_device_handle* dev_h = NULL;
            r = libusb_open(dev, &dev_h);
            if(r < 0) {
                _error_logger("Failed to open device. Error: %s", libusb_strerror(static_cast<libusb_error>(r)));

            } else {
                // Next ID
                int id = _dev.size();

                // Open successfully, we can add the device to the library
                _dev.push_back(dev_h);

                // Structures
                _locks.push_back(new USMC_mutex());
                _params.push_back(new USMC_Parameters);
                _mode.push_back(new USMC_Mode);
                _start_params.push_back(new USMC_StartParameters);
                _speed.push_back(200.0f);

                try {
                    // Read serial number
                    char buffer[32];
                    memset(buffer, 0, 32);
                    r = usmc_get_serial(id, buffer, 32);
                    if(r < 0) {
                        _error_logger("Failed to get serial number. Error: %d", r);
                        throw std::exception();
                    }
                    std::string serial(buffer);
                    _serial.push_back(serial);

                    // Read version
                    uint32_t version = 0;
                    r = usmc_get_version(id, version);
                    if(r < 0) {
                        _error_logger("Failed to get version. Error: %d", r);
                        throw std::exception();
                    }
                    _version.push_back(version);

                    // Init default params
                    initDefaults(id);

                    // Write values to hardware to get a consistent state
                    r = usmc_set_mode(id, *(_mode[id]));
                    if(r < 0) {
                        _error_logger("Failed to initialize mode. Error: %d", r);
                        throw std::exception();
                    }
                    r = usmc_set_parameters(id, *(_params[id]));
                    if(r < 0) {
                        _error_logger("Failed to initialize parameters. Error: %d", r);
                        throw std::exception();
                    }

                    _info_logger("Device found and open successfully.");
                    count++;

                } catch(std::exception) {
                    // Remove device
                    libusb_close(_dev[id]);
                    _dev.pop_back();
                    delete _locks[id];
                    _locks.pop_back();
                    delete _params[id];
                    _params.pop_back();
                    delete _mode[id];
                    _mode.pop_back();
                    delete _start_params[id];
                    _start_params.pop_back();
                    _speed.pop_back();
                    if(_serial.size() > id)
                        _serial.pop_back();
                    if(_version.size() > id)
                        _version.pop_back();
                }
            }
        }
    }

    // Free device list
    libusb_free_device_list(devs, 1);

    return count;
}

// Return device count
size_t USMC_impl::countDevices()const {
    return _dev.size();
}

// Get device ID by serial number
int USMC_impl::getDeviceID(const std::string& serial)const {
    for(size_t i = 0; i < _serial.size(); i++) {
        if(_serial[i] == serial)
            return int(i);
    }
    // Not found
    return -1;
}

// Check if device ID is valid
bool USMC_impl::checkDevice(int device)const {
    if(device >= 0 && device < _dev.size())
        return true;
    else
        return false;
}

// Get serial number
int USMC_impl::getSerialNumber(int device, std::string& serial)const {
    if(!checkDevice(device))
        return ERR_INVALID_ID;
    serial = _serial[device];
    return ERR_SUCCESS;
}

// Get firmware version
int USMC_impl::getVersion(int device, uint32_t& version)const {
    if(!checkDevice(device))
        return ERR_INVALID_ID;
    version = _version[device];
    return ERR_SUCCESS;
}

// Get device state
int USMC_impl::getState(int device, USMC_State *state) {
    if(!checkDevice(device))
        return ERR_INVALID_ID;
    if(NULL == state)
        return ERR_INVALID_PARAM;
    // Call USB
    return usmc_get_state(device, *state);
}

// Get device mode
int USMC_impl::getMode(int device, USMC_Mode* mode)const {
    if(!checkDevice(device))
        return ERR_INVALID_ID;
    if(NULL == mode)
        return ERR_INVALID_PARAM;

    // Copy structure to output
    memcpy((void*)mode, (void*)_mode[device], sizeof(USMC_Mode));
    return ERR_SUCCESS;
}

// Set device mode
int USMC_impl::setMode(int device, const USMC_Mode* mode) {
    if(!checkDevice(device))
        return ERR_INVALID_ID;
    if(NULL == mode)
        return ERR_INVALID_PARAM;

    // USB call
    int r = usmc_set_mode(device, *mode);
    if(r < 0)
        return r;

    // Store structure
    memcpy((void*)_mode[device], (void*)mode, sizeof(USMC_Mode));
    return ERR_SUCCESS;
}

// Get device parameters
int USMC_impl::getParameters(int device, USMC_Parameters* parameters)const {
    if(!checkDevice(device))
        return ERR_INVALID_ID;
    if(NULL == parameters)
        return ERR_INVALID_PARAM;

    // Copy structure to output
    memcpy((void*)parameters, (void*)_params[device], sizeof(USMC_Parameters));
    return ERR_SUCCESS;
}

// Set device parameters
int USMC_impl::setParameters(int device, const USMC_Parameters* parameters) {
    if(!checkDevice(device))
        return ERR_INVALID_ID;
    if(NULL == parameters)
        return ERR_INVALID_PARAM;

    // Check input values
    if(parameters->AccelT < 49.0 || parameters->AccelT > 1518.0)
        return ERR_INVALID_VALUE;
    if(parameters->DecelT < 49.0 || parameters->DecelT > 1518.0)
        return ERR_INVALID_VALUE;

    if(parameters->PTimeout < 1.0f || parameters->PTimeout > 9961.0f)
        return ERR_INVALID_VALUE;
    if(parameters->BTimeout1 < 1.0f || parameters->BTimeout1 > 9961.0f)
        return ERR_INVALID_VALUE;
    if(parameters->BTimeout2 < 1.0f || parameters->BTimeout2 > 9961.0f)
        return ERR_INVALID_VALUE;
    if(parameters->BTimeout3 < 1.0f || parameters->BTimeout3 > 9961.0f)
        return ERR_INVALID_VALUE;
    if(parameters->BTimeout4 < 1.0f || parameters->BTimeout4 > 9961.0f)
        return ERR_INVALID_VALUE;
    if(parameters->BTimeoutR < 1.0f || parameters->BTimeoutR > 9961.0f)
        return ERR_INVALID_VALUE;
    if(parameters->BTimeoutD < 1.0f || parameters->BTimeoutD > 9961.0f)
        return ERR_INVALID_VALUE;

    if(parameters->MaxLoft < 1 || parameters->MaxLoft > 1023)
        return ERR_INVALID_VALUE;
    if(parameters->RTDelta < 4 || parameters->RTDelta > 1023)
        return ERR_INVALID_VALUE;
    if(parameters->RTMinError < 4 || parameters->RTMinError > 1023)
        return ERR_INVALID_VALUE;
    if(parameters->MaxTemp < 0.0f || parameters->MaxTemp > 100.0f)
        return ERR_INVALID_VALUE;

    if(parameters->MinP < 2.0f || parameters->MinP > 625.0f)
        return ERR_INVALID_VALUE;
    if(parameters->BTO1P < 2.0f || parameters->BTO1P > 625.0f)
        return ERR_INVALID_VALUE;
    if(parameters->BTO2P < 2.0f || parameters->BTO2P > 625.0f)
        return ERR_INVALID_VALUE;
    if(parameters->BTO3P < 2.0f || parameters->BTO3P > 625.0f)
        return ERR_INVALID_VALUE;
    if(parameters->BTO4P < 2.0f || parameters->BTO4P > 625.0f)
        return ERR_INVALID_VALUE;

    if(parameters->LoftPeriod != 0 && (parameters->LoftPeriod < 16.0f || parameters->LoftPeriod > 5000.0f))
        return ERR_INVALID_VALUE;

    // USB call
    int r = usmc_set_parameters(device, *parameters);
    if(r < 0)
        return r;

    // Store structure
    memcpy((void*)_params[device], (void*)parameters, sizeof(USMC_Parameters));
    return ERR_SUCCESS;
}

// Get move parameters
int USMC_impl::getStartParameters(int device, USMC_StartParameters* start_params)const {
    if(!checkDevice(device))
        return ERR_INVALID_ID;
    if(NULL == start_params)
        return ERR_INVALID_PARAM;

    // Copy structure to output
    memcpy((void*)start_params, (void*)_start_params[device], sizeof(USMC_StartParameters));
    return ERR_SUCCESS;
}

// Set move parameters
int USMC_impl::setStartParameters(int device, const USMC_StartParameters* start_params) {
    if(!checkDevice(device))
        return ERR_INVALID_ID;
    if(NULL == start_params)
        return ERR_INVALID_PARAM;

    // Store structure
    memcpy((void*)_start_params[device], (void*)start_params, sizeof(USMC_StartParameters));
    return ERR_SUCCESS;
}

// Get speed
int USMC_impl::getSpeed(int device, float& speed)const {
    if(!checkDevice(device))
        return ERR_INVALID_ID;
    speed = _speed[device];
    return ERR_SUCCESS;
};

// Set speed
int USMC_impl::setSpeed(int device, float speed) {
    if(!checkDevice(device))
        return ERR_INVALID_ID;
    if(speed < 16.0f || speed > 5000.0f)
        return ERR_INVALID_VALUE;

    _speed[device] = speed;
    return ERR_SUCCESS;
}

// Move device to position
int USMC_impl::moveTo(int device, int destination) {
    if(!checkDevice(device))
        return ERR_INVALID_ID;

    // USB call
    return usmc_goto(device, destination, _speed[device], *(_start_params[device]));
}

// Stop device
int USMC_impl::stop(int device) {
    if(!checkDevice(device))
        return ERR_INVALID_ID;

    // USB call
    return usmc_stop(device);
}

// Set current position
int USMC_impl::setCurrentPosition(int device, int position) {
    if(!checkDevice(device))
        return ERR_INVALID_ID;

}

// Get encoder state
int USMC_impl::getEncoderState(int device, USMC_EncoderState* state) {
    if(!checkDevice(device))
        return ERR_INVALID_ID;
    if(NULL == state)
        return ERR_INVALID_PARAM;

    return usmc_get_encoder_state(device, *state);
}

// USB call to get version
int USMC_impl::usmc_get_version(int id, uint32_t& version) {
    uint8_t  bRequestType = LIBUSB_ENDPOINT_IN      |
                            LIBUSB_RECIPIENT_DEVICE |
                            LIBUSB_REQUEST_TYPE_STANDARD;
    uint8_t  bRequest = 0x06;
    uint16_t wValue = 0x0304;
    uint16_t wIndex = 0x0409;
    uint16_t wLength = 0x0006;
    uint8_t buffer[wLength+1];
    buffer[wLength] = 0;

    // Access lock
    USMC_lock access_lock(_locks[id]);

    int res = libusb_control_transfer(_dev[id], bRequestType, bRequest, wValue, wIndex, buffer, wLength, _timeout);

    if(res < 0) {
        // Call failed
        _error_logger("Failed to get version. Error: %s", libusb_strerror(static_cast<libusb_error>(res)));
        return res;

    } else {
        res = sscanf((const char*)(buffer+2), "%X", &version);
    }

    return 0;
}

// USB call to get serial number
int USMC_impl::usmc_get_serial(int id, char* serial, size_t len) {
    uint8_t  bRequestType = LIBUSB_ENDPOINT_IN      |
                            LIBUSB_RECIPIENT_DEVICE |
                            LIBUSB_REQUEST_TYPE_VENDOR;
    uint8_t  bRequest = 0xC9;
    uint16_t wValue = 0x0000;
    uint16_t wIndex = 0x0000;
    uint16_t wLength = 0x0010;
    uint8_t buffer[wLength];

    // Access lock
    USMC_lock access_lock(_locks[id]);

    int res = libusb_control_transfer(_dev[id], bRequestType, bRequest, wValue, wIndex, (uint8_t*)buffer, wLength, _timeout);

    if(res < 0) {
        // Call failed
        _error_logger("Failed to get serial number. Error: %s", libusb_strerror(static_cast<libusb_error>(res)));
        return res;

    } else {
        memset(serial, 0, len);
        strncpy(serial, (char*)buffer, (len > wLength) ? wLength : len-1);
    }

    return 0;
}

// USB call to get encoder state
int USMC_impl::usmc_get_encoder_state(int id, USMC_EncoderState& state) {
    uint8_t  bRequestType = LIBUSB_ENDPOINT_IN      |
                            LIBUSB_RECIPIENT_DEVICE |
                            LIBUSB_REQUEST_TYPE_VENDOR;
    uint8_t  bRequest = 0x85;
    uint16_t wValue = 0x0000;
    uint16_t wIndex = 0x0000;
    uint16_t wLength = sizeof(ENCODER_STATE_PACKET);
    ENCODER_STATE_PACKET getEncoderStateData;

    // Access lock
    USMC_lock access_lock(_locks[id]);

    int res = libusb_control_transfer(_dev[id], bRequestType, bRequest, wValue, wIndex, reinterpret_cast<uint8_t*>(&getEncoderStateData), wLength, _timeout);

    if(res < 0) {
        // Call failed
        _error_logger("Failed to get encoder state. Error: %s", libusb_strerror(static_cast<libusb_error>(res)));
        return res;

    } else {
        state.ECurPos    = getEncoderStateData.ECurPos;
        state.EncoderPos = getEncoderStateData.EncPos;
    }

    return 0;
}

// USB call to get device state
int USMC_impl::usmc_get_state(int id, USMC_State& state) {
    uint8_t  bRequestType = LIBUSB_ENDPOINT_IN      |
                            LIBUSB_RECIPIENT_DEVICE |
                            LIBUSB_REQUEST_TYPE_VENDOR;
    uint8_t  bRequest = 0x82;
    uint16_t wValue = 0x0000;
    uint16_t wIndex = 0x0000;
    uint16_t wLength = sizeof(STATE_PACKET);
    STATE_PACKET getStateData;

    // Access lock
    USMC_lock access_lock(_locks[id]);

    int res = libusb_control_transfer(_dev[id], bRequestType, bRequest, wValue, wIndex, reinterpret_cast<uint8_t*>(&getStateData), wLength, _timeout);

    if(res < 0) {
        // Call failed
        _error_logger("Failed to get device state. Error: %s", libusb_strerror(static_cast<libusb_error>(res)));
        return res;

    } else {
        state.AReset    = getStateData.AFTRESET;
        state.CurPos    = ( ( signed int ) getStateData.CurPos ) / 8;
        state.CW_CCW    = getStateData.CW_CCW;
        state.EmReset   = getStateData.EMRESET;
        state.FullPower = getStateData.REFIN;
        state.FullSpeed = getStateData.FULLSPEED;
        state.Loft      = getStateData.LOFT;
        state.Power     = getStateData.RESET;
        state.RotTr     = getStateData.ROTTR;
        state.RotTrErr  = getStateData.ROTTRERR;
        state.RUN       = getStateData.RUN;
        /* state.SDivisor= See below; */
        state.SyncIN    = getStateData.SYNCIN;
        state.SyncOUT   = getStateData.SYNCOUT;
        /* state.Temp    = See below; */
        state.Trailer1  = getStateData.TRAILER1;
        state.Trailer2  = getStateData.TRAILER2;
        /* state.Voltage = See below; */

        state.SDivisor  = ( uint8_t ) ( 1 << ( getStateData.M2 << 1 | getStateData.M1 ) );
        double t        = ( double ) getStateData.Temp;

        if ( _version[id] < 0x2400 )
        {
            t = t * 3.3 / 65536.0;
            t = t * 10.0 / ( 5.0 - t );
            t = ( 1.0 / 298.0 ) + ( 1.0 / 3950.0 ) * log ( t / 10.0 );
            t = 1.0 / t - 273.0;
        }
        else
        {
            t = ( ( t * 3.3 * 100.0 / 65536.0 ) - 50.0 );
        }

        state.Temp    = ( float ) t;
        state.Voltage = ( float ) ( ( ( double ) getStateData.Voltage ) / 65536.0 * 3.3 * 20.0 );
        state.Voltage = state.Voltage < 5.0f ? 0.0f : state.Voltage;
    }

    return 0;
}

// USB call to move device
int USMC_impl::usmc_goto(int id, int position, float speed, const USMC_StartParameters& params) {
    uint8_t  bRequestType = LIBUSB_ENDPOINT_OUT     |
                            LIBUSB_RECIPIENT_DEVICE |
                            LIBUSB_REQUEST_TYPE_VENDOR;
    uint8_t  bRequest = 0x80;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength = 3;
    GO_TO_PACKET goToData;

    /*=====================*/
    /* ----Conversion:---- */
    /*=====================*/
    goToData.DestPos     = ( uint32_t ) ( position * 8 );
    goToData.TimerPeriod = PACK_WORD ( ( uint16_t ) ( 65536.0f - ( 1000000.0f / clamp ( speed, 16.0f, 5000.0f ) ) + 0.5f ) );
    switch (params.SDivisor) {
        case 1:
            goToData.M1 = goToData.M2 = 0;
            break;
        case 2:
            goToData.M1 = 1;
            goToData.M2 = 0;
            break;
        case 4:
            goToData.M1 = 0;
            goToData.M2 = 1;
            break;
        case 8:
            goToData.M1 = 1;
            goToData.M2 = 1;
            break;
    }
    //goToData.M1          = params.SDivisor && 0x01;
    //goToData.M2          = params.SDivisor && 0x02;
    goToData.DEFDIR      = params.DefDir;
    goToData.LOFTEN      = params.LoftEn;
    goToData.SLSTRT      = params.SlStart;
    goToData.WSYNCIN     = params.WSyncIN;
    goToData.SYNCOUTR    = params.SyncOUTR;
    goToData.FORCELOFT   = params.ForceLoft;

    wIndex   = FIRST_WORD  ( reinterpret_cast<uint32_t*>(&goToData) );
    wValue   = SECOND_WORD ( reinterpret_cast<uint32_t*>(&goToData) );

    // Access lock
    USMC_lock access_lock(_locks[id]);

    int res = libusb_control_transfer(_dev[id], bRequestType, bRequest, wValue, wIndex, reinterpret_cast<uint8_t*>(&goToData)+4, wLength, _timeout);

    if(res < 0) {
        // Call failed
        _error_logger("Failed to move device. Error: %s", libusb_strerror(static_cast<libusb_error>(res)));
        return res;
    }

    return 0;
}

// USB call to move device
int USMC_impl::usmc_set_mode(int id, const USMC_Mode& mode) {
    uint8_t  bRequestType = LIBUSB_ENDPOINT_OUT     |
                            LIBUSB_RECIPIENT_DEVICE |
                            LIBUSB_REQUEST_TYPE_VENDOR;
    uint8_t  bRequest = 0x81;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength = 3;
    MODE_PACKET setModeData;

    // Byte 0:
    setModeData.PMODE     = mode.PMode;
    setModeData.REFINEN   = mode.PReg;
    setModeData.RESETD    = mode.ResetD;
    setModeData.EMRESET   = mode.EMReset;
    setModeData.TR1T      = mode.Tr1T;
    setModeData.TR2T      = mode.Tr2T;
    setModeData.ROTTRT    = mode.RotTrT;
    setModeData.TRSWAP    = mode.TrSwap;
    // Byte 1:
    setModeData.TR1EN     = mode.Tr1En;
    setModeData.TR2EN     = mode.Tr2En;
    setModeData.ROTTREN   = mode.RotTeEn;
    setModeData.ROTTROP   = mode.RotTrOp;
    setModeData.BUTT1T    = mode.Butt1T;
    setModeData.BUTT2T    = mode.Butt2T;
    /*setModeData.BUTSWAP = ...;*/
    setModeData.RESETRT   = mode.ResetRT;
    // Byte 2:
    setModeData.SNCOUTEN  = mode.SyncOUTEn;
    setModeData.SYNCOUTR  = mode.SyncOUTR;
    setModeData.SYNCINOP  = mode.SyncINOp;
    setModeData.SYNCOPOL  = mode.SyncInvert;
    setModeData.ENCODER   = mode.EncoderEn;
    setModeData.INVENC    = mode.EncoderInv;
    setModeData.RESBENC   = mode.ResBEnc;
    setModeData.RESENC    = mode.ResEnc;

    setModeData.SYNCCOUNT = PACK_DWORD ( mode.SyncCount );

    wValue        = FIRST_WORD_SWAPPED  ( reinterpret_cast<uint32_t*>(&setModeData) );
    wIndex        = SECOND_WORD_SWAPPED ( reinterpret_cast<uint32_t*>(&setModeData) );

    // Access lock
    USMC_lock access_lock(_locks[id]);

    int res = libusb_control_transfer(_dev[id], bRequestType, bRequest, wValue, wIndex, reinterpret_cast<uint8_t*>(&setModeData)+4, wLength, _timeout);

    if(res < 0) {
        // Call failed
        _error_logger("Failed to set device mode. Error: %s", libusb_strerror(static_cast<libusb_error>(res)));
        return res;
    }

    return 0;
}

// USB call to set device parameters
int USMC_impl::usmc_set_parameters(int id, const USMC_Parameters& params) {
    uint8_t  bRequestType = LIBUSB_ENDPOINT_OUT     |
                            LIBUSB_RECIPIENT_DEVICE |
                            LIBUSB_REQUEST_TYPE_VENDOR;
    uint8_t  bRequest = 0x83;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength = 0x0035;

    PARAMETERS_PACKET setParametersData;

    /*=====================*/
    /* ----Conversion:---- */
    /*=====================*/
    setParametersData.DELAY1       = ( uint8_t ) clamp ( ( int ) ( params.AccelT / 98.0f + 0.5f ), 1, 15 );
    setParametersData.DELAY2       = ( uint8_t ) clamp ( ( int ) ( params.DecelT / 98.0f + 0.5f ), 1, 15 );
    setParametersData.RefINTimeout = ( uint16_t ) ( clamp ( params.PTimeout , 1.0f, 9961.0f ) / 0.152f + 0.5f );
    setParametersData.BTIMEOUT1    = PACK_WORD ( ( uint16_t ) ( clamp ( params.BTimeout1, 1.0f, 9961.0f ) / 0.152f + 0.5f ) );
    setParametersData.BTIMEOUT2    = PACK_WORD ( ( uint16_t ) ( clamp ( params.BTimeout2, 1.0f, 9961.0f ) / 0.152f + 0.5f ) );
    setParametersData.BTIMEOUT3    = PACK_WORD ( ( uint16_t ) ( clamp ( params.BTimeout3, 1.0f, 9961.0f ) / 0.152f + 0.5f ) );
    setParametersData.BTIMEOUT4    = PACK_WORD ( ( uint16_t ) ( clamp ( params.BTimeout4, 1.0f, 9961.0f ) / 0.152f + 0.5f ) );
    setParametersData.BTIMEOUTR    = PACK_WORD ( ( uint16_t ) ( clamp ( params.BTimeoutR, 1.0f, 9961.0f ) / 0.152f + 0.5f ) );
    setParametersData.BTIMEOUTD    = PACK_WORD ( ( uint16_t ) ( clamp ( params.BTimeoutD, 1.0f, 9961.0f ) / 0.152f + 0.5f ) );
    setParametersData.MINPERIOD    = PACK_WORD ( ( uint16_t ) ( 65536.0f - ( 125000.0f / clamp ( params.MinP , 2.0f, 625.0f ) ) + 0.5f ) );
    setParametersData.BTO1P        = PACK_WORD ( ( uint16_t ) ( 65536.0f - ( 125000.0f / clamp ( params.BTO1P, 2.0f, 625.0f ) ) + 0.5f ) );
    setParametersData.BTO2P        = PACK_WORD ( ( uint16_t ) ( 65536.0f - ( 125000.0f / clamp ( params.BTO2P, 2.0f, 625.0f ) ) + 0.5f ) );
    setParametersData.BTO3P        = PACK_WORD ( ( uint16_t ) ( 65536.0f - ( 125000.0f / clamp ( params.BTO3P, 2.0f, 625.0f ) ) + 0.5f ) );
    setParametersData.BTO4P        = PACK_WORD ( ( uint16_t ) ( 65536.0f - ( 125000.0f / clamp ( params.BTO4P, 2.0f, 625.0f ) ) + 0.5f ) );
    setParametersData.MAX_LOFT     = PACK_WORD ( ( uint16_t ) ( clamp ( params.MaxLoft, 1, 1023 ) * 64 ) );

    if ( _version[id] < 0x2407 ) {
        setParametersData.STARTPOS = 0x00000000L;
    } else {
        setParametersData.STARTPOS = PACK_DWORD ( params.StartPos * 8 & 0xFFFFFF00 );
    }

    setParametersData.RTDelta    = PACK_WORD ( ( uint16_t ) ( clamp ( params.RTDelta   , 4, 1023 ) * 64 ) );
    setParametersData.RTMinError = PACK_WORD ( ( uint16_t ) ( clamp ( params.RTMinError, 4, 1023 ) * 64 ) );

    double t = ( double ) clamp ( params.MaxTemp, 0.0f, 100.0f );

    if ( _version[id] < 0x2400 )
    {
        t = 10.0 * exp ( 3950.0 * ( 1.0 / ( t + 273.0 ) - 1.0 / 298.0 ) );
        t = ( ( 5 * t / ( 10 + t ) ) * 65536.0 / 3.3 + 0.5 );
    }
    else
    {
        t = ( t + 50.0 ) / 330.0 * 65536.0;
        t = ( t + 0.5f );
    }

    setParametersData.MaxTemp    = PACK_WORD ( ( uint16_t ) t );
    setParametersData.SynOUTP    = params.SynOUTP;
    setParametersData.LoftPeriod = params.LoftPeriod == 0.0f ? 0 : PACK_WORD ( ( uint16_t ) ( 65536.0f - ( 125000.0f / clamp ( params.LoftPeriod, 16.0f, 5000.0f ) ) + 0.5f ) );
    setParametersData.EncVSCP    = ( uint8_t ) ( params.EncMult * 4.0f + 0.5f );
    //setParametersData.EncVSCP = 1;

    memset ( setParametersData.Reserved, 0, 15 );

    wValue        = FIRST_WORD_SWAPPED ( reinterpret_cast<uint32_t*>(&setParametersData) );
    wIndex        = SECOND_WORD        ( reinterpret_cast<uint32_t*>(&setParametersData) );

    // Access lock
    USMC_lock access_lock(_locks[id]);

    int res = libusb_control_transfer(_dev[id], bRequestType, bRequest, wValue, wIndex, reinterpret_cast<uint8_t*>(&setParametersData)+4, wLength, _timeout);

    if(res < 0) {
        // Call failed
        _error_logger("Failed to set device parameters. Error: %s", libusb_strerror(static_cast<libusb_error>(res)));
        return res;
    }

    return 0;
}

// USB call to set current position
int USMC_impl::usmc_set_current_position(int id, int32_t position) {
    uint8_t  bRequestType = LIBUSB_ENDPOINT_OUT     |
                            LIBUSB_RECIPIENT_DEVICE |
                            LIBUSB_REQUEST_TYPE_VENDOR;
    uint8_t  bRequest = 0x01;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength = 0;

    position *= 8;
    position &= 0xFFFFFFE0;
    wValue        = SECOND_WORD ( &position );
    wIndex        = FIRST_WORD  ( &position );

    // Access lock
    USMC_lock access_lock(_locks[id]);

    int res = libusb_control_transfer(_dev[id], bRequestType, bRequest, wValue, wIndex, NULL, wLength, _timeout);

    if(res < 0) {
        // Call failed
        _error_logger("Failed to set device current position. Error: %s", libusb_strerror(static_cast<libusb_error>(res)));
        return res;
    }

    return 0;
}

int USMC_impl::usmc_stop(int id) {
    uint8_t  bRequestType = LIBUSB_ENDPOINT_OUT     |
                            LIBUSB_RECIPIENT_DEVICE |
                            LIBUSB_REQUEST_TYPE_VENDOR;
    uint8_t  bRequest = 0x07;
    uint16_t wValue = 0;
    uint16_t wIndex = 0;
    uint16_t wLength = 0;

    // Access lock
    USMC_lock access_lock(_locks[id]);

    int res = libusb_control_transfer(_dev[id], bRequestType, bRequest, wValue, wIndex, NULL, wLength, _timeout);

    if(res < 0) {
        // Call failed
        _error_logger("Failed to stop device. Error: %s", libusb_strerror(static_cast<libusb_error>(res)));
        return res;
    }

    return 0;
}

int USMC_impl::usmc_save(int id) {
    uint8_t  bRequestType = LIBUSB_ENDPOINT_OUT     |
                            LIBUSB_RECIPIENT_DEVICE |
                            LIBUSB_REQUEST_TYPE_VENDOR;
    uint8_t  bRequest = 0x84; // Dec: 132
    uint16_t wValue = 0;
    uint16_t wIndex = 0;
    uint16_t wLength = 0;

    // Access lock
    USMC_lock access_lock(_locks[id]);

    int res = libusb_control_transfer(_dev[id], bRequestType, bRequest, wValue, wIndex, NULL, wLength, _timeout);

    if(res < 0) {
        // Call failed
        _error_logger("Failed to save parameters to EEPROM. Error: %s", libusb_strerror(static_cast<libusb_error>(res)));
        return res;
    }

    return 0;
}

void USMC_impl::initDefaults(int id) {
    // Reset structures
    memset(_mode[id], 0, sizeof(USMC_Mode));
    memset(_params[id], 0, sizeof(USMC_Parameters));
    memset(_start_params[id], 0, sizeof(USMC_StartParameters));

    // USMC_Mode defaults:
    _mode[id]->PReg      = true;
    _mode[id]->Tr1En     = true;
    _mode[id]->Tr2En     = true;
    _mode[id]->RotTrOp   = true;
    _mode[id]->SyncOUTEn = true;
    _mode[id]->SyncINOp  = true;
    _mode[id]->SyncCount = 4;

    // USMC_Parameters defaults:
    _params[id]->MaxTemp    = 70.0f;
    _params[id]->AccelT     = 200.0f;
    _params[id]->DecelT     = 200.0f;
    _params[id]->BTimeout1  = 500.0f;
    _params[id]->BTimeout2  = 500.0f;
    _params[id]->BTimeout3  = 500.0f;
    _params[id]->BTimeout4  = 500.0f;
    _params[id]->BTO1P      = 200.0f;
    _params[id]->BTO2P      = 300.0f;
    _params[id]->BTO3P      = 400.0f;
    _params[id]->BTO4P      = 500.0f;
    _params[id]->MinP       = 500.0f;
    _params[id]->BTimeoutR  = 500.0f;
    _params[id]->LoftPeriod = 32.0f;
    _params[id]->RTDelta    = 200;
    _params[id]->RTMinError = 15;
    _params[id]->EncMult    = 2.5f;
    _params[id]->MaxLoft    = 32;
    _params[id]->PTimeout   = 100.0f;
    _params[id]->SynOUTP    = 1;
    _params[id]->StartPos   = 0;

    // USMC_StartParameters defaults:
    _start_params[id]->SDivisor = 8;
    _start_params[id]->LoftEn   = true;
    _start_params[id]->SlStart  = true;
}
