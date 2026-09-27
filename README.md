# AI Bot Connect-N
Embedded AI Bot using C++, Arduino and 13.56 MHz OOK communication to autonomously play Connect-N, developed as part of EEN1092 at Dublin City University.

The project involved designing and building a PCB-based AI Bot capable of communicating wirelessly with a central hub and playing Connect-N against another autonomous bot.

<img width="384" height="512" alt="image" src="https://github.com/user-attachments/assets/d5496263-0ce2-45ba-a33d-04c0aa353758" />

## Project Overview

The AI Bot combines embedded software, analogue electronics and game-playing logic.

The system includes:

* Custom TX and RX coils designed for approximately 13.56 MHz NFC communication
* On-Off Keying (OOK) wireless data transmission
* Analogue receiver and demodulation circuitry
* Arduino/C++ transmit and receive software
* PCB assembly and soldering
* Connect-N game logic
* PC-based game-board visualisation

## Hardware

Two five-turn copper coils were constructed for transmission and reception.

Measured values were approximately:

* **TX Coil:** 2.7 µH
* **RX Coil:** 2.4 µH
* **Carrier Frequency:** 13.56 MHz

The receiver circuit was designed and simulated using LTspice before being constructed on a breadboard and transferred to the AI Bot PCB.

## Communications

Communication between the Bot and Hub uses On-Off Keying of a 13.56 MHz carrier.

Messages are transmitted as 16-bit Protocol Data Units (PDUs). The communications software handles:

* Message transmission
* Message reception
* Bit timing
* Preamble detection
* Sequence information
* Move messages
* Communication with the game logic

## Software

The software is primarily written in **C++ and Arduino C++**.

The project contains classes for:

* Game-board representation
* Players
* Move selection
* Message storage
* Communications
* Game visualisation

Different player implementations were developed, including random and more strategic move-selection behaviour.

## Tools & Technologies

* C++
* Arduino
* Embedded Systems
* PCB Design & Assembly
* LTspice
* NFC / 13.56 MHz RF Communication
* On-Off Keying (OOK)
* Oscilloscope Testing
* Soldering
* Git/GitHub

## Repository Structure

```text
src/        C++ game logic and supporting classes
arduino/    Embedded TX/RX communications code
docs/       Project documentation and specifications
images/     Hardware, simulation and test-result images
```

## What I Learned

This project provided practical experience integrating software and hardware into a complete embedded system. It involved debugging both analogue circuits and embedded software, analysing oscilloscope measurements, designing resonant circuits, implementing a communications protocol and integrating wireless communication with game-playing software.

## Author

Kian Nalaza
Electronic and Computer Engineering
Dublin City University
