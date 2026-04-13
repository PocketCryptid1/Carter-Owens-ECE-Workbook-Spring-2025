#include "camera_app.h"
#include <iostream>
#include <csignal>

//packet injection test libraries
#include <pcap.h>
#include <cstring>

using namespace std;

// Global flag to signal when the application should exit
bool running = true;

// Signal handler for graceful shutdown
void signalHandler(int signal) {
    cout << "\nReceived signal " << signal << ", initiating graceful shutdown...\n";
    running = false;
}

void injectiontest() {
    const char* iface = NETWORK_INTERFACE;
    char errbuf[PCAP_ERRBUF_SIZE];

    // Open the network interface for packet injection
    pcap_t* handle = pcap_open_live(iface, 2048, 1, 1, errbuf);
    if (!handle) {
        cerr << "Error opening interface " << iface << ": " << errbuf << endl;
        return;
    }

    uint8_t frame[64] = {
        // 802.11 header
        0x08, 0x01, 0x00, 0x00, // Frame Control, Duration
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Destination: broadcast
        0x54, 0xc9, 0xff, 0x02, 0xe8, 0xbc, // Source: pi MAC
        0x54, 0xc9, 0xff, 0x02, 0xe8, 0xbc, // BSSID: pi MAC
        0x00, 0x00, // Sequence
        // Payload (your test data)
        'G', 'R', 'E', 'E', 'T', 'I', 'N', 'G', 'S', '_', 'P', 'R', 'O', 'G', 'R', 'A', 'M', 'S', '!'
    };

    size_t frame_len = 24 + 19 ; // header + payload

    if (pcap_inject(handle, frame, frame_len) == -1) {
        cerr << "Error injecting packet: " << pcap_geterr(handle) << endl;
    } else {
        cout << "Packet injected successfully\n" << endl;
    }

    pcap_close(handle);
    return;
}

// Entry point for the application
int main() {
    // Set up signal handlers for graceful shutdown
    //signal(SIGINT, signalHandler);
    //signal(SIGTERM, signalHandler);

    // Create camera application instance and run it
    //CameraApp app;
    //return app.run();

    injectiontest();
    return 0;

}