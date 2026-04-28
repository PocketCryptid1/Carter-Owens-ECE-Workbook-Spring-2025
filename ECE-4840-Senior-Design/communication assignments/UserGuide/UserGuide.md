# PRISM FPV Video Transmission System – User Guide

---

## Introduction

### What will these instructions help me do?

This user guide provides complete setup and operation instructions for the PRISM digital FPV video transmission system. By following this guide, you will be able to:

- Assemble the transmitter and receiver units
- Configure the software on both Raspberry Pi units
- Establish a wireless video connection between transmitter and receiver
- Stream live H.264 video from an onboard camera to a ground receiver with ~250ms latency
- Switch between low-latency mode (no confirmation) and high-quality confirmed mode (ARQ)
- Monitor link health and diagnose RF performance
- Troubleshoot common connection and performance issues

### Is there anything special I need to know?

- **Raspberry Pi experience recommended**: Familiarity with Linux command line and SSH connections is helpful
- **Monitor mode required**: Both WiFi adapters must operate in 802.11 monitor mode for packet injection
- **5 GHz band only**: PRISM operates on 5 GHz channels; 2.4 GHz is not supported
- **Direct line of sight**: Best performance requires clear line of sight between transmitter and receiver antennas
- **Power management important**: Ensure both units have adequate power throughout operation

---

## Description of Equipment

### Transmitter Unit (VTX)

![VTX Parts](VTXparts.jpg)

The transmitter unit captures video from a camera, encodes it with H.264, and broadcasts it over 802.11 monitor mode.

**Main Components:**

- **Raspberry Pi Zero 2 W** – Compact single-board computer that processes video
- **Raspberry Pi Camera Module 3 (IMX708)** – CSI-2 camera interface with wide-angle lens
- **USB WiFi Adapter** (RTL8821AU chipset) – Wireless transmission interface
- **RHCP Antenna** – Right-hand circularly polarized antenna for transmission
- **Mounting bracket** – Hardware for drone integration

**Processing Pipeline:**
1. libcamera captures raw video at 320×240, 30 fps
2. h264_v4l2m2m hardware encoder produces ~400 kbps H.264 stream
3. PRISM packetizes encoded data into 1024-byte chunks
4. Packets are injected as 802.11 probe-request frames via pcap monitor mode

### Receiver Unit (VRX)

![VRX Parts](VRXparts.jpg)

The receiver unit captures transmitted packets, reassembles them, decodes H.264, and displays video on HDMI.

**Main Components:**

- **Raspberry Pi 4 or Zero 2 W** – Single-board computer for packet capture and decoding
- **USB WiFi Adapter** (RTL8821AU chipset) – Wireless reception interface
- **RHCP Antenna** – Right-hand circularly polarized antenna for reception
- **Micro-HDMI to HDMI Cable** – Connection to display (1920×1080 framebuffer)
- **Power supply** – 5V, 2.5A minimum

**Processing Pipeline:**
1. Monitor mode capture via pcap with 128KB buffer and immediate mode
2. Packet filtering by source MAC and magic number
3. Per-frame chunk reassembly with timeout/abandonment logic
4. H.264 decode with error concealment (FF_EC_GUESS_MVS | FF_EC_DEBLOCK)
5. Scaled nearest-neighbor blit to HDMI framebuffer (15 fps rate-limited)
6. Idle screen fallback after 1.5 seconds of silence

### Software Components

- **libcamera** – Modern camera capture interface for Raspberry Pi
- **FFmpeg (libavcodec/libavutil/libswscale)** – H.264 encoding and decoding
- **libpcap** – Wireless packet capture and injection
- **Custom C++17 application** – PRISM encoder, packetizer, decoder, and display logic

---

## List of Materials/Tools Needed

### Hardware Components

| Component | Quantity | Notes |
|-----------|----------|-------|
| Raspberry Pi Zero 2 W | 2 | One for transmitter, one for receiver |
| Raspberry Pi Camera Module 3 (IMX708) | 1 | For transmitter unit only; wide-angle lens |
| USB WiFi Adapter (RTL8821AU) | 2 | One per Raspberry Pi |
| RHCP Antenna | 2 | One per WiFi adapter |
| Micro-HDMI to HDMI Cable | 1 | For receiver video output |
| MicroSD Card (32GB+) | 2 | For operating system storage |
| USB Power Adapter | 2 | 5V, 2.5A minimum per unit |

### Software and Tools

- **Raspberry Pi OS Lite (Bookworm, 64-bit)** – Official operating system
- **SSH client** – For remote access (OpenSSH, PuTTY, or terminal)
- **Raspberry Pi Imager** – For flashing microSD cards
- **Text editor** – For configuration file editing (nano, vim, etc.)

---

## Safety Warnings

**ELECTRICAL SAFETY**
- Never connect power while performing hardware assembly
- Always use approved power adapters rated for 5V, 2.5A minimum
- Do not modify power cables or connectors
- Keep liquids away from all electronic components
- Disconnect power before handling internal components

**HARDWARE SAFETY**
- Handle Raspberry Pi boards by the edges only
- Avoid static electricity – use an anti-static wrist strap when handling components
- Do not force connectors; align carefully before inserting
- Allow adequate ventilation for cooling, especially during extended operation
- The ribbon cable connectors are fragile—handle with care

**RF SAFETY AND REGULATIONS**
- Do not operate transmitter near hospitals, aircraft, or restricted areas
- Check local regulations regarding unlicensed wireless operation on 5 GHz band
- Maintain visual line of sight with any associated drone or vehicle
- Keep antennas away from people during operation
- Do not exceed legal RF power limits in your jurisdiction

---

## Directions

### Phase 1: Hardware Assembly

#### Step 1: Prepare the Transmitter Unit

1. Gather transmitter components: Raspberry Pi Zero 2 W, Camera Module 3 (IMX708), WiFi adapter, RHCP antenna
2. Power is OFF – verify by checking no lights are active

3. Insert the camera ribbon cable into the CSI-2 port:
   - Lift the plastic retention tab on the CSI-2 connector
   - Align the ribbon cable (contacts face downward on Zero 2 W)
   - Slide the ribbon in fully
   - Press the tab down firmly to secure

   **Information:** The connector is stiff on new boards. Ensure the ribbon is straight and fully inserted before closing the tab.

4. Attach the USB WiFi adapter to a USB port using a USB-A to micro-B adapter cable

5. Attach the RHCP antenna to the WiFi adapter's antenna connector (screw-on type)

6. Verify the camera ribbon is secure and antenna is firmly attached

#### Step 2: Prepare the Receiver Unit

1. Gather receiver components: Raspberry Pi (Zero 2 W or 4), WiFi adapter, RHCP antenna, Micro-HDMI to HDMI cable
2. Power is OFF

3. Attach the USB WiFi adapter to a USB port using a USB-A to micro-B adapter cable

4. Attach the RHCP antenna to the WiFi adapter's antenna connector

5. Connect the Micro-HDMI to HDMI cable to the Micro-HDMI port on the Raspberry Pi
   - The connector is keyed; align before inserting

6. Connect the USB power adapter (but do NOT plug it in yet)

7. Set the receiver unit aside in a safe, ventilated location

### Phase 2: Software Installation

#### Step 3: Flash Operating System

1. Insert a microSD card into your workstation

2. Open **Raspberry Pi Imager** (download from raspberrypi.com if needed)

3. Click **Choose OS** → **Raspberry Pi OS (other)** → **Raspberry Pi OS Lite (64-bit)**

4. Click **Choose Storage** and select your microSD card

5. Click **Edit Settings** (gear icon) to configure:
   - **Hostname:** `prism-vtx` (transmitter) or `prism-vrx` (receiver)
   - **Username:** `pi`
   - **Password:** (set a secure password, e.g., 12+ characters)
   - **Configure wireless LAN:** (optional, can set up manually later)
   - **Locale settings:** Your timezone and keyboard layout

6. Click **Write** and wait for the operation to complete (5–10 minutes)

7. Eject the microSD card safely

8. Repeat Steps 1–7 for the second Raspberry Pi with the other hostname

#### Step 4: Boot and Initial Network Configuration

1. Insert the flashed microSD card into the Raspberry Pi

2. Connect the USB power adapter to power on the Raspberry Pi

3. Wait 60 seconds for first boot to complete

4. From your workstation, SSH into the transmitter:

       ssh pi@prism-vtx.local

   (If `.local` doesn't work, find the IP address on your router or use a network scan tool, then: `ssh pi@192.168.x.x`)

5. Enter the password you configured in Step 3

6. Update system packages:

       sudo apt update && sudo apt upgrade -y

   This may take 10–15 minutes. Allow it to complete.

7. Repeat Steps 4–6 for the receiver unit (`prism-vrx`)

#### Step 5: Install PRISM Software and Dependencies

1. On the transmitter, SSH in and install build tools and libraries:

       sudo apt install -y build-essential git cmake pkg-config \
         libcamera-dev libavcodec-extra libavutil-dev libswscale-dev \
         libpcap-dev libjpeg-dev

   This may take 5–10 minutes.

2. Clone the PRISM repository:

       cd ~
       git clone github.com/PRISMorg/PRISM
       cd PRISM/Code



3. Compile the PRISM application:

       make clean
       make vtx

   Compilation may take 5–10 minutes. Do NOT power off during this.

4. Verify the binary was created:

       ls -l bin/vtx

5. Repeat Steps 1–4 on the receiver unit, but compile `make vrx` instead

#### Step 6: Configure WiFi Monitor Mode

This step enables packet injection on 802.11 monitor mode, which is required for PRISM.

1. SSH into the transmitter:

       ssh pi@prism-vtx.local

2. Check the current WiFi interface name:

       ip link show | grep -i wlan

   (Usually `wlan0` or `wlan1`)

3. Bring down the WiFi interface:

       sudo ip link set wlan1 down

4. Set the interface to monitor mode:

       sudo iw dev wlan1 set type monitor

5. Bring the interface back up:

       sudo ip link set wlan1 up

6. Verify monitor mode is active:

       iw dev wlan1 link

   **Expected output:** Should show `Not connected` and indicate monitor mode is enabled.

7. Repeat Steps 2–6 on the receiver unit

**Information:** Monitor mode must be re-enabled after every reboot. You can automate this by adding the commands to `/etc/rc.local` or a startup systemd service if you prefer.

### Phase 3: System Operation and Testing

#### Step 7: Launch Transmitter in Low-Latency Mode

1. SSH into the transmitter:

       ssh pi@prism-vtx.local

2. Navigate to the PRISM directory:

       cd ~/PRISM/Code

3. Start the transmitter in low-latency mode:

       ./bin/vtx --latency

   **Expected output:**
   ```
   VTX transmitting on wlan0 — Ctrl+C to stop
   Confirm mode: OFF
   Mode: camera+hardware-encoder
   ```

   You should see encoding frames printed to the terminal.

4. Leave this running; the transmitter is now broadcasting H.264 packets

#### Step 8: Launch Receiver in Low-Latency Mode

1. SSH into the receiver in a new terminal:

       ssh pi@prism-vrx.local

2. Connect your HDMI display to the receiver's Micro-HDMI output if not already done

3. Navigate to the PRISM directory:

       cd ~/PRISM/Code

4. Start the receiver in low-latency mode:

       ./bin/vrx

   **Expected output:**
   ```
   VRX listening on wlan0...
   [VRX stats] frames=0 abandoned=0 loss%=0 dup=0 resyncs=0 ok=0 fail=0 skip=0 silence_ms=XXXX
   ```

5. You should see live video on your HDMI display within 2–5 seconds

6. Monitor the receiver statistics every second:
   - `frames=` total frames completed
   - `loss%=` RF packet loss percentage
   - `ok=` frames successfully displayed
   - `skip=` frames dropped by rate limiter (15 fps max)

**Information:** Initial "silence_ms" will be large (>1500ms) until first frame arrives; then it should be <50ms.

#### Step 9: Switch to High-Quality (Confirmed) Mode

High-quality mode uses ARQ (automatic repeat request) to confirm packet delivery, resulting in higher latency (~500–1000ms) but more reliable delivery.

1. On the transmitter, stop the current application:

       CTRL+C

2. Restart in quality mode:

       ./bin/vtx --quality

   **Expected output:**
   ```
   VTX transmitting on wlan0 — Ctrl+C to stop
   Confirm mode: ON
   Mode: camera+hardware-encoder
   ```

3. On the receiver, the stats line will now include:

       ack_tx=XXXX ack_fail=X

   where `ack_tx` is the count of ACK packets sent and `ack_fail` is ACK transmission failures.

4. Latency should increase (you'll see longer delays between camera motion and screen update)

5. Packet loss should decrease as chunks are retransmitted until ACKed

**Information:** If `ack_fail` climbs rapidly, it indicates the return ACK path is having issues (possibly RF attenuation or interference). Try adjusting antenna positions.

#### Step 10: Switch Back to Low-Latency Mode

1. On the transmitter, stop the application:

       CTRL+C

2. Restart in low-latency mode:

       ./bin/vtx --latency

   (or simply `./bin/vtx` since low-latency is the default)

3. Latency should return to ~250ms

---

## Configuration

### Tunable Parameters

Most configuration is in `include/config.h`. Recompile after changes:

```bash
make clean && make vtx  # or make vrx
```

**Key parameters:**

- `CAMERA_WIDTH` / `CAMERA_HEIGHT` – Video resolution (default 320×240)
- `CAMERA_FRAMERATE` – Encoding framerate (default 30 fps)
- `VIDEO_BITRATE` – H.264 bitrate in bits/sec (default 400000 = 400 kbps)
- `TX_RATE_500KBPS_UNITS` – 802.11 TX rate in 500 kbps units (default 12 = 6 Mbps)
- `ACK_TIMEOUT_MS` – Time to wait for ACK per transmit attempt (default 8 ms)
- `ACK_MAX_RETRIES` – Maximum retry attempts per chunk in quality mode (default 4)
- `VRX_MAX_DISPLAY_FPS` – Maximum blit rate to HDMI framebuffer (default 15 fps)

---

## Performance and Diagnostics

### Reading the VRX Stats Line

The receiver prints statistics every second:

```
[VRX stats] frames=100 abandoned=2 loss%=3 dup=0 resyncs=0 ok=85 fail=0 skip=15 ack_tx=300 ack_fail=0 silence_ms=45
```

| Field | Meaning |
|-------|---------|
| `frames` | Total frames completed (assembled and displayed) |
| `abandoned` | Frames started but dropped before completion (new frame arrived) |
| `loss%` | Estimated RF packet loss percentage |
| `dup` | Duplicate chunks received |
| `resyncs` | Number of times frame metadata changed unexpectedly |
| `ok` | Frames successfully decoded and blitted to HDMI |
| `fail` | Frames that failed to decode |
| `skip` | Frames dropped by 15 fps display rate limiter |
| `ack_tx` | ACK packets transmitted (quality mode only) |
| `ack_fail` | ACK transmission failures (quality mode only) |
| `silence_ms` | Milliseconds since last packet received |

**Healthy low-latency session:**
- `loss%` 1–5%
- `ok` high, `fail` low
- `silence_ms` <50ms

**Healthy quality-mode session:**
- `loss%` <1% (due to retransmissions)
- `ack_tx` and `ack_fail` both present; ratio should be ~100:1 or better
- Latency ~500–1000ms

### Adjusting for Poor RF Conditions

If you see high loss or frequent abandonment:

1. **Move antennas farther apart** (at least 1–2 feet) to avoid saturation
2. **Check antenna alignment** – RHCP antennas work best when parallel and vertical
3. **Reduce bitrate** – Edit `config.h`, set `VIDEO_BITRATE 300000` (300 kbps), recompile
4. **Reduce resolution** – Set `CAMERA_WIDTH 240 CAMERA_HEIGHT 180`, recompile
5. **Use quality mode** – ACK retransmissions improve delivery at cost of latency

---

## Troubleshooting

### Issue: Cannot SSH into Raspberry Pi

**Symptom:** Connection timeout or "host unreachable"

**Solutions:**
1. Verify power is on (LED should be lit)
2. Try ping to check network connectivity:

       ping prism-vtx.local

3. If no response, find the IP on your router or use:

       sudo arp-scan --localnet  # Linux/Mac

4. SSH using IP directly:

       ssh pi@192.168.x.x

5. If still no connection, restart the Pi by disconnecting power, waiting 10 seconds, and reconnecting

### Issue: Camera Module Not Detected

**Symptom:** "Camera initialization failed" error on VTX startup

**Solutions:**
1. Power off the Raspberry Pi:

       sudo poweroff

2. Verify the camera ribbon cable is fully inserted and aligned
3. Check that the CSI-2 connector tab is closed securely
4. Power on and test with:

       libcamera-hello --list-cameras

5. If camera is listed but PRISM fails, try re-seating the ribbon:
   - Power off and disconnect power
   - Lift the connector tab on the CSI-2 port
   - Gently remove and re-insert the ribbon (contacts face down)
   - Close the tab firmly
   - Reconnect power

### Issue: Monitor Mode Fails to Activate

**Symptom:** Error message when running `iw dev wlan0 set type monitor`

**Solutions:**
1. Verify the interface name:

       ip link show | grep wlan

2. Ensure the interface is down before changing mode:

       sudo ip link set wlan0 down

3. Stop conflicting network services:

       sudo systemctl stop NetworkManager
       sudo systemctl stop dhcpcd

4. Try the mode change again:

       sudo iw dev wlan0 set type monitor
       sudo ip link set wlan0 up

5. Verify success:

       iw dev wlan0 link

### Issue: No Video Signal on Receiver

**Symptom:** Receiver says "VRX listening..." but displays idle screen (silence_ms growing)

**Solutions:**
1. Verify both units are compiled and running:
   - VTX terminal should show encoding frames
   - VRX terminal should show stats line updating

2. Check RF signal by examining loss% in VRX stats (should be <50% initially)

3. Verify monitor mode is active on both units:

       iw dev wlan0 link

4. Restart both applications:
   - Stop both with `CTRL+C`
   - Wait 5 seconds
   - Start VTX first, wait 3 seconds, then start VRX

5. Check for antenna connection:
   - Verify antenna is screwed on firmly to WiFi adapter
   - Try moving antennas farther apart (1–2 feet)
   - Check line of sight between antennas

6. Verify HDMI connection:
   - Unplug and reconnect the Micro-HDMI cable firmly
   - Test with a different HDMI cable if available
   - Connect to a different display if possible

### Issue: Video Freezes or Artifacts

**Symptom:** Video playback is choppy, freezes intermittently, or shows distorted frames

**Solutions:**
1. Check transmitter CPU usage:

       top

   Press `q` to exit.

2. If CPU >80%, reduce load:
   - Reduce framerate in `config.h` (try 15 fps instead of 30)
   - Reduce resolution (try 240×180)
   - Reduce bitrate (try 300 kbps)
   - Recompile and test

3. Improve RF conditions:
   - Move antennas farther apart
   - Ensure clear line of sight
   - Reduce multi-path reflections (move away from metal objects)

4. Switch to quality mode for more reliable delivery (but higher latency)

5. Check for background network traffic:

       iftop -i wlan0  # (install if needed: sudo apt install iftop)

### Issue: High Latency Even in Low-Latency Mode

**Symptom:** Noticeable delay (>500ms) between camera motion and screen update

**Solutions:**
1. Verify you're using low-latency mode:

       # On VTX
       ./bin/vtx --latency  # or just ./bin/vtx (default)

2. Check that encoding is hardware-accelerated:
   - The VTX terminal should print frame info consistently
   - If dropping frames, CPU may be saturated

3. Reduce resolution or framerate:
   - Try 240×180 at 15 fps in `config.h`
   - Recompile and test

4. Check for background processes:

       top -o %CPU  # Sort by CPU usage

5. Move antennas closer together (but not too close, to avoid saturation):
   - Start at 1–2 feet apart
   - If signal is weak, increase distance

### Issue: Cannot Recompile or Build Errors

**Symptom:** `make` fails with missing headers or library errors

**Solutions:**
1. Verify all dependencies are installed:

       sudo apt install -y build-essential git cmake pkg-config \
         libcamera-dev libavcodec-extra libavutil-dev libswscale-dev \
         libpcap-dev libjpeg-dev

2. Clean and rebuild:

       make clean
       make vtx   # or make vrx

3. If you see "cannot find -l<library>", check it's installed:

       dpkg -l | grep libname

4. If still failing, check the Makefile is present:

       ls -la Makefile

---

## Conclusion

### What should I have when done?

Upon successful completion, you should have:

**Two fully assembled and operational Raspberry Pi units:**
- Transmitter with camera and WiFi adapter in monitor mode
- Receiver with HDMI output and WiFi adapter in monitor mode

**Functional PRISM software:**
- `bin/vtx` compiled and runnable
- `bin/vrx` compiled and runnable
- Both units able to switch between low-latency and quality modes via CLI flags

**Live H.264 video transmission:**
- Camera video displayed on receiver HDMI output
- ~250ms latency in low-latency mode
- <1% packet loss in quality mode with ARQ
- Idle screen shown on receiver when signal drops for >1.5 seconds

### Next Steps

- **Optimize for your environment:** Adjust antenna placement and distance to minimize loss and latency
- **Monitor statistics:** Use the VRX stats line to diagnose RF and decode issues
- **Test quality mode:** Verify ACK delivery improves reliability for longer-range flights
- **Reduce latency further (advanced):** Consider MJPEG or JPEG-only modes if H.264 pipeline adds too much delay
- **Add drone integration:** Mount VTX on your drone and verify video quality during flight

For additional support or feature requests, contact the PRISM development team.
