#ifndef A3_MESSAGE_HANDLING_H
#define A3_MESSAGE_HANDLING_H

#define MAIN_PROC 0

#include "structs.h"

void send_rows(SHARED_DATA *shared, int **arr, int nprocs);
void receive_rows(int rows, int **arr, int size);
void send_results_to_main(SHARED_DATA *shared, int **arr);
void receive_results(SHARED_DATA *data, int **matrix, int nprocs);

#endif //A3_MESSAGE_HANDLING_H
