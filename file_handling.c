//
// Created by Andrew Mckenzie on 23/8/2024.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include "matrix.h"

// TODO This is from lectures
void get_matrix_from_file(int fd, int matrix_size, int **matrix)
{
        int row, col, slot;

        for(row = 1; row <= matrix_size; row++)
                for(col = 1; col <= matrix_size; col++){
                        if(get_slot(fd, matrix_size, row, col, &slot) == -1){
                                fprintf(stderr,"get_slot failed at [%d][%d]\n", row, col);
                                exit(-1);
                        }
                        matrix[row - 1][col - 1] = slot;
                }
}

int get_matrix_size(char* file_name)
{
        int fd;
        if ((fd = open(file_name, O_RDONLY)) == -1) {
                fprintf(stderr, "Failed to open input file\n");
                exit(1);
        }
        // Calculate the file size
        off_t file_size = lseek(fd, 0, SEEK_END);
        if (file_size == -1) {
                perror("lseek failed");
                return -1;
        }

        int num_elements = file_size / sizeof(int);
        int matrix_size = (int)sqrt(num_elements);

        if (close(fd) == -1) {
                perror("Failed to close file");
                exit(EXIT_FAILURE);
        }

        return matrix_size;
}

void write_to_file(int rows, int **matrix, int fd_out)
{
        for (int i = 0; i < rows; i++) {
                if (set_row(fd_out, rows, i + 1, matrix[i]) < 0) {
                        fprintf(stderr, "Failed to write row %d to file\n", i + 1);
                        close(fd_out);
                        exit(EXIT_FAILURE);
                }
        }
}
