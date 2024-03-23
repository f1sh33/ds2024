// File: modified_sender_with_host.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <libgen.h> // For basename()

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    int opt;
    char *filename = NULL;
    int port = 0; // Initialize to an invalid value to ensure it's set by the user
    char *hostAddress = "127.0.0.1"; // Default to localhost, modify with -h argument

    // Parse command-line arguments for input file, destination port, and host address
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

    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if(inet_pton(AF_INET, hostAddress, &serv_addr.sin_addr)<=0) {
        printf("\nInvalid address/ Address not supported\n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("\nConnection Failed \n");
        return -1;
    }

    // Send the file name first
    char *baseName = basename(filename); // Extracts the file name from the path
    send(sock, baseName, strlen(baseName) + 1, 0); // +1 to include null terminator

    // Open the file
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Can't open file");
        return 1;
    }

    char buffer[BUFFER_SIZE] = {0};
    printf("Sending file: %s to %s:%d\n", filename, hostAddress, port);

    while (!feof(file)) {
        int bytes_read = fread(buffer, 1, BUFFER_SIZE, file);
        if (bytes_read > 0) {
            send(sock, buffer, bytes_read, 0);
        }
        if (bytes_read < BUFFER_SIZE) {
            if (feof(file)) {
                printf("End of file reached.\n");
            }
            if (ferror(file)) {
                perror("Error reading from file");
                break;
            }
        }
    }

    printf("File has been sent successfully.\n");

    fclose(file);
    close(sock);

    return 0;
}
