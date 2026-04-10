# ECE CAPSTONE PROJECT
## PRISM – Digital FPV Video Transmission System

**NAME(S)**  
**EMAIL ADDRESS(ES)**

**April 2026**

---

## Table of Contents
1. [Executive Summary](#1-executive-summary)
2. [Introduction](#2-introduction)
3. [Methods](#3-methods)
4. [Results](#4-results)
5. [Discussion](#5-discussion)
6. [Conclusion](#6-conclusion)

---

## 1. Executive Summary

The executive summary provides an overview of the content contained in the report. Many people write this section after the rest of the document is completed. This section is important in that it provides a high-level summary of the detail contained within the rest of the document.

---

## 2. Introduction

First-person view (FPV) drone pilots rely on real-time video feeds transmitted from their aircraft to goggles or monitors on the ground. Traditional analog video systems offer low latency but suffer from poor image quality and interference susceptibility. Existing digital solutions, while offering superior image quality, often introduce unacceptable latency or require expensive proprietary hardware that limits customization and educational value.

PRISM addresses these challenges by developing an open-source digital video transmission system specifically designed for FPV applications. The system leverages commodity hardware—Raspberry Pi single-board computers and USB WiFi adapters—to create a cost-effective, customizable platform. A key innovation is the dual transmission mode approach: a "latency mode" that mimics UDP's fire-and-forget behavior for time-critical flying, and a "quality mode" with TCP-like packet acknowledgment for scenarios where image fidelity matters more than response time.

The core networking concept utilizes WiFi chips operating in monitor/inject mode, bypassing the standard networking stack to create a minimal custom packet format. This approach reduces overhead and provides fine-grained control over transmission behavior—something not possible with off-the-shelf streaming solutions.

This project serves dual purposes: providing a practical FPV video system for drone pilots while also creating an educational platform that demonstrates embedded systems, real-time video processing, and wireless networking concepts.

**Project Objectives:**
1. Successfully transmit live video from the airborne transmitter (VTX) to the ground receiver (VRX) with latency suitable for FPV flight
2. Support switchable one-way and two-way transmission modes to balance latency versus quality
3. Interface with drone flight controllers to generate an On-Screen Display (OSD) overlay with telemetry data

---

## 3. Methods

### Hardware Platform

The PRISM system consists of two nearly identical units: a video transmitter (VTX) mounted on the aircraft and a video receiver (VRX) on the ground.

**Transmitter (VTX) Components:**
- Raspberry Pi Zero 2 W – Quad-core ARM Cortex-A53 running Raspberry Pi OS (Bookworm)
- Raspberry Pi Camera Module 3 connected via CSI-2 interface
- USB WiFi adapter with RTL8821AU chipset (capable of monitor/inject mode)
- RHCP antenna 
- Power supplied from drone battery via voltage regulator

**Receiver (VRX) Components:**
- Raspberry Pi Zero 2 W (identical to transmitter for part commonality)
- USB WiFi adapter with RTL8821AU chipset
- RHCP antenna
- HDMI output to FPV goggles or monitor


### Software Architecture

The software is written in C++17, compiled with GCC, and uses a modular architecture with clearly separated concerns:

| Module | Purpose |
|--------|---------|
| `main.cpp` | Application entry point, signal handling for graceful shutdown |
| `camera_app.cpp` | Camera initialization using libcamera, frame capture management |
| `h265_encoder.cpp` | Video encoding using FFmpeg/libavcodec with hardware acceleration |
| `config.h` | Centralized configuration (resolution, framerate, bitrate) |

**Video Pipeline (VTX):**
1. Camera captures frames and passes them to the H.264 hardware encoder (`h264_v4l2m2m`) via the V4L2 memory-to-memory interface
3. Encoded packets are prepared for wireless transmission

**Build System:**
The project uses a Makefile build system with pkg-config for dependency management (libcamera, libavcodec, libavformat, libavutil). Development occurs on a host machine with VSCode, deploying to the Raspberry Pi via rsync over USB Ethernet.

### Development Workflow

Development occurs on a host machine using VSCode. Code is deployed to the Raspberry Pi via rsync over SSH on the local network, enabling rapid iteration and testing. Remote debugging and log monitoring are performed through standard SSH sessions.

### Coding Standards

The codebase follows modern C++ conventions:
- RAII (Resource Acquisition Is Initialization) for memory management using `std::unique_ptr`
- Consistent naming: PascalCase for classes, camelCase for methods/variables
- Documented code with comments explaining non-obvious logic
- Signal handlers for graceful shutdown (SIGINT, SIGTERM)

### Safety and Ethical Considerations

- The system operates on WiFi frequencies, requiring compliance with local radio regulations
- FPV flying carries inherent safety risks; the system is designed with clear latency specifications to inform pilots of expected performance
- Open-source design promotes transparency and allows community review of safety-critical code

---

## 4. Results

This section describes the results from your testing.

- This is where you present the results (especially testing results) that you got.
- Use graphs and tables if appropriate, but also summarize your main findings in the text.
- Do **NOT** discuss the results or speculate as to why something happened; that goes in the Discussion section.
- You don't necessarily have to include all the data you've gotten during the semester. This isn't a diary, just a report of final results.
- Use appropriate methods of showing data. Don't try to manipulate the data to make it look like you did more than you actually did.

---

## 5. Discussion

This section discusses the implications of your test results and of your project overall.

- Highlight the most significant results, but don't just repeat what you've written in the Results section.
- How do these results relate to the original problem?
- Does the data suggest that your solution worked?
- Are your results consistent with what other engineers have reported? If your results were unexpected, try to explain why. Is there another way to interpret your results?
- What further research would be necessary to answer the questions raised by your results?
- How do your results fit into the big picture?
- End with a one-sentence summary of your conclusion, emphasizing why it is relevant.

---

## 6. Conclusion

This section should discuss what you learned from doing this project.

- What did you learn from doing the project?
- What would you have done differently?
- What improvements would you make in the future?
- If the project is being passed to another team, what advice would you give them?
