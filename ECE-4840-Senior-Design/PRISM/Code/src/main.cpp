#include "camera_app.h"
#include <iostream>
#include <csignal>

using namespace std;

// Global flag to signal when the application should exit
bool running = true;

// Signal handler for graceful shutdown
void signalHandler(int signal) {
    cout << "\nReceived signal " << signal << ", initiating graceful shutdown...\n";
    running = false;
}

// Entry point for the application
int main() {
    // Set up signal handlers for graceful shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Create camera application instance and run it
    CameraApp app;
    return app.run();
}