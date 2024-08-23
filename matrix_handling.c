//
// Created by Andrew Mckenzie on 23/8/2024.
//
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "matrix_handling.h"

void build_empty_matrix(int ***matrix, int rows, int col)
{
        *matrix = (int **) malloc(rows * sizeof(int *));
        for (int i = 0; i < rows; i++) {
                (*matrix)[i] = (int *) malloc(col * sizeof(int));
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
        int row = 0;
        for (int i = p_data->first_calc_row; i <= p_data->last_calc_row; i++) {
                for (int j = 0; j < shared->matrix_size; j++) {
                        arr_out[row][j] = rounded_weighted_average(arr_in, i, j, shared->max_depth, shared->matrix_size);
                }
                row++;
        }
}

int rounded_weighted_average(int **local_matrix, int x, int y, int n, int size)
{
        double sub_total = 0;
        double num_neighbours = 0;

        for (int i = x + -n; i <= x + n; i++) {
                for (int j = y + -n; j <= y + n; j++) {
                        double ay = abs(i - x);
                        double ax = abs(j - y);
                        double c_depth = (ax > ay) ? ax : ay;
                        double m_n = n / c_depth;
                        if ((i == x && j == y) || i > size-1 || j > size-1 || i < 0 || j < 0) {
                                continue;
                        }
                        sub_total += m_n * local_matrix[i][j];
                        num_neighbours++;
                }
        }

        if (num_neighbours == 0) {
                fprintf(stderr, "Error: Division by zero in rounded_weighted_average.\n");
                exit(EXIT_FAILURE);
        }
        int total = sub_total / num_neighbours;
        return total;
}

void free_int_array_memory(int **array, int rows)
{
        for (int i = 0; i < rows; i++) {
                free(array[i]);
        }
        free(array);
}
