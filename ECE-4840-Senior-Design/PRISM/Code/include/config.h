#ifndef CONFIG_H
#define CONFIG_H

// Camera and encoding configuration

// Resolution settings
#define CAMERA_WIDTH 320
#define CAMERA_HEIGHT 240

// Framerate (frames per second)
#define CAMERA_FRAMERATE 30

// Video encoding settings
#define VIDEO_BITRATE 400000  // 400 kbps

// VRX display settings
// Decode can run faster, but blit to HDMI is rate-limited to reduce CPU usage.
#define VRX_MAX_DISPLAY_FPS 15

// Confirmed quality mode (ARQ) timing
// Mode selection is runtime-only via VTX CLI args (--confirm/--quality).
// ACK wait timeout per transmit attempt
#define ACK_TIMEOUT_MS 8
// Retries after initial send (total attempts = ACK_MAX_RETRIES + 1)
#define ACK_MAX_RETRIES 4
// Source MAC used by VRX when transmitting ACK packets
#define VRX_ACK_SOURCE_MAC {0x02, 0x11, 0x22, 0x33, 0x44, 0x55}

// Recording duration (set to 0 for continuous streaming)
#define RECORDING_DURATION_SECONDS 0
#define FRAMES_TO_RECORD (CAMERA_FRAMERATE * RECORDING_DURATION_SECONDS)

// Network settings
#define NETWORK_INTERFACE "wlan1"
#define PAYLOAD_MAX_SIZE 1024  // Max payload size for network packets
// Radiotap legacy rate field uses 500 kbps units (12 = 6 Mbps, valid on 5 GHz)
#define TX_RATE_500KBPS_UNITS 12

#endif // CONFIG_H
