#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 1024

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    if (world_size < 2) {
        fprintf(stderr, "World size must be at least 2 for %s\n", argv[0]);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (world_rank == 0) {
        // Sender
        if (argc < 2) {
            fprintf(stderr, "Usage: mpiexec -n 2 %s <filename>\n", argv[0]);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        char* filename = argv[1];
        FILE* file = fopen(filename, "rb");
        if (!file) {
            perror("Failed to open file");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        char buffer[BUFFER_SIZE];
        while (!feof(file)) {
            int bytes_read = fread(buffer, 1, BUFFER_SIZE, file);
            MPI_Send(&bytes_read, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
            if (bytes_read > 0) {
                MPI_Send(buffer, bytes_read, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
            }
        }

        // Send a zero to signal the end of the file
        int end_signal = 0;
        MPI_Send(&end_signal, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);

        fclose(file);
    } else if (world_rank == 1) {
        // Receiver
        FILE* file = fopen("received_file", "wb");
        if (!file) {
            perror("Failed to open file for writing");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        int message_size;
        char buffer[BUFFER_SIZE];
        MPI_Status status;

        do {
            MPI_Recv(&message_size, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
            if (message_size > 0) {
                MPI_Recv(buffer, message_size, MPI_BYTE, 0, 0, MPI_COMM_WORLD, &status);
                fwrite(buffer, 1, message_size, file);
            }
        } while (message_size > 0);

        fclose(file);
    }

    MPI_Finalize();
    return 0;
}
