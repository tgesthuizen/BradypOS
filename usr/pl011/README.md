# PL011 driver

This is a driver for the PL011 UART chip, as specified by ARM.
It is the uart driver in the RP2040 microcontroller.
The driver supports various different ways of receiving data, each
differently sophisticated and easy to implement.

## Current implementation state

The driver is in a very early state and fetches and writes data
character by character. The FIFOs are not yet used, which is very much
needed for efficient operation.
Furthermore, the driver is just polling and does not use interrupts
yet.

## Configuration

Right now the driver hardcodes the serial port to 8 bit words, one
stop bit, odd parity, 115200 baud.
