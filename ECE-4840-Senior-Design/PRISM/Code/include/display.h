#ifndef DISPLAY_H
#define DISPLAY_H

#include <string>
#include <vector>

// Writes a 24-bit BMP image to the Linux framebuffer (/dev/fb0).
// Returns true on success.
bool displayBMP(const std::string& bmpPath);

// Decodes a JPEG buffer and writes it to the Linux framebuffer (/dev/fb0).
// Returns true on success.
bool displayJPEGBuffer(const std::vector<uint8_t>& jpegData);

// Decodes an H.264 access unit buffer and writes decoded frame to framebuffer.
// If doBlit is false, decodes to maintain reference chain but skips writing to screen.
// Returns true if at least one frame was decoded and (if doBlit) displayed.
bool displayH264Buffer(const std::vector<uint8_t>& h264Data, bool doBlit = true);

// Flushes internal H.264 decode buffers to drop stale queued frames.
void flushH264Decoder();

// Clears the framebuffer to black.
void clearDisplay();

#endif // DISPLAY_H
