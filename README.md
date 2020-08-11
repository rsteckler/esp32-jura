# ESP32 JURA
This project aims to control newer JURA coffee maker models with an ESP32 and a new [XMPP-IoT](https://github.com/COM8/esp32-xmpp-iot) extension as reference implementation.
Therefore we need to understand how the protocol for controlling JURA coffee makers works.

This work is based on the excellent work done by the people over at [Protocol JURA](http://protocoljura.wiki-site.com/index.php/Hauptseite).
They were able to figure out the basic protocol used for communication with older JURA coffee makers.

Since newer models do not use this old V1Protocol any more I started this project to understand the new one.

## Table of Contents
1. [Protocol](#protocol)
2. [JURA Commands](#jura-commands)
3. [Requirements](#requirements)
4. [Building](#building)
5. [Project Structure](#project-structure)

## Protocol

### General
There are several steps of obfuscation being done by the JURA coffee maker to prevent others from reading the bare protocol.

#### Connecting to an JURA coffee maker
To connect to an JURA coffee maker we are using a 5V UART signal with the following configuration:
* **Baud Rate:** 9600
* **Data Bits:** 8
* **Parity:** Disabled
* **Stop Bits:** 1
* **Flow Control:** Hardware flow control disabled
* **RX Flow Control Threshold:** 0

#### Deobfuscating
Once a connection has been established we can start sending and receiving data.  
**But** all data send and received is obfuscated.
The following description shows how to **deobfuscate** data received from the coffee maker.  
To obfuscate data just follow the steps in reverse.

**Step 0**
The coffee maker always sends 4 "raw" byte per one byte of data with a break of 8ms in between each "raw" byte.
This looks something like this:
```
01011011 <8ms break> 01011111 <8ms break> 01011111 <8ms break> 01011111 <8ms break>
01011111 <8ms break> 01111011 <8ms break> 01011111 <8ms break> 01011111 <8ms break>
01111011 <8ms break> 01111011 <8ms break> 01111111 <8ms break> 01011011 <8ms break>
01011111 <8ms break> 01111111 <8ms break> 01011011 <8ms break> 01011011 <8ms break>
01111011 <8ms break> 01111011 <8ms break> 01011011 <8ms break> 01011011 <8ms break>
```
Each line corresponds to one actual data byte.

**Step 1**  
Each of our 4 "raw" bytes (each line) contains only 2 bits of our resulting data bit.  
Bit 2 and 5.  
All other bits (except 0) are set to 1.

```
0b01011011
    ^  ^
  DB1  DB2
```
`DB1` and `DB2` are our actual data bits here.
We have to combine alle 8 (of our 4 "raw" bytes) into a single data byte.

Examples:
```C++
const std::array<uint8_t, 4> encData{0b01011011, 0b01011111, 0b01011111, 0b01011111};

// Bit mask for the 2. bit from the left:
constexpr uint8_t B2_MASK = (0b10000000 >> 2);

// Bit mask for the 5. bit from the left:
constexpr uint8_t B5_MASK = (0b10000000 >> 5);

uint8_t decData = 0;
decData |= (encData[0] & B2_MASK) << 2;
decData |= (encData[0] & B5_MASK) << 4;
decData |= (encData[1] & B2_MASK);
decData |= (encData[1] & B5_MASK) << 2;
decData |= (encData[2] & B2_MASK) >> 2;
decData |= (encData[2] & B5_MASK);
decData |= (encData[3] & B2_MASK) >> 4;
decData |= (encData[3] & B5_MASK) >> 2; // 0b00010101
```
```
Input            -> Output
0 0 0 1  0 1 0 1 -> 0 1 0 1  0 0 0 1
0 1 1 0  0 1 0 1 -> 0 1 0 1  0 1 1 0
1 0 1 0  1 1 0 0 -> 1 1 0 0  1 0 1 0
0 1 1 1  0 0 0 0 -> 0 0 0 0  0 1 1 1
1 0 1 0  0 0 0 0 -> 0 0 0 0  1 0 1 0
```

**Step 2**  
In this step we switch both nibbles (4 bit) of each byte.
Examples:
`1100 1100 -> 0011 0011`
```C++
uint8_t in = 0b00010101;
uint8_t out = ((in & 0xF0) >> 4) | ((in & 0x0F) << 4); // 0b01010001
```
```
Input                                                                               -> Output
01011011 <8ms break> 01011111 <8ms break> 01011111 <8ms break> 01011111 <8ms break> -> 0 1 0 1  0 0 0 1
01011111 <8ms break> 01111011 <8ms break> 01011111 <8ms break> 01011111 <8ms break> -> 0 1 0 1  0 1 1 0
01111011 <8ms break> 01111011 <8ms break> 01111111 <8ms break> 01011011 <8ms break> -> 1 1 0 0  1 0 1 0
01011111 <8ms break> 01111111 <8ms break> 01011011 <8ms break> 01011011 <8ms break> -> 0 0 0 0  0 1 1 1
01111011 <8ms break> 01111011 <8ms break> 01011011 <8ms break> 01011011 <8ms break> -> 0 0 0 0  1 0 1 0
```

**Step 3**  
A last time we have to shift all bits in our byte around.
Here we have to split up our byte into two nibbles (4 bit) and switch two bits each.  
Examples:
`1100 1100 -> 0011 0011`
```C++
uint8_t in = 0b01010001;
uint8_t out = ((in & 0xC0) >> 2) | ((in & 0x30) << 2) | ((in & 0x0C) >> 2) | ((in & 0x03) << 2); // 0b01010100
```
```
Input            -> Output           -> Output_Hex Output_Dec Output_Char
0 1 0 1  0 0 0 1 -> 0 1 0 1  0 1 0 0 -> 54	84	T 
0 1 0 1  0 1 1 0 -> 0 1 0 1  1 0 0 1 -> 59	89	Y 
1 1 0 0  1 0 1 0 -> 0 0 1 1  1 0 1 0 -> 3A	58	:
0 0 0 0  0 1 1 1 -> 0 0 0 0  1 1 0 1 -> 0d	13	'\r'
0 0 0 0  1 0 1 0 -> 0 0 0 0  1 0 1 0 -> 0a	10	'\n'
```
Which results in the message `TY:\r\n`.

## JURA Commands
Every message/command send from or to the coffee maker has to end with `\r\n` to be valid.
For simplicity reasons we omit the `\r\n` from all of the following messages and examples.

### Command Structure
In general for every **valid** command a response will be send from the coffee maker.
The actual command is always uppercase (e.g. `TY:`) and the response send back is lowercase (`ty:EF532M V02.03`).

### Available Commands
The following list of commands has been tested on an `Jura E6 2019 platin (15326)`.

| Name | Command | Response | Description |
|----|----|----|----|
| UNKNOWN | `AN:01` | `ok:` | - |
| Turn off | `AN:02` | `ok:` | Turns off the coffee maker. |
| Erase EPROM | `AN:0A` | UNKNOWN | **Untested!** Erases the EPROM. Do not use. |
| Test UCHI | `AN:0C` | `ok:` | Test the UCHI steam plate. |
| Test Mode on | `AN:20` | `ok:` | Turns on the test mode. |
| Test Mode off | `AN:21` | `ok:` | Turns off the test mode. |
| UNKNOWN | `AN:40` | `an:40` | - |
| UNKNOWN | `AN:AA` | `ok:` | - |
| Get Type of Machine | `TY:` | `ty:` (e.g. `ty:EF532M V02.03`) | Returns the type of the machine. |
| UNKNOWN | `FA:01` | `ok:` | - |
| (Button 1) | `FA:04` | `ok:` | Simulates the button 1 press (left top). |
| (Button 2) | `FA:05` | `ok:` | Simulates the button 2 press (left center). |
| (Button 3) | `FA:06` | `ok:` | Simulates the button 3 press (left bottom). |
| (Button 4) | `FA:07` | `ok:` | Simulates the button 4 press (right top). |
| (Button 5) | `FA:08` | `ok:` | Simulates the button 5 press (right center). |
| (Button 6) | `FA:09` | `ok:` | Simulates the button 6 press (right bottom). |
| Coffee Pump on | `FN:01` | `ok:` | Turns on the coffee pump. |
| Coffee Pump off | `FN:02` | `ok:` | Turns off the coffee pump. |
| Coffee Heater on | `FN:03` | `ok:` | Turns on the coffee heater. |
| Coffee Heater off | `FN:04` | `ok:` | Turns off the coffee heater. |
| Grinder on | `FN:07` | `ok:` | Turns on the coffee grinder. |
| Grinder off | `FN:08` | `ok:` | Turns off the coffee grinder. |
| Coffee press on | `FN:09` | `ok:` | Turns on the coffee press. **Probably** |
| Coffee press off | `FN:0A` | `ok:` | Turns off the coffee press. **Probably** |
| **Something** on | `FN:0B` | `ok:` | Turns **something** on. |
| **Something** off | `FN:0C` | `ok:` | Turns **something** on. |
| Init Brew Group | `FN:0D` | `ok:` | Initializes the brew group. |
| Brew Group to **unknown** Postion XYZ | `FN:0E` | `ok:` | Moves the brew group into an currently unknown position. |
| Brew Group to **unknown** Postion XYZ | `FN:0F` | `ok:` | Moves the brew group into an currently unknown position. |
| Brew Group to **unknown** Postion XYZ | `FN:13` | `ok:` | Moves the brew group into an currently unknown position. |
| Brew Group to **unknown** Postion XYZ | `FN:13` | `ok:` | Moves the brew group into an currently unknown position. |
| Brew Group to **unknown** Postion XYZ | `FN:1B` | `ok:` | Moves the brew group into an currently unknown position. |
| Brew Group to **unknown** Postion XYZ | `FN:1C` | `ok:` | Moves the brew group into an currently unknown position. |
| Brew Group **something** | `FN:22` | `ok:` | Some unknown operation with the brew group. |
| UNKNOWN | `FN:24` | `ok:` | - |
| UNKNOWN | `FN:25` | `ok:` | - |
| UNKNOWN | `FN:26` | `ok:` | - |
| UNKNOWN | `FN:27` | `ok:` | - |
| UNKNOWN | `FN:44` | `ok:` | - |
| UNKNOWN | `FN:45` | `ok:` | - |
| UNKNOWN | `FN:50` | `ok:` | - |
| Turn off | `FN:51` | `ok:` | Turns off the coffee maker. |
| UNKNOWN | `FN:54` | `ok:` | - |
| UNKNOWN | `FN:55` | `ok:` | - |
| UNKNOWN | `FN:60` | `ok:` | - |
| UNKNOWN | `FN:61` | `ok:` | - |
| UNKNOWN | `FN:62` | `ok:` | - |
| UNKNOWN | `FN:63` | `ok:` | - |
| UNKNOWN | `FN:64` | `ok:` | - |
| UNKNOWN | `FN:65` | `ok:` | - |
| UNKNOWN | `FN:66` | `ok:` | - |
| UNKNOWN | `FN:67` | `ok:` | - |
| UNKNOWN | `FN:70` | `ok:` | - |
| UNKNOWN | `FN:71` | `ok:` | - |
| UNKNOWN | `FN:72` | `ok:` | - |
| UNKNOWN | `FN:73` | `ok:` | - |
| UNKNOWN | `FN:80` | `ok:` | - |
| UNKNOWN | `FN:81` | `ok:` | - |
| UNKNOWN | `FN:88` | `ok:` | - |
| UNKNOWN | `FN:89` | `ok:` | - |
| Debug mode on | `FN:8A` | `ku:`, `Ku:` pause `ku:`, `Ku:`, ... | Enables the debug mode. Sends continuously `ku:`, `Ku:`, ... Once an action like opening the hot water valve accrues, outputs information like percentage done. To disable it again disconnect the coffee maker from power.  |
| UNKNOWN | `FN:90` | `ok:` | - |
| UNKNOWN | `FN:99` | `ok:` | - |

## Requirements
This project builds on top the excellent [Espressif IoT Development Framework](https://github.com/espressif/esp-idf).
Follow [this](https://docs.espressif.com/projects/esp-idf/en/release-v4.0/get-started/#installation-step-by-step) installation guide and install version **4.0.1** of the framework on your computer.

## Building

### Step 0: Setup
```bash
# Clone the repository:
git clone https://github.com/COM8/esp32-jura.git
# Switch into the newly cloned repository:
cd esp32-jura
# Initialize the repository.
# This call will download further repositories and moves them to their appropriate place.
./init.sh
```

### Step 1: Credentials.hpp
To be able to build the project you have to create the following file: ``  
Paste the following content in this newly created file:
```c++
#pragma once

#include <string>

//---------------------------------------------------------------------------
namespace esp32jura {
//---------------------------------------------------------------------------
/**
 * This file contains credentials for debugging the XMPP and WIFI connection,
 * without having to go through the usual setup procedure.
 * To apply these settings uncomment the "initWithDummyValues()" call in the "Esp32Jura.cpp" file.
 *
 * !!NEVER COMMIT THIS FILE!!
 **/
const std::string SSID = "Your WIFI SSID";
const std::string PASSWORD = "Your WIFI Password";
const std::string JID = "JID of this device";
const std::string JID_PASSWORD = "Password for the JID of this device";
const std::string JID_SENDER = "The owner JID of this device";
//---------------------------------------------------------------------------
}  // namespace esp32jura
//---------------------------------------------------------------------------
```
This file contains debug and testing credentials for deploying the application on an esp32 without having to go through the usual setup procedure.

### Step 3: Build
You can build the project with the following command:
```bash
cd esp32
idf.py build
```

### Step 4: Flash
To flash the application on an ESP32 run the following commands:
```bash
cd esp32
idf.py -p /dev/ttyUSB0 flash
```
Replace `/dev/ttyUSB0` with the port, where you plugged your ESP32 in to.

It might be required, that you add your current user to the `dialout` group.  
For this run the following command and **reboot** your pc afterwards.
```bash
sudo adduser "$USER" dialout
```

## Project Structure
```
.
├── esp32 # ESP IDF project
│   ├── ...
│   ├── CMakeLists.txt # Root CMake file
│   └── main # Source Code
│       ├── ...
│       ├── Esp32Jura.cpp # Main entry point
│       └── CMakeLists.txt
├── ...
├── init.sh # Initializes the project
├── LICENSE
├── protocol # Snoops of the JURA communication with a Smart Connect [1]
│   └── ...
└── README.md
```
`[1]`: https://uk.jura.com/en/homeproducts/accessories/SmartConnect-Main-72167
