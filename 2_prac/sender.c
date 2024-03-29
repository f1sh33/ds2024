#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <libgen.h> // For basename()
#include <sys/stat.h> // For stat()

#define BUFFER_SIZE 1024

// Struct for file metadata
typedef struct {
    char type; // 'M' for metadata
    size_t size; // Size of the file
    char filename[256]; // Name of the file
} RPCMessage;

int main(int argc, char *argv[]) {
    int opt;
    char *filename = NULL;
    int port = 0;
    char *hostAddress = "127.0.0.1";

    while((opt = getopt(argc, argv, "i:d:h:")) != -1) {
        switch(opt) {
            case 'i':
                filename = optarg;
                break;
            case 'd':
                port = atoi(optarg);
                break;
            case 'h':
                hostAddress = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s -i inputfile -d port -h hostAddress\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (filename == NULL || port <= 0) {
        fprintf(stderr, "Input file and destination port are required\n");
        exit(EXIT_FAILURE);
    }

    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation error");
        return -1;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if(inet_pton(AF_INET, hostAddress, &serv_addr.sin_addr) <= 0) {
        printf("Invalid address/ Address not supported\n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return -1;
    }

    // Prepare and send file metadata
    RPCMessage metadata;
    metadata.type = 'M';
    strncpy(metadata.filename, basename(filename), sizeof(metadata.filename) - 1);
    metadata.filename[sizeof(metadata.filename) - 1] = '\0';

    struct stat st;
    if (stat(filename, &st) == 0) {
        metadata.size = st.st_size;
    } else {
        perror("Failed to get file size");
        close(sock);
        return 1;
    }

    send(sock, &metadata, sizeof(metadata), 0);

    // Open and send the file
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Can't open file");
        close(sock);
        return 1;
    }

    char buffer[BUFFER_SIZE] = {0};
    printf("Sending file: %s to %s:%d\n", filename, hostAddress, port);

    while (!feof(file)) {
        int bytes_read = fread(buffer, 1, BUFFER_SIZE, file);
        if (bytes_read > 0) {
            send(sock, buffer, bytes_read, 0);
        }
    }



    RPCMessage response;
    recv(sock, &response, sizeof(response), 0); // Wait for the confirmation message

    if (response.type == 'S') {
        printf("File has been successfully received by the server.\n");
    } else {
        printf("An error occurred while transferring the file.\n");
    }

    fclose(file);
    close(sock);

    return 0;
}
