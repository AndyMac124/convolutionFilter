
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "conv_filter_utils.h"
#include "mpi.h"

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

        // TODO Can depth be 0?
        *max_depth = atoi(argv[3]);
        if (*max_depth < 0) {
                fprintf(stderr, "Usage: %s input_file, output_file, neighbourhood_depth\n", argv[0]);
                close(*fd_in);
                close(*fd_out);
                MPI_Abort(MPI_COMM_WORLD, -1);
        }
}

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

bool process_has_jobs(SHARED_DATA *shared, int me)
{
        return ((shared->m_rows + ((me - 1) * shared->rows_each)) < shared->matrix_size);
}

void set_shared_data(SHARED_DATA *shared, int max_depth, int matrix_size, int m_rows, int rows_each)
{
        shared->max_depth = max_depth;
        shared->matrix_size = matrix_size;
        shared->m_rows = m_rows;
        shared->rows_each = rows_each;
        shared->final_row = shared->matrix_size - 1;
}

int set_recv_rows_main(SHARED_DATA *shared)
{
        int recv = shared->m_rows + shared->max_depth;
        if (recv > shared->matrix_size) {
                recv = shared->matrix_size;
        }
        return recv;
}

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

int last_calc_row(SHARED_DATA *shared, int first_row)
{
        int last_row = first_row + shared->rows_each - 1;
        if (last_row > shared->final_row) {
                last_row = shared->final_row;
        }
        return last_row;
}

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
