/*******************************************************************************
 Copyright(c) 2024 astrojolo.com
 .
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.
 .
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.
 .
 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/
#include "indi_astrolink4micro.h"
#include "indicom.h"
#include "connectionplugins/connectionserial.h"

#define VERSION_MAJOR 0
#define VERSION_MINOR 2

#define ASTROLINK4_LEN 250
#define ASTROLINK4_TIMEOUT 3

#define POLLTIME 500

#include <memory>

/**************************************************************************************
** Initialization stuff
***************************************************************************************/
std::unique_ptr<AstroLink4micro> astroLink4micro(new AstroLink4micro());

AstroLink4micro::AstroLink4micro() : FI(this), WI(this)
{
    setVersion(VERSION_MAJOR, VERSION_MINOR);
}

/**************************************************************************************
** Init / update properties
***************************************************************************************/
bool AstroLink4micro::initProperties() 
{
    INDI::DefaultDevice::initProperties();     
    setDriverInterface(AUX_INTERFACE | FOCUSER_INTERFACE | WEATHER_INTERFACE);  
    
    FI::SetCapability(FOCUSER_CAN_ABS_MOVE |
                      FOCUSER_CAN_REL_MOVE |
                      FOCUSER_CAN_REVERSE  |
                      FOCUSER_CAN_SYNC     |
                      FOCUSER_CAN_ABORT    |
                      FOCUSER_HAS_BACKLASH);

    FI::initProperties(FOCUS_TAB);
    WI::initProperties(ENVIRONMENT_TAB, ENVIRONMENT_TAB);

    addAuxControls();     

    serialConnection = new Connection::Serial(this);
    serialConnection->registerHandshake([&]()
    {
        return Handshake();
    });
    registerConnection(serialConnection);
    
    // Power lines
    IUFillSwitch(&Power1S[0], "PWR1BTN_ON", "ON", ISS_OFF);
    IUFillSwitch(&Power1S[1], "PWR1BTN_OFF", "OFF", ISS_ON);
    IUFillSwitchVector(&Power1SP, Power1S, 2, getDeviceName(), "DC1", "Port 1", POWER_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    IUFillSwitch(&Power2S[0], "PWR2BTN_ON", "ON", ISS_OFF);
    IUFillSwitch(&Power2S[1], "PWR2BTN_OFF", "OFF", ISS_ON);
    IUFillSwitchVector(&Power2SP, Power2S, 2, getDeviceName(), "DC2", "Port 2", POWER_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    IUFillSwitch(&Power3S[0], "PWR3BTN_ON", "ON", ISS_OFF);
    IUFillSwitch(&Power3S[1], "PWR3BTN_OFF", "OFF", ISS_ON);
    IUFillSwitchVector(&Power3SP, Power3S, 2, getDeviceName(), "DC3", "Port 3", POWER_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    IUFillNumber(&PWMN[0], "PWM1_VAL", "A", "%3.0f", 0, 100, 10, 0);
    IUFillNumber(&PWMN[1], "PWM2_VAL", "B", "%3.0f", 0, 100, 10, 0);
    IUFillNumberVector(&PWMNP, PWMN, 2, getDeviceName(), "PWM", "PWM", POWER_TAB, IP_RW, 60, IPS_IDLE);

    IUFillSwitch(&PowerDefaultOnS[0], "POW_DEF_ON1", "DC1", ISS_OFF);
    IUFillSwitch(&PowerDefaultOnS[1], "POW_DEF_ON2", "DC2", ISS_OFF);
    IUFillSwitch(&PowerDefaultOnS[2], "POW_DEF_ON3", "DC3", ISS_OFF);
    IUFillSwitchVector(&PowerDefaultOnSP, PowerDefaultOnS, 3, getDeviceName(), "POW_DEF_ON", "Power default ON", POWER_TAB, IP_RW, ISR_NOFMANY, 60, IPS_IDLE);    
    
    // focuser settings
    IUFillNumber(&Focuser1SettingsN[FS1_SPEED], "FS1_SPEED", "Speed [pps]", "%.0f", 10, 200, 1, 100);
    IUFillNumber(&Focuser1SettingsN[FS1_CURRENT], "FS1_CURRENT", "Current [mA]", "%.0f", 100, 2000, 100, 400);
    IUFillNumber(&Focuser1SettingsN[FS1_HOLD], "FS1_HOLD", "Hold torque [%]", "%.0f", 0, 100, 10, 0);
    IUFillNumber(&Focuser1SettingsN[FS1_STEP_SIZE], "FS1_STEP_SIZE", "Step size [um]", "%.2f", 0, 100, 0.1, 5.0);
    IUFillNumber(&Focuser1SettingsN[FS1_COMPENSATION], "FS1_COMPENSATION", "Compensation [steps/C]", "%.2f", -1000, 1000, 1, 0);
    IUFillNumber(&Focuser1SettingsN[FS1_COMP_THRESHOLD], "FS1_COMP_THRESHOLD", "Compensation threshold [steps]", "%.0f", 1, 1000, 10, 10);
    IUFillNumberVector(&Focuser1SettingsNP, Focuser1SettingsN, 6, getDeviceName(), "FOCUSER1_SETTINGS", "Focuser 1 settings", SETTINGS_TAB, IP_RW, 60, IPS_IDLE);    
    
    IUFillSwitch(&Focuser1ModeS[FS1_MODE_UNI], "FS1_MODE_UNI", "Unipolar", ISS_ON);
    IUFillSwitch(&Focuser1ModeS[FS1_MODE_MICRO_L], "FS1_MODE_MICRO_L", "Microstep 1/8", ISS_OFF);
    IUFillSwitch(&Focuser1ModeS[FS1_MODE_MICRO_H], "FS1_MODE_MICRO_H", "Microstep 1/32", ISS_OFF);
    IUFillSwitchVector(&Focuser1ModeSP, Focuser1ModeS, 3, getDeviceName(), "FOCUSER1_MODE", "Focuser mode", SETTINGS_TAB, IP_RW, ISR_1OFMANY, 60, IPS_IDLE);    
    
    // Power readings
    IUFillNumber(&PowerDataN[POW_VIN], "VIN", "Input voltage [V]", "%.1f", 0, 15, 10, 0);
    IUFillNumber(&PowerDataN[POW_ITOT], "ITOT", "Total current [A]", "%.1f", 0, 15, 10, 0);
    IUFillNumber(&PowerDataN[POW_AH], "AH", "Energy consumed [Ah]", "%.1f", 0, 1000, 10, 0);
    IUFillNumber(&PowerDataN[POW_WH], "WH", "Energy consumed [Wh]", "%.1f", 0, 10000, 10, 0);
    IUFillNumberVector(&PowerDataNP, PowerDataN, 4, getDeviceName(), "POWER_DATA", "Power data", POWER_TAB, IP_RO, 60, IPS_IDLE);
    

    return true;    
}

bool AstroLink4micro::updateProperties()
{
    // Call parent update properties first
    INDI::DefaultDevice::updateProperties();

    if (isConnected())
    {
        FI::updateProperties();
        WI::updateProperties();
        defineProperty(&Focuser1SettingsNP);
        defineProperty(&Focuser1ModeSP);
        defineProperty(&Power1SP);
        defineProperty(&Power2SP);
        defineProperty(&Power3SP);
        defineProperty(&PWMNP);        
        defineProperty(&PowerDataNP);       
        defineProperty(&PowerDefaultOnSP); 
    }
    else
    {
        deleteProperty(PowerDataNP.name);
        deleteProperty(Focuser1ModeSP.name);
        deleteProperty(Focuser1SettingsNP.name);
        deleteProperty(Power1SP.name);
        deleteProperty(Power2SP.name);
        deleteProperty(Power3SP.name);
        deleteProperty(PWMNP.name);      
        deleteProperty(PowerDefaultOnSP.name); 
        WI::updateProperties();
        FI::updateProperties();        
    }
    return true;
}

/**************************************************************************************
** Client is asking us to establish connection to the device
***************************************************************************************/
bool AstroLink4micro::Handshake()
{
    PortFD = serialConnection->getPortFD();

    char res[ASTROLINK4_LEN] = {0};
    if (sendCommand("#", res))
    {
        if (strncmp(res, "#:AstroLink4mini", 16) != 0)
        {
            DEBUG(INDI::Logger::DBG_ERROR, "Device not recognized.");
            return false;
        }
        else
        {
            DEBUG(INDI::Logger::DBG_DEBUG, "Handshake success");
            //~ SetTimer(POLLTIME);
            return true;
        }
    }
    return false;
}

bool AstroLink4micro::sendCommand(const char *cmd, char *res)
{
    int nbytes_read = 0, nbytes_written = 0, tty_rc = 0;
    char command[ASTROLINK4_LEN];
    
    tcflush(PortFD, TCIOFLUSH);
    sprintf(command, "%s\n", cmd);
    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD %s", cmd);
    if ((tty_rc = tty_write_string(PortFD, command, &nbytes_written)) != TTY_OK)
        return false;

    if (!res)
    {
        tcflush(PortFD, TCIOFLUSH);
        return true;
    }

    if ((tty_rc = tty_nread_section(PortFD, res, ASTROLINK4_LEN, stopChar, ASTROLINK4_TIMEOUT, &nbytes_read)) != TTY_OK || nbytes_read == 1)
        return false;

    tcflush(PortFD, TCIOFLUSH);
    res[nbytes_read - 1] = '\0';
    DEBUGF(INDI::Logger::DBG_DEBUG, "RES %s", res);
    if (tty_rc != TTY_OK)
    {
        char errorMessage[MAXRBUF];
        tty_error_msg(tty_rc, errorMessage, MAXRBUF);
        LOGF_ERROR("Serial error: %s", errorMessage);
        return false;
    }
            
    return (cmd[0] == res[0]);
}

/**************************************************************************************
** INDI is asking us for our default device name
***************************************************************************************/
const char *AstroLink4micro::getDefaultName()
{
    return "AstroLink 4 micro";
}

/**************************************************************************************
** Helper functions
***************************************************************************************/
std::vector<std::string> AstroLink4micro::split(const std::string &input, const std::string &regex)
{
    // passing -1 as the submatch index parameter performs splitting
    std::regex re(regex);
    std::sregex_token_iterator
        first{input.begin(), input.end(), re, -1},
        last;
    return {first, last};
}
