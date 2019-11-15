//
// Created by grilo on 12/11/2019.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <limits.h>
#include <math.h>

typedef struct grade_index_t
{
    int index;
    int grade;
} grade_index;

//Quicksort adaptado de //https://www.geeksforgeeks.org/quick-sort/
int partition (grade_index *arr, int low, int high, int C){
    int i, j;
    grade_index pivot,swap;

    // pivot (Element to be placed at right position)
    pivot = arr[high*C];

    i = (low - 1);  // Index of smaller element

    for (j = low; j <= high-1; j++)
    {
        // If current element is smaller than or
        // equal to pivot
        if (arr[j*C].grade <= pivot.grade)
        {
            i++;    // increment index of smaller element

            // swap arr[i] and arr[j]
            swap = arr[i*C];
            arr[i*C] = arr[j*C];
            arr[j*C] = swap;
        }
    }

    //swap arr[i + 1] and arr[high]
    swap = arr[(i + 1)*C];
    arr[(i + 1)*C] = arr[high*C];
    arr[high*C] = swap;

    return (i + 1);

} // fim partition


/* low  --> Starting index,  high  --> Ending index */
void quicksort(grade_index *arr, int low, int high, int C){
    int pi;

    if (low < high)  {
        /* pi is partitioning index, arr[pi] is now
           at right place */
        pi = partition(arr, low, high, C);

        quicksort(arr, low, pi - 1, C);  // Before pi
        quicksort(arr, pi + 1, high, C); // After pi
    }

} // fim quicksort

/* This function takes last element as pivot, places
   the pivot element at its correct position in sorted
    array, and places all smaller (smaller than pivot)
   to left of pivot and all greater elements to right
   of pivot
   https://www.geeksforgeeks.org/quick-sort/
*/

void ordena_array(grade_index *array, int length)
{
    //manda o endereco do primeiro elemento da coluna, limites inf e sup e a largura da array
    quicksort(array, 0, length - 1, 1);
}

void calcula_mediana(int *array, float* ret, int length)
{
    *ret = array[length / 2];
    if(!(length % 2))
    {
        *ret += array[(length - 1) / 2];
        *ret *= 0.5;
    }
}

void custom_median(int* array, float* median, int length)
{
    int i, j;
    for (j = 0; j < length; j++)
    {
        for(i = 0; i < length; i++)
        {

        }
    }
}

#define MAX(A, B) A>B?A:B
#define MIN(A, B) A<B?A:B

double example_matrix[] =  {
        30,  40,  20,  80,  85,  10,
        10,  20,  30,  40,  50,  60,
        60,  50,  40,  30,  20,  10,
        70,  55,  35,  80,  95,  27,

        35,  45,  25,  85,  90,  15,
        15,  25,  35,  45,  55,  65,
        65,  55,  45,  35,  25,  15,
        75,  60,  40,  85, 100,  32,

        20,  30,  10,  70,  75,   0,
         0,  10,  20,  30,  40,  50,
        50,  40,  30,  20,  10,   0,
        60,  45,  25,  70,  85,  17,
};

int example_regions = 3;
int example_cities = 4;
int example_students = 6;
int example_seed = 7;


int main(int argc, char* argv[])
{
    int total_students = example_regions * example_cities * example_students;
    int total_cities = example_regions * example_cities;
    int students_per_region = example_cities * example_students;

#define IDX2REG(I) I / students_per_region
#define IDX2CIT(I) I / example_students
//#define IDX2STU(I) (I - IDX2REG(I) * students_per_region) % example_students
#define RCT2IDX(R,C,S) R*students_per_region+C*example_students+S
    grade_index* gradeIndexes = (grade_index*) calloc(total_students, sizeof(grade_index));

    for(int i = 0; i< total_students; i++)
    {
        gradeIndexes[i].index = i;
        gradeIndexes[i].grade = example_matrix[i];
        //printf("Region %d\tCity %d\t%5d\n", IDX2REG(i), IDX2CIT(i), i);
    }

    for(int r = 0; r < example_regions; r++)
    {
        for(int c = 0; c < example_cities; c++)
        {
            for(int s = 0; s < example_students; s++)
            {
                printf("(%2d,%3d) ", gradeIndexes[RCT2IDX(r,c,s)].index, gradeIndexes[RCT2IDX(r,c,s)].grade);
            }
            printf("\n");
        }
        printf("\n");
    }


    printf("Ordenado!\n");

    ordena_array(gradeIndexes, total_students);

    for(int r = 0; r < example_regions; r++)
    {
        for(int c = 0; c < example_cities; c++)
        {
            for(int s = 0; s < example_students; s++)
            {
                printf("(%2d,%3d, c%2d|r%d) ", gradeIndexes[RCT2IDX(r,c,s)].index, gradeIndexes[RCT2IDX(r,c,s)].grade, IDX2CIT(gradeIndexes[RCT2IDX(r,c,s)].index), IDX2REG(gradeIndexes[RCT2IDX(r,c,s)].index) );

            }
            printf("\n");
        }
        //printf("\n");
    }

    int*    min_cit = (int*)calloc(total_cities, sizeof(int));
    char*   flag_min_cit = (char*)calloc(total_cities, sizeof(char));
    int all_min_cit = 0;

    int*    max_cit = (int*)calloc(total_cities, sizeof(int));
    int*    count_elems_cit = (int*)calloc(total_cities, sizeof(int));

    float*  med_cit = (float*)calloc(total_cities, sizeof(float));
    float*  avg_cit = (float*)calloc(total_cities, sizeof(float));
    float*  std_cit = (float*)calloc(total_cities, sizeof(float));
    //float*  stq_cit = (float*)calloc(total_cities, sizeof(float));

    memset(flag_min_cit, 0, total_cities * sizeof(char));
    memset(count_elems_cit, 0, total_cities * sizeof(int));
    memset(avg_cit, 0, total_cities * sizeof(float));
    memset(std_cit, 0, total_cities * sizeof(float));
    //memset(stq_cit, 0, total_cities * sizeof(float));
    if(example_students % 2 == 0)
    {
        memset(med_cit, 0, total_cities * sizeof(int));
    }

    /*for (int i = 0; i < total_cities; ++i) {
        //min_cit[i] = INT_MAX;
        //max_cit[i] = INT_MIN;
        //med_cit[i] = 0;
        avg_cit[i] = 0;
        std_cit[i] = 0;
    }*/

    int*    min_reg = (int*)calloc(example_regions, sizeof(int));
    char*   flag_min_reg = (char*)calloc(example_regions, sizeof(char));
    int all_min_reg = 0;

    int*    max_reg = (int*)calloc(example_regions, sizeof(int));
    int*    count_elems_reg = (int*)calloc(example_regions, sizeof(int));

    float*  med_reg = (float*)calloc(example_regions, sizeof(float));
    float*  avg_reg = (float*)calloc(example_regions, sizeof(float));
    float*  std_reg = (float*)calloc(example_regions, sizeof(float));
    //float*  stq_reg = (float*)calloc(example_regions, sizeof(float));


    memset(flag_min_reg, 0, example_regions * sizeof(char));
    memset(count_elems_reg, 0, example_regions * sizeof(int));
    memset(avg_reg, 0, example_regions * sizeof(float));
    memset(std_reg, 0, example_regions * sizeof(float));
    //memset(stq_reg, 0, example_regions * sizeof(float));
    if(students_per_region % 2 == 0)
    {
        memset(med_reg, 0, example_regions * sizeof(int));
    }

    /*for (int i = 0; i < example_regions; ++i) {
        //min_reg[i] = INT_MAX;
        //max_reg[i] = INT_MIN;
        //med_reg[i] = 0;
        avg_reg[i] = 0;
        std_reg[i] = 0;
    }*/

    int   min_cou = gradeIndexes[0].grade;
    int   max_cou = gradeIndexes[total_students-1].grade;
    float med_cou = gradeIndexes[total_students/2].grade;

    if(total_students % 2 == 0)
    {
        med_cou += gradeIndexes[total_students/2 + 1].grade;
        med_cou /= 2;
    }

    float avg_cou = 0;
    float std_cou = 0;
    //float stq_cou = 0;


    int cit_idx, reg_idx;

    int q = 0;

    for (int i = 0; i < total_students; ++i)
    {
        //city statistics
        cit_idx = IDX2CIT(gradeIndexes[i].index);

        if( (all_min_cit != total_cities) && (flag_min_cit[cit_idx] == 0))
        {
            min_cit[cit_idx] = gradeIndexes[i].grade;
            flag_min_cit[cit_idx] = 1;
            all_min_cit++;

            /*For testing!
            for (int j = 0; j < total_cities; ++j) {
                printf("city %2d, min %2d, flagMin %d", j, min_cit[j], flag_min_cit[j]);
                if(j == cit_idx)
                {
                    printf("|| idx %2d (%d)", i, cit_idx);
                }
                printf("\n");
            }
            printf("---------------------------------------------\n");
            */
        }

        count_elems_cit[cit_idx]++;

        if(count_elems_cit[cit_idx] == example_students)
        {
            max_cit[cit_idx] = gradeIndexes[i].grade;
        }
        /*else if(count_elems_cit[cit_idx] > example_students)
        {
            printf("\ncaraio...\n");
        }*/

        if(count_elems_cit[cit_idx] == (example_students / 2))
        {
            med_cit[cit_idx] += gradeIndexes[i].grade;
        }

        if((example_students % 2 == 0) &&(count_elems_cit[cit_idx] == ((example_students / 2)+1)))
        {
            med_cit[cit_idx] += gradeIndexes[i].grade;
            med_cit[cit_idx] /= 2;
        }

        avg_cit[cit_idx] += gradeIndexes[i].grade;



        //region statistics
        reg_idx = IDX2REG(gradeIndexes[i].index);

        if( (all_min_reg != example_regions) && (flag_min_reg[reg_idx] == 0))
        {
            min_reg[reg_idx] = gradeIndexes[i].grade;
            flag_min_reg[reg_idx] = 1;
            all_min_reg++;
        }

        count_elems_reg[reg_idx]++;

        if(count_elems_reg[reg_idx] == students_per_region)
        {
            max_reg[reg_idx] = gradeIndexes[i].grade;
        }

        if(count_elems_reg[reg_idx] == (students_per_region / 2))
        {
            med_reg[reg_idx] += gradeIndexes[i].grade;
        }

        if((students_per_region % 2 == 0) &&(count_elems_reg[reg_idx] == ((students_per_region / 2)+1)))
        {
            med_reg[reg_idx] += gradeIndexes[i].grade;
            med_reg[reg_idx] /= 2;
        }

        avg_reg[reg_idx] += gradeIndexes[i].grade;

        //country statistics

        avg_cou += gradeIndexes[i].grade;


        //standards deviations
        for (int j = (i+1); j < total_students; ++j)
        {
            q = (gradeIndexes[i].grade - gradeIndexes[j].grade) * (gradeIndexes[i].grade - gradeIndexes[j].grade);

            if(cit_idx == IDX2CIT(gradeIndexes[j].index))
            {
                std_cit[cit_idx] += q;
            }

            if(reg_idx == IDX2REG(gradeIndexes[j].index))
            {
                std_reg[reg_idx] += q;
            }

            std_cou += q;
        }
    }

    for (int c = 0; c < total_cities; ++c)
    {
        avg_cit[c] /= example_students;
    }

    for (int r = 0; r < example_regions; ++r)
    {
        avg_reg[r] /= students_per_region;
    }


    avg_cou /= total_students;



    for (int c = 0; c < total_cities; ++c)
    {
        std_cit[c] = sqrt((std_cit[c])/((example_students*(example_students-1))));
    }

    for (int r = 0; r < example_regions; ++r)
    {
        std_reg[r] = sqrt((std_reg[r])/((students_per_region*(students_per_region-1))));
    }


    std_cou = sqrt((std_cou)/((total_students*(total_students-1))));


    //PRINTING RESULTS!!!

    for (int c = 0; c < total_cities; ++c)
    {
        printf("Reg %d - ", c / example_cities);
        printf("Cid %d: ", c % example_cities );

        printf("menor: %d, ", min_cit[c]);
        printf("maior: %d, ", max_cit[c]);
        printf("mediana: %4.2f, ", med_cit[c]);
        printf("media: %4.2f e ", avg_cit[c]);
        printf("DP: %4.2f\n", std_cit[c]);
    }
    printf("\n");


    for(int r = 0; r < example_regions; ++r)
    {
        printf("Reg %d:", r);

        printf("menor: %d, ", min_reg[r]);
        printf("maior: %d, ", max_reg[r]);
        printf("mediana: %5.2f, ", med_reg[r]);
        printf("media: %5.2f e ", avg_reg[r]);
        printf("DP: %5.2f\n", std_reg[r]);
    }

    printf("\n");

    printf("Brasil: ");
    printf("menor: %d, ", min_cou);
    printf("maior: %d, ", max_cou);
    printf("mediana: %5.2f, ", med_cou);
    printf("media: %5.2f e ", avg_cou);
    printf("DP: %5.2f\n", std_cou);

    printf("\n");

    printf("Melhor regiao: Regiao %d\n", 0);
    printf("Melhor cidade: Regiao %d, Cidade %d\n", 0, 0);

    printf("\n");

    //FREEING!

    free(min_cit);
    free(flag_min_cit);

    free(max_cit);
    free(count_elems_cit);

    free(med_cit);
    free(avg_cit);
    free(std_cit);

    free(min_reg);
    free(flag_min_reg);

    free(max_reg);
    free(count_elems_reg);

    free(med_reg);
    free(avg_reg);
    free(std_reg);

    free(gradeIndexes);
}