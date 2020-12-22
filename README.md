## USB OLED ##

USB OLED allows a USB CDC serial connection to SSD1306 OLED Display Modules.


### Protocol ###

This driver will present itself to the system as CDC USB device. Communication
protocol is text-based and available [here](PROTOCOL.md).

### Setup ###

In order to have the full control, I advise the following port settings under
Linux:

    stty -F /dev/ttyACM0 -echo -onlcr

This will disable echo and automatic CRLF conversion.

---

*You can check my blog and other projects at [www.medo64.com](https://www.medo64.com/electronics/).*
