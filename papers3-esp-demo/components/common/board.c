#include "board.h"
int getRandInt(int start, int end)
{
    int step = end - start;
    return esp_random() % step + start; 
}