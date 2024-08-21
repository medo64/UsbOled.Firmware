# USB OLED (Firmware)

USB OLED allows a USB CDC serial connection to SSD1306 OLED Display Modules.
This firmware is for UsbOled and OledFEC device.


## Setup

This driver will present itself to the system as CDC USB device. Communication
protocol is text-based and available below.

### Linux

In order to have the full control, I advise the following port settings under
Linux:

    stty -F /dev/ttyACM0 -echo -onlcr

This will disable echo and automatic CRLF conversion.


## Protocol

Device uses USB CDC serial port interface. All commands should be terminal
friendly for troubleshooting.

Output is text based. All content will be displayed and tracked in rows and
columns.

Cursor is tracked in 8 pixel resolution. First line starts at pixel 0, second
line starts at pixel 8, third line is at pixel 16, and so on. When movement to
the next line is needed, pixel location will increase by the font height. For
example, if cursor is currently at pixel line 8 and font size is 8x16, the
next pixel line will be 24.


### Text Mode

When device is turned on, it will be in text mode. Once `LF` (`0x0A`) or `CR`
(`0x0D`) are received, the collected text will be shown on display.

If writing was successful, a `LF` or `CR` will be returned. Otherwise, it will
echo exclamation point (`!`) character followed by an optional text and ending
with `LF` or `CR`.

Whether output will be `LF` or `CR` terminated depends on the previous input.
That is, if previous text ended with `LF`, response will also use `LF`. If
previous text used `CR` as a line-ending characters, `CR` will be used for
reply.

If there is no text specified, cursor will move to the next line. Otherwise,
it will stay on the same line. Please note that if you send `CRLF` as line
ending, the first line ending (`CR`) will write the text and the second one
will move cursor to the next line.

Characters lower than ASCII 32 are ignored unless they are listed in escape
characters. Characters higher than ASCII 126 are just ignored.


#### Escape characters

##### `0x07` `BEL` (`\a`)

Clears display and moves cursor to the upper-left corner.

##### `0x08` `BS` (`\b`)

Backspace will cause cursor to be moved into the upper left corner. Display
will not be cleared.

##### `0x09` `HT` (`\t`)

If line starts with tab character (`HT`), the rest of the line will be treated
as a command. Usage within text will be ignored. Command mode is limited only
to line being processed. If null character (`NUL`) is encountered, command
processing will stop and rest of line will revert to text mode.

##### `0x0A` `LF` (`\n`)

Line feed character will start line processing. In Text mode it will show
characters on OLED display. If no characters are present before it, it will
move cursor to the next line. If previous line had double-height text, two
movements will be made. After the fourth line no movement will be made until
the current line is reset (e.g. via `BS` escape character).

##### `0x0B` `VT` (`\v`)

Text following it will use double-height font (8x16). If used again, it will
switch double-height off (8x8 character size).

##### `0x0C` `FF` (`\f`)

Clears the rest of the line leaving the cursor at the current position.

##### `0x0D` `CR` (`\r`)

Functions the same way as `LF`.


### Command mode

Command mode is entered using `HT` (`0x09`) character as the first character
of the new line,  followed by a single character command. Command will be
processed until `LF`, `CR`, or `NUL` is detected.

If command is successful, it will echo a `LF` or `CR`, otherwise it will
echo exclamation point (`!`) followed by optional text and finished with `LF`
or `CR`.

Either output will exit the command mode.


#### `#` (set size)

This command will set screen size. Argument is either `A` (128x64) or `B`
(128x32). Changes to setting are saved immediately. If called without
argument, the current value will be returned.

##### Example 1 (128x64)

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `HT` `#A` `LF`                                                 |
| Response: | `LF`                                                           |
| Result:   | Display size is 128x64.                                        |

##### Example 2 (128x32)

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `HT` `#B` `LF`                                                 |
| Response: | `LF`                                                           |
| Result:   | Display size is 128x32.                                        |

##### Example 3 (current value)

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `HT` `#` `LF`                                                  |
| Response: | `A` `LF`                                                       |
| Result:   | Currently set display is 128x64.                               |

##### Example 4 (invalid)

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `HT` `#Z` `LF`                                                 |
| Response: | `!` `LF`                                                       |
| Result:   | No change to display size.                                     |




#### `$` (invert)

This command will set whether display will be inverted by default.

##### Example 1 (Normal)

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `HT` `$N` `LF`                                                 |
| Response: | `LF`                                                           |
| Result:   | Display is not inverted by default.                            |

##### Example 2 (Inverse)

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `HT` `$I` `LF`                                                 |
| Response: | `LF`                                                           |
| Result:   | Display is inverted by default.                                |

##### Example 3 (current value)

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `HT` `$` `LF`                                                  |
| Response: | `N` `LF`                                                       |
| Result:   | Display is not inverted by default.                            |


#### `%` (reset)

This command will reboot the device, including it's USB stack.

##### Example 1

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `HT` `%` `LF`                                                  |
| Response: | None                                                           |
| Result:   | Device is restarted.                                           |


#### `*` (brightness)

This command will set OLED contrast/brightness. Argument is value in
hexadecimal format. Changes to setting are saved immediately. If called
without argument, the current value will be returned.

##### Example 1 (minimum)

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `HT` `*00` `LF`                                                |
| Response: | `LF`                                                           |
| Result:   | Display is at minimum brightness.                              |

##### Example 2 (maximum)

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `HT` `*FF` `LF`                                                |
| Response: | `LF`                                                           |
| Result:   | Display is at maximum brightness.                              |

##### Example 3 (current value)

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `HT` `*` `LF`                                                  |
| Response: | `CF` `LF`                                                      |
| Result:   | Display brightness is at 0xCF.                                 |


#### `@` (set address)

This command will set OLED module I²C address. Argument is address in
hexadecimal format. Changes to setting are saved immediately. If called
without argument, the current value will be returned.

##### Example 1 (0x3D)

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `HT` `@3D` `LF`                                                |
| Response: | `LF`                                                           |
| Result:   | OLED module should use address 0x3D.                           |

##### Example 2 (current value)

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `HT` `@` `LF`                                                  |
| Response: | `3C` `LF`                                                      |
| Result:   | OLED module is at 0x3C.                                        |


#### `^` (set speed)

This command will set OLED module I²C speed. Argument is speed in 100
kHz steps (with `0` being `1 Mbps`). Changes to setting are saved
immediately. If called without argument, the current value will be
returned.

##### Example 1 (100 kHz)

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `HT` `^1` `LF`                                                 |
| Response: | `LF`                                                           |
| Result:   | Speed is set to 100 kHz.                                       |

##### Example 2 (400 kHz)

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `HT` `^4` `LF`                                                 |
| Response: | `LF`                                                           |
| Result:   | Speed is set to 400 kHz.                                       |

##### Example 3 (1 Mbps)

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `HT` `^0` `LF`                                                 |
| Response: | `LF`                                                           |
| Result:   | Speed is set to 1000 kHz.                                      |

##### Example 4 (current value)

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `HT` `^` `LF`                                                  |
| Response: | `1` `LF`                                                       |
| Result:   | Speed is 100 kHz.                                              |


#### `~` (restore defaults)

This parameter-less command restores all setting to their default value. This
means OLED module is assumed to be on `0x3C` I²C address, working at 100 kHz,
display size is 128x64, and brightness is at `0xCF`. Settings are
automatically committed to permantent memory.

##### Example (default)

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `~` `LF`                                                       |
| Response: | `LF`                                                           |
| Result:   | All settings are back to default.                              |


#### `c` (custom character) 

Draws a custom character based on raw data. Data has to be hexadecimal, 8
bytes in length, with each byte describing one column of data.

##### Example 1 (diagonal line)

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `c0102040810204080` `LF`                                       |
| Response: | `LF`                                                           |
| Result:   | Draws line from upper-left to lower-right.                     |

##### Example 2 (box)

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `c007E424242427E00` `LF`                                       |
| Response: | `LF`                                                           |
| Result:   | Draws box with one empty space around each side.               |


#### `C` (large custom character) 

Draws a custom character based on raw data. Data has to be hexadecimal, 16
bytes in length, with each byte describing one column of data. Top 8 pixels
are drawn first.

##### Example 1 (diagonal line)

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `C030C30C00000000000000000030C30C0` `LF`                        |
| Response: | `LF`                                                           |
| Result:   | Draws line from upper-left to lower-right.                     |

##### Example 2 (box)

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `C00FE02020202FE00007F404040407F00` `LF`                       |
| Response: | `LF`                                                           |
| Result:   | Draws box with one empty space around each side.               |


#### `i` (inverse)

Display will be inverted.

##### Example 1 (invert)

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `i` `LF`                                                       |
| Response: | `LF`                                                           |
| Result:   | Display will be inverted.                                      |


#### `I` (inverse cancel)

Display will not be inverted.

##### Example 1 (cancel invert)

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `I` `LF`                                                       |
| Response: | `LF`                                                           |
| Result:   | Display will not be inverted.                                  |


#### `m` (move) 

Moves cursor to specified row and column. Command takes two parameters, both
in hexadecimal format with leading 0. Both rows and columns start from 1. Move
for either can be omitted using `00` in the place of value. Rows are always
expressed in 8x8 units (even when 8x16 font is used).

##### Example 1 (row 2, column 6)

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `m0206` `LF`                                                   |
| Response: | `LF`                                                           |
| Result:   | Moves cursor to row 2 and column 6.                            |

##### Example 2 (column 12)

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `m000C` `LF`                                                   |
| Response: | `LF`                                                           |
| Result:   | Moves cursor to column 12.                                     |

##### Example 3 (row 3)

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `m03` `LF`                                                     |
| Response: | `LF`                                                           |
| Result:   | Moves cursor to row 3, column 1.                               |

##### Example 4 (invalid)

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `m09` `LF`                                                     |
| Response: | `!` `LF`                                                       |
| Result:   | No action is taken since row is outside of range.              |


#### `\`` (set serial)

Using grave (`), one sets the last portion of USB serial number and
reboots device afterward. The first portion of serial number is always
`EA9A`.

##### Example

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `\`12345678` `LF`                                              |
| Result:   | Sets serial number to EA9A12345678.                            |



---

*You can check my blog and other projects at [www.medo64.com](https://www.medo64.com/electronics/).*
