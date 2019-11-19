#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <limits.h>
#include <math.h>
#include <float.h>

#define MAX(A, B) A>B?A:B
#define MIN(A, B) A<B?A:B

#define EXAMPLE
#undef EXAMPLE

#define VERBOSE
#undef VERBOSE

#define VERBOSE_ORDERED
#undef VERBOSE_ORDERED

#define VERBOSE_RESULTS
#undef  VERBOSE_RESULTS

#ifdef EXAMPLE
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
#endif

typedef struct grades_t{
    int regions;    // amount of regions
    int cities;     // amount of cities per region
    int students;   // amount of students per city
    int* grades;    // all the grades in a linear array organized in:
                    //      regions depth
                    //      cities rows and
                    //      students columns
    // city statistics
    int* lowest_cities;    //lowest grades of each city
    int* highest_cities;   //highest grades of each city
    float* median_cities;  //medians of each city
    float* mean_cities;    //means of each city
    float* stddev_cities;   //standard deviations of each city

    // region statistics
    int* lowest_regions;    //lowest grades of each region
    int* highest_regions;   //highest grades of each region
    float* median_regions;  //medians of each region
    float* mean_regions;    //means of each region
    float* stddev_regions;   //standard deviations of each region

    // country statistics
    int lowest_country;    //lowest grades of country
    int highest_country;   //highest grades of country
    float median_country;  //medians of country
    float mean_country;    //means of country
    float stddev_country;   //standard deviations of country

} Grades;

//Quicksort adaptado de //https://www.geeksforgeeks.org/quick-sort/
int partition (int *arr, int low, int high, int C){
    int i, j;
    int pivot,swap;

    // pivot (Element to be placed at right position)
    pivot = arr[high*C];

    i = (low - 1);  // Index of smaller element

    for (j = low; j <= high-1; j++)
    {
        // If current element is smaller than or
        // equal to pivot
        if (arr[j*C] <= pivot)
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
void quicksort(int *arr, int low, int high, int C){
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

void ordena_linhas(int *array, int length)
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

void calcula_media(int *array, float* ret, int length)
{
    int i,j;
    float soma = 0;
    for(i=0;i<length;i++)
    {
        soma+=array[i];
    }
    *ret = soma/length;
}

void calcula_desvio_padrao(int *array, float media, float *dp, int length)
{
    int i,j;
    float soma = 0;

    for( j = 0; j < length; j++)
    {
        soma += pow((array[ j ] - media), 2);
    }

    *dp = sqrt(soma/(length-1));
}

int main(int argc, char* argv[])
{

    Grades brazil; // structure that holds everything that is data important
    int seed; //seed for random values
    int r, c, s; //just some counter for region, city and student, respectively

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
    brazil.regions      = atoi(argv[1]);
    brazil.cities       = atoi(argv[2]);
    brazil.students     = atoi(argv[3]);
    seed                = atoi(argv[4]);

#ifdef EXAMPLE
    brazil.regions      = example_regions;
    brazil.cities       = example_cities;
    brazil.students     = example_students;
    seed                = example_seed;
#endif

    srand(seed);

    int students_per_region = brazil.cities * brazil.students;
    int students_total = brazil.regions * brazil.cities * brazil.students;
    int cities_total = brazil.regions * brazil.cities;

    brazil.grades   = (int*) calloc(brazil.regions * brazil.cities * brazil.students, sizeof(int));

    brazil.lowest_cities  = (int*) calloc(brazil.regions * brazil.cities, sizeof(int));
    brazil.highest_cities = (int*) calloc(brazil.regions * brazil.cities, sizeof(int));
    brazil.median_cities  = (float*) calloc(brazil.regions * brazil.cities, sizeof(float));
    brazil.mean_cities    = (float*) calloc(brazil.regions * brazil.cities, sizeof(float));
    brazil.stddev_cities   = (float*) calloc(brazil.regions * brazil.cities, sizeof(float));

    brazil.lowest_regions  = (int*) calloc(brazil.regions, sizeof(int));
    brazil.highest_regions = (int*) calloc(brazil.regions, sizeof(int));
    brazil.median_regions  = (float*) calloc(brazil.regions, sizeof(float));
    brazil.mean_regions    = (float*) calloc(brazil.regions, sizeof(float));
    brazil.stddev_regions   = (float*) calloc(brazil.regions, sizeof(float));


    brazil.lowest_country = INT_MAX;
    brazil.highest_country = INT_MIN;
    brazil.mean_country = 0;

    for(r = 0; r < brazil.regions; ++r)
    {
        brazil.lowest_regions[r] = INT_MAX;
        brazil.highest_regions[r] = INT_MIN;
        brazil.mean_regions[r] = 0;

        for (c = 0; c < brazil.cities; ++c)
        {
            brazil.lowest_cities[r * brazil.cities + c] = INT_MAX;
            brazil.highest_cities[r * brazil.cities + c] = INT_MIN;
            brazil.mean_cities[r * brazil.cities + c] = 0;

            for (s = 0; s < brazil.students; ++s)
            {
            #ifdef EXAMPLE
                brazil.grades[r * students_per_region + c * brazil.students + s ] = example_matrix[r * students_per_region + c * brazil.students + s ];
            #else
                brazil.grades[ r * brazil.cities * brazil.students + c * brazil.students + s ] = 100*(rand()/(1.0*RAND_MAX));
                //for testing porpouses (grades go increasing by 1 from the first to the last student)
                //brazil.grades[ r * brazil.cities * brazil.students + c * brazil.students + s ] = r * brazil.cities * brazil.students + c * brazil.students + s;
            #endif
            }
        }
    }

#ifdef VERBOSE
    printf("As is:\n\n");
    for(r = 0; r < brazil.regions; ++r)
    {
        printf("Region %5d:\n", r);
        for (c = 0; c < brazil.cities; ++c)
        {
            printf("             ");
            for (s = 0; s < brazil.students; ++s)
            {
                printf("%5d", brazil.grades[r * students_per_region + c * brazil.students + s ]);
            }
            printf("\n");
        }
    }

    printf("\n");
#endif

#ifdef VERBOSE_ORDERED
    for(r = 0; r < brazil.regions; ++r)
    {
        for (c = 0; c < brazil.cities; ++c)
        {
            ordena_linhas(&(brazil.grades[r * students_per_region + c * brazil.students ]), brazil.students);
        }
    }

    printf("Cities Ordered:\n\n");
    for(r = 0; r < brazil.regions; ++r)
    {
        printf("Region %5d:\n", r);
        for (c = 0; c < brazil.cities; ++c)
        {
            printf("             ");
            for (s = 0; s < brazil.students; ++s)
            {
                printf("%5d", brazil.grades[r * students_per_region + c * brazil.students + s ]);
            }
            printf("\n");
        }
    }

    printf("\n");
#endif

    double time = omp_get_wtime();
    int grade;
    int index_cities, index_grades;

    for(r = 0; r < brazil.regions; ++r)
    {
        for (c = 0; c < brazil.cities; ++c)
        {
#ifndef VERBOSE_ORDERED
            ordena_linhas(&(brazil.grades[r * students_per_region + c * brazil.students ]), brazil.students);
#endif
            index_cities = r * brazil.cities + c;
            index_grades = r * students_per_region + c * brazil.students;

            brazil.lowest_cities[index_cities] = brazil.grades[index_grades + 0 ];
            brazil.highest_cities[index_cities] = brazil.grades[index_grades + brazil.students - 1 ];
            calcula_mediana(&brazil.grades[ index_grades ], &brazil.median_cities[index_cities], brazil.students);
            calcula_media(&brazil.grades[ index_grades ], &brazil.mean_cities[index_cities], brazil.students);
            calcula_desvio_padrao(&brazil.grades[ index_grades ], brazil.mean_cities[index_cities], &brazil.stddev_cities[index_cities], brazil.students);

            brazil.lowest_regions[r] = MIN(brazil.lowest_cities[index_cities], brazil.lowest_regions[r]);
            brazil.highest_regions[r] = MAX(brazil.highest_cities[index_cities], brazil.highest_regions[r]);
            brazil.mean_regions[r] += brazil.mean_cities[index_cities];
        }

        /*calcula_mediana(&brazil.grades[ index_grades ], &brazil.median_cities[index_cities], brazil.students);
        calcula_media(&brazil.grades[ index_grades ], &brazil.mean_cities[index_cities], brazil.students);
        calcula_desvio_padrao(&brazil.grades[ index_grades ], brazil.mean_cities[index_cities], &brazil.stddev_cities[index_cities], brazil.students);
*/
        brazil.highest_country = MAX(brazil.highest_regions[r], brazil.highest_country);
        brazil.lowest_country = MIN(brazil.lowest_regions[r], brazil.lowest_country);
        brazil.mean_regions[r] = brazil.mean_regions[r] / brazil.cities;

        brazil.mean_country += brazil.mean_regions[r];
        calcula_desvio_padrao(&brazil.grades[r * students_per_region], brazil.mean_regions[r], &brazil.stddev_regions[r], students_per_region);
    }

    brazil.mean_country = brazil.mean_country / brazil.regions;
    calcula_desvio_padrao(brazil.grades, brazil.mean_country, &brazil.stddev_country, students_total);

    for(r = 0; r < brazil.regions; ++r)
    {
        ordena_linhas(&brazil.grades[r * students_per_region], students_per_region);

        calcula_mediana(&brazil.grades[r * students_per_region], &brazil.median_regions[r], students_per_region);
    }


#ifdef VERBOSE_ORDERED
    printf("Region Ordered:\n\n");
    for(r = 0; r < brazil.regions; ++r)
    {
        printf("Region %5d:\n", r);
        for (c = 0; c < brazil.cities; ++c)
        {
            printf("             ");
            for (s = 0; s < brazil.students; ++s)
            {
                printf("%5d", brazil.grades[r * students_per_region + c * brazil.students + s ]);
            }
            printf("\n");
        }
    }

    printf("\n");
#endif

    ordena_linhas(brazil.grades, students_total);

    calcula_mediana(brazil.grades, &brazil.median_country, students_total);

    float best_reg_avg = -FLT_MAX, best_cit_avg = -FLT_MAX;
    int best_reg_idx, best_cit_idx;

    for (int c = 0; c < cities_total; ++c) {
        if(brazil.mean_cities[c] > best_cit_avg)
        {
            best_cit_avg = brazil.mean_cities[c];
            best_cit_idx = c;
        }
    }

    for (int r = 0; r < students_per_region; ++r) {
        if(brazil.mean_regions[r] > best_reg_avg)
        {
            best_reg_avg = brazil.mean_regions[r];
            best_reg_idx = r;
        }
    }


    time = omp_get_wtime() - time;
    //Output

#ifdef VERBOSE_RESULTS
    for(r = 0; r < brazil.regions; ++r)
    {
        for (c = 0; c < brazil.cities; ++c)
        {
            index_cities = r * brazil.cities + c;

            printf("Reg %d - ", r);
            printf("Cid %d: ", c);

            printf("menor: %d, ", brazil.lowest_cities[index_cities]);
            printf("maior: %d, ", brazil.highest_cities[index_cities]);
            printf("mediana: %4.2f, ", brazil.median_cities[index_cities]);
            printf("media: %4.2f e ", brazil.mean_cities[index_cities]);
            printf("DP: %4.2f\n", brazil.stddev_cities[index_cities]);
        }
        printf("\n");
    }

    for(r = 0; r < brazil.regions; ++r)
    {
        printf("Reg %d:", r);

        printf("menor: %d, ", brazil.lowest_regions[r]);
        printf("maior: %d, ", brazil.highest_regions[r]);
        printf("mediana: %5.2f, ", brazil.median_regions[r]);
        printf("media: %5.2f e ", brazil.mean_regions[r]);
        printf("DP: %5.2f\n", brazil.stddev_regions[r]);
    }

    printf("\n");

    printf("Brasil: ");
    printf("menor: %d, ", brazil.lowest_country);
    printf("maior: %d, ", brazil.highest_country);
    printf("mediana: %5.2f, ", brazil.median_country);
    printf("media: %5.2f e ", brazil.mean_country);
    printf("DP: %5.2f\n", brazil.stddev_country);

    printf("\n");

    printf("Melhor regiao: Regiao %d\n", best_reg_idx);
    printf("Melhor cidade: Regiao %d, Cidade %d\n", best_cit_idx / brazil.cities, best_cit_idx % brazil.cities);

    printf("\n");

    printf("Tempo de resposta sem considerar E/S, em segundos:%13gs", time);
#else
    printf("%g", time);
#endif

    free(brazil.grades);

    free(brazil.lowest_cities);
    free(brazil.highest_cities);
    free(brazil.median_cities);
    free(brazil.mean_cities);
    free(brazil.stddev_cities);

    free(brazil.lowest_regions);
    free(brazil.highest_regions);
    free(brazil.median_regions);
    free(brazil.mean_regions);
    free(brazil.stddev_regions);

    return 0;
}