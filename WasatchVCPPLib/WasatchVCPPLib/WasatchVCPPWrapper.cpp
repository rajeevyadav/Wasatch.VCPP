/**
    @file   WasatchVCPPWrapper.cpp
    @author Mark Zieg <mzieg@wasatchphotonics.com>
    @brief  Implementation of the flattened C API exported by WasatchVCPP.dll
    @note   customers normally wouldn't access this file; use WasatchVCPP.h instead

    This file implements the "C" interface to the WasatchVCPP library and DLL
    which is defined in WasatchCPP.h.

    This file is the one and only place where the WasatchVCPP::Driver Singleton
    is actually instantiated (meaning it probably doesn't actually have to be
    a Singleton...)
*/

#include "pch.h"
#include "WasatchVCPP.h"

#include <string>
#include <map>

#include "Util.h"
#include "Driver.h"
#include "Spectrometer.h"

using WasatchVCPP::Util;
using WasatchVCPP::Driver;
using WasatchVCPP::Spectrometer;

using std::string;
using std::map;

////////////////////////////////////////////////////////////////////////////////
// globals
////////////////////////////////////////////////////////////////////////////////

Driver* driver = Driver::getInstance();

////////////////////////////////////////////////////////////////////////////////
// helper functions
////////////////////////////////////////////////////////////////////////////////

//! copy a std::string to a C string
//!
//! @param s (Input) a populated std::string
//! @param buf (Output) the destination where the string is to be copied
//! @param len (Input) allocated size of 'buf'
//! @returns WP_SUCCESS or WP_ERROR_INSUFFICIENT_STORAGE
int exportString(string s, char* buf, int len)
{
    memset(buf, 0, len);
    for (int i = 0; i < s.size(); i++)
        if (i < len)
            buf[i] = s[i];
        else
            return WP_ERROR_INSUFFICIENT_STORAGE;

    return WP_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// Lifecycle
////////////////////////////////////////////////////////////////////////////////

int wp_open_all_spectrometers()
{
    return driver->openAllSpectrometers();
}

int wp_close_all_spectrometers()
{
    for (int i = 0; i < driver->getNumberOfSpectrometers(); i++)
        wp_close_spectrometer(i); // ignore errors
    return WP_SUCCESS;
}

int wp_close_spectrometer(int specIndex)
{
    auto spec = driver->getSpectrometer(specIndex);
    if (spec == nullptr)
        return WP_ERROR_INVALID_SPECTROMETER;

    return spec->close();
}

////////////////////////////////////////////////////////////////////////////////
// Gettors
////////////////////////////////////////////////////////////////////////////////

int wp_get_library_version(char* value, int len)
{ return exportString(WasatchVCPP::libraryVersion, buf, len); }

int wp_get_number_of_spectrometers()
{
    return driver->getNumberOfSpectrometers();
}

int wp_get_pixels(int specIndex)
{
    auto spec = driver->getSpectrometer(specIndex);
    if (spec == nullptr)
        return WP_ERROR_INVALID_SPECTROMETER;
    return spec->pixels;
}

int wp_get_model(int specIndex, char* value, int len)
{
    auto spec = driver->getSpectrometer(specIndex);
    if (spec == nullptr)
        return WP_ERROR_INVALID_SPECTROMETER;
    return exportString(spec->eeprom.model, value, len);
}

int wp_get_serial_number(int specIndex, char* value, int len)
{
    auto spec = driver->getSpectrometer(specIndex);
    if (spec == nullptr)
        return WP_ERROR_INVALID_SPECTROMETER;
    return exportString(spec->eeprom.serialNumber, value, len);
}

int wp_get_wavelengths(int specIndex, double* wavelengths, int len)
{
    auto spec = driver->getSpectrometer(specIndex);
    if (spec == nullptr)
        return WP_ERROR_INVALID_SPECTROMETER;

    for (int i = 0; i < spec->wavelengths.size(); i++)
        if (i < len)
            wavelengths[i] = spec->wavelengths[i];
        else
            return WP_ERROR_INSUFFICIENT_STORAGE;

    return WP_SUCCESS;
}

int wp_get_wavenumbers(int specIndex, double* wavenumbers, int len)
{
    auto spec = driver->getSpectrometer(specIndex);
    if (spec == nullptr)
        return WP_ERROR_INVALID_SPECTROMETER;

    if (spec->eeprom.excitationNM <= 0)
        return WP_ERROR_NO_LASER;

    for (int i = 0; i < spec->wavenumbers.size(); i++)
        if (i < len)
            wavenumbers[i] = spec->wavenumbers[i];
        else
            return WP_ERROR_INSUFFICIENT_STORAGE;

    return WP_SUCCESS;
}

int wp_get_spectrum(int specIndex, double* spectrum, int len)
{
    auto spec = driver->getSpectrometer(specIndex);
    if (spec == nullptr)
        return WP_ERROR_INVALID_SPECTROMETER;

    auto intensities = spec->getSpectrum();
    if (intensities.empty())
        return WP_ERROR;

    for (int i = 0; i < intensities.size(); i++)
        if (i < len)
            spectrum[i] = intensities[i];
        else
            return WP_ERROR_INSUFFICIENT_STORAGE;

    return WP_SUCCESS;
}

int wp_get_eeprom_field_count(int specIndex)
{
    auto spec = driver->getSpectrometer(specIndex);
    if (spec == nullptr)
        return WP_ERROR_INVALID_SPECTROMETER;

    return (int)spec->eeprom.stringified.size();
}

int wp_get_eeprom(int specIndex, const char** names, const char** values, int len)
{
    auto spec = driver->getSpectrometer(specIndex);
    if (spec == nullptr)
        return WP_ERROR_INVALID_SPECTROMETER;
    
    int count = 0;
    const map<string, string>& entries = spec->eeprom.stringified;
    for (map<string, string>::const_iterator i = entries.begin(); i != entries.end(); i++, count++)
    {
        if (count >= len)
            return WP_ERROR_INSUFFICIENT_STORAGE;

        const string& name = i->first;
        const string& value = i->second;

        names[count] = name.c_str();
        values[count] = value.c_str();
    }

    return WP_SUCCESS;
}

int wp_get_eeprom_field(int specIndex, const char* name, char* valueOut, int len)
{
    auto spec = driver->getSpectrometer(specIndex);
    if (spec == nullptr)
        return WP_ERROR_INVALID_SPECTROMETER;

    string lc = Util::toLower(name);

    const map<string, string>& entries = spec->eeprom.stringified;
    for (map<string, string>::const_iterator i = entries.begin(); i != entries.end(); i++)
    {
        const string& name = i->first;
        const string& value = i->second;

        if (lc == Util::toLower(name))
        {
            strncpy_s(valueOut, len, value.c_str(), value.size());
            return WP_SUCCESS;
        }
    }

    // field was not found
    return WP_ERROR;
}

////////////////////////////////////////////////////////////////////////////////
// Settors
////////////////////////////////////////////////////////////////////////////////

int wp_set_logfile_path(const char* pathname)
{
    if (!driver->setLogfile(pathname))
        return WP_ERROR;

    return WP_SUCCESS;
}

int wp_set_integration_time_ms(int specIndex, unsigned long ms)
{
    auto spec = driver->getSpectrometer(specIndex);
    if (spec == nullptr)
        return WP_ERROR_INVALID_SPECTROMETER;

    if (!spec->setIntegrationTimeMS(ms))
        return WP_ERROR;

    return WP_SUCCESS;
}

int wp_set_laser_enable(int specIndex, int value)
{
    auto spec = driver->getSpectrometer(specIndex);
    if (spec == nullptr)
        return WP_ERROR_INVALID_SPECTROMETER;

    if (!spec->setLaserEnable(value))
        return WP_ERROR;

    return WP_SUCCESS;
}

int wp_set_detector_gain(int specIndex, float value)
{
    auto spec = driver->getSpectrometer(specIndex);
    if (spec == nullptr)
        return WP_ERROR_INVALID_SPECTROMETER;

    if (!spec->setDetectorGain(value))
        return WP_ERROR;

    return WP_SUCCESS;
}

int wp_set_detector_gain_odd(int specIndex, float value)
{
    auto spec = driver->getSpectrometer(specIndex);
    if (spec == nullptr)
        return WP_ERROR_INVALID_SPECTROMETER;

    if (!spec->setDetectorGainOdd(value))
        return WP_ERROR;

    return WP_SUCCESS;
}

int wp_set_detector_offset(int specIndex, int16_t value)
{
    auto spec = driver->getSpectrometer(specIndex);
    if (spec == nullptr)
        return WP_ERROR_INVALID_SPECTROMETER;

    if (!spec->setDetectorOffset(value))
        return WP_ERROR;

    return WP_SUCCESS;
}

int wp_set_detector_offset_odd(int specIndex, int16_t value)
{
    auto spec = driver->getSpectrometer(specIndex);
    if (spec == nullptr)
        return WP_ERROR_INVALID_SPECTROMETER;

    if (!spec->setDetectorOffsetOdd(value))
        return WP_ERROR;

    return WP_SUCCESS;
}

int wp_set_tec_enable(int specIndex, int value);
{
    auto spec = driver->getSpectrometer(specIndex);
    if (spec == nullptr)
        return WP_ERROR_INVALID_SPECTROMETER;

    if (!spec->setTECEnable(value != 0))
        return WP_ERROR;

    return WP_SUCCESS;
}

int wp_set_detector_tec_setpoint_deg_c(int specIndex, int value);
{
    auto spec = driver->getSpectrometer(specIndex);
    if (spec == nullptr)
        return WP_ERROR_INVALID_SPECTROMETER;

    if (!spec->setTECSetpointDegC(value))
        return WP_ERROR;

    return WP_SUCCESS;
}

int wp_set_high_gain_mode(int specIndex, int value);
{
    auto spec = driver->getSpectrometer(specIndex);
    if (spec == nullptr)
        return WP_ERROR_INVALID_SPECTROMETER;

    if (!spec->setHighGainMode(value != 0))
        return WP_ERROR;

    return WP_SUCCESS;
}

int wp_get_firmware_version(int specIndex, char* value, int len);
{
    auto spec = driver->getSpectrometer(specIndex);
    if (spec == nullptr)
        return WP_ERROR_INVALID_SPECTROMETER;

    auto s = spec->getFirmwareVersion();
    if (s.empty())
        return WP_ERROR;

    return exportString(s, value, len);
}

int wp_get_fpga_version(int specIndex, char* value, int len);
{
    auto spec = driver->getSpectrometer(specIndex);
    if (spec == nullptr)
        return WP_ERROR_INVALID_SPECTROMETER;

    auto s = spec->getFPGAVersion();
    if (s.empty()) 
        return WP_ERROR;

    return exportString(s, value, len);
}

float wp_get_detector_temperature_deg_c(int specIndex)
{
    auto spec = driver->getSpectrometer(specIndex);
    if (spec == nullptr)
        return -999;

    return spec->getDetectorTemperatureDegC();
}
