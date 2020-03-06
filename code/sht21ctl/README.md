# SHT21CTL
A basic driver to perform all functionalities available on the SHT-21 that uses the standard i2c linux device driver to communicate with the SHT-21. This may also work with other SHT-X IC's, although this should be ratified by the individual datasheets of each device. Tested on Raspberry Pi 4.

### Usage

|Command|Argument|Sub Argument|Description|
|-------|--------|------------|-----------|
|readtemp|[options1]||reads a single temperature value|
|readrh|[options1]||reads a single relative humidity value|
|readall|[options1]||reads both temperature and rh values
|readUser|||reads the user register and outputs
|writeuser|[options2]||writes certain user data values 
|[options1]|-nhm||'no hold master' option when taking measurements
||-c||'continuous' mode which keeps reading data untill stopped
|[options2]|-res||data resolution
|||1|rh 12-bit & temp 14 bit (default)|
|||2|rh 8-bit & temp 12 bit|
|||3|rh 10-bit & temp 13 bit|
|||4|rh 11-bit & temp 11 bit|
||-heat||on-chip heater|
|||on|enables heater|
|||off|disables heater (default)|
||-otp||OTP Reload|
|||on|enables OTP|
|||off|disables OTP (default)|


### Instructions
1. Clone repo to Rasberry Pi.
2. Make sure i2c is enabled.
3. Run `make`, to build the program.
3. Run program with `./sht21ctl arg1 [-options]`.


Comments and suggestions welcome!

Check out [georgehowelldsp.com](http://www.georgehowelldsp.com) for more projects and resources!
