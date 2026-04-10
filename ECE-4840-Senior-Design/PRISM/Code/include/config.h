#ifndef CONFIG_H
#define CONFIG_H

// Camera and encoding configuration

// Resolution settings
#define CAMERA_WIDTH 256
#define CAMERA_HEIGHT 240

// Framerate (frames per second)
#define CAMERA_FRAMERATE 30

// Video encoding settings
#define VIDEO_BITRATE 2000000  // 2 Mbps (reduced for real-time encoding)

// Recording duration
#define RECORDING_DURATION_SECONDS 3
#define FRAMES_TO_RECORD (CAMERA_FRAMERATE * RECORDING_DURATION_SECONDS)

#endif // CONFIG_H
