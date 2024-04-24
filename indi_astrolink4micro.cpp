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


std::unique_ptr<AstroLink4micro> al4micro(new AstroLink4micro());

AstroLink4micro::AstroLink4micro() : FI(this), WI(this)
{
    setVersion(VERSION_MAJOR, VERSION_MINOR);
    lastQuery.reserve(250);
}

//////////////////////////////////////////////////////////////////////
/// Communication
//////////////////////////////////////////////////////////////////////
void ISPoll(void *p);

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
            SetTimer(POLLTIME);
            return true;
        }
    }
    return false;
}

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

    // Power readings
    IUFillNumber(&PowerDataN[POW_VIN], "VIN", "Input voltage [V]", "%.1f", 0, 15, 10, 0);
    IUFillNumber(&PowerDataN[POW_REG], "REG", "Regulated voltage [V]", "%.1f", 0, 15, 10, 0);
    IUFillNumber(&PowerDataN[POW_ITOT], "ITOT", "Total current [A]", "%.1f", 0, 15, 10, 0);
    IUFillNumber(&PowerDataN[POW_AH], "AH", "Energy consumed [Ah]", "%.1f", 0, 1000, 10, 0);
    IUFillNumber(&PowerDataN[POW_WH], "WH", "Energy consumed [Wh]", "%.1f", 0, 10000, 10, 0);
    IUFillNumberVector(&PowerDataNP, PowerDataN, 4, getDeviceName(), "POWER_DATA", "Power data", POWER_TAB, IP_RO, 60, IPS_IDLE);

    FI::initProperties(FOCUS_TAB);
    WI::initProperties(ENVIRONMENT_TAB, ENVIRONMENT_TAB);    

    addDebugControl();
    addSimulationControl();
    addConfigurationControl();
    
    serialConnection = new Connection::Serial(this);
    serialConnection->registerHandshake([&]()                                    
    { 
        return Handshake(); 
    });
    registerConnection(serialConnection);

    serialConnection->setDefaultPort("/dev/ttyUSB0");
    serialConnection->setDefaultBaudRate(serialConnection->B_38400);    

    return true;
}

const char *AstroLink4micro::getDefaultName()
{
    return (char *)"AstroLink 4 micro";
}

bool AstroLink4micro::updateProperties()
{
    INDI::DefaultDevice::updateProperties();

    FI::updateProperties();
    WI::updateProperties();

    return true;
}

bool AstroLink4micro::ISNewSwitch(const char * dev, const char * name, ISState * states, char * names[], int n)
{
    return true;
}


bool AstroLink4micro::ISNewNumber(const char * dev, const char * name, double values[], char * names[], int n)
{
    return true;
}


bool AstroLink4micro::ISNewText(const char * dev, const char * name, char * texts[], char * names[], int n)
{
    return true;
}

bool AstroLink4micro::saveConfigItems(FILE *fp)
{
    //~ IUSaveConfigSwitch(fp, &FocuserSelectSP);
    FI::saveConfigItems(fp);
    INDI::DefaultDevice::saveConfigItems(fp);
    return true;
}

bool AstroLink4micro::loadConfig(bool silent, const char *property)
{
    bool result = INDI::DefaultDevice::loadConfig(silent, property);
    DEBUG(INDI::Logger::DBG_DEBUG, "Init complete");
    //~ initComplete = true;

    return result;
}

//////////////////////////////////////////////////////////////////////
/// Serial commands
//////////////////////////////////////////////////////////////////////
bool AstroLink4micro::sendCommand(const char *cmd, char *res)
{
    int nbytes_read = 0, nbytes_written = 0, tty_rc = 0;
    char command[ASTROLINK4_LEN];

    if (isSimulation())
    {
        if (strncmp(cmd, "#", 1) == 0)
            sprintf(res, "%s\n", "#:AstroLink4mini");
        if (strncmp(cmd, "q", 1) == 0)
            sprintf(res, "%s\n", "q:AL4MIC:4671:0:0:0:0.43:1:23.1:45.4:9.7:1:13.3:40:31:0:0:1:12.1:7.9:12.1:144.7:0:0:0:0:1:-4.1:18.5:1:34.1:88.9:4.7:1:19.77");
        if (strncmp(cmd, "p", 1) == 0)
            sprintf(res, "%s\n", "p:1234");
        if (strncmp(cmd, "i", 1) == 0)
            sprintf(res, "%s\n", "i:0");
        if (strncmp(cmd, "u", 1) == 0)
            sprintf(res, "%s\n", "u:1:1:80:120:30:50:200:800:200:800:0:2:10000:80000:0:0:50:18:30:15:5:10:10:0:1:0:0:0:0:0:0:0:40:90:10:1100:14000:10000:100:0");
        if (strncmp(cmd, "A", 1) == 0)
            sprintf(res, "%s\n", "A:4.8.1 micro");
        if (strncmp(cmd, "R", 1) == 0)
            sprintf(res, "%s\n", "R:");
        if (strncmp(cmd, "C", 1) == 0)
            sprintf(res, "%s\n", "C:");
        if (strncmp(cmd, "B", 1) == 0)
            sprintf(res, "%s\n", "B:");
        if (strncmp(cmd, "H", 1) == 0)
            sprintf(res, "%s\n", "H:");
        if (strncmp(cmd, "P", 1) == 0)
            sprintf(res, "%s\n", "P:");
        if (strncmp(cmd, "U", 1) == 0)
            sprintf(res, "%s\n", "U:");
        if (strncmp(cmd, "S", 1) == 0)
            sprintf(res, "%s\n", "S:");
    }
    else
    {
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
    }
    return (cmd[0] == res[0]);
}


std::vector<std::string> AstroLink4micro::split(const std::string &input, const std::string &regex)
{
    // passing -1 as the submatch index parameter performs splitting
    std::regex re(regex);
    std::sregex_token_iterator
        first{input.begin(), input.end(), re, -1},
        last;
    return {first, last};
}
