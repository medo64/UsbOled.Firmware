## Protocol ##

Device uses USB CDC serial port interface. All commands should be terminal
friendly for troubleshooting.

Output is text based. All content will be displayed and tracked in rows and
columns.

Cursor is tracked in 8 pixel resolution. First line starts at pixel 0, second
line starts at pixel 8, third line is at pixel 16, and so on. When movement to
the next line is needed, pixel location will increase by the font height. For
example, if cursor is currently at pixel line 8 and font size is 8x16, the
next pixel line will be 24.


### Text Mode ###

When device is turned on, it will be in text mode. Once `LF` (`0x0A`) or
`CRLF` (`0x0D0A`) are received, the collected text will be shown on display.

If writing was successful, a `LF` or `CRLF` will be returned. Otherwise, it
will echo exclamation point (`!`) character followed by an optional text and
ending with `LF` or `CRLF`.

Whether output will be `LF` or `CRLF` terminated depends on the previous
input. That is, if previous text ended with `LF`, response will also use `LF`.
If previous text used `CRLF` as a line-ending characters, `CRLF` will be used
for reply.

Characters lower than ASCII 32 or higher than ASCII 126 are ignored unless
specified explicitly as an escape character.


#### Escape characters ####

##### `0x07` `BEL` (`\a`) #####

Clears display and moves cursor to the upper-left corner. Can only be used as
a first character in line. Usage within text will be ignored.

##### `0x08` `BS` (`\b`) #####

Backspace will cause cursor to be moved into the upper left corner. Display
will not be cleared. Can only be used as a first character in line. Usage
within text will be ignored.

##### `0x09` `HT` (`\t`) #####

Tab will switch device into the "Command Mode" thus treating rest of the line
as a command. Can only be used as a first character in line. Usage within text
will be ignored. It's processed only as a first character in line. Command
mode stops once `LF` or `CRLF` is detected and command has been executed.

##### `0x0A` `LF` (`\n`) #####

Line feed character will print out all previously sent characters and move the
cursor to the next line. After the fourth line no movement will be made until
line is reset (e.g. via `BS` escape character). If used to execute a command,
no cursor movement will occur.

##### `0x0B` `VT` (`\v`) #####

Reserved for future use.

##### `0x0C` `FF` (`\f`) #####

Text following it will use double-height font (8x16). Can only be used as a
first character in line. Usage within text will be ignored.

##### `0x0D` `CR` (`\r`) #####

Used just in pair with `LF`.


### Command mode ###

Command mode is entered using `HT` (`0x09`) character as the first character
of the new line,  followed by a single character command. Command will be
processed once `LF` or `CRLF` is detected.

If command is successful, it will echo a `LF` or `CRLF`, otherwise it will
echo exclamation point (`!`) followed by optional text and finished with `LF`
or `CRLF`.

Either output will exit the command mode.


#### `~` (restore defaults) ####

This parameter-less command restores all setting to their default value.

##### Example 1 (default) #####

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `~` `LF`                                                       |
| Response: | `LF`                                                           |
| Result:   | Display size is 128x64.                                        |

##### Example 2 (default 128x64) #####

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `~` `A` `LF`                                                   |
| Response: | `LF`                                                           |
| Result:   | Display size is 128x64.                                        |

##### Example 3 (default 128x32) #####

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `~` `B` `LF`                                                   |
| Response: | `LF`                                                           |
| Result:   | Display size is 128x32.                                        |

##### Example 4 (invalid) #####

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `~` `Z` `LF`                                                   |
| Response: | `!` `LF`                                                       |
| Result:   | No change to display size.                                     |


##### `m` (move)  #####

Moves cursor to specified row and column. Command takes two parameters, both
in hexadecimal format with leading 0. Either can be omitted using underscore
character (`_`). Rows are always expressed in 8x8 units (even when 8x16 font
is used).

##### Example 1 (row 2, column 6) #####

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `m0206` `LF`                                                   |
| Response: | `LF`                                                           |
| Result:   | Moves cursor to row 2 and column 6.                            |

##### Example 2 (column 12) #####

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `m__0C` `LF`                                                   |
| Response: | `LF`                                                           |
| Result:   | Moves cursor to column 12.                                     |

##### Example 3 (row 3) #####

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `m03` `LF`                                                     |
| Response: | `LF`                                                           |
| Result:   | Moves cursor to row 3 without changing column.                 |

##### Example 4 (invalid) #####

|           |                                                                |
|-----------|----------------------------------------------------------------|
| Request:  | `m08` `LF`                                                     |
| Response: | `!` `LF`                                                       |
| Result:   | No action is taken since row is outside of range.              |
