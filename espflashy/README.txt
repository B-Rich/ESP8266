espflashy.exe    This document and the program is released under GPLv2.


This is a Windows based utility for programming and debugging the ESP8266. It 
uses the RTS for the reset pin and the DTR for the GPIO0 program enable pin.


Usage:
This tool is very simple to use and has 4 main tabs, the 

Upload tab: allows you to program individual files. 

Table  tab: is a table driven programmer which allows you to automate the process of programming multiple files. 

Terminal tab: is just a simple terminal emulation program.

Dump Tab: shows everything in hex.


Ports:
The serial port is selected using the drop down at the top control bar, click 
on Open button and it will open the conection at the selected baud.

Typical usage:
Go to upload tab and click on the [...] button to select a file or simply type
it or paste it into the file to upload field. Then click on the button [Flash]
to upload the file to the flash ram at the selected offset address.


Protocol:

If GPIO0 and GPIO15 is pulled down and GPIO2 is pulled high when the module 
leaves reset, then the bootloader will enter the UART download mode. The ROM
auto-bauds, that is, it will automagically detect which baud rate you are using. 
espflashy defaults to 115200, whatever you select will be saved for next time.

espflashy uses the RTS and DTR modem status lines to automatically enter the bootloader. Connect RTS to CH_PD (Chip Power Down, which is used as 
active-low reset) and DTR to GPIO0.

The bootloader protocol uses [SLIP](http://en.wikipedia.org/wiki/SLIP) framing.
Each packet begin and end with `0xC0`, all occurrences of `0xC0` and `0xDB` inside the packet
are replaced with `0xDB 0xDC` and `0xDB 0xDD`, respectively.

Inside the frame, the packet consists of a header and a variable-length body.
All multi-byte fields are little-endian.


Request:
-----------------------------------------------------------------------------------------
Byte   | Name		| Comment
-------|----------------|-------------------------------
0      | Direction	| Always `0x00` for requests
1      | Command	| Requested operation, according to separate table
2-3    | Size		| Size of body
4-7    | Checksum	| XOR checksum of payload, only used in block transfer packets
8..n   | Body		| Depends on operation


Response:
-----------------------------------------------------------------------------------------
Byte   | Name		| Comment
-------|----------------|-------------------------------
0      | Direction	| Always `0x01` for responses
1      | Command	| Same value as in the request packet that trigged the response
2-3    | Size		| Size of body, normally 2
4-7    | Value		| Response data for some operations
8..n   | Body		| Depends on operation
8      | Status		| Status flag, success (`0`) or failure (`1`)
9      | Error		| Last error code, not reset on success

Opcodes:
-----------------------------------------------------------------------------------------
Byte   | Name			| Input		| Output
-------|------------------------|---------------|------------------------
`0x02` | Flash Download Start	| total size, number of blocks, block size, offset	|
`0x03` | Flash Download Data	| size, sequence number, data. checksum in value field. |
`0x04` | Flash Download Finish	| reboot flag? |
`0x05` | RAM Download Start	| total size, packet size, number of packets, memory offset |
`0x06` | RAM Download Finish	| execute flag, entry point |
`0x07` | RAM Download Data	| size, sequence numer, data. checksum in dedicated field. |
`0x08` | Sync Frame		| `0x07 0x07 0x12 0x20`, `0x55` 32 times |
`0x09` | Write register		| Four 32-bit words: address, value, mask and delay (in microseconds) | Body is `0x00 0x00` if successful
`0x0a` | Read register		| Address as 32-bit word | Read data as 32-bit word in `value` field
`0x0b` | Configure SPI params	| 24 bytes of unidentified SPI parameters |

Checksum:
Each byte in the payload is XOR'ed together, as well as the magic number `0xEF`.
The result is stored as a zero-padded byte in the 32-bit checksum field in the header.

Firmware image format:
The firmware file consists of a header, a variable number of data segments and a footer.
Multi-byte fields are little-endian.


File header:
-----------------------------------------------------------------------------------------
Byte	| Description
--------|-----------------------
0	| Always `0xE9`
1	| Number of segments
2	| SPI Flash Interface (`0` = QIO, `1` = QOUT, `2` = DIO, `0x3` = DOUT)
3	| High four bits: `0` = 512K, `1` = 256K, `2` = 1M, `3` = 2M, `4` = 4M, Low four bits: `0` = 40MHz, `1`= 26MHz, `2` = 20MHz, `0xf` = 80MHz
4-7	| Entry point
8-n	| Segments


Segment:
-----------------------------------------------------------------------------------------
Byte	| Description
--------|-----------------------
0-3	| Memory offset
4-7	| Segment size
8...n	| Data

Footer:
The file is padded with zeros until its size is one byte less than a multiple of 16 bytes. A last byte (thus making the file size a multiple of 16) is the checksum of the data of all segments. The checksum is defined as the xor-sum of all bytes and the byte `0xEF`.

Boot log:
The boot rom writes a log to the UART when booting. The timing is a little bit unusual: 74880 baud

```
ets Jan  8 2014,rst cause 1, boot mode:(3,7)

load 0x40100000, len 24236, room 16 
tail 12
chksum 0xb7
ho 0 tail 12 room 4
load 0x3ffe8000, len 3008, room 12 
tail 4
chksum 0x2c
load 0x3ffe8bc0, len 4816, room 4
tail 12
chksum 0x46
csum 0x46
```

Final Notes about booting:

MTD0 (also known as GPIO15) : GPIO0 : GPIO2 are the pins that control the boot mode on the ESP9266.
To be able to download new firmware the module must boot in mode 1 ( GND : GND : 3V3 ) by setting GPIO15 : GPIO0 : GPIO2
To run the module with the new downloaded firmware the module must boot in mode 3 ( GND : 3V3 : 3V3 )
For modules when a pin should be 3.3V it is enough to leave it not connected and if it should be 0 I have connected it to GND.


