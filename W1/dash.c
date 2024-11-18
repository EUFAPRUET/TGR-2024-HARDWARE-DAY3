#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

#define PORT 8080
#define BUFFER_SIZE 1024

pid_t process_pid = -1;

void start_process() {
    if (process_pid == -1) {
        process_pid = fork();
        if (process_pid == 0) {
            // Child process: Replace "your-process-command" with the actual command to start the process
            execlp("./sound_app", "./sound_app", "plughw:2,0", "480000", (char *)NULL);
            perror("execlp");
            exit(EXIT_FAILURE);
        } else if (process_pid < 0) {
            perror("fork");
        } else {
            printf("Process started with PID %d\n", process_pid);
        }
    } else {
        printf("Process is already running with PID %d\n", process_pid);
    }
}


void stop_process() {
    if (process_pid != -1) {
        if (kill(process_pid, SIGTERM) == 0) {
            printf("Process with PID %d stopped\n", process_pid);
            process_pid = -1;
        } else {
            perror("kill");
        }
    } else {
        printf("No process is running\n");
    }
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    int bytes_read;

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Define the server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Waiting for connections...\n");

    // Accept an incoming connection
    while ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) >= 0) {
        printf("Connection accepted\n");

        // Read the command from the client
        bytes_read = read(new_socket, buffer, BUFFER_SIZE);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            printf("Received command: %s\n", buffer);

            if (strcmp(buffer, "start") == 0) {
                start_process();
            } else if (strcmp(buffer, "stop") == 0) {
                stop_process();
            } else {
                printf("Unknown command: %s\n", buffer);
            }
        }

        close(new_socket);
    }

    if (new_socket < 0) {
        perror("accept");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    close(server_fd);
    return 0;
}