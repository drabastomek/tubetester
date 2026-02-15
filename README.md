# VTTester / TTesterLCD

DIY tube tester for popular audio tubes: standalone operation with LCD and encoder, plus optional remote control over serial (VTTester protocol).

**Original authors:**  
- **Adam Tatus** (traxman) — hardware design and concept  
- **Tomasz Gumny** — microcontroller firmware  

The project started on [Forum Trioda](https://forum-trioda.pl/viewtopic.php?t=12209) (November 2008). Full construction and user documentation appeared in *Elektronika Praktyczna* 4/5 and 6/2010.

This repository contains the firmware (ATmega32, avr-gcc / ICCAVR), the VTTester binary protocol (SET/MEAS, CRC-8), and a Python desktop app for remote control of the tester.

# New updates
In January 2026 the project is undergoing a modernization:

- minor hardware updates: the major one is moving away from ATMEGA16 in favor of ATMEGA32 [Modyfikacje ukladu miernika](https://forum-trioda.pl/viewtopic.php?t=42505&sid=e96a9c149bc85ab42df1796027ff0b44)
- adding remote control via serial messages to the tester. The modifications aim to enable adding the ability to remotely control the power supply mode as well as performing measurements. Proposed communication protocol [Propozycja protokolu komunikacji](https://forum-trioda.pl/viewtopic.php?t=42511&sid=e96a9c149bc85ab42df1796027ff0b44)
- adding desktop app written in Python so it can be deployed on any platform (Windows, Mac, Linux): [Aplikacja na komputer](https://forum-trioda.pl/viewtopic.php?t=42507&sid=e96a9c149bc85ab42df1796027ff0b44)