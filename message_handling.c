/*H*
 * FILENAME: message_handling.c
 *
 * AUTHOR: Andrew McKenzie
 * UNE EMAIL: amcken33@myune.edu.au
 * STUDENT NUMBER: 220263507
 *
 * PURPOSE: Contains the message handling functions for the
 * conv_filter program.
 *
 *H*/

#include <mpi.h>
#include <stdio.h>

#include "message_handling.h"
#include "conv_filter_utils.h"

/**
 * send_rows() - Sends rows from main to all processes
 *
 * @arg1: Pointer to shared data struct among MPI_COMM_WORLD.
 * @arg2: Pointer to matrix to send from.
 * @arg3: Number of processes in MPI_COMM_WORLD.
 *
 * Function sends rows from main to process, checking the process has
 * rows it requires and calculates the exact amount to send.
 *
 * Return: void (pass by reference).
 */
void send_rows(SHARED_DATA *shared, int **arr, int nProcs)
{
        for (int i = 1; i < nProcs; i++) {
                if (process_has_jobs(shared, i)) {

                        // First row to send
                        int firstRow = (shared->mainRows + ((i - 1) *
                                shared->rowsEach)) - shared->maxDepth;

                        if (firstRow < 0) {
                                firstRow = 0;
                        }

                        // Last row to send
                        int lastRow = (shared->mainRows + ((i - 1) *
                                shared->rowsEach) + (shared->rowsEach - 1)
                                        + shared->maxDepth);

                        if (lastRow > shared->finalRow) {
                                lastRow = shared->finalRow;
                        }



                        for (int j = firstRow; j <= lastRow; j++) {
                                if (MPI_Send(arr[j], shared->matrixSize,
                                             MPI_INT, i, 0, MPI_COMM_WORLD)
                                             != MPI_SUCCESS) {
                                        fprintf(stderr,
                                                "Failed sending rows");
                                        MPI_Abort(MPI_COMM_WORLD, -1);
                                }
                        }
                }
        }
}

/**
 * receive_rows() - To receive rows from main.
 *
 * @arg1: Number of rows to expect.
 * @arg2: Pointer to matrix to store rows.
 * @arg3: Number of columns in matrix.
 *
 * Function receives the given number of rows from Main and stores them in
 * the indicated matrix.
 *
 * Return: void (pass by reference).
 */
void receive_rows(int rows, int **arr, int size)
{
        for (int i = 0; i < rows; i++) {
                if (MPI_Recv(arr[i], size, MPI_INT, MAIN_PROC, 0,
                             MPI_COMM_WORLD, MPI_STATUS_IGNORE)
                             != MPI_SUCCESS) {
                        fprintf(stderr, "Failed receiving results");
                        MPI_Abort(MPI_COMM_WORLD, -1);
                }
        }
}

/**
 * send_results_to_main() - For slave nodes to send rows to main.
 *
 * @arg1: Pointer to shared data struct among MPI_COMM_WORLD.
 * @arg2: Pointer to array to read from for sending.
 *
 * Function reads from given array and sends the rows to the Main Process
 * based on the number of rows each stored in the shared data struct.
 *
 * Return: void (pass by reference).
 */
void send_results_to_main(SHARED_DATA *shared, int **arr)
{
        for (int i = 0; i < shared->rowsEach; i++) {
                if (MPI_Send(arr[i], shared->matrixSize, MPI_INT,
                             MAIN_PROC, 0,
                             MPI_COMM_WORLD) != MPI_SUCCESS) {
                        fprintf(stderr, "Failed sending results to main");
                        MPI_Abort(MPI_COMM_WORLD, -1);
                }
        }
}

/**
 * receive_results() - For main to receive the results from slave nodes.
 *
 * @arg1: Pointer to shared data struct among MPI_COMM_WORLD.
 * @arg2: Pointer to matrix to store the incoming results.
 * @arg3: Number of processes in MPI_COMM_WORLD.
 *
 * Function will run through every process in the MPI_COMM_WORLD and wait
 * to receive the results from them if they have rows to process, storing
 * each row in the matrix given.
 *
 * Return: void (pass by reference).
 */
void receive_results(SHARED_DATA *data, int **matrix, int nProcs)
{
        int row = data->mainRows;
        for (int i = 1; i < nProcs; i++) {
                if (process_has_jobs(data, i)) {
                        for (int j = 0; j < data->rowsEach; j++) {
                                if (MPI_Recv(matrix[row], data->matrixSize,
                                         MPI_INT, i, 0, MPI_COMM_WORLD,
                                         MPI_STATUS_IGNORE) != MPI_SUCCESS) {
                                        fprintf(stderr, "Failed receiving "
                                                        "results");
                                        MPI_Abort(MPI_COMM_WORLD, -1);
                                }
                                row++;
                        }
                }

        }
}
