#ifndef A3_MATRIX_HANDLING_H
#define A3_MATRIX_HANDLING_H

#include "structs.h"

void build_empty_matrix(int ***matrix, int rows, int col);
void copy_matrix(SHARED_DATA *shared, int **arr_in, int **arr_out);
void calculate_values(PROCESS_DATA *p_data, SHARED_DATA *shared, int **arr_out, int **arr_in);
int rounded_weighted_average(int **local_matrix, int x, int y, int n, int size);
void free_int_array_memory(int **array, int rows);

#endif //A3_MATRIX_HANDLING_H
