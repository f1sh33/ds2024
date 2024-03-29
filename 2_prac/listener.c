#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>

#define BUFFER_SIZE 1024

typedef struct {
    char type; // 'M' for metadata
    size_t size; // Size of the file
    char filename[256]; // Name of the file
} RPCMessage;

int main(int argc, char *argv[]) {
    int port = 0;
    char outputFolder[256] = "./"; // Default output folder
    char bindAddress[INET_ADDRSTRLEN] = "0.0.0.0";
    int opt;

    while((opt = getopt(argc, argv, "p:o:a:")) != -1) {
        switch(opt) {
            case 'p':
                port = atoi(optarg);
                break;
            case 'o':
                strncpy(outputFolder, optarg, sizeof(outputFolder) - 1);
                break;
            case 'a':
                strncpy(bindAddress, optarg, sizeof(bindAddress) - 1);
                break;
            default:
                fprintf(stderr, "Usage: %s -p port -o outputFolder -a bindAddress\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (port <= 0) {
        fprintf(stderr, "Port is required.\n");
        exit(EXIT_FAILURE);
    }

    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(bindAddress);
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Listener on %s:%d, waiting for files...\n", bindAddress, port);

    new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
    if (new_socket < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    // Receive file metadata
    RPCMessage metadata;
    recv(new_socket, &metadata, sizeof(metadata), 0);
    if (metadata.type == 'M') {
        printf("Receiving file: %s, size: %lu\n", metadata.filename, metadata.size);

        char filePath[512];
        snprintf(filePath, sizeof(filePath), "%s/%s", outputFolder, metadata.filename);

        FILE *file = fopen(filePath, "wb");
        if (file == NULL) {
            perror("Failed to open file for writing");
            exit(EXIT_FAILURE);
        }

        // Receive file data
        char buffer[BUFFER_SIZE];
        size_t totalBytes = 0;
        while (totalBytes < metadata.size) {
            ssize_t bytesReceived = recv(new_socket, buffer, BUFFER_SIZE, 0);
            if (bytesReceived <= 0) {
                // Handle errors or connection closed by client
                break;
            }
            fwrite(buffer, 1, bytesReceived, file);
            totalBytes += bytesReceived;
        }

        // After successfully receiving and saving the file
        RPCMessage response;
        response.type = 'S'; // Confirmation message type "S" for Success
        send(new_socket, &response, sizeof(response), 0);


        printf("File %s has been received and saved successfully.\n", metadata.filename);

        fclose(file);
    } else {
        RPCMessage response;
        response.type = 'U'; // Confirmation message type "U" for Unsuccess
        send(new_socket, &response, sizeof(response), 0);
        printf("Unexpected message type received.\n");
    }

    close(new_socket);
    close(server_fd);

    return 0;
}
