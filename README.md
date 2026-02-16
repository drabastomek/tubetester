# VTTester / TTesterLCD

**DIY tube tester for popular audio tubes.**  
Operates as a standalone instrument with LCD and encoder, with optional remote control via serial interface using the VTTester protocol.

**Original authors:**  
- **Adam Tatus (traxman)** — hardware design and concept  
- **Tomasz Gumny** — microcontroller firmware  

The project began on [Forum Trioda](https://forum-trioda.pl/viewtopic.php?t=12209) in November 2008.  
Full construction details and user documentation were later published in *Elektronika Praktyczna* issues 4/5 and 6/2010.

This repository includes:
- Microcontroller firmware for ATmega32 (avr-gcc)  
- VTTester binary protocol implementation (SET/MEAS commands, CRC-8)  
- Python desktop application for PC-based remote control  

---

## Project updates — January 2026

The VTTester project is currently being modernized with several improvements:

- **Hardware updates:** transition from ATmega16 to ATmega32  
  [Forum discussion – Hardware modifications](https://forum-trioda.pl/viewtopic.php?t=42505&sid=e96a9c149bc85ab42df1796027ff0b44)
- **Remote control capabilities:** implementation of serial communication for controlling the power supply mode and performing measurements  
  [Proposed communication protocol](https://forum-trioda.pl/viewtopic.php?t=42511&sid=e96a9c149bc85ab42df1796027ff0b44)
- **Cross-platform desktop application:** new Python-based GUI for Windows, macOS, and Linux  
  [Forum post – Desktop app](https://forum-trioda.pl/viewtopic.php?t=42507&sid=e96a9c149bc85ab42df1796027ff0b44)

---

The goal of this modernization is to ensure long-term maintainability and make the VTTester easier to integrate with modern development workflows and operating systems.
