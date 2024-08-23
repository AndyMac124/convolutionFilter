#ifndef A3_STRUCTS_H
#define A3_STRUCTS_H

#include "structs.h"

typedef struct {
    int max_depth;
    int matrix_size;
    int m_rows;
    int rows_each;
    int final_row;
} SHARED_DATA;

typedef struct {
    int first_calc_row;
    int last_calc_row;
    int me;
    int first_local_row;
    int last_local_row;
    int total_rows;
    int length_local_array;
} PROCESS_DATA;

#endif //A3_STRUCTS_H
