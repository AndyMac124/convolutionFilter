/*H*
 * FILENAME: conv_filter_utils.c
 *
 * AUTHOR: Andrew McKenzie
 * UNE EMAIL: amcken33@myune.edu.au
 * STUDENT NUMBER: 220263507
 *
 * PURPOSE: Utility functions to aid conv_filter program.
 *
 *H*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "conv_filter_utils.h"
#include "mpi.h"

#define FILE_NAME_ARG 0
#define FILE_IN_ARG 1
#define FILE_OUT_ARG 2
#define DEPTH 3

/**
 * parse_args() - Processes command line arguments.
 *
 * @arg1: Number of arguments in argv
 * @arg2: Array of arguments
 *
 * Checks for correct number of args, ensures file in is valid, file out
 * is valid and max_depth is not negative.
 *
 * Return: void (pass by reference).
 */
void parse_args(int argc, char *argv[], int *fdIn, int *fdOut, int *maxDepth)
{

        if (argc != 4) {
                fprintf(stderr, "Usage: %s matrix_file dimension\n",
                        argv[FILE_NAME_ARG]);
                MPI_Abort(MPI_COMM_WORLD, -1);
        }

        *fdIn = open(argv[FILE_IN_ARG], O_RDONLY);
        *fdOut = open(argv[FILE_OUT_ARG], O_WRONLY | O_CREAT, 0666);

        if (*fdIn == -1) {
                fprintf(stderr, "Usage: %s input_file, output_file, "
                                "neighbourhood_depth\n", argv[FILE_NAME_ARG]);
                MPI_Abort(MPI_COMM_WORLD, -1);
        }

        if (*fdOut == -1) {
                fprintf(stderr, "Usage: %s input_file, output_file, "
                                "neighbourhood_depth\n", argv[FILE_NAME_ARG]);
                close(*fdIn);
                MPI_Abort(MPI_COMM_WORLD, -1);
        }

        *maxDepth = atoi(argv[DEPTH]);
        if (*maxDepth < 0) {
                fprintf(stderr, "Usage: %s input_file, output_file, "
                                "neighbourhood_depth\n", argv[FILE_NAME_ARG]);
                close(*fdIn);
                close(*fdOut);
                MPI_Abort(MPI_COMM_WORLD, -1);
        }
}


/**
 * divide_rows() - divides the matrix rows between processes
 *
 * @arg1: Number of rows for slave nodes
 * @arg2: Number of rows for Main Process
 * @arg3: Size of square matrix
 * @arg4: Number of processes in MPI environment
 *
 * Function divides the rows between processes giving the remainder to the
 * main process.
 *
 * Return: void (pass by reference).
 */
void divide_rows(int* rowsEach, int* mainRows, int matrixSize, int nProcs)
{
        if (nProcs <= 0) {
                fprintf(stderr,
                        "Number of processes must be more than zero.\n");
                MPI_Abort(MPI_COMM_WORLD, -1);
        }
        *rowsEach = matrixSize / nProcs;
        *mainRows = *rowsEach + (matrixSize % nProcs);
        if (nProcs > matrixSize) {
                *rowsEach = 1;
                *mainRows = 1;
        }
}

/**
 * process_has_jobs () -
 *
 * @arg1: Pointer to shared data struct among MPI_COMM_WORLD.
 * @arg2: Process rank.
 *
 * Function checks if process rank is within matrix rows meaning it will
 * have rows to process.
 *
 * Return: Boolean, true is process has jobs, else false.
 */
bool process_has_jobs(SHARED_DATA *shared, int me)
{
        return ((shared->mainRows +
                ((me - 1) * shared->rowsEach)) < shared->matrixSize);
}

/**
 * set_shared_data() -
 *
 * @arg1: Pointer to shared data struct among MPI_COMM_WORLD.
 * @arg2: maximum neighbourhood depth.
 * @arg3: size of square matrix.
 * @arg4: Number of rows for Main Process to handle.
 * @arg5: Number of rows for slave nodes to process.
 *
 * Function takes in the required arguments to set the values in the shared
 * data struct.
 *
 * Return: void (pass by reference).
 */
void set_shared_data(SHARED_DATA *shared, int maxDepth, int matrixSize,
                     int mainRows, int rowsEach)
{
        shared->maxDepth = maxDepth;
        shared->matrixSize = matrixSize;
        shared->mainRows = mainRows;
        shared->rowsEach = rowsEach;
        shared->finalRow = shared->matrixSize - 1;
}

/**
 * set_recv_rows_main() - Calculate and set the rows to be received by main.
 *
 * @arg1: Pointer to shared data struct among MPI_COMM_WORLD.
 *
 * Function calculates and sets the number of rows the Main Process will have
 * in its local array.
 *
 * Return: int, number of rows.
 */
void set_recv_rows_main(SHARED_DATA *shared, PROCESS_DATA *pData)
{
        pData->lengthLocalArray = shared->mainRows + shared->maxDepth;
        if (pData->lengthLocalArray > shared->matrixSize) {
                pData->lengthLocalArray = shared->matrixSize;
        }
}

/**
 * set_process_data() - Sets the struct values for local process data.
 *
 * @arg1: Pointer to local data struct for this process.
 * @arg2: Pointer to shared data struct among MPI_COMM_WORLD.
 * @arg3: Whether the process is the Main Process.
 * @arg4: Process rank.
 *
 * Function uses the arguments to set the variables for the processes local
 * data struct.
 *
 * Return: void (pass by reference).
 */
void set_process_data(PROCESS_DATA *pData, SHARED_DATA *shared,
                      bool isMain, int me)
{
        if (isMain) {
                pData->firstCalcRow = 0;
                pData->lastCalcRow = shared->mainRows - 1;
                pData->me = me;
                pData->firstLocalRow = 0;

                // Last local row
                pData->lastLocalRow = shared->mainRows - 1;
                if (pData->lastLocalRow > shared->finalRow) {
                        pData->lastLocalRow = shared->finalRow;
                }

                // Length local array
                pData->lengthLocalArray = shared->mainRows + shared->maxDepth;
                if (pData->lengthLocalArray > shared->finalRow + 1) {
                        pData->lengthLocalArray = shared->finalRow + 1;
                }

                pData->totalRows = shared->mainRows;
                set_recv_rows_main(shared, pData);
        } else {
                pData->firstCalcRow = first_calc_row(shared, me);
                pData->lastCalcRow = last_calc_row(shared,
                                                   pData->firstCalcRow);
                pData->me = me;
                pData->totalRows = shared->rowsEach;
                calc_receiving_rows(shared, pData);
        }
}

/**
 * first_calc_row() - Find the first row this process will calculate.
 *
 * @arg1: Pointer to shared data struct among MPI_COMM_WORLD.
 * @arg2: Process rank.
 *
 * Function calculates which row the given process will perform its first
 * calculation on relative to the full matrix.
 *
 * Return: int, first row for calculations.
 */
int first_calc_row(SHARED_DATA *shared, int me)
{
        int firstRow = shared->mainRows + ((me - 1) * shared->rowsEach);
        if (firstRow > shared->finalRow) {
                firstRow = shared->finalRow;
        }
        if (firstRow < 0) {
                firstRow = 0;
        }
        return firstRow;
}

/**
 * last_calc_row() - Find the last row this process will calculate.
 *
 * @arg1: Pointer to shared data struct among MPI_COMM_WORLD.
 * @arg2: The first row this process will perform a calculation on.
 *
 * Function calculates which row the given process will perform its last
 * calculation on relative to the full matrix.
 *
 * Return: int, last row for calculations.
 */
int last_calc_row(SHARED_DATA *shared, int firstRow)
{
        int lastRow = firstRow + shared->rowsEach - 1;
        if (lastRow > shared->finalRow) {
                lastRow = shared->finalRow;
        }
        return lastRow;
}

/**
 * calc_receiving_rows() - Calculates how many rows will be received from Main
 *
 * @arg1: Pointer to shared data struct among MPI_COMM_WORLD.
 * @arg2: Pointer to local data struct for this process.
 *
 * Function calculates how many rows the given process will expect to receive
 * from the main process, this includes rows used for neighbourhood depth and
 * accounts for boundaries of the matrix size.
 *
 * Return: int, total number of rows.
 */
void calc_receiving_rows(SHARED_DATA *shared, PROCESS_DATA *pData)
{
        int firstRow = shared->mainRows + ((pData->me - 1) * shared->rowsEach)
                - shared->maxDepth;

        int lastRow = (shared->mainRows + ((pData->me - 1) * shared->rowsEach)
                + (shared->rowsEach - 1) + shared->maxDepth);
        if (lastRow > shared->finalRow) {
                lastRow = shared->finalRow;
        }
        if (firstRow < 0) {
                firstRow = 0;
        }
        int receivingRows = lastRow - firstRow + 1;
        pData->lengthLocalArray = receivingRows;
        pData->firstLocalRow = pData->firstCalcRow - firstRow;
        pData->lastLocalRow = pData->firstLocalRow + shared->rowsEach - 1;
}
