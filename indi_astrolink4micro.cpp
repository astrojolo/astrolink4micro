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
