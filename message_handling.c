//
// Created by Andrew Mckenzie on 23/8/2024.
//
#include <mpi.h>

#include "message_handling.h"
#include "conv_filter_utils.h"

void send_rows(SHARED_DATA *shared, int **arr, int nprocs)
{
        for (int i = 1; i < nprocs; i++) {
                if (process_has_jobs(shared, i)) {
                        int first_row = (shared->m_rows + ((i - 1) * shared->rows_each)) - shared->max_depth;
                        int last_row = (shared->m_rows + ((i - 1) * shared->rows_each) + (shared->rows_each - 1) + shared->max_depth);
                        if (last_row > shared->final_row) {
                                last_row = shared->final_row;
                        }
                        if (first_row < 0) {
                                first_row = 0;
                        }
                        for (int j = first_row; j <= last_row; j++) {
                                MPI_Send(arr[j], shared->matrix_size, MPI_INT, i, 0, MPI_COMM_WORLD);
                        }
                }
        }
}

/*
void receive_rows(PROCESS_DATA *p_data, SHARED_DATA *shared, int **arr_in)
{
        int first_row = p_data->first_calc_row - shared->max_depth;
        int last_row = p_data->last_calc_row + shared->max_depth;
        if (last_row > shared->matrix_size - 1) {
                last_row = shared->matrix_size - 1;
        }
        if (first_row < 0) {
                first_row = 0;
        }
        int recv_rows = last_row - first_row + 1;
        for (int k = 0; k < recv_rows; k++) {
                MPI_Recv(arr_in[k], shared->matrix_size, MPI_INT, MainProc, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
}*/

void receive_rows(int rows, int **arr, int size)
{
        for (int k = 0; k < rows; k++) {
                MPI_Recv(arr[k], size, MPI_INT, MainProc, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
}

void send_results_to_main(SHARED_DATA *shared, int **arr)
{
        for (int i = 0; i < shared->rows_each; i++) {
                MPI_Send(arr[i], shared->matrix_size, MPI_INT, MainProc, 0,
                         MPI_COMM_WORLD);
        }
}

void receive_results(SHARED_DATA *data, int **matrix, int nprocs)
{
        int row = data->m_rows;
        for (int i = 1; i < nprocs; i++) {
                if (process_has_jobs(data, i)) {
                        for (int j = 0; j < data->rows_each; j++) {
                                MPI_Recv(matrix[row], data->matrix_size,
                                         MPI_INT, i, 0, MPI_COMM_WORLD,
                                         MPI_STATUS_IGNORE);
                                row++;
                        }
                }

        }
}
