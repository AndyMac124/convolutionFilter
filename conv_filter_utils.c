//
// Created by Andrew Mckenzie on 23/8/2024.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "conv_filter_utils.h"

int parse_args(int argc, char *argv[], int *fd_in, int *fd_out, int *max_depth)
{

        if (argc != 4) {
                fprintf(stderr, "Usage: %s matrix_file dimension\n", argv[0]);
                return -1;
        }

        *fd_in = open (argv[1], O_RDONLY);
        *fd_out = open (argv[2], O_WRONLY | O_CREAT, 0666);

        if (*fd_in == -1) {
                fprintf(stderr, "Usage: %s input_file, output_file, neighbourhood_depth\n", argv[0]);
                return -1;
        }

        if (*fd_out == -1) {
                fprintf(stderr, "Usage: %s input_file, output_file, neighbourhood_depth\n", argv[0]);
                close(*fd_in);
                return -1;
        }

        // TODO Can depth be 0?
        *max_depth = atoi(argv[3]);
        if (*max_depth <= 0) {
                fprintf(stderr, "Usage: %s input_file, output_file, neighbourhood_depth\n", argv[0]);
                close(*fd_in);
                close(*fd_out);
                return -1;
        }
        return 0;
}

void divide_rows(int* rows_each, int* M_rows, int matrix_size, int nprocs)
{
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

void set_required_data(SHARED_DATA *shared, int max_depth, int matrix_size, int m_rows, int rows_each)
{
        shared->max_depth = max_depth;
        shared->matrix_size = matrix_size;
        shared->m_rows = m_rows;
        shared->rows_each = rows_each;
        shared->final_row = shared->matrix_size - 1;
}

void set_process_data(PROCESS_DATA *p_data, SHARED_DATA shared, bool is_main, int me)
{
        if (is_main) {
                p_data->first_calc_row = 0;
                p_data->last_calc_row = shared.m_rows - 1;
                p_data->me = me;
        } else {
                p_data->first_calc_row = calc_first_row(&shared, me);
                p_data->last_calc_row = calc_last_row(&shared, p_data->first_calc_row);
                p_data->me = me;
        }
}

int calc_first_row(SHARED_DATA *shared, int me)
{
        int first_row = shared->m_rows + ((me - 1) * shared->rows_each);
        if (first_row > shared->max_depth) {
                first_row = shared->max_depth;
        }
        if (first_row < 0) {
                first_row = 0;
        }
        return first_row;
}

int calc_last_row(SHARED_DATA *shared, int first_row)
{
        int last_row = first_row + shared->rows_each - 1;
        if (last_row > shared->final_row) {
                last_row = shared->final_row;
        }
        return last_row;
}

int calc_receiving_rows(SHARED_DATA *shared, int me)
{
        int first_row = shared->m_rows + ((me - 1) * shared->rows_each) - shared->max_depth;

        int last_row = (shared->m_rows + ((me - 1) * shared->rows_each) + (shared->rows_each - 1) + shared->max_depth);
        if (last_row > shared->matrix_size - 1) {
                last_row = shared->matrix_size - 1;
        }
        if (first_row < 0) {
                first_row = 0;
        }
        return last_row - first_row + 1;
}
