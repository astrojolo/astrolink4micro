![AstroLink 4 MAX Banner](https://shop.astrojolo.com/wp-content/uploads/sites/2/2025/08/astrolinks-banner-micro-2025.jpg)

# AstroLink 4 micro

![Compact Size](https://img.shields.io/badge/Size-15×5cm_Compact-lightgrey)  
![ASCOM & INDI Drivers](https://img.shields.io/badge/Drivers-ASCOM%20%7C%20INDI-blue)  
![Dew & Power Outputs](https://img.shields.io/badge/Outputs-12V%20%7C%20PWM-orange)  
![Environmental Sensors](https://img.shields.io/badge/Sensors-Temp%2FHumidity%2FSkyBrightness-green)

---

## Overview  
AstroLink 4 micro is a multifunctional controller designed for astrophotography rigs, optimized to be small yet powerful. It includes a focuser motor controller that supports stepper or DC focusers for precise focusing. There are both switchable 12V DC outputs and a permanent 12V DC output for reliable power to main devices. Peripherals like dew heaters or fans are managed through PWM-regulated channels. The unit supports environmental sensors: temperature, humidity, cloud coverage, and sky brightness to monitor your observing conditions. It also includes features for voltage and current monitoring to help protect your gear. Compact enough to mount on small rigs or dovetail bars, the 4 micro is ideal when space is limited. Software control is via ASCOM, with INDI support available / planned. Safety features include protection against over-load and over-voltage. Designed for both amateur and semi-pro setups needing reliable control in a compact form.

---

## 🔧 Key Features  
- Motor focuser control (stepper / DC)  
- Multiple switchable 12V DC outputs + permanent 12V output  
- PWM-regulated outputs for dew heat / cooling  
- Environmental sensors (temperature, humidity, sky brightness, cloud coverage)  
- Voltage & current monitoring
- Stackable with 7-port USB 3.0 hub
- Compact form factor (around 15 × 5 cm)  
- ASCOM & INDI driver compatibility  
- Protection: over-voltage, over-current  


https://shop.astrojolo.com/products-overview/astrolink-4-micro/

# AstroLink 4 micro INDI driver
INDI driver for AstroLink 4 micro device (under development)

# Installing INDI server and libraries
To start you need to download and install INDI environment. See [INDI page](http://indilib.org/download.html) for details. 

Then AstroLink 4 micro INDI driver needs to be fetched and installed:

```
git clone https://github.com/astrojolo/astrolink4micro.git
cd astrolink4micro
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
make
sudo make install
```

Then indiserver needs to be started with AstroLink 4 micro drivers:

```
indiserver -v indi_astrolink4micro
```

Now AstroLink 4 micro can be used with any software that supports INDI drivers, like KStars with Ekos.
