#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "parking_engine.h"
#include "http_server.h"

#define DB_FILE "parking_system.db"

static void handle_sigint(int sig) {
    (void)sig;
    printf("\nShutting down Smart Parking C11 Server gracefully...\n");
    http_server_stop();
    engine_shutdown();
    exit(0);
}

int main(int argc, char* argv[]) {
    int port = HTTP_PORT;
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0) port = HTTP_PORT;
    }

    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    printf("====================================================\n");
    printf("   Smart Parking Reservation & Management System    \n");
    printf("                 C11 REST Backend                   \n");
    printf("====================================================\n");

    // Initialize engine & database (automatically ensures schema & seed)
    if (engine_init(DB_FILE) != 0) {
        fprintf(stderr, "[Fatal Error] Failed to initialize parking engine with %s\n", DB_FILE);
        return 1;
    }

    // Start Winsock HTTP REST API Server
    if (http_server_start(port) != 0) {
        fprintf(stderr, "[Fatal Error] Failed to start HTTP server on port %d\n", port);
        engine_shutdown();
        return 1;
    }

    engine_shutdown();
    return 0;
}
