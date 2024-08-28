/*H*
 * FILENAME: conv_filter.c
 *
 * AUTHOR: Andrew McKenzie
 * UNE EMAIL: amcken33@myune.edu.au
 * STUDENT NUMBER: 220263507
 *
 * PURPOSE: Use MPI to process a matrix of integers.
 *
 * This program uses MPI to read from an input file a matrix a square matrix
 * of integers and process it replacing each element with a rounded weighted
 * average of its neighbours based on max_depth/n_depth. Then writing this
 * to the output file.
 *
 * COMPILING: The included makefile can be run with the 'make' command.
 *
 * RUNNING: The program is run by the following:
 *          mpiexec -np <number of processes> conv_filter <input file>
 *                      <output file> <max neighbourhood depth>
 * Run Target Example: make run ARGS="A.mat B.mat 2"
 *
 * As per the Linux Kernel C programming guide:
 * - Function names use snake case for emphasis.
 * - Variables use camel case for brevity.
 * - Global variables use snake case.
 * - Constants and macros use snake case and are upper case.
 * - Everything except function declarations use K&R style braces.
 * - Functions use Allman style braces.
 *
 *H*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "mpi.h"
#include "conv_filter_utils.h"
#include "file_handling.h"
#include "matrix_handling.h"
#include "message_handling.h"

/**
 * main() - Main function for the conv_filter program.
 * @arg1: Number of args from the terminal.
 * @arg2: Array of the args from the terminal.
 *
 * The function follows these generic steps:
 * - creates the MPI environment.
 * - Reads in the file.
 * - Distributes the work.
 * - Collects the results.
 * - Writes the result matrix to output file.
 *
 * Return: Int, zero on success, non-zero on failure.
 */
int main(int argc, char *argv[])
{
        // Declaring vars for main process to handle
        int fdIn, fdOut, matrixSize, maxDepth, procRows, mainRows, nProcs, me;
        int **matrix= NULL;

        // Initialising MPI environment
        MPI_Init(&argc, &argv);
        MPI_Comm_size(MPI_COMM_WORLD, &nProcs);
        MPI_Comm_rank(MPI_COMM_WORLD, &me);

        // local and shared vars
        int **localMatrix = NULL;
        int **subResult = NULL;
        SHARED_DATA shared;
        PROCESS_DATA pData;

        // Alias for brevity in the if statements
        bool isMain = (me == MAIN_PROC);

        // Setting up the main process
        if (isMain) {
                parse_args(argc, argv, &fdIn, &fdOut, &maxDepth);

                // Setting up the matrix from file in
                matrixSize = get_matrix_size(fdIn);
                divide_rows(&procRows, &mainRows, matrixSize, nProcs);
                build_empty_matrix(&matrix, matrixSize, matrixSize);
                get_matrix_from_file(fdIn, matrixSize, matrix);

                // Setting up shared struct
                set_shared_data(&shared, maxDepth, matrixSize, mainRows,
                                procRows);

                // Setting up mains local data
                set_process_data(&pData, &shared, isMain, me);
                build_empty_matrix(&localMatrix, pData.lengthLocalArray,
                                   shared.matrixSize);
        }

        // Broadcasting out the shared data struct
        if (MPI_Bcast(&shared, sizeof(SHARED_DATA), MPI_BYTE, MAIN_PROC,
                      MPI_COMM_WORLD) != MPI_SUCCESS) {
                perror("Failed in broadcast of shared data");
                MPI_Abort(MPI_COMM_WORLD, -1);
        }

        // If nprocs greater than rows, they won't have work to do
        bool hasJobs = process_has_jobs(&shared, me);

        // For processes other than main with rows to compute
        if (hasJobs && !isMain) {
                // Setting their local data
                set_process_data(&pData, &shared, isMain, me);

                // Allocating memory for arrays they will use
                build_empty_matrix(&localMatrix, pData.lengthLocalArray,
                                   shared.matrixSize);
                build_empty_matrix(&subResult, shared.rowsEach,
                                   shared.matrixSize);
        }

        // Work for Main Process to do
        if (isMain) {
                // Sending rows from input matrix
                send_rows(&shared, matrix, nProcs);

                // Calculating mains own rows
                copy_matrix(&shared, localMatrix, matrix);
                calculate_values(&pData, &shared, matrix, localMatrix);

                // Receiving rest of rows and writing to file
                receive_results(&shared, matrix, nProcs);
                write_to_file(matrixSize, matrix, fdOut);

                // Cleaning up memory and files
                free_int_array_memory(localMatrix, pData.lengthLocalArray);
                free_int_array_memory(matrix, shared.matrixSize);
                close(fdIn);
                close(fdOut);
        } else if (hasJobs) {
                // Other processes with rows to compute will do their work
                receive_rows(pData.lengthLocalArray, localMatrix,
                             shared.matrixSize);
                calculate_values(&pData, &shared, subResult, localMatrix);
                send_results_to_main(&shared, subResult);

                // Cleaning up their own malloc memory
                free_int_array_memory(subResult, shared.rowsEach);
                free_int_array_memory(localMatrix, pData.lengthLocalArray);
        }

        MPI_Finalize();
        exit(EXIT_SUCCESS);
}
