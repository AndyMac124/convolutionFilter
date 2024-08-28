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
void copy_matrix(SHARED_DATA *shared, int **arrIn, int **arrOut)
{
        for (int row = 0; row < shared->mainRows + shared->maxDepth; row++) {
                if (row > shared->finalRow) {
                        return;
                }
                for (int col = 0; col < shared->matrixSize; col++) {
                        arrIn[row][col] = arrOut[row][col];
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
void calculate_values(PROCESS_DATA *pData, SHARED_DATA *shared,
                      int **arrOut, int **arrIn)
{
        int row = 0;
        for (int i = pData->firstLocalRow; i <= pData->lastLocalRow; i++) {
                for (int j = 0; j < shared->matrixSize; j++) {
                        int val = weighted_average(arrIn, i, j,
                                                   shared->maxDepth,
                                                   pData->lengthLocalArray,
                                                   shared->matrixSize);
                        arrOut[row][j] = val;
                }
                row++;
        }
}

/**
 * rounded_weighted_average() - Calculates the new value for a given index
 *
 * @arg1: Local processes matrix to read from.
 * @arg2: Target y coordinate for calculation.
 * @arg3: Target x coordinate for calculation.
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
int weighted_average(int **localMatrix, int y, int x, int d, int maxRows,
                     int size)
{
        double subTotal = 0;
        double numNeighbours = 0;

        // i is set to our y cord minus depth
        for (int i = y + -d; i <= y + d; i++) {
                // j is set to our x cord minus depth
                for (int j = x + -d; j <= x + d; j++) {
                        // Taking the absolute difference between target
                        // and current cord
                        double ay = abs(i - y);
                        double ax = abs(j - x);

                        // Take the greater of the two
                        double cDepth = (ay > ax) ? ay : ax;
                        // Precaution for division by zero next
                        if (cDepth == 0) { continue; }

                        // Maximum / Current depth (mc) is out multiplier
                        double mc = d / cDepth;

                        // If not within matrix constraints, continue
                        if ((i == y && j == x) || i > maxRows - 1 ||
                            j > size - 1 || i < 0 || j < 0) {
                                continue;
                        }

                        // Add to sub_total the value at index times m_c
                        subTotal += mc * localMatrix[i][j];
                        numNeighbours++;
                }
        }

        if (numNeighbours == 0) {
                // If 1x1 matrix
                return localMatrix[y][x];
        }

        return subTotal / numNeighbours;
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
void free_int_array_memory(int **array, int rows)
{
        if (array == NULL) {
                return;
        }
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
void get_slot(int fd, int matrixSize, int row, int col, int *slot)
{
        if ((row <= 0) ||
           (col <= 0) ||
           (row > matrixSize) ||
           (col > matrixSize)) {
                fprintf(stderr,"indexes out of range");
                MPI_Abort(MPI_COMM_WORLD, -1);
        } else {
                off_t offset = (((row - 1) * matrixSize) + (col - 1))
                        * sizeof(int);
                if (offset < 0) {
                        fprintf(stderr,"offset overflow");
                        MPI_Abort(MPI_COMM_WORLD, -1);
                } else if (lseek(fd, offset, 0) < 0) {
                        perror("lseek failed");
                        MPI_Abort(MPI_COMM_WORLD, -1);
                } else if (read(fd, slot, sizeof(int)) < 0) {
                        perror("read failed");
                        MPI_Abort(MPI_COMM_WORLD, -1);
                }
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
void set_row(int fd, int matrixSize, int row, int matrixRow[])
{
        if (row > matrixSize) {
                fprintf(stderr,"Indexes out of range");
                MPI_Abort(MPI_COMM_WORLD, -1);
        } else {
                int column;
                off_t offset = ((row - 1) * matrixSize) * sizeof(int);
                if (offset < 0) {
                        fprintf(stderr, "Offset overflow");
                        MPI_Abort(MPI_COMM_WORLD, -1);
                } else if (lseek(fd, offset, 0) < 0) {
                        perror("lseek failed");
                        MPI_Abort(MPI_COMM_WORLD, -1);
                } else {
                        for (column = 0; column < matrixSize; column++) {
                                if (write(fd, &matrixRow[column],
                                          sizeof(int)) < 0) {
                                        perror("Write failed");
                                        MPI_Abort(MPI_COMM_WORLD, -1);
                                }
                        }
                }
        }
}
