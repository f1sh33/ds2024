// File: modified_receiver_with_address.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>

#define BUFFER_SIZE 1024
#define FILE_NAME_MAX 255 // Assuming maximum file name length

int main(int argc, char *argv[]) {
    int port = 0; // Port to listen on, must be provided as an argument
    char outputFolder[FILE_NAME_MAX] = {0}; // Output folder for received files
    char bindAddress[INET_ADDRSTRLEN] = "0.0.0.0"; // Default bind address (all interfaces)
    int opt;

    while((opt = getopt(argc, argv, "p:o:a:")) != -1) {
        switch(opt) {
            case 'p':
                port = atoi(optarg);
                break;
            case 'o':
                strncpy(outputFolder, optarg, sizeof(outputFolder) - 1);
                outputFolder[sizeof(outputFolder) - 1] = '\0'; // Ensure null termination
                break;
            case 'a':
                strncpy(bindAddress, optarg, sizeof(bindAddress) - 1);
                bindAddress[sizeof(bindAddress) - 1] = '\0'; // Ensure null termination
                break;
            default:
                fprintf(stderr, "Usage: %s -p port -o outputFolder -a bindAddress\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (port <= 0 || strlen(outputFolder) == 0) {
        fprintf(stderr, "Port and output folder are required.\n");
        exit(EXIT_FAILURE);
    }

    int server_fd, new_socket;
    struct sockaddr_in address;
    int optval = 1;
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &optval, sizeof(optval))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(bindAddress); // Bind to specified address
    address.sin_port = htons(port);

    // Bind the socket to the specified address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Listener on %s:%d, waiting for files...\n", bindAddress, port);

    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    // First, receive the file name
    char fileName[FILE_NAME_MAX];
    read(new_socket, fileName, sizeof(fileName)); // Read the file name sent by the sender

    // Construct the full path where the file will be saved
    char filePath[FILE_NAME_MAX + FILE_NAME_MAX];
    snprintf(filePath, sizeof(filePath), "%s/%s", outputFolder, fileName);

    // Open the file for writing
    FILE *file = fopen(filePath, "wb");
    if (file == NULL) {
        perror("Failed to open file for writing");
        exit(EXIT_FAILURE);
    }

    // Then, receive the file content
    char buffer[BUFFER_SIZE];
    int bytesReceived;
    while ((bytesReceived = read(new_socket, buffer, BUFFER_SIZE)) > 0) {
        fwrite(buffer, 1, bytesReceived, file);
    }

    printf("File %s has been received and saved successfully in %s.\n", fileName, outputFolder);

    fclose(file);
    close(new_socket);
    close(server_fd);
    return 0;
}
