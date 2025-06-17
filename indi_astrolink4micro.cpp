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

#define POLL_PERIOD 500

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
                      FOCUSER_CAN_ABORT);

    FI::initProperties(FOCUS_TAB);
    WI::initProperties(ENVIRONMENT_TAB, ENVIRONMENT_TAB);

    addAuxControls();     

    serialConnection = new Connection::Serial(this);
    serialConnection->registerHandshake([&]()
    {
        return Handshake();
    });
    registerConnection(serialConnection);
    
    serialConnection->setDefaultPort("/dev/ttyUSB0");
    serialConnection->setDefaultBaudRate(serialConnection->B_38400);
    
    // focuser settings
    IUFillNumber(&Focuser1SettingsN[FS1_SPEED], "FS1_SPEED", "Speed [pps]", "%.0f", 10, 200, 1, 100);
    IUFillNumber(&Focuser1SettingsN[FS1_CURRENT], "FS1_CURRENT", "Current [mA]", "%.0f", 100, 2000, 100, 400);
    IUFillNumber(&Focuser1SettingsN[FS1_HOLD], "FS1_HOLD", "Hold torque [%]", "%.0f", 0, 100, 10, 0);
    IUFillNumber(&Focuser1SettingsN[FS1_STEP_SIZE], "FS1_STEP_SIZE", "Step size [um]", "%.2f", 0, 100, 0.1, 5.0);
    IUFillNumber(&Focuser1SettingsN[FS1_COMPENSATION], "FS1_COMPENSATION", "Compensation [steps/C]", "%.2f", -1000, 1000, 1, 0);
    IUFillNumber(&Focuser1SettingsN[FS1_COMP_THRESHOLD], "FS1_COMP_THRESHOLD", "Compensation threshold [steps]", "%.0f", 1, 1000, 10, 10);
    IUFillNumberVector(&Focuser1SettingsNP, Focuser1SettingsN, 6, getDeviceName(), "FOCUSER1_SETTINGS", "Focuser settings", SETTINGS_TAB, IP_RW, 60, IPS_IDLE);    
    
    IUFillSwitch(&Focuser1ModeS[FS1_MODE_UNI], "FS1_MODE_UNI", "Unipolar", ISS_ON);
    IUFillSwitch(&Focuser1ModeS[FS1_MODE_MICRO_L], "FS1_MODE_MICRO_L", "Microstep 1/8", ISS_OFF);
    IUFillSwitch(&Focuser1ModeS[FS1_MODE_MICRO_H], "FS1_MODE_MICRO_H", "Microstep 1/32", ISS_OFF);
    IUFillSwitchVector(&Focuser1ModeSP, Focuser1ModeS, 3, getDeviceName(), "FOCUSER1_MODE", "Focuser mode", SETTINGS_TAB, IP_RW, ISR_1OFMANY, 60, IPS_IDLE);    
    
    // Power readings
    IUFillNumber(&PowerDataN[POW_VIN], "VIN", "Input voltage [V]", "%.1f", 0, 15, 10, 0);
    IUFillNumber(&PowerDataN[POW_ITOT], "ITOT", "Total current [A]", "%.2f", 0, 15, 10, 0);
    IUFillNumber(&PowerDataN[POW_AH], "AH", "Energy consumed [Ah]", "%.2f", 0, 1000, 10, 0);
    IUFillNumber(&PowerDataN[POW_WH], "WH", "Energy consumed [Wh]", "%.2f", 0, 10000, 10, 0);
    IUFillNumberVector(&PowerDataNP, PowerDataN, 4, getDeviceName(), "POWER_DATA", "Power data", POWER_TAB, IP_RO, 60, IPS_IDLE);
    
	IUFillText(&RelayLabelsT[LAB_OUT1], "LAB_OUT1", "OUT 1", "OUT 1");
	IUFillText(&RelayLabelsT[LAB_OUT2], "LAB_OUT2", "OUT 2", "OUT 2");
	IUFillText(&RelayLabelsT[LAB_OUT3], "LAB_OUT3", "OUT 3", "OUT 3");
	IUFillText(&RelayLabelsT[LAB_PWM1], "LAB_PWM1", "PWM 1", "PWM 1");
	IUFillText(&RelayLabelsT[LAB_PWM2], "LAB_PWM2", "PWM 2", "PWM 2");
	IUFillTextVector(&RelayLabelsTP, RelayLabelsT, 5, getDeviceName(), "RELAYLABELS", "Relay Labels", OPTIONS_TAB, IP_RW, 60, IPS_IDLE);    
	IUFillNumber(&SQMOffsetN[0], "SQMOffset", "mag/arcsec2", "%0.2f", -1, 1, 0.01, 0);
	IUFillNumberVector(&SQMOffsetNP, SQMOffsetN, 1, getDeviceName(), "SQMOFFSET", "SQM calibration", OPTIONS_TAB, IP_RW, 60, IPS_IDLE);    
    
	// Load options before connecting
	// load config before defining switches
	defineProperty(&RelayLabelsTP);
	loadConfig();
        
	IUFillSwitch(&Switch1S[S1_ON], "S1_ON", "ON", ISS_OFF);
	IUFillSwitch(&Switch1S[S1_OFF], "S1_OFF", "OFF", ISS_ON);
	IUFillSwitchVector(&Switch1SP, Switch1S, 2, getDeviceName(), "SWITCH_1", RelayLabelsT[LAB_OUT1].text, POWER_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

	IUFillSwitch(&Switch2S[S2_ON], "S2_ON", "ON", ISS_OFF);
	IUFillSwitch(&Switch2S[S2_OFF], "S2_OFF", "OFF", ISS_ON);
	IUFillSwitchVector(&Switch2SP, Switch2S, 2, getDeviceName(), "SWITCH_2", RelayLabelsT[LAB_OUT2].text, POWER_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

	IUFillSwitch(&Switch3S[S3_ON], "S3_ON", "ON", ISS_OFF);
	IUFillSwitch(&Switch3S[S3_OFF], "S3_OFF", "OFF", ISS_ON);
	IUFillSwitchVector(&Switch3SP, Switch3S, 2, getDeviceName(), "SWITCH_3", RelayLabelsT[LAB_OUT3].text, POWER_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

	IUFillNumber(&PWM1N[0], "PWMout1", "%", "%0.0f", 0, 100, 10, 0);
	IUFillNumberVector(&PWM1NP, PWM1N, 1, getDeviceName(), "PWMOUT1", RelayLabelsT[LAB_PWM1].text, POWER_TAB, IP_RW, 60, IPS_IDLE);

	IUFillNumber(&PWM2N[0], "PWMout2", "%", "%0.0f", 0, 100, 10, 0);
	IUFillNumberVector(&PWM2NP, PWM2N, 1, getDeviceName(), "PWMOUT2", RelayLabelsT[LAB_PWM2].text, POWER_TAB, IP_RW, 60, IPS_IDLE);    
    
    // Environment Group
	addParameter("WEATHER_TEMPERATURE", "Temperature [C]", -15, 35, 15);
	addParameter("WEATHER_HUMIDITY", "Humidity %", 0, 100, 15);
	addParameter("WEATHER_DEWPOINT", "Dew Point [C]", -25, 20, 15);
	addParameter("WEATHER_SKY_TEMP", "Sky temperature [C]", -50, 20, 20);
	addParameter("WEATHER_SKY_DIFF", "Temperature difference [C]", -5, 40, 10);
	addParameter("SQM_READING", "Sky brightness [mag/arcsec2]", 10, 25, 15);

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
		defineProperty(&PWM1NP);
		defineProperty(&PWM2NP);  
		defineProperty(&Switch1SP);
		defineProperty(&Switch2SP);            
		defineProperty(&Switch3SP);            
        defineProperty(&PowerDataNP);   
        defineProperty(&SQMOffsetNP);    
    }
    else
    {
        deleteProperty(SQMOffsetNP.name);
        deleteProperty(PowerDataNP.name);
        deleteProperty(Focuser1ModeSP.name);
        deleteProperty(Focuser1SettingsNP.name);
		deleteProperty(Switch1SP.name);
		deleteProperty(Switch2SP.name);
		deleteProperty(Switch3SP.name);
		deleteProperty(PWM1NP.name);
		deleteProperty(PWM2NP.name);
        //~ deleteProperty(RelayLabelsTP.name);
        WI::updateProperties();
        FI::updateProperties();        
    }
    return true;
}

/**************************************************************************************
** INDI properties updates
***************************************************************************************/

bool AstroLink4micro::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev && !strcmp(dev, getDeviceName()))
    {
        char cmd[ASTROLINK4_LEN] = {0};
        char res[ASTROLINK4_LEN] = {0};
        
        // Handle PWM
        if (!strcmp(name, PWM1NP.name))
        {
            bool allOk = true;
            if (PWM1N[0].value != values[0])
            {
                sprintf(cmd, "B:0:%d", static_cast<uint8_t>(values[0]));
                allOk = allOk && sendCommand(cmd, res);
            }
            PWM1NP.s = (allOk) ? IPS_BUSY : IPS_ALERT;
            if (allOk)
                IUUpdateNumber(&PWM1NP, values, names, n);
            IDSetNumber(&PWM1NP, nullptr);
            return true;
        }     
        if (!strcmp(name, PWM2NP.name))
        {
            bool allOk = true;
            if (PWM2N[0].value != values[0])
            {
                sprintf(cmd, "B:1:%d", static_cast<uint8_t>(values[0]));
                allOk = allOk && sendCommand(cmd, res);
            }
            PWM2NP.s = (allOk) ? IPS_BUSY : IPS_ALERT;
            if (allOk)
                IUUpdateNumber(&PWM2NP, values, names, n);
            IDSetNumber(&PWM2NP, nullptr);
            return true;
        }              
        
        // SQM calibration
        if (!strcmp(name, SQMOffsetNP.name))
        {
            SQMOffsetNP.s = IPS_BUSY;
            IUUpdateNumber(&SQMOffsetNP, values, names, n);
            SQMOffsetNP.s = IPS_OK;
            IDSetNumber(&SQMOffsetNP, nullptr);
            return true;
        }            
          
        // Focuser settings
        if (!strcmp(name, Focuser1SettingsNP.name))
        {
            bool allOk = true;
            std::map<int, std::string> updates;
            updates[U_FOC1_STEP] = doubleToStr(values[FS1_STEP_SIZE] * 100.0);
            updates[U_FOC1_COMPSTEPS] = doubleToStr(values[FS1_COMPENSATION] * 100.0);
            updates[U_FOC1_COMPTRIGGER] = doubleToStr(values[FS1_COMP_THRESHOLD]);
            updates[U_FOC1_SPEED] = intToStr(values[FS1_SPEED]);
            updates[U_FOC1_ACC] = intToStr(values[FS1_SPEED] * 5.0);
            updates[U_FOC1_CUR] = intToStr(values[FS1_CURRENT] / 10.0);
            updates[U_FOC1_HOLD] = intToStr(values[FS1_HOLD]);
            allOk = allOk && updateSettings("u", "U", updates);
            updates.clear();
            if (allOk)
            {
                Focuser1SettingsNP.s = IPS_BUSY;
                IUUpdateNumber(&Focuser1SettingsNP, values, names, n);
                IDSetNumber(&Focuser1SettingsNP, nullptr);
                DEBUGF(INDI::Logger::DBG_SESSION, "Focuser temperature compensation is %s", (values[FS1_COMPENSATION] > 0) ? "enabled" : "disabled");
                return true;
            }
            Focuser1SettingsNP.s = IPS_ALERT;
            return true;
        }        
        
        if (strstr(name, "FOCUS_"))
            return FI::processNumber(dev, name, values, names, n);
        if (strstr(name, "WEATHER_"))
            return WI::processNumber(dev, name, values, names, n);                 
    }
    return INDI::DefaultDevice::ISNewNumber(dev, name, values, names, n);
}

bool AstroLink4micro::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
	// first we check if it's for our device
	if (dev && !strcmp(dev, getDeviceName()))
	{
		// handle relay labels
		if (!strcmp(name, RelayLabelsTP.name))
		{
			if (isConnected())
			{
				DEBUG(INDI::Logger::DBG_WARNING, "Cannot set labels while device is connected.");
				return false;
			}

			IUUpdateText(&RelayLabelsTP, texts, names, n);
			RelayLabelsTP.s = IPS_OK;
			IDSetText(&RelayLabelsTP, nullptr);
			DEBUG(INDI::Logger::DBG_SESSION, "AstroLink 4 micro labels set . You need to save configuration and restart driver to activate the changes.");
			DEBUGF(INDI::Logger::DBG_DEBUG, "AstroLink 4 micro labels set to OUT1: %s, OUT2: %s, OUT3: %s, PWM1: %s, PWM2: %s", RelayLabelsT[0].text, RelayLabelsT[1].text, RelayLabelsT[2].text, RelayLabelsT[3].text, RelayLabelsT[4].text);

			return true;
		}
	}

	return INDI::DefaultDevice::ISNewText(dev, name, texts, names, n);
}

bool AstroLink4micro::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
	// first we check if it's for our device
	if (dev && !strcmp(dev, getDeviceName()))
	{
        char cmd[ASTROLINK4_LEN] = {0};
        char res[ASTROLINK4_LEN] = {0};
        
		// handle relay 1
		if (!strcmp(name, Switch1SP.name))
		{
            sprintf(cmd, "C:0:%s", (strcmp(Switch1S[S1_ON].name, names[0])) ? "0" : "1");
            bool allOk = sendCommand(cmd, res);
            Switch1SP.s = allOk ? IPS_BUSY : IPS_ALERT;
            if (allOk)
                IUUpdateSwitch(&Switch1SP, states, names, n);

            IDSetSwitch(&Switch1SP, nullptr);
            return true;            
		}        
		// handle relay 2
		if (!strcmp(name, Switch2SP.name))
		{
            sprintf(cmd, "C:1:%s", (strcmp(Switch2S[S2_ON].name, names[0])) ? "0" : "1");
            bool allOk = sendCommand(cmd, res);
            Switch2SP.s = allOk ? IPS_BUSY : IPS_ALERT;
            if (allOk)
                IUUpdateSwitch(&Switch2SP, states, names, n);

            IDSetSwitch(&Switch2SP, nullptr);
            return true;            
		}     
		// handle relay 3
		if (!strcmp(name, Switch3SP.name))
		{
            sprintf(cmd, "C:2:%s", (strcmp(Switch3S[S3_ON].name, names[0])) ? "0" : "1");
            bool allOk = sendCommand(cmd, res);
            Switch3SP.s = allOk ? IPS_BUSY : IPS_ALERT;
            if (allOk)
                IUUpdateSwitch(&Switch3SP, states, names, n);

            IDSetSwitch(&Switch3SP, nullptr);
            return true;            
		}          
        // Focuser Mode
        if (!strcmp(name, Focuser1ModeSP.name))
        {
            std::string value = "0";
            if (!strcmp(Focuser1ModeS[FS1_MODE_UNI].name, names[0]))
                value = "0";
            if (!strcmp(Focuser1ModeS[FS1_MODE_MICRO_L].name, names[0]))
                value = "1";
            if (!strcmp(Focuser1ModeS[FS1_MODE_MICRO_H].name, names[0]))
                value = "2";
            if (updateSettings("u", "U", U_FOC1_MODE, value.c_str()))
            {
                Focuser1ModeSP.s = IPS_BUSY;
                IUUpdateSwitch(&Focuser1ModeSP, states, names, n);
                IDSetSwitch(&Focuser1ModeSP, nullptr);
                return true;
            }
            Focuser1ModeSP.s = IPS_ALERT;
            return true;
        }                   
        if (strstr(name, "FOCUS_")) 
            return FI::processSwitch(dev, name, states, names, n);
        if (strstr(name, "WEATHER_")) 
            return WI::processSwitch(dev, name, states, names, n);
    }
    return INDI::DefaultDevice::ISNewSwitch(dev, name, states, names, n);
}

bool AstroLink4micro::updateSettings(const char *getCom, const char *setCom, int index, const char *value)
{
    std::map<int, std::string> values;
    values[index] = value;
    return updateSettings(getCom, setCom, values);
}

bool AstroLink4micro::updateSettings(const char *getCom, const char *setCom, std::map<int, std::string> values)
{
    char cmd[ASTROLINK4_LEN] = {0}, res[ASTROLINK4_LEN] = {0};
    snprintf(cmd, ASTROLINK4_LEN, "%s", getCom);
    if (sendCommand(cmd, res))
    {
        std::string concatSettings = "";
        std::vector<std::string> result = split(res, ":");
        if (result.size() >= values.size())
        {
            result[0] = setCom;
            for (std::map<int, std::string>::iterator it = values.begin(); it != values.end(); ++it)
                result[it->first] = it->second;

            for (const auto &piece : result)
                concatSettings += piece + ":";

            snprintf(cmd, ASTROLINK4_LEN, "%s", concatSettings.c_str());
            if (sendCommand(cmd, res))
                return true;
        }
    }
    return false;
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
            SetTimer(POLL_PERIOD);
            return true;
        }
    }
    return false;
}


void AstroLink4micro::TimerHit()
{
	if (!isConnected()) 
    {
        SetTimer(POLL_PERIOD);
		return;
    }
    readDevice();
    SetTimer(POLL_PERIOD);
}


bool AstroLink4micro::readDevice()
{
    char res[ASTROLINK4_LEN] = {0};
    if (sendCommand("q", res))
    {
        std::vector<std::string> result = split(res, ":");
        result.erase(result.begin());

        int focuserPosition = std::stoi(result[Q_FOC1_POS]);
        int stepsToGo = std::stoi(result[Q_FOC1_TO_GO]);
        
        FocusAbsPosNP[0].setValue(focuserPosition);
        if (stepsToGo == 0)
        {
            FocusAbsPosNP.setState(IPS_OK);
            FocusRelPosNP.setState(IPS_OK);
        }
        else
        {
            FocusAbsPosNP.setState(IPS_BUSY);
            FocusRelPosNP.setState(IPS_BUSY);
        }
        FocusAbsPosNP.apply();
        FocusRelPosNP.apply();

        if (result.size() > 5)
        {
            if (std::stoi(result[Q_SENS1_PRESENT]) > 0)
            {
                setParameterValue("WEATHER_TEMPERATURE", std::stod(result[Q_SENS1_TEMP]));
                setParameterValue("WEATHER_HUMIDITY", std::stod(result[Q_SENS1_HUM]));
                setParameterValue("WEATHER_DEWPOINT", std::stod(result[Q_SENS1_DEW]));
            }
            else
            {
                setParameterValue("WEATHER_TEMPERATURE", 0.0);
                setParameterValue("WEATHER_HUMIDITY", 0.0);
                setParameterValue("WEATHER_DEWPOINT", 0.0);
                
            }
            if (std::stoi(result[Q_MLX_PRESENT]) > 0)
            {
                setParameterValue("WEATHER_SKY_TEMP", std::stod(result[Q_MLX_TEMP]));
                setParameterValue("WEATHER_SKY_DIFF", std::stod(result[Q_MLX_TEMP]) - std::stod(result[Q_MLX_AUX]));     
            }
            else
            {
                setParameterValue("WEATHER_SKY_TEMP", 0.0);
                setParameterValue("WEATHER_SKY_DIFF", 0.0);              
            }
            if (std::stoi(result[Q_SBM_PRESENT]) > 0)
            {
                setParameterValue("SQM_READING", std::stod(result[Q_SBM]) + SQMOffsetN[0].value);
            }
            else
            {
                setParameterValue("SQM_READING", 0.0);
            }

            if (Switch1SP.s != IPS_OK || Switch2SP.s != IPS_OK || Switch3SP.s != IPS_OK)
            {
                Switch1S[S1_ON].s = (std::stoi(result[Q_OUT1]) > 0) ? ISS_ON : ISS_OFF;
                Switch1S[S1_OFF].s = (std::stoi(result[Q_OUT1]) == 0) ? ISS_ON : ISS_OFF;
                Switch1SP.s = IPS_OK;
                IDSetSwitch(&Switch1SP, nullptr);
                Switch2S[S2_ON].s = (std::stoi(result[Q_OUT2]) > 0) ? ISS_ON : ISS_OFF;
                Switch2S[S2_OFF].s = (std::stoi(result[Q_OUT2]) == 0) ? ISS_ON : ISS_OFF;
                Switch2SP.s = IPS_OK;
                IDSetSwitch(&Switch2SP, nullptr);
                Switch3S[S3_ON].s = (std::stoi(result[Q_OUT3]) > 0) ? ISS_ON : ISS_OFF;
                Switch3S[S3_OFF].s = (std::stoi(result[Q_OUT3]) == 0) ? ISS_ON : ISS_OFF;
                Switch3SP.s = IPS_OK;
                IDSetSwitch(&Switch3SP, nullptr);
            }

            PWM1N[0].value = std::stod(result[Q_PWM1]);
            PWM2N[1].value = std::stod(result[Q_PWM2]);
            PWM1NP.s = IPS_OK;
            IDSetNumber(&PWM1NP, nullptr);
            PWM2NP.s = IPS_OK;
            IDSetNumber(&PWM2NP, nullptr);

            PowerDataN[POW_ITOT].value = std::stod(result[Q_ITOT]);
            PowerDataN[POW_VIN].value = std::stod(result[Q_VIN]);
            PowerDataN[POW_AH].value = std::stod(result[Q_AH]);
            PowerDataN[POW_WH].value = std::stod(result[Q_WH]);
            PowerDataNP.s = IPS_OK;
            IDSetNumber(&PowerDataNP, nullptr);
        }
    }
    
    // update settings data if was changed
    if (FocusMaxPosNP.getState() != IPS_OK || FocusReverseSP.getState() != IPS_OK || Focuser1SettingsNP.s != IPS_OK || Focuser1ModeSP.s != IPS_OK)
    {
        if (sendCommand("u", res))
        {
            std::vector<std::string> result = split(res, ":");

            if (Focuser1SettingsNP.s != IPS_OK)
            {

                DEBUGF(INDI::Logger::DBG_DEBUG, "Update settings, focuser 1, res %s", res);
                Focuser1SettingsN[FS1_STEP_SIZE].value = std::stod(result[U_FOC1_STEP]) / 100.0;
                Focuser1SettingsN[FS1_COMPENSATION].value = std::stod(result[U_FOC1_COMPSTEPS]) / 100.0;
                Focuser1SettingsN[FS1_COMP_THRESHOLD].value = std::stod(result[U_FOC1_COMPTRIGGER]);
                Focuser1SettingsN[FS1_SPEED].value = std::stod(result[U_FOC1_SPEED]);
                Focuser1SettingsN[FS1_CURRENT].value = std::stod(result[U_FOC1_CUR]) * 10.0;
                Focuser1SettingsN[FS1_HOLD].value = std::stod(result[U_FOC1_HOLD]);
                Focuser1SettingsNP.s = IPS_OK;
                IDSetNumber(&Focuser1SettingsNP, nullptr);
            }

            if (Focuser1ModeSP.s != IPS_OK)
            {
                Focuser1ModeS[FS1_MODE_UNI].s = Focuser1ModeS[FS1_MODE_MICRO_L].s = Focuser1ModeS[FS1_MODE_MICRO_H].s = ISS_OFF;
                if (!strcmp("0", result[U_FOC1_MODE].c_str()))
                    Focuser1ModeS[FS1_MODE_UNI].s = ISS_ON;
                if (!strcmp("1", result[U_FOC1_MODE].c_str()))
                    Focuser1ModeS[FS1_MODE_MICRO_L].s = ISS_ON;
                if (!strcmp("2", result[U_FOC1_MODE].c_str()))
                    Focuser1ModeS[FS1_MODE_MICRO_H].s = ISS_ON;
                Focuser1ModeSP.s = IPS_OK;
                IDSetSwitch(&Focuser1ModeSP, nullptr);
            }

            if (FocusMaxPosNP.getState() != IPS_OK)
            {
                FocusMaxPosNP[0].setValue(std::stoi(result[U_FOC1_MAX]));
                FocusMaxPosNP.setState(IPS_OK);
                FocusMaxPosNP.apply();
            }
            if (FocusReverseSP.getState() != IPS_OK)
            {
                FocusReverseSP[0].setState((std::stoi(result[U_FOC1_REV]) > 0) ? ISS_ON : ISS_OFF);
                FocusReverseSP[1].setState((std::stoi(result[U_FOC1_REV]) == 0) ? ISS_ON : ISS_OFF);
                FocusReverseSP.setState(IPS_OK);
                FocusReverseSP.apply();
            }
        }
    }
        
    return true;
}

bool AstroLink4micro::sendCommand(const char *cmd, char *res)
{
    int nbytes_read = 0, nbytes_written = 0, tty_rc = 0;
    char command[ASTROLINK4_LEN];
    
    tcflush(PortFD, TCIOFLUSH);
    sprintf(command, "%s\n", cmd);
    //~ DEBUGF(INDI::Logger::DBG_SESSION, "CMD %s", cmd);
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
    //~ DEBUGF(INDI::Logger::DBG_SESSION, "RES %s", res);
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
** Focuser interface
***************************************************************************************/
IPState AstroLink4micro::MoveAbsFocuser(uint32_t targetTicks)
{
    char cmd[ASTROLINK4_LEN] = {0}, res[ASTROLINK4_LEN] = {0};
    snprintf(cmd, ASTROLINK4_LEN, "R:%i:%u", 0, targetTicks);
    return (sendCommand(cmd, res)) ? IPS_BUSY : IPS_ALERT;
}

IPState AstroLink4micro::MoveRelFocuser(FocusDirection dir, uint32_t ticks)
{
    return MoveAbsFocuser(dir == FOCUS_INWARD ? FocusAbsPosNP[0].getValue() - ticks : FocusAbsPosNP[0].getValue() + ticks);
}

bool AstroLink4micro::AbortFocuser()
{
    char res[ASTROLINK4_LEN] = {0};
    return (sendCommand("H", res));
}

bool AstroLink4micro::ReverseFocuser(bool enabled)
{
    if (updateSettings("u", "U", U_FOC1_REV, (enabled) ? "1" : "0"))
    {
        FocusReverseSP.setState(IPS_BUSY);
        return true;
    }
    else
    {
        return false;
    }
}

bool AstroLink4micro::SyncFocuser(uint32_t ticks)
{
    char cmd[ASTROLINK4_LEN] = {0}, res[ASTROLINK4_LEN] = {0};
    snprintf(cmd, ASTROLINK4_LEN, "P:%i:%u", 0, ticks);
    if (sendCommand(cmd, res))
    {
        FocusAbsPosNP.setState(IPS_BUSY);
        return true;
    }
    else
    {
        return false;
    }
}

bool AstroLink4micro::SetFocuserMaxPosition(uint32_t ticks)
{
    if (updateSettings("u", "U", U_FOC1_MAX, std::to_string(ticks).c_str()))
    {
        FocusMaxPosNP.setState(IPS_BUSY);
        return true;
    }
    else
    {
        return false;
    }
}

/**************************************************************************************
** INDI is asking us for our default device name
***************************************************************************************/
const char *AstroLink4micro::getDefaultName()
{
    return "AstroLink 4 micro";
}

bool AstroLink4micro::saveConfigItems(FILE *fp)
{
	IUSaveConfigText(fp, &RelayLabelsTP);
	IUSaveConfigNumber(fp, &PWM1NP);
	IUSaveConfigNumber(fp, &PWM2NP);
    IUSaveConfigNumber(fp, &SQMOffsetNP);

	FI::saveConfigItems(fp);
	WI::saveConfigItems(fp);
    INDI::DefaultDevice::saveConfigItems(fp);

	return true;
}

/**************************************************************************************
** Helper functions
***************************************************************************************/
std::vector<std::string> AstroLink4micro::split(const std::string &input, const std::string &regex)
{
    std::regex re(regex);
    std::sregex_token_iterator
        first{input.begin(), input.end(), re, -1},
        last;
    return {first, last};
}

std::string AstroLink4micro::doubleToStr(double val)
{
    char buf[10];
    sprintf(buf, "%.0f", val);
    return std::string(buf);
}

std::string AstroLink4micro::intToStr(double val)
{
    char buf[10];
    sprintf(buf, "%i", (int)val);
    return std::string(buf);
}
