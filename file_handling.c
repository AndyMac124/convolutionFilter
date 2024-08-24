/*H*
 * FILENAME: file_handling.c
 *
 * AUTHOR: Andrew McKenzie
 * UNE EMAIL: amcken33@myune.edu.au
 * STUDENT NUMBER: 220263507
 *
 * PURPOSE: Contains file handling functions for the conv_filter program.
 *
 *H*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include "matrix_handling.h"
#include "mpi.h"

/**
 * get_matrix_from_file() - Reads matrix in from file.
 *
 * @arg1: file descriptor for the input file.
 * @arg2: Size of square matrix expected.
 * @arg3: Pointer to matrix.
 *
 * Reference: Function is from lectures for COSC330 at UNE.
 *
 * Function reads the matrix in from the file and stores in the referenced
 * array.
 *
 * Return: void (pass by reference).
 */
void get_matrix_from_file(int fd, int matrix_size, int **matrix)
{
        int row, col, slot;

        for(row = 1; row <= matrix_size; row++)
                for(col = 1; col <= matrix_size; col++){
                        get_slot(fd, matrix_size, row, col, &slot);
                        matrix[row - 1][col - 1] = slot;
                }
}

/**
 * get_matrix_size() - Reads a file to calculate size of matrix
 *
 * @arg1: The file name to get the size from.
 *
 * Function reads in a file assuming it is a square matrix of ints to
 * calculate the number of ints in a single row.
 *
 * Return: int, size of matrix.
 */
int get_matrix_size(char* file_name)
{
        int fd;
        if ((fd = open(file_name, O_RDONLY)) == -1) {
                fprintf(stderr, "Failed to open input file\n");
                MPI_Abort(MPI_COMM_WORLD, -1);
        }

        off_t file_size = lseek(fd, 0, SEEK_END);
        if (file_size == -1) {
                perror("lseek failed");
                MPI_Abort(MPI_COMM_WORLD, -1);
        }

        int num_elements = file_size / sizeof(int);
        int matrix_size = (int)sqrt(num_elements);

        if (close(fd) == -1) {
                perror("Failed to close file");
                MPI_Abort(MPI_COMM_WORLD, -1);
        }

        return matrix_size;
}

/**
 * write_to_file() - Writes from a matrix to a file.
 *
 * @arg1: Number of rows to write.
 * @arg2: Pointer to matrix to write from.
 * @arg3: File descriptor to write to.
 *
 * Function reads from the given matrix and writes the rows into the file.
 *
 * Return: void (pass by reference).
 */
void write_to_file(int rows, int **matrix, int fd_out)
{
        for (int i = 0; i < rows; i++) {
                set_row(fd_out, rows, i + 1, matrix[i]);
        }
}
