#ifndef A3_MATRIX_HANDLING_H
#define A3_MATRIX_HANDLING_H

#include "structs.h"

void build_empty_matrix(int ***matrix, int rows, int col);
void copy_matrix(SHARED_DATA *shared, int **arr_in, int **arr_out);
void calculate_values(PROCESS_DATA *p_data, SHARED_DATA *shared,
                      int **arr_out, int **arr_in);
int weighted_average(int **local_matrix, int x, int y, int n, int size,
                     int max_rows);
void free_int_array_memory(int **array, int rows);
void set_row(int fd, int matrix_size, int row, int matrix_row[]);
void get_slot(int fd, int matrix_size, int row, int col, int *slot);

#endif //A3_MATRIX_HANDLING_H
