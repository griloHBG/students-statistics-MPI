//
// Created by grilo on 12/11/2019.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <limits.h>
#include <float.h>
#include <math.h>

#define EXAMPLE
#undef EXAMPLE

#define VERBOSE
#undef VERBOSE

#define VERBOSE_ORDERED
#undef VERBOSE_ORDERED

#define VERBOSE_RESULTS
#undef VERBOSE_RESULTS

#define NO_IF
#undef NO_IF

typedef struct GradeIndex_t
{
    int index;
    int grade;
} GradeIndex;

//Quicksort adaptado de //https://www.geeksforgeeks.org/quick-sort/
int partition (GradeIndex *arr, int low, int high, int C){
    int i, j;
    GradeIndex pivot,swap;

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
void quicksort(GradeIndex *arr, int low, int high, int C){
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

void ordena_array(GradeIndex *array, int length)
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

void calcula_desvio_padrao(int* array, float media, float *dp, int length)
{
    int i;
    float soma = 0;

    for(i = 0; i < length; i++)
    {
        soma += pow((array[ i ] - media), 2);
    }

    *dp = sqrt(soma/(length-1));
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

    int seed; //seed for random values

    if( argc < 5)
    {
        printf("Usage: %s <R> <C> <A> <SEED>\n"
               "\n"
               "Where:\n"
               "R: amount of regions (integer)\n"
               "C: amount of cities per regions (integer)\n"
               "A: amount of students per city (integer)\n"
               "SEED: a number used as a seed for pseudo-randomic number generation (integer)\n\n",
               argv[0]);
        exit(0);
    }

    //storing and converting each command-line argument
    int regions      = atoi(argv[1]);
    int cities       = atoi(argv[2]);
    int students     = atoi(argv[3]);
    seed                = atoi(argv[4]);

#ifdef EXAMPLE
    regions      = example_regions;
    cities       = example_cities;
    students     = example_students;
    seed                = example_seed;
#endif

    srand(seed);

    int total_students = regions * cities * students;
    int total_cities = regions * cities;
    int students_per_region = cities * students;

#define IDX2REG(I) I / students_per_region
#define IDX2CIT(I) I / students
//#define IDX2STU(I) (I - IDX2REG(I) * students_per_region) % students
#define RCS2IDX(R,C,S) R*students_per_region+C*students+S
    GradeIndex* grades = (GradeIndex*) calloc(total_students, sizeof(GradeIndex));
    int* aux_grades     = (int*) calloc(total_students, sizeof(int));

    for(int i = 0; i < total_students; i++)
    {
        grades[i].index = i;
#ifdef EXAMPLE
        grades[i].grade = example_matrix[i];
        aux_grades[i] = example_matrix[i];
#else
        grades[i].grade = 100 * (rand() / (1.0 * RAND_MAX));
        aux_grades[i] = grades[i].grade;
        //for testing porpouses (grades go increasing by 1 from the first to the last student)
        //grades[i] = i;
#endif
        //printf("Region %d\tCity %d\t%5d\n", IDX2REG(i), IDX2CIT(i), i);
    }

#ifdef VERBOSE
    for(int r = 0; r < regions; r++)
    {
        for(int c = 0; c < cities; c++)
        {
            for(int s = 0; s < students; s++)
            {
                printf("(%2d,%3d) ", grades[RCS2IDX(r, c, s)].index, grades[RCS2IDX(r, c, s)].grade);
            }
            printf("\n");
        }
        printf("\n");
    }
#endif


#ifdef VERBOSE_ORDERED
    printf("Ordenado!\n");
    ordena_array(grades, total_students);

    for(int r = 0; r < regions; r++)
    {
        for(int c = 0; c < cities; c++)
        {
            for(int s = 0; s < students; s++)
            {
                printf("(%2d,%3d, c%2d|r%d) ", grades[RCS2IDX(r, c, s)].index, grades[RCS2IDX(r, c, s)].grade, IDX2CIT(grades[RCS2IDX(r, c, s)].index), IDX2REG(grades[RCS2IDX(r, c, s)].index) );

            }
            printf("\n");
        }
        //printf("\n");
    }
#endif

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
    if(students % 2 == 0)
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

    int*    min_reg = (int*)calloc(regions, sizeof(int));
    char*   flag_min_reg = (char*)calloc(regions, sizeof(char));
    int all_min_reg = 0;

    int*    max_reg = (int*)calloc(regions, sizeof(int));
    int*    count_elems_reg = (int*)calloc(regions, sizeof(int));

    float*  med_reg = (float*)calloc(regions, sizeof(float));
    float*  avg_reg = (float*)calloc(regions, sizeof(float));
    float*  std_reg = (float*)calloc(regions, sizeof(float));
    //float*  stq_reg = (float*)calloc(regions, sizeof(float));


    memset(flag_min_reg, 0, regions * sizeof(char));
    memset(count_elems_reg, 0, regions * sizeof(int));
    memset(avg_reg, 0, regions * sizeof(float));
    memset(std_reg, 0, regions * sizeof(float));
    //memset(stq_reg, 0, regions * sizeof(float));
    if(students_per_region % 2 == 0)
    {
        memset(med_reg, 0, regions * sizeof(int));
    }

    /*for (int i = 0; i < regions; ++i) {
        //min_reg[i] = INT_MAX;
        //max_reg[i] = INT_MIN;
        //med_reg[i] = 0;
        avg_reg[i] = 0;
        std_reg[i] = 0;
    }*/

    int   min_cou = grades[0].grade;
    int   max_cou = grades[total_students - 1].grade;
    float med_cou = grades[total_students / 2].grade;

    if(total_students % 2 == 0)
    {
        med_cou += grades[total_students / 2 + 1].grade;
        med_cou /= 2;
    }

    float avg_cou = 0;
    float std_cou = 0;
    //float stq_cou = 0;


    int cit_idx, reg_idx;

    int q = 0;

    double time = omp_get_wtime();

    int i, j;

#ifndef VERBOSE_ORDERED
    ordena_array(grades, total_students);
#endif

#ifdef NO_IF
    int test;
    int med_divsor[2] = {1, 2};
#endif

    for (i = 0; i < total_students; ++i) {
        //city statistics
        cit_idx = IDX2CIT(grades[i].index);
#ifdef NO_IF
        test = (all_min_cit != total_cities) && (flag_min_cit[cit_idx] == 0);
        min_cit[cit_idx] = test * grades[i].grade;
        flag_min_cit[cit_idx] = test * 1;
        all_min_cit += test;
#else
        if( (all_min_cit != total_cities) && (flag_min_cit[cit_idx] == 0))
        {
            min_cit[cit_idx] = grades[i].grade;
            flag_min_cit[cit_idx] = 1;
            all_min_cit++;
        }
#endif

        count_elems_cit[cit_idx]++;

#ifdef NO_IF
        max_cit[cit_idx] = (count_elems_cit[cit_idx] == students) * grades[i].grade;
#else
        if(count_elems_cit[cit_idx] == students)
        {
            max_cit[cit_idx] = grades[i].grade;
        }
#endif

#ifdef NO_IF
        med_cit[cit_idx] += (count_elems_cit[cit_idx] == (students / 2)) * grades[i].grade;
#else
        if(count_elems_cit[cit_idx] == (students / 2))
        {
            med_cit[cit_idx] += grades[i].grade;
        }
#endif

#ifdef NO_IF
        test = (students % 2 == 0) &&(count_elems_cit[cit_idx] == ((students / 2)+1));
        med_cit[cit_idx] += test * grades[i].grade;
        med_cit[cit_idx] /= med_divsor[test];
#else
        if((students % 2 == 0) &&(count_elems_cit[cit_idx] == ((students / 2)+1)))
        {
            med_cit[cit_idx] += grades[i].grade;
            med_cit[cit_idx] /= 2;
        }
#endif
        avg_cit[cit_idx] += grades[i].grade;



        //region statistics
        reg_idx = IDX2REG(grades[i].index);

#ifdef NO_IF
        test = (all_min_reg != regions) && (flag_min_reg[reg_idx] == 0);
        min_reg[reg_idx] = test * grades[i].grade;
        flag_min_reg[reg_idx] = test;
        all_min_reg += test;
#else
        if( (all_min_reg != regions) && (flag_min_reg[reg_idx] == 0))
        {
            min_reg[reg_idx] = grades[i].grade;
            flag_min_reg[reg_idx] = 1;
            all_min_reg++;
        }
#endif

        count_elems_reg[reg_idx]++;

#ifdef NO_IF
        max_reg[reg_idx] = (count_elems_reg[reg_idx] == students_per_region) * grades[i].grade;
#else
        if(count_elems_reg[reg_idx] == students_per_region)
        {
            max_reg[reg_idx] = grades[i].grade;
        }
#endif

#ifdef NO_IF
        med_reg[reg_idx] += (count_elems_reg[reg_idx] == (students_per_region / 2)) * grades[i].grade;
#else
        if(count_elems_reg[reg_idx] == (students_per_region / 2))
        {
            med_reg[reg_idx] += grades[i].grade;
        }
#endif

#ifdef NO_IF
        test = (students_per_region % 2 == 0) &&(count_elems_reg[reg_idx] == ((students_per_region / 2)+1));
        med_reg[reg_idx] += test * grades[i].grade;
        med_reg[reg_idx] /= med_divsor[test];
#else
        if((students_per_region % 2 == 0) &&(count_elems_reg[reg_idx] == ((students_per_region / 2)+1)))
        {
            med_reg[reg_idx] += grades[i].grade;
            med_reg[reg_idx] /= 2;
        }
#endif

        avg_reg[reg_idx] += grades[i].grade;

        //countryStats statistics

        avg_cou += grades[i].grade;


        //standards deviations
/*        for (j = (i+1); j < total_students; ++j)
        {
            q = (grades[i].grade - grades[j].grade) * (grades[i].grade - grades[j].grade);

#ifdef NO_IF
            std_cit[cit_idx] += (cit_idx == IDX2CIT(grades[j].index)) * q;
#else
            if(cit_idx == IDX2CIT(grades[j].index))
            {
                std_cit[cit_idx] += q;
            }
#endif

#ifdef NO_IF
            std_reg[reg_idx] += (reg_idx == IDX2REG(grades[j].index)) * q;
#else
            if(reg_idx == IDX2REG(grades[j].index))
            {
                std_reg[reg_idx] += q;
            }
#endif

            std_cou += q;
        }
*/
    }


    for (i = 0; i < total_cities; ++i)
    {
        avg_cit[i] /= students;
    }

    for (i = 0; i < regions; ++i)
    {
        avg_reg[i] /= students_per_region;
    }


    avg_cou /= total_students;

    int c, r;
    /*Calculation of standard deviation without mean knowledge is almost O((n-1)!), which nearly hurts my heart
    for (c = 0; c < total_cities; ++c)
    {
        for(i = 0; i < students; i++)
        {
            for (j = (i+1); j < students; j++)
            {
                std_cit[c] += (grades[i].grade - grades[j].grade) * (grades[i].grade - grades[j].grade);
            }
        }
        std_cit[c] = sqrt(std_cit[c]/(students*(students-1)));
    }
    */

    for (c = 0; c < total_cities; ++c)
    {
        calcula_desvio_padrao(&(aux_grades[c * students]), avg_cit[c], &(std_cit[c]), students);
    }

    /*Calculation of standard deviation without mean knowledge is almost O((n-1)!), which nearly hurts my heart
    for (r = 0; r < regions; ++r)
    {
        for(i = 0; i < students_per_region; i++)
        {
            for (j = (i+1); j < students_per_region; j++)
            {
                std_reg[r] += (grades[i].grade - grades[j].grade) * (grades[i].grade - grades[j].grade);
            }
        }
        std_reg[r] = sqrt(std_reg[r]/(students_per_region*(students_per_region-1)));
    }
     */

    for (r = 0; r < regions; ++r)
    {
        calcula_desvio_padrao(&aux_grades[r * students_per_region], avg_reg[r], &std_reg[r], students_per_region);
    }

    /*for (c = 0; c < total_cities; ++c)
    {
        std_cit[c] = sqrt((std_cit[c])/((students*(students-1))));
    }

    for (r = 0; r < regions; ++r)
    {
        std_reg[r] = sqrt((std_reg[r])/((students_per_region*(students_per_region-1))));
    }

    std_cou = sqrt((std_cou)/((total_students*(total_students-1))));
*/

    calcula_desvio_padrao(aux_grades, avg_cou, &std_cou, total_students);

    float best_reg_avg = -FLT_MAX, best_cit_avg = -FLT_MAX;
    int best_reg_idx, best_cit_idx;

    for (c = 0; c < total_cities; ++c) {
        if(avg_cit[c] > best_cit_avg)
        {
            best_cit_avg = avg_cit[c];
            best_cit_idx = c;
        }
    }

    for (r = 0; r < students_per_region; ++r) {
        if(avg_reg[r] > best_reg_avg)
        {
            best_reg_avg = avg_reg[r];
            best_reg_idx = r;
        }
    }

    time = omp_get_wtime() - time;

    //PRINTING RESULTS!!!
#ifdef VERBOSE_RESULTS
    for (i = 0; i < total_cities; ++i)
    {
        if((i % cities == 0) && (i > 0))
        {
            printf("\n");
        }
        printf("Reg %d - ", i / cities);
        printf("Cid %d: ", i % cities );

        printf("menor: %d, ", min_cit[i]);
        printf("maior: %d, ", max_cit[i]);
        printf("mediana: %4.2f, ", med_cit[i]);
        printf("media: %4.2f e ", avg_cit[i]);
        printf("DP: %4.2f\n", std_cit[i]);
    }
    printf("\n");


    for(i = 0; i < regions; ++i)
    {
        printf("Reg %d:", i);

        printf("menor: %d, ", min_reg[i]);
        printf("maior: %d, ", max_reg[i]);
        printf("mediana: %5.2f, ", med_reg[i]);
        printf("media: %5.2f e ", avg_reg[i]);
        printf("DP: %5.2f\n", std_reg[i]);
    }

    printf("\n");

    printf("Brasil: ");
    printf("menor: %d, ", min_cou);
    printf("maior: %d, ", max_cou);
    printf("mediana: %5.2f, ", med_cou);
    printf("media: %5.2f e ", avg_cou);
    printf("DP: %5.2f\n", std_cou);

    printf("\n");

    printf("Melhor regiao: Regiao %d\n", best_reg_idx);
    printf("Melhor cidade: Regiao %d, Cidade %d\n", best_cit_idx / cities, best_cit_idx % cities);

    printf("\n");

    printf("Tempo de resposta sem considerar E/S, em segundos:%13gs", time);
#else
    printf("%g", time);
#endif

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

    free(grades);
}