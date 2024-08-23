
#include <mpi.h>
#include <stdio.h>

#include "message_handling.h"
#include "conv_filter_utils.h"

void send_rows(SHARED_DATA *shared, int **arr, int nprocs)
{
        for (int i = 1; i < nprocs; i++) {
                if (process_has_jobs(shared, i)) {
                        int first_row = (shared->m_rows + ((i - 1) *
                                shared->rows_each)) - shared->max_depth;

                        int last_row = (shared->m_rows + ((i - 1) *
                                shared->rows_each) + (shared->rows_each - 1)
                                        + shared->max_depth);

                        if (last_row > shared->final_row) {
                                last_row = shared->final_row;
                        }

                        if (first_row < 0) {
                                first_row = 0;
                        }

                        for (int j = first_row; j <= last_row; j++) {
                                if (MPI_Send(arr[j], shared->matrix_size, MPI_INT,
                                         i, 0, MPI_COMM_WORLD) != MPI_SUCCESS) {
                                        fprintf(stderr, "Failed sending rows");
                                        MPI_Abort(MPI_COMM_WORLD, -1);
                                }
                        }
                }
        }
}

void receive_rows(int rows, int **arr, int size)
{
        for (int i = 0; i < rows; i++) {
                if (MPI_Recv(arr[i], size, MPI_INT, MainProc, 0, MPI_COMM_WORLD,
                         MPI_STATUS_IGNORE) != MPI_SUCCESS) {
                        fprintf(stderr, "Failed receiving results");
                        MPI_Abort(MPI_COMM_WORLD, -1);
                }
        }
}

void send_results_to_main(SHARED_DATA *shared, int **arr)
{
        for (int i = 0; i < shared->rows_each; i++) {
                if (MPI_Send(arr[i], shared->matrix_size, MPI_INT, MainProc, 0,
                         MPI_COMM_WORLD) != MPI_SUCCESS) {
                        fprintf(stderr, "Failed sending results to main");
                        MPI_Abort(MPI_COMM_WORLD, -1);
                }
        }
}

void receive_results(SHARED_DATA *data, int **matrix, int nprocs)
{
        int row = data->m_rows;
        for (int i = 1; i < nprocs; i++) {
                if (process_has_jobs(data, i)) {
                        for (int j = 0; j < data->rows_each; j++) {
                                if (MPI_Recv(matrix[row], data->matrix_size,
                                         MPI_INT, i, 0, MPI_COMM_WORLD,
                                         MPI_STATUS_IGNORE) != MPI_SUCCESS) {
                                        fprintf(stderr, "Failed receiving results");
                                        MPI_Abort(MPI_COMM_WORLD, -1);
                                }
                                row++;
                        }
                }

        }
}
