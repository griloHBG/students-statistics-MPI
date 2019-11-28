// para compilar: mpicc hello.c -o hello -Wall
// para rodar: mpirun -np 2 hello
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mpi.h"

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

enum AggloType
{
    Cities, //aglomeração das cidades (ou seja, poucas cidades no problema)
    Blocks, //aglomeração dos blocos (ou seja, muitas cidades no problema)
};


typedef struct GradeIndex_t
{
    int index;
    int grade;
} GradeIndex;

typedef struct City_t
{
    GradeIndex* grades;
    int city;
    int region;
    int is_main_city;
    int destination;
} City;

int  main(int argc, char *argv[])
{

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
    int regions     = atoi(argv[1]);
    int cities      = atoi(argv[2]);
    int students    = atoi(argv[3]);
    int seed        = atoi(argv[4]); //seed for random values

    srand(seed);

    int total_cities        = regions * cities;
    int students_per_region = cities * students;
    int total_students      = regions * cities * students;
    
    //conversão de índice global para índice da região da nota
#define IDX2REG(I) I / students_per_region
    //conversão de global geral para índice da cidade da nota
#define IDX2CIT(I) I / students
    //conversão de indice global de cidade para índice da região da nota
#define CIT2REG(I) I / cities
    //conversão de índice da região, cidade e estudante da nota para índice global
#define RCS2IDX(R,C,S) R*students_per_region+C*students+S

    int main_region_task = 0;

    int blocks      = 1;

    enum AggloType aggloType;

    int myrank, npes;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &npes);
    
    //guarda para qual tarefa_principal_das_cidades eu tenho que mandar os dados
    int* main_city_tasks = (int*)calloc(regions, sizeof(int));
    
    for (int i = 0; i < regions; ++i)
    {
        main_city_tasks[i] = i * cities;
        printf("[%d] main_city_tasks[%d]=%d\n", myrank, i, main_city_tasks[i]);
    }
    
    if(cities*regions >= npes)
    {
        aggloType = Blocks;
    }
    else
    {
        aggloType = Cities;
        if (students <= 20) //até 20 estudantes não particiona em blocos
        {
            blocks = 1;
        }
        else
        {
            blocks = ceil(npes/(1.0*cities*regions));

            if(blocks > students/(2.0))
            {
                blocks = students/(2.0);
            }
        }

        if(myrank == 0)
        {
            printf("cities*regions is less than np!\nAgglomeration should be along CITIES, but it is NOT IMPLEMENTED!\nThen Agglomeration will be along BLOCKS!\n");
        }
    }
    
    aggloType = Blocks;
    blocks = 1;
    
    //printf("[%d] aggloType: %c\n", myrank, aggloType == Blocks ? 'B' : 'C');

    int total_tasks = blocks * cities * regions;

    int I_can_rest = (myrank >= total_tasks);

    if(I_can_rest)
    {
        //se houver mais nós que o necessário, deixá-los disponíveis (econimizar energia tbm :p).
        MPI_Finalize();
        exit(0);
    }

    int src, dest, msgtag, ret=0;
    char *bufrecv, *bufsend;
    MPI_Status status;
    
    City* allMyCities;
    GradeIndex* grades;

    MPI_Request *requests;

    MPI_Datatype grade_index_dtype;
    MPI_Datatype type[2] = { MPI_INT, MPI_INT};
    int blocklen[2] = { 1, 1 };
    MPI_Aint disp[2] = { 0, sizeof(int)};
    MPI_Type_create_struct(2, blocklen, disp, type, &grade_index_dtype);
    MPI_Type_commit(&grade_index_dtype);

    int data_equal_distributed;
    int data_remaining;
    int more_data = 0;

    int* cities_per_process;

    int cityAmount;
    int* myCities;
    
    int* myRegions;
    
    if(aggloType == Blocks)
    {
    
        data_equal_distributed = total_cities / npes;
        data_remaining = total_cities % npes;
        more_data = 0;
    
        if (myrank == 0) {
            cities_per_process = (int *) calloc(npes, sizeof(int));
        
            for (int r = 0; r < npes; ++r) {
                //se for nó inicial (com índices menor), receberá mais dados que os outros nós
                more_data = (r <= (data_remaining - 1));
            
                cities_per_process[r] = data_equal_distributed + more_data;
            }
    
            cityAmount = cities_per_process[0];
        }
        else
        {
            more_data = (myrank <= (data_remaining - 1));
            cityAmount = data_equal_distributed + more_data;
        }
        
        myRegions = (int*)calloc(cityAmount, sizeof(int));
        myCities = (int*)calloc(cityAmount, sizeof(int));
        allMyCities = (City*)calloc(cityAmount, sizeof(City));
        
        printf("[%d] cityAmount = %d\n", myrank, cityAmount);
        
        //allocating and creating grades if rank 0 and allocating grades if rank >0
        if (myrank == 0)
        {
            grades = (GradeIndex *) calloc(total_students, sizeof(GradeIndex));
            
            for (int i = 0; i < total_students; i++)
            {
                grades[i].index = i;
#ifdef EXAMPLE
                grades[i].grade = example_matrix[i];
        aux_grades[i] = example_matrix[i];
#else
                //grades[i].grade = 100 * (rand() / (1.0 * RAND_MAX));
                //for testing porpouses (grades go increasing by 1 from the first to the last student)
                grades[i].grade = i % 101;
#endif
            }

        }
        else
        {
            grades = (GradeIndex *) calloc((cityAmount) * students, sizeof(GradeIndex));
        }

        if (myrank == 0)
        {
            requests = (MPI_Request *) calloc(npes, sizeof(MPI_Request));

            int offset = 0;
            for (int p = 1; p < npes; p++) {
                offset += cities_per_process[p - 1] * students;
                MPI_Isend(&(grades[offset]), students * cities_per_process[p], grade_index_dtype, p, 0, MPI_COMM_WORLD,
                          &requests[p]);
                for (int c = 0; c < cities_per_process[p]; ++c)
                {
                    MPI_Isend(&(grades[offset]), students * cities_per_process[p], grade_index_dtype, p, 0, MPI_COMM_WORLD,
                              &requests[p]);
                    
                }
            }
        }
        else
        {
            requests = (MPI_Request *) calloc(1, sizeof(MPI_Request));
            printf("[%d] waiting ... | ", myrank);
            MPI_Recv(grades, cityAmount * students, grade_index_dtype, 0, 0, MPI_COMM_WORLD, &status);
            printf("[%d] received!\n", myrank);
        }
    
    
        
        int *i_have_cities_main_task = (int*) calloc(cityAmount, sizeof(int));
        
        for (   int s = 0,                      c = 0;
                s < cityAmount * students            ;
                s = s + students,               c++  )
        {
            allMyCities[c].region = IDX2REG(grades[s].index);
            allMyCities[c].city = IDX2CIT(grades[s].index);
            allMyCities[c].is_main_city = (allMyCities[c].city == main_city_tasks[CIT2REG(c)]);
            allMyCities[c].grades = &(grades[c * students]);
            
            printf("[%d] myregion[%d]=%d\n", myrank, c, allMyCities[c].region);
            
        }
        

        float *cit_avg = (float*)calloc(cityAmount, sizeof(float));
        float *cit_dev = (float*)calloc(cityAmount, sizeof(float));
        float *cit_med = (float*)calloc(cityAmount, sizeof(float));
        float *cit_max = (float*)calloc(cityAmount, sizeof(float));
        float *cit_min = (float*)calloc(cityAmount, sizeof(float));
        
        long int *cit_sum    = (long int*)calloc(cityAmount, sizeof(long int));
        long int *cit_sq_sum = (long int*)calloc(cityAmount, sizeof(long int));
        
        int s;
        
        for (int c = 0; c < cityAmount; ++c)
        {
            cit_sum[c] = 0;
            cit_sq_sum[c] = 0;
            
            for (s = 0; s < students; ++s)
            {
                cit_sum[c] += allMyCities[c].grades[s].grade;
                cit_sq_sum[c] += allMyCities[c].grades[s].grade * allMyCities[c].grades[s].grade;
            }
    
            cit_avg[c] = cit_sum[c] / (1.0 * students);
            cit_dev[c] = sqrt((cit_sq_sum[c] - 2*cit_avg[c]*cit_sum[c] + students * cit_avg[c] * cit_avg[c])/(1.0*(students - 1)));
        }
        
        
        //enviando para as tarefas_principais das cidades
        int city_offset = 0;
        int rank_destination;
        for (int c = 0; c < cityAmount; ++c)
        {
        }
        
        if(myrank == 0)
        {
            for (int p = 1; p < npes; p++)
            {
                MPI_Wait(&requests[p], &status);
            }
        }
    
        free(cit_avg);
        free(cit_dev);
        free(cit_med);
        free(cit_max);
        free(cit_min);
    
        free(cit_sum);
        free(cit_sq_sum);
    }
    else if(aggloType == Cities)
    {
        printf("------------------------------------------\n"
                       "-------------NOT IMPLEMENTED!-------------\n"
                       "------------------------------------------\n");
    }
    
    free(main_city_tasks);
    free(myRegions);
    free(grades);
    
    free(allMyCities);
    free(main_city_tasks);
    
    if(myrank == 0)
    {
        free(cities_per_process);
    }

    MPI_Finalize();
    
    printf("\n[%d] the end! [%d]\n", myrank, myrank);
    
    return 0;
}


