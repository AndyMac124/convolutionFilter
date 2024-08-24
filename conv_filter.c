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
#include "matrix.h"
#include "conv_filter_utils.h"
#include "file_handling.h"
#include "matrix_handling.h"
#include "message_handling.h"

void print2DArray(int **array, int rows, int columns)
{
        for (int i = 0; i < rows; i++) {
                for (int j = 0; j < columns; j++) {
                        printf("%d ", array[i][j]);
                }
                printf("\n");
        }
        printf("\n\n");
}

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
int main(int argc, char *argv[]) {
        // Declaring vars for main process to handle
        int fd_in, fd_out, matrix_size, max_depth, me, nprocs, p_rows, m_rows;
        SHARED_DATA shared;

        // Initialising MPI environment
        MPI_Init(&argc, &argv);
        MPI_Comm_rank(MPI_COMM_WORLD, &me);
        MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

        // Data for each process
        int **matrix= NULL;
        int **local_matrix = NULL;
        int **sub_result = NULL;
        int recv = 0;
        PROCESS_DATA p_data;

        // Alias for brevity in the if statements
        bool is_main = (me == MainProc);

        if (is_main) {
                parse_args(argc, argv, &fd_in, &fd_out, &max_depth);

                // Setting up the matrix from file in
                matrix_size = get_matrix_size(fd_in);
                divide_rows(&p_rows, &m_rows, matrix_size, nprocs);
                build_empty_matrix(&matrix, matrix_size, matrix_size);
                get_matrix_from_file(fd_in, matrix_size, matrix);

                // Setting up shared struct
                set_shared_data(&shared, max_depth, matrix_size, m_rows, p_rows);

                // Setting up mains local data
                set_process_data(&p_data, &shared, is_main, me);
                recv = set_recv_rows_main(&shared);
                build_empty_matrix(&local_matrix, recv, shared.matrix_size);
                print2DArray(matrix, matrix_size, matrix_size); // TODO This will be removed
        }

        // Broadcasting out the shared data struct
        if (MPI_Bcast(&shared, sizeof(SHARED_DATA), MPI_BYTE, MainProc, MPI_COMM_WORLD) != MPI_SUCCESS) {
                perror("Failed in broadcast of shared data");
                MPI_Abort(MPI_COMM_WORLD, -1);
        }

        // If nprocs greater than rows, they won't have work to do
        bool has_jobs = process_has_jobs(&shared, me);

        // For processes other than main with rows to compute
        if (has_jobs && !is_main) {
                // Setting their local data
                set_process_data(&p_data, &shared, is_main, me);
                recv = calc_receiving_rows(&shared, &p_data);

                // Allocating memory for arrays they will use
                build_empty_matrix(&local_matrix, recv, shared.matrix_size);
                build_empty_matrix(&sub_result, shared.rows_each, shared.matrix_size);
        }

        if (is_main) {
                // Sending rows from input matrix
                send_rows(&shared, matrix, nprocs);

                // Calculating mains own rows
                copy_matrix(&shared, local_matrix, matrix);
                calculate_values(&p_data, &shared, matrix, local_matrix);

                // Receiving rest of rows and writing to file
                receive_results(&shared, matrix, nprocs);
                write_to_file(matrix_size, matrix, fd_out);
                print2DArray(matrix, matrix_size, matrix_size); // TODO Remove this last

                // Cleaning up memory and files
                free_int_array_memory(local_matrix, recv);
                free_int_array_memory(matrix, shared.matrix_size);
                close(fd_in);
                close(fd_out);
        } else if (has_jobs) {
                // Other processes with rows to compute will do their work
                receive_rows(recv, local_matrix, shared.matrix_size);
                calculate_values(&p_data, &shared, sub_result, local_matrix);
                send_results_to_main(&shared, sub_result);

                // Cleaning up their own malloc memory
                free_int_array_memory(sub_result, shared.rows_each);
                free_int_array_memory(local_matrix, recv);
        }

        MPI_Finalize ();
        exit (EXIT_SUCCESS);
}
