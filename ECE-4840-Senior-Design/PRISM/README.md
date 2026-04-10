# PRISM – Digital FPV Video Transmission System

PRISM is a senior design engineering project that develops a low-latency digital video transmission system for FPV (First Person View) drones. The system streams live video from an onboard camera to a ground receiver using embedded Linux computers and wireless networking.

This project combines computer engineering, embedded systems, networking, and real-time video processing into a practical aerospace and robotics application.

the networking Core concept is using packet confirmation (like tcp) in quality mode, and ignoring packet confirmation (like udp) in latency mode. the plan is to use the wifi chip in Inject/Monitor mode so i can create a minimal packet format

## Project Goals
- Provide a real-time digital video feed from drone to pilot
- Support multiple transmission modes (low-latency and high-quality)
- Operate on small, lightweight hardware
- Be reliable and easy to configure
- Serve as an educational platform for embedded video systems

## Hardware (Version 1 Prototype)
- Raspberry Pi Zero 2 W (transmitter and receiver)
- Raspberry Pi Camera 3 (CSI-2)
- USB WiFi adapter (RTL8821AU chipset)
- RHCP and LHCP antennas
- HDMI monitor or FPV goggles (receiver)
- Drone battery (transmitter power)

## Software Stack
- Language: C++
- OS: Raspberry Pi OS (Bookworm)
- Camera interface: libcamera / rpicam
- Networking: undetermined
- Development: VSCode with rsync deployment

## System Architecture
**Transmitter (VTX):**
Camera → libcamera → encode → WiFi transmit

**Receiver (VRX):**
WiFi receive → decode → HDMI output
