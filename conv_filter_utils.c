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
void parse_args(int argc, char *argv[], int *fd_in, int *fd_out, int *max_depth)
{

        if (argc != 4) {
                fprintf(stderr, "Usage: %s matrix_file dimension\n", argv[0]);
                MPI_Abort(MPI_COMM_WORLD, -1);
        }

        *fd_in = open (argv[1], O_RDONLY);
        *fd_out = open (argv[2], O_WRONLY | O_CREAT, 0666);

        if (*fd_in == -1) {
                fprintf(stderr, "Usage: %s input_file, output_file, neighbourhood_depth\n", argv[0]);
                MPI_Abort(MPI_COMM_WORLD, -1);
        }

        if (*fd_out == -1) {
                fprintf(stderr, "Usage: %s input_file, output_file, neighbourhood_depth\n", argv[0]);
                close(*fd_in);
                MPI_Abort(MPI_COMM_WORLD, -1);
        }

        *max_depth = atoi(argv[3]);
        if (*max_depth < 0) {
                fprintf(stderr, "Usage: %s input_file, output_file, neighbourhood_depth\n", argv[0]);
                close(*fd_in);
                close(*fd_out);
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
void divide_rows(int* rows_each, int* M_rows, int matrix_size, int nprocs)
{
        if (nprocs <= 0) {
                fprintf(stderr, "Number of processes must be more than zero.\n");
                MPI_Abort(MPI_COMM_WORLD, -1);
        }
        *rows_each = matrix_size / nprocs;
        *M_rows = *rows_each + (matrix_size % nprocs);
        if (nprocs > matrix_size) {
                *rows_each = 1;
                *M_rows = 1;
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
        return ((shared->m_rows + ((me - 1) * shared->rows_each)) < shared->matrix_size);
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
void set_shared_data(SHARED_DATA *shared, int max_depth, int matrix_size, int m_rows, int rows_each)
{
        shared->max_depth = max_depth;
        shared->matrix_size = matrix_size;
        shared->m_rows = m_rows;
        shared->rows_each = rows_each;
        shared->final_row = shared->matrix_size - 1;
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
int set_recv_rows_main(SHARED_DATA *shared)
{
        int recv = shared->m_rows + shared->max_depth;
        if (recv > shared->matrix_size) {
                recv = shared->matrix_size;
        }
        return recv;
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
void set_process_data(PROCESS_DATA *p_data, SHARED_DATA *shared, bool is_main, int me)
{
        if (is_main) {
                p_data->first_calc_row = 0;
                p_data->last_calc_row = shared->m_rows - 1;
                p_data->me = me;
                p_data->first_local_row = 0;
                p_data->last_local_row = shared->m_rows - 1;
                if (p_data->last_local_row > shared->final_row) {
                        p_data->last_local_row = shared->final_row;
                }
                p_data->length_local_array = shared->m_rows + shared->max_depth;
                if (p_data->length_local_array > shared->final_row + 1) {
                        p_data->length_local_array = shared->final_row + 1;
                }
                p_data->total_rows = shared->m_rows;
        } else {
                p_data->first_calc_row = first_calc_row(shared, me);
                p_data->last_calc_row = last_calc_row(shared, p_data->first_calc_row);
                p_data->me = me;
                p_data->total_rows = shared->rows_each;
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
        int first_row = shared->m_rows + ((me - 1) * shared->rows_each);
        if (first_row > shared->final_row) {
                first_row = shared->final_row;
        }
        if (first_row < 0) {
                first_row = 0;
        }
        return first_row;
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
int last_calc_row(SHARED_DATA *shared, int first_row)
{
        int last_row = first_row + shared->rows_each - 1;
        if (last_row > shared->final_row) {
                last_row = shared->final_row;
        }
        return last_row;
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
int calc_receiving_rows(SHARED_DATA *shared, PROCESS_DATA *p_data)
{
        int first_row = shared->m_rows + ((p_data->me - 1) * shared->rows_each) - shared->max_depth;

        int last_row = (shared->m_rows + ((p_data->me - 1) * shared->rows_each) + (shared->rows_each - 1) + shared->max_depth);
        if (last_row > shared->final_row) {
                last_row = shared->final_row;
        }
        if (first_row < 0) {
                first_row = 0;
        }
        int receiving_rows = last_row - first_row + 1;
        p_data->length_local_array = receiving_rows;
        p_data->first_local_row = p_data->first_calc_row - first_row;
        p_data->last_local_row = p_data->first_local_row + shared->rows_each - 1;
        return receiving_rows;
}
