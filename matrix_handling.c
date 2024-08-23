
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "matrix_handling.h"
#include "mpi.h"

void build_empty_matrix(int ***matrix, int rows, int col)
{
        //fprintf(stderr, "ENTERED WITH %d rows and %d col\n", rows, col);
        if ((*matrix = (int **) malloc(rows * sizeof(int *))) == NULL) {
                //fprintf(stderr, "Couldn't allocate memory for array");
                MPI_Abort(MPI_COMM_WORLD, -1);
        }

        for (int i = 0; i < rows; i++) {
                (*matrix)[i] = (int *) malloc(col * sizeof(int));
                //fprintf(stderr, "Moved to here\n");
                if ((*matrix)[i] == NULL) {
                        fprintf(stderr, "Couldn't allocate memory for index");
                        for (int j = 0; j < col; j++) {
                                free((*matrix)[j]);
                        }
                        free(*matrix);
                        MPI_Abort(MPI_COMM_WORLD, -1);
                }
        }
}

void copy_matrix(SHARED_DATA *shared, int **arr_in, int **arr_out)
{
        for (int row = 0; row < shared->m_rows + shared->max_depth; row++) {
                if (row > shared->final_row) {
                        return;
                }
                for (int col = 0; col < shared->matrix_size; col++) {
                        arr_in[row][col] = arr_out[row][col];
                }
        }
}

void calculate_values(PROCESS_DATA *p_data, SHARED_DATA *shared, int **arr_out, int **arr_in)
{
        // TODO PRINT LENGTH LOCAL ARRAY AND MAKE SURE ITS ALL FILLED?
        int row = 0;
        for (int i = p_data->first_local_row; i <= p_data->last_local_row; i++) {
                for (int j = 0; j < shared->matrix_size; j++) {
                        if (p_data->me == 0) {fprintf(stderr, "P%d passing in: LOCAL ARR:%d and size:%d with row %d, i:%d and col:%d\n", p_data->me, p_data->length_local_array, shared->matrix_size, row, i, j);}
                        int val = rounded_weighted_average(arr_in, i, j, shared->max_depth, p_data->length_local_array, shared->matrix_size, p_data->me);
                        //if (p_data->me != 0) {fprintf(stderr, "P:%d x:%d val:%d\n", p_data->me, i, val);}
                        arr_out[row][j] = val;
                }
                row++;
        }
}

int rounded_weighted_average(int **local_matrix, int x, int y, int n, int max_rows, int size, int me)
{
        double sub_total = 0;
        double num_neighbours = 0;
        fprintf(stderr, "x %d, y %d, n %d, size %d, max_rows %d\n", x, y, n, size, max_rows);
        for (int i = x + -n; i <= x + n; i++) {
                for (int j = y + -n; j <= y + n; j++) {
                        double ay = abs(i - x);
                        double ax = abs(j - y);
                        double c_depth = (ax > ay) ? ax : ay;
                        double m_n = n / c_depth;
                        if ((i == x && j == y) || i > max_rows - 1 || j > size - 1 || i < 0 || j < 0) {
                                //fprintf(stderr, "continue\n");
                                continue;
                        }
                        if (me == 0) {fprintf(stderr, "i %d, j %d, ay %f, ax %f, c_depth %f, m_n %f, VAL: %d\n", i, j, ay, ax, c_depth, m_n, local_matrix[i][j]);}
                        //fprintf(stderr, "sub %f plus val: %d with m_n %f for x: %d, y: %d FOR %d\n", sub_total, local_matrix[i][j], m_n, i, j, me);
                        sub_total += m_n * local_matrix[i][j];
                        //fprintf(stderr, "sub %f\n", sub_total);
                        num_neighbours++;
                }
        }

        if (num_neighbours == 0) {
                fprintf(stderr, "CANT DIVIDE BY ZERO\n");
                return local_matrix[x][y];
        }
        int total = sub_total / num_neighbours;
        if (me == 2) {fprintf(stderr, "RETURNING: %d\n", total);}
        return total;
}

void free_int_array_memory(int **array, int rows) {
        if (array == NULL) return;
        printf("Freeing memory for array at %p\n", (void *)array);
        for (int i = 0; i < rows; i++) {
                free(array[i]);
        }
        free(array);
}
