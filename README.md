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
