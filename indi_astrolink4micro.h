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

#ifndef ASTROLINK4_H
#define ASTROLINK4_H

#include <defaultdevice.h>
#include <indifocuserinterface.h>
#include <indiweatherinterface.h>
#include <string>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <memory>
#include <regex>
#include <cstring>
#include <map>
#include <sstream>


#define Q_DEVICE_CODE 0
#define Q_FOC1_POS 1
#define Q_FOC1_TO_GO 2
#define Q_FOC2_POS 3
#define Q_FOC2_TO_GO 4
#define Q_ITOT 5
#define Q_SENS1_PRESENT 6
#define Q_SENS1_TEMP 7
#define Q_SENS1_HUM 8
#define Q_SENS1_DEW 9
#define Q_SENS2_PRESENT 10
#define Q_SENS2_TEMP 11
#define Q_PWM1 12
#define Q_PWM2 13
#define Q_OUT1 14
#define Q_OUT2 15
#define Q_OUT3 16
#define Q_VIN 17
#define Q_VREG 18
#define Q_AH 19
#define Q_WH 20
#define Q_FOC1_COMP 21
#define Q_FOC2_COMP 22
#define Q_OVERTYPE 23
#define Q_OVERVALUE 24
#define Q_MLX_PRESENT 25
#define Q_MLX_TEMP 26
#define Q_MLX_AUX 27
#define Q_SENS2E_PRESENT 28
#define Q_SENS2E_TEMP 29
#define Q_SENS2E_HUM 30
#define Q_SENS2E_DEW 31
#define Q_SBM_PRESENT 32
#define Q_SBM 33


namespace Connection
{
    class Serial;
}

class AstroLink4micro : public INDI::DefaultDevice, public INDI::FocuserInterface, public INDI::WeatherInterface
{
    public:
        AstroLink4micro();

    protected:
        virtual bool initProperties() override;
     
        const char *getDefaultName() override;
        
        // Weather Overrides
        virtual IPState updateWeather() override
        {
            return IPS_OK;
        }        
        
    private:
        int PortFD { -1 };
        Connection::Serial *serialConnection { nullptr };
        char stopChar { 0xA };
        bool Handshake();
        virtual bool sendCommand(const char *cmd, char *res);
        
        std::vector<std::string> split(const std::string &input, const std::string &regex);
        
        static constexpr const char *POWER_TAB{"Power"};
        static constexpr const char *ENVIRONMENT_TAB{"Environment"};        
        
};

#endif
