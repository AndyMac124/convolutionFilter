/*H*
 * FILENAME: matrix_handling.c
 *
 * AUTHOR: Andrew McKenzie
 * UNE EMAIL: amcken33@myune.edu.au
 * STUDENT NUMBER: 220263507
 *
 * PURPOSE: Contains matrix handling functions for the conv_filter program.
 *
 *H*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "matrix_handling.h"
#include "mpi.h"

/**
 * build_empty_matrix() - Allocates memory for matrix
 *
 * @arg1: Pointer to matrix pointer.
 * @arg2: Number of rows to allocate.
 * @arg3: Number of columns to allocate.
 *
 * Function allocates the required memory for a matrix of ints.
 *
 * Return: void (pass by reference).
 */
void build_empty_matrix(int ***matrix, int rows, int col)
{
        if ((*matrix = (int **) malloc(rows * sizeof(int *))) == NULL) {
                MPI_Abort(MPI_COMM_WORLD, -1);
        }

        for (int i = 0; i < rows; i++) {
                (*matrix)[i] = (int *) malloc(col * sizeof(int));
                if ((*matrix)[i] == NULL) {
                        fprintf(stderr, "Couldn't allocate memory for index");
                        for (int j = 0; j < col; j++) {
                                free((*matrix)[j]);
                        }
                        free(*matrix);
                        MPI_Abort(MPI_COMM_WORLD, -1);
                }
        }
}

/**
 * copy_matrix() - Copies one matrix to another.
 *
 * @arg1: Pointer to shared data struct among MPI_COMM_WORLD.
 * @arg2: Pointer to input matrix.
 * @arg3: Pointer to output matrix.
 *
 * Function copies one matrix to another based on the required rows for Main
 * Process calculated from the given shared data.
 *
 * Return: void (pass by reference).
 */
void copy_matrix(SHARED_DATA *shared, int **arr_in, int **arr_out)
{
        for (int row = 0; row < shared->m_rows + shared->max_depth; row++) {
                if (row > shared->final_row) {
                        return;
                }
                for (int col = 0; col < shared->matrix_size; col++) {
                        arr_in[row][col] = arr_out[row][col];
                }
        }
}

/**
 * calculate_values() - Function calculates values for given matrix
 *
 * @arg1: Pointer to local data struct for this process.
 * @arg2: Pointer to shared data struct among MPI_COMM_WORLD.
 * @arg3: Pointer to output matrix.
 * @arg4: Pointer to input matrix.
 *
 * Function uses the unique process data and shared data to calculate the
 * values for the required rows determined by the conv_filter program.
 * The p_data contains the indices of the processes local array the need
 * calculated and written to the output array.
 *
 * Return: void (pass by reference).
 */
void calculate_values(PROCESS_DATA *p_data, SHARED_DATA *shared, int **arr_out, int **arr_in)
{
        int row = 0;
        for (int i = p_data->first_local_row; i <= p_data->last_local_row; i++) {
                for (int j = 0; j < shared->matrix_size; j++) {
                        int val = rounded_weighted_average(arr_in, i, j, shared->max_depth, p_data->length_local_array, shared->matrix_size);
                        arr_out[row][j] = val;
                }
                row++;
        }
}

/**
 * rounded_weighted_average() - Calculates the new value for a given index
 *
 * @arg1: Local processes matrix to read from.
 * @arg2: Target x coordinate for calculation.
 * @arg3: Target y coordinate for calculation.
 * @arg4: Maximum depth to consider.
 * @arg5: Maximum rows in this local matrix.
 * @arg6: Maximum columns in this local matrix.
 *
 * Function uses inspiration from the COSC330 lecture to iterate through
 * all possible cells within the given neighbourhood depth and check they are
 * valid within the processes local matrix and calculate the rounded weighted
 * average.
 *
 * Return: int, rounded weighted average for the index.
 */
int rounded_weighted_average(int **local_matrix, int y, int x, int n, int max_rows, int size)
{
        double sub_total = 0;
        double num_neighbours = 0;
        for (int i = y + -n; i <= y + n; i++) {
                for (int j = x + -n; j <= x + n; j++) {
                        double ay = abs(i - y);
                        double ax = abs(j - x);
                        double c_depth = (ay > ax) ? ay : ax;
                        double m_n = n / c_depth;
                        if ((i == y && j == x) || i > max_rows - 1 || j > size - 1 || i < 0 || j < 0) {
                                continue;
                        }
                        sub_total += m_n * local_matrix[i][j];
                        num_neighbours++;
                }
        }

        if (num_neighbours == 0) {
                return local_matrix[y][x];
        }
        int total = sub_total / num_neighbours;
        return total;
}

/**
 * free_int_array_memory() - Frees memory allocated to a matrix style array
 *
 * @arg1: Pointer to matrix to free.
 * @arg2: Number of rows to free.
 *
 * Function frees the memory allocate for a matrix.
 *
 * Return: void (pass by reference).
 */
void free_int_array_memory(int **array, int rows) {
        if (array == NULL) return;
        for (int i = 0; i < rows; i++) {
                free(array[i]);
        }
        free(array);
}

/**
 * get_slot() - Reads a slot from a file.
 *
 * @arg1: File descriptor to read from.
 * @arg2: Size of given square matrix.
 * @arg3: Row to read from.
 * @arg4: Column to read from.
 * @arg5: Pointer to store value.
 *
 * Function seeks through a file for given row and column and
 * stores the value in the given pointer.
 *
 * Reference: AUTHOR: ian a. mason @ une  march 15  '99
 * Original File: matrix.c
 *
 * Return: void (pass by reference).
 */
void get_slot(int fd, int matrix_size, int row, int col, int *slot){
        if((row <= 0) ||
           (col <= 0) ||
           (row > matrix_size) ||
           (col > matrix_size)){
                fprintf(stderr,"indexes out of range");
                MPI_Abort(MPI_COMM_WORLD, -1);
        } else {
                off_t offset = (((row - 1)*matrix_size) + (col - 1))*sizeof(int);
                if(offset < 0){
                        fprintf(stderr,"offset overflow");
                        MPI_Abort(MPI_COMM_WORLD, -1); }
                else if(lseek(fd, offset, 0) < 0){
                        perror("lseek failed");
                        MPI_Abort(MPI_COMM_WORLD, -1); }
                else if(read(fd, slot, sizeof(int)) < 0){
                        perror("read failed");
                        MPI_Abort(MPI_COMM_WORLD, -1); };
        }
}

/**
 * set_row() - Sets a given row in a file with new array of ints
 *
 * @arg1: File descriptor to read from.
 * @arg2: Size of given square matrix.
 * @arg3: Row to set from.
 * @arg4: Values to set in row
 *
 * Function: Will seek through given file and set the requested row
 * with the new int array and passed in.
 *
 * Reference: AUTHOR: ian a. mason @ une  march 15  '99
 * Original File: matrix.c
 *
 * Return: void (pass by reference).
 */
void set_row(int fd, int matrix_size, int row, int matrix_row[])
{
        if (row > matrix_size) {
                fprintf(stderr,"indexes out of range");
                MPI_Abort(MPI_COMM_WORLD, -1);
        } else {
                int column;
                off_t offset = ((row - 1) * matrix_size) * sizeof(int);
                if (offset < 0) {
                        fprintf(stderr, "offset overflow");
                        MPI_Abort(MPI_COMM_WORLD, -1);
                } else if (lseek(fd, offset, 0) < 0) {
                        perror("lseek failed");
                        MPI_Abort(MPI_COMM_WORLD, -1);
                } else {
                        for (column = 0; column < matrix_size; column++) {
                                if (write(fd, &matrix_row[column],
                                          sizeof(int)) < 0) {
                                        perror("write failed");
                                        MPI_Abort(MPI_COMM_WORLD, -1);
                                }
                        }
                }
        }
}
