#ifndef A3_STRUCTS_H
#define A3_STRUCTS_H

#include "structs.h"

typedef struct {
    int maxDepth;
    int matrixSize;
    int mainRows;
    int rowsEach;
    int finalRow;
} SHARED_DATA;

typedef struct {
    int firstCalcRow;
    int lastCalcRow;
    int me;
    int firstLocalRow;
    int lastLocalRow;
    int totalRows;
    int lengthLocalArray;
} PROCESS_DATA;

#endif //A3_STRUCTS_H
