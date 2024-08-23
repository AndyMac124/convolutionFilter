

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

int main(int argc, char *argv[]) {
        int fd_in, fd_out, matrix_size, max_depth, me, nprocs, p_rows, m_rows;
        SHARED_DATA shared;

        MPI_Init(&argc, &argv);
        MPI_Comm_rank(MPI_COMM_WORLD, &me);
        MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

        int **matrix = NULL;
        int **local_matrix;
        int **sub_result;
        PROCESS_DATA p_data;

        bool is_main = (me == MainProc);

        if (is_main) {
                if (parse_args(argc, argv, &fd_in, &fd_out, &max_depth) < 0) {
                        MPI_Finalize();
                        exit(EXIT_FAILURE);
                }
                matrix_size = get_matrix_size(argv[1]);
                divide_rows(&p_rows, &m_rows, matrix_size, nprocs);
                build_empty_matrix(&matrix, matrix_size, matrix_size);
                get_matrix_from_file(fd_in, matrix_size, matrix);
                print2DArray(matrix, matrix_size, matrix_size); // TODO This will be removed
                set_required_data(&shared, max_depth, matrix_size, m_rows, p_rows);
        }

        MPI_Bcast(&shared, sizeof(SHARED_DATA), MPI_BYTE, MainProc, MPI_COMM_WORLD);

        bool has_jobs = process_has_jobs(&shared, me);

        if (has_jobs) {
                set_process_data(&p_data, shared, is_main, me);
                build_empty_matrix(&local_matrix, shared.rows_each + (2 * shared.max_depth), shared.matrix_size);
                build_empty_matrix(&sub_result, shared.rows_each, shared.matrix_size);
        }

        // DO WORK
        if (is_main) {
                build_empty_matrix(&local_matrix, (shared.m_rows + shared.max_depth), shared.matrix_size);
                copy_matrix(&shared, local_matrix, matrix);
                calculate_values(&p_data, &shared, matrix, local_matrix);
                send_rows(&shared, matrix, nprocs);
                receive_results(&shared, matrix, nprocs);
                write_to_file(matrix_size, matrix, fd_out);
                print2DArray(matrix, matrix_size, matrix_size); // TODO Remove this last
                free_int_array_memory(local_matrix, shared.m_rows + shared.max_depth);
                close(fd_in);
                close(fd_out);
        } else if (has_jobs) {
                receive_rows(calc_receiving_rows(&shared, p_data.me), local_matrix, shared.matrix_size);
                calculate_values(&p_data, &shared, sub_result, local_matrix);
                send_results_to_main(&shared, sub_result);
                free_int_array_memory(sub_result, shared.rows_each);
                free_int_array_memory(local_matrix, shared.rows_each + (2 * shared.max_depth));
        }

        MPI_Finalize ();
        exit (EXIT_SUCCESS);
}
