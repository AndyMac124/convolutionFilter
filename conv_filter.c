
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

void print2DArrayAddresses(int **array, int rows, int columns) {
        for (int i = 0; i < rows; i++) {
                for (int j = 0; j < columns; j++) {
                        printf("Address of element [%d][%d]: %p\n", i, j, (void *)&array[i][j]);
                }
        }
}

int main(int argc, char *argv[]) {
        int fd_in, fd_out, matrix_size, max_depth, me, nprocs, p_rows, m_rows;
        SHARED_DATA shared;

        MPI_Init(&argc, &argv);
        MPI_Comm_rank(MPI_COMM_WORLD, &me);
        MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

        int **matrix, **local_matrix, **sub_result;
        int recv;
        PROCESS_DATA p_data;

        bool is_main = (me == MainProc);

        if (is_main) {
                //fprintf(stderr, "NPROCS: %d\n", nprocs);
                parse_args(argc, argv, &fd_in, &fd_out, &max_depth);
                matrix_size = get_matrix_size(argv[1]);
                //fprintf(stderr, "MSIZE %d\n", matrix_size);
                divide_rows(&p_rows, &m_rows, matrix_size, nprocs);
                //fprintf(stderr, "M_ROWS %d\n", m_rows);
                build_empty_matrix(&matrix, matrix_size, matrix_size);
                get_matrix_from_file(fd_in, matrix_size, matrix);
                set_shared_data(&shared, max_depth, matrix_size, m_rows, p_rows);
                set_process_data(&p_data, shared, is_main, me);
                recv = shared.m_rows + shared.max_depth;
                if (recv > matrix_size) {
                        recv = matrix_size;
                }
                //fprintf(stderr, "firstLocalRow %d\n", p_data.first_local_row);
                build_empty_matrix(&local_matrix, recv, shared.matrix_size);
                print2DArray(matrix, matrix_size, matrix_size); // TODO This will be removed
        }

        if (MPI_Bcast(&shared, sizeof(SHARED_DATA), MPI_BYTE, MainProc, MPI_COMM_WORLD) != MPI_SUCCESS) {
                perror("Failed in broadcast of shared data");
                MPI_Abort(MPI_COMM_WORLD, -1);
        }

        bool has_jobs = process_has_jobs(&shared, me);
        
        if (has_jobs && !is_main) {
                set_process_data(&p_data, shared, is_main, me);
                //fprintf(stderr, "TOTAL RECV: %d\n", calc_receiving_rows(&shared, &p_data));
                // TODO BUILDING TOO MANY ROWS??
                recv = calc_receiving_rows(&shared, &p_data);
                //fprintf(stderr, "BUILDING EMPTIES\n");
                build_empty_matrix(&local_matrix, recv, shared.matrix_size);
                //fprintf(stderr, "BUILDING EMPTIES 2\n");

                build_empty_matrix(&sub_result, shared.rows_each, shared.matrix_size);
                //fprintf(stderr, "DONE BUILDING EMPTIES\n");
                //fprintf(stderr, "local_matrix pointer: %p, sub_result pointer: %p\n", (void *)local_matrix, (void *)sub_result);
                //print2DArrayAddresses(local_matrix, recv, shared.matrix_size);
                //print2DArrayAddresses(sub_result, shared.rows_each, shared.matrix_size);
        }

        MPI_Barrier(MPI_COMM_WORLD);
        fprintf(stderr, "BARRIER 1 EMPTIES\n");

        if (is_main) {
                send_rows(&shared, matrix, nprocs);
                //fprintf(stderr, "ROWS SENT\n");
                copy_matrix(&shared, local_matrix, matrix);
                //fprintf(stderr, "COPIED MATRIX\n");
                calculate_values(&p_data, &shared, matrix, local_matrix);
        } else if (has_jobs) {
                // Receive rows, calculate results and send back to main
                receive_rows(recv, local_matrix, shared.matrix_size);
                // TODO PRINT ALL LOCAL ARRAYS
                print2DArray(local_matrix, recv, matrix_size);
                fprintf(stderr, "Recv: %d\n", recv);
                calculate_values(&p_data, &shared, sub_result, local_matrix);
                fprintf(stderr, "me: %d sending\n", me);
                send_results_to_main(&shared, sub_result);
        }

        if (is_main) {
                receive_results(&shared, matrix, nprocs);
                fprintf(stderr, "RECEIVIED \t########\n");
                write_to_file(matrix_size, matrix, fd_out);
                print2DArray(matrix, matrix_size, matrix_size); // TODO Remove this last
        }

        MPI_Barrier(MPI_COMM_WORLD);
        fprintf(stderr, "BARRIER REACHED\n");

        if (has_jobs && !is_main) {
                // Clean up
                free_int_array_memory(sub_result, shared.rows_each);
                free_int_array_memory(local_matrix, recv);
        }

        MPI_Barrier(MPI_COMM_WORLD);
        fprintf(stderr, "BARRIER 2 REACHED\n");

        if (is_main) {
                // Clean up
                free_int_array_memory(local_matrix, recv);
                free_int_array_memory(matrix, shared.matrix_size);
                fprintf(stderr, "FREE MEM COMPLETE\n");
                close(fd_in);
                close(fd_out);
        }

        MPI_Finalize ();
        exit (EXIT_SUCCESS);
}
