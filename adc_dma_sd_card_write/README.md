## Description

- This demo reads audio samples from the ADC at 384kHz and writes a .wav file to the SD card
- It runs on a MAX32666 FTHR2 board

## Software

### Project Usage

Universal instructions on building, flashing, and debugging this project can be found in the **[MSDK User Guide](https://analogdevicesinc.github.io/msdk/USERGUIDE/)**.

### Project-Specific Build Notes

(None - this project builds as a standard example)

## Required Connections

- This demo requires the FTHR2 to be installed in the custom Magpie hardware stack with AFE board installed
- Set FTHR2 jumper J4 to the 1.8v position to tie the I2C pullups to 1.8v 
- Insert an SD card into the FTHR2 slot (format the SD card to exFAT prior to inserting)
- Connect a USB cable between the PC and the FTHR2 to power the boards

## Expected Output

The demo attempts to write a short audio file to the root of the SD card file system. LEDs indicate the state of the system.

During execution the LEDs show which stage we are on

- Red LED while initializing, mounting, and opening the file on the SD card
- Blue while initializing and starting the ADC and DMA
- Green while recording audio
- Note that some stages may finish very quickly, so you may not see the LEDs on during initialization and setup

If there is a problem then one of the onboard FTHR2 LEDs rapidly blink forever as a primitive error handler

- rapid Red LED indicates an error initializing, mounting, or writing to the SD card
- rapid Blue LED indicates a problem with the ADC or DMA stream


At the end a slow Green LED blink indicates that all operations were successful

After execution is complete a WAVE file will be created at the root of the SD card file system. You can listen to this
file with an audio player and inspect the contents with a text editor able to view files as raw hex.
