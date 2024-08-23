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
} PROCESS_DATA;

#endif //A3_STRUCTS_H
