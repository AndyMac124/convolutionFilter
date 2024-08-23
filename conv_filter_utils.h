#ifndef A3_CONV_FILTER_UTILS_H
#define A3_CONV_FILTER_UTILS_H

#include <stdbool.h>
#include "structs.h"

void parse_args(int argc, char *argv[], int *fd_in, int *fd_out, int *max_depth);
void divide_rows(int* rows_each, int* M_rows, int matrix_size, int nprocs);
bool process_has_jobs(SHARED_DATA *shared, int me);
void set_shared_data(SHARED_DATA *shared, int max_depth, int matrix_size, int m_rows, int rows_each);
int set_recv_rows_main(SHARED_DATA *shared);
void set_process_data(PROCESS_DATA *p_data, SHARED_DATA *shared, bool is_main, int me);
int first_calc_row(SHARED_DATA *shared, int me);
int last_calc_row(SHARED_DATA *shared, int first_row);
int calc_receiving_rows(SHARED_DATA *shared, PROCESS_DATA *p_data);

#endif //A3_CONV_FILTER_UTILS_H
