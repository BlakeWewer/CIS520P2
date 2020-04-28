#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int calc_difference(char* line1, char* line2, int line1_length, int line2_length);

void main()
{
    char* example_line1 = "abcd";
    char* example_line2 = "efgh";

    printf(calc_difference(example_line1, example_line2, strlen(example_line1), strlen(example_line2)), "%d\n");
}

int calc_difference(char* line1, char* line2, int line1_length, int line2_length)
{
    int sum1 = 0;
    int sum2 = 0;

    for(int i = 0; i < line1_length; i++)
        sum1 += line1[i];

    for(int i = 0; i < line2_length; i++)
        sum2 += line2[i];

    return sum1 - sum2;
}