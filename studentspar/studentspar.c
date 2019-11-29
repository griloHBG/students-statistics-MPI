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

int sniffer = 0;
#define SNIFFER printf("linha %d (s %d)\n", __LINE__, sniffer++)

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

typedef struct LevelGrades_t
{
    int city;
    int region;
    int is_main_city;
    int destination;
    GradeIndex* grades;
} LevelGrades;

int myself = -1;

int  main(int argc, char *argv[]) {
    /*printf("sizeof(GradeIndex*) = %d\n", sizeof(GradeIndex*));
    printf("sizeof(GradeIndex) = %d\n\n", sizeof(GradeIndex));
    printf("sizeof(LevelGrades*) = %d\n", sizeof(LevelGrades*));
    printf("sizeof(LevelGrades) = %d\n", sizeof(LevelGrades));
    
    printf("sizeof(int) = %d\n", sizeof(int));
    printf("sizeof(int*) = %d\n", sizeof(int*));
    */
    if( argc < 5) {
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
    
    printf("-----------------------------------\nreg = %d\tcit = %d\tstu = %d\tseed = %d\n-----------------------------------\n", regions, cities, students, seed);
    
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

    int blocks      = 1;

    enum AggloType aggloType;

    int myrank, npes;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &npes);
    
    //guarda para qual tarefa_principal_das_cidades eu tenho que mandar os dados
    int* main_city_tasks = (int*)calloc(regions, sizeof(int));
    
    for (int i = 0; i < regions; ++i) {
        main_city_tasks[i] = i * cities;
       // printf("[%d] main_city_tasks[%d]=%d\n", myrank, i, main_city_tasks[i]);
    }
    
    if(cities*regions >= npes) {
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

            if(blocks > students/(2.0)){
                blocks = students/(2.0);
            }
        }

        if(myrank == 0) {
            printf("cities*regions is less than np!\nAgglomeration should be along CITIES, but it is NOT IMPLEMENTED!\nThen Agglomeration will be along BLOCKS!\n");
        }
    }
    
    aggloType = Blocks;
    blocks = 1;
    
    //printf("[%d] aggloType: %c\n", myrank, aggloType == Blocks ? 'B' : 'C');

    int total_tasks = blocks * cities * regions;

    int I_can_rest = (myrank >= total_tasks);

    if(I_can_rest) {
        //se houver mais nós que o necessário, deixá-los disponíveis (econimizar energia tbm :p).
        MPI_Finalize();
        exit(0);
    }

    int src, dest, msgtag, ret=0;
    char *bufrecv, *bufsend;
    MPI_Status status;
    
    LevelGrades* allMyCities;

    MPI_Request *requests;
    
    MPI_Datatype CityGrades_dtype;
    MPI_Datatype type = MPI_INT;
    int blocklen = 6;
    MPI_Aint disp = 0;
    MPI_Type_create_struct(1, &blocklen, &disp, &type, &CityGrades_dtype);
    MPI_Type_commit(&CityGrades_dtype);
    
    MPI_Datatype GradeIndex_dtype;
    type =  MPI_INT;
    blocklen = 2;
    disp = 0;
    MPI_Type_create_struct(1, &blocklen, &disp, &type, &GradeIndex_dtype);
    MPI_Type_commit(&GradeIndex_dtype);
    
    int data_equal_distributed;
    int data_remaining;
    int more_data = 0;

    int* cities_per_process;

    int citiesInThisNode;
    
    if(aggloType == Blocks) {
    
        data_equal_distributed = total_cities / npes;
        data_remaining = total_cities % npes;
    
        if (myrank == 0) {
            cities_per_process = (int *) calloc(npes, sizeof(int));
        
            for (int r = 0; r < npes; ++r) {
                //se for nó inicial (com índices menor), receberá mais dados que os outros nós
                more_data = (r <= (data_remaining - 1));
            
                cities_per_process[r] = data_equal_distributed + more_data;
            }
    
            citiesInThisNode = cities_per_process[0];
        }
        else
        {
            more_data = (myrank <= (data_remaining - 1));
            citiesInThisNode = data_equal_distributed + more_data;
        }
        
        printf("[%d] citiesInThisNode = %d\n", myrank, citiesInThisNode);
        
        //allocating and creating grades if rank 0 and allocating grades if rank >0
        if (myrank == 0) {
            //contador para as cidades de um processo
            int cities_per_process_counter = 0;
            //o processo atual
            int p = 0;
            //indica que um processo devera enviar as coisas da cidade para outro processo
            int is_other_process = 0;
            //marca a destinação das coisas de uma cidade
            int destination = 0;
    
            allMyCities = (LevelGrades*)calloc(total_cities, sizeof(LevelGrades));
            
            for (int r = 0; r < regions; ++r) {
                is_other_process = 0;
                for (int c = 0; c < cities; ++c) {
                    allMyCities[r*cities + c].grades = (GradeIndex *) calloc(students, sizeof(GradeIndex));
    
                    for (int s = 0; s < students; ++s) {
                        allMyCities[r*cities + c].grades[s].index = r * cities * students + c * students + s;
#ifdef EXAMPLE
                        grades[c].grade = example_matrix[c];
        aux_grades[c] = example_matrix[c];
#else
                        //grades[c].grade = 100 * (rand() / (1.0 * RAND_MAX));
                        //for testing porpouses (grades go increasing by 1 from the first to the last student)
                        allMyCities[r*cities + c].grades[s].grade = allMyCities[r*cities + c].grades[s].index % 101;
    
                        //printf("(%3d,%3d) ", allMyCities[r*cities + c].grades[s].index, allMyCities[r*cities + c].grades[s].grade);
#endif
                    }
                    
                    allMyCities[r*cities + c].city = r * cities + c;
                    allMyCities[r*cities + c].region = r;
                    allMyCities[r*cities + c].is_main_city = (c==0);
                    
                    
                    
                    if (cities_per_process_counter == cities_per_process[p]) {
                        cities_per_process_counter = 0;
                        is_other_process = 1;
                        destination = p;
    
                        //printf("[%d] p = %d c = %6d dest = %d is other process = %d\n", myrank, p, r*cities+c, destination, is_other_process);
                        
                        p++;
                    }
                    /*else
                    {
                        printf("[%d] p = %d c = %6d dest = %d is other process =%6d\n", myrank, p, r*cities+c, destination, is_other_process);
                    }*/
                    
                    if(is_other_process) {
                        //processo deve enviar as coisas de sua cidade para outro processo
                        allMyCities[r*cities + c].destination = destination;
                        //printf("\t\t[%d] dest = %d c = %d (OTHER)\n", myrank, allMyCities[r*cities + c].destination , c);
                    }
                    else
                    {
                        //processo NÃO deve enviar as coisas de sua cidade para outro processo
                        allMyCities[r*cities + c].destination = -1;
                        //printf("\t\t[%d] dest = %d c = %d (NOT OTHER)\n", myrank, allMyCities[r*cities + c].destination , c);
                    }
                    
                    //printf("\n");
                    cities_per_process_counter++;
                }
            }

        }
        /*
        int csum = 0;
        
        if(myrank == 0)
            for (int p = 0; p < npes; ++p) {
                printf("p = %d\n", p);
               
                for (int c = csum; c <  csum + cities_per_process[p]; ++c) {
                    printf("\tdest = %d\n", allMyCities[c].destination);
                }
                csum += cities_per_process[p];
            }
          */
        
        int NumberOfMainCityIHave = 0;
        int myFirstMainCityIndex = -1;
        
        if (myrank == 0) {
            NumberOfMainCityIHave = (citiesInThisNode + cities) / cities;
            myFirstMainCityIndex = 0;
            requests = (MPI_Request *) calloc(npes, sizeof(MPI_Request));

            int offset = 0;
            for (int p = 1; p < npes; p++) {
                offset += cities_per_process[p - 1];
                MPI_Isend(&(allMyCities[offset]), cities_per_process[p], CityGrades_dtype, p, 0, MPI_COMM_WORLD,
                          &requests[p]);
                for (int c = 0; c < cities_per_process[p]; ++c) {
                    MPI_Isend(allMyCities[offset + c].grades, students, GradeIndex_dtype, p, 0, MPI_COMM_WORLD,
                              &requests[p]);
                }
            }
        }
        else
        {

            //printf("[%d] citiesInThisNode = %d\n",myrank, citiesInThisNode);


            allMyCities = (LevelGrades *) calloc(citiesInThisNode, sizeof(LevelGrades));

            requests = (MPI_Request *) calloc(2*citiesInThisNode, sizeof(MPI_Request));

            printf("[%d] waiting ...\n", myrank);

            MPI_Recv(allMyCities, citiesInThisNode, CityGrades_dtype, 0, 0, MPI_COMM_WORLD, &status);

            printf("[%d] received!\n", myrank);

            
            for (int c = 0; c < citiesInThisNode; ++c) {
                if(allMyCities[c].city % cities == 0)
                {
                    NumberOfMainCityIHave += 1;
                    if(myFirstMainCityIndex == -1)
                    {
                        myFirstMainCityIndex = allMyCities[c].city;
                    }
                }
                allMyCities[c].grades = (GradeIndex *) calloc(students, sizeof(GradeIndex));
                MPI_Recv(allMyCities[c].grades, students, GradeIndex_dtype, 0, 0, MPI_COMM_WORLD, &status);
            }
    

            /*for (int c = 0; c < citiesInThisNode; ++c) {
                for (int s = 0; s < students; ++s) {
                    printf("(%3d,%3d) ", allMyCities[c].grades[s].index, allMyCities[c].grades[s].grade);
                }
            }*/
        }
        
        float *cit_avg = (float*)calloc(citiesInThisNode, sizeof(float));
        float *cit_dev = (float*)calloc(citiesInThisNode, sizeof(float));
        float *cit_med = (float*)calloc(citiesInThisNode, sizeof(float));
        float *cit_max = (float*)calloc(citiesInThisNode, sizeof(float));
        float *cit_min = (float*)calloc(citiesInThisNode, sizeof(float));
        
        float *cit_sum    = (float*)calloc(citiesInThisNode, sizeof(float));
        float *cit_sq_sum = (float*)calloc(citiesInThisNode, sizeof(float));
        
        printf("[%d] bora fazer conta\n", myrank);
        
        int dontNeedToRecvFromOtherCities = MIN(cities, citiesInThisNode);
        int thisNodelastCityIndex = -1;
        
        for (int c = 0; c < citiesInThisNode; ++c) {
            if(NumberOfMainCityIHave > 0)
            {
                printf("NumberOfMainCityIHave = %d\n", NumberOfMainCityIHave);
                if(allMyCities[c].city % cities == 0)
                {
                    dontNeedToRecvFromOtherCities = 0;
                    thisNodelastCityIndex = allMyCities[c].city - 1;
                }
            }
    
            dontNeedToRecvFromOtherCities +=  (allMyCities[c].destination == myself);
            
            cit_sum[c] = 0;
            cit_sq_sum[c] = 0;
            
            for (int s = 0; s < students; ++s) {
                cit_sum[c] += allMyCities[c].grades[s].grade;
                cit_sq_sum[c] += allMyCities[c].grades[s].grade * allMyCities[c].grades[s].grade;
            }
    
            cit_avg[c] = cit_sum[c] / (1.0 * students);
            cit_dev[c] = sqrt((cit_sq_sum[c] - 2*cit_avg[c]*cit_sum[c] + students * cit_avg[c] * cit_avg[c])/(1.0*(students - 1)));
            //printf("[%d] cit_avg[%6d]=%f\tcit_dev[%6d]=%f\n", myrank, c, cit_avg[c], c, cit_dev[c]);
            thisNodelastCityIndex += (NumberOfMainCityIHave > 0);
        }
        
        int needToRecvFromOtherCities = NumberOfMainCityIHave > 0 ? cities - dontNeedToRecvFromOtherCities : 0;
        
        printf("[%d] needToRecvFromOtherCities = %d\n", myrank, needToRecvFromOtherCities);
        
        //enviando para as tarefas_principais das cidades
        int city_offset = 0;
        int rank_destination;
        float statsBuffer[6];
        
        for (int c = 0; c < citiesInThisNode; ++c) {
            //printf("[%d] city = %6d ||  dest = %6d\n", myrank, allMyCities[c].city, allMyCities[c].destination);
            if(allMyCities[c].destination != myself) {
                statsBuffer[0] = cit_min[c];
                statsBuffer[1] = cit_max[c];
                statsBuffer[2] = cit_med[c];
                statsBuffer[3] = cit_avg[c];
                statsBuffer[4] = cit_dev[c];
                statsBuffer[5] = cit_sq_sum[c];
                MPI_Isend(statsBuffer, 6, MPI_FLOAT, allMyCities[c].destination, allMyCities[c].city, MPI_COMM_WORLD, &requests[2*c]);
                MPI_Isend(allMyCities[c].grades, students, GradeIndex_dtype, allMyCities[c].destination, allMyCities[c].city+total_students, MPI_COMM_WORLD, &requests[2*c+1]);
                printf("[%d] Isend: dest(%d), tag(%5d)\n", myrank, allMyCities[c].destination, allMyCities[c].city);
            }
        }
        LevelGrades *regionCities;
        if(NumberOfMainCityIHave > 0)
        {
            regionCities = (LevelGrades*) calloc(NumberOfMainCityIHave * cities, sizeof(LevelGrades));
        }
        
        printf("[%d] thisNodelastCityIndex: %d, needToRecvFromOtherCities: %d\n", myrank, thisNodelastCityIndex, needToRecvFromOtherCities);
        for (int c = thisNodelastCityIndex + 1; c < thisNodelastCityIndex + 1 + needToRecvFromOtherCities; ++c)
        {
            regionCities[c - (thisNodelastCityIndex - 1)].grades = (GradeIndex*)calloc(students, sizeof(GradeIndex));
            MPI_Recv(&regionCities[c - (thisNodelastCityIndex - 1)], 1, CityGrades_dtype, MPI_ANY_SOURCE, c, MPI_COMM_WORLD, &status);
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            //MPI_Recv(&regionCities[c - (thisNodelastCityIndex - 1)].grades, students, CityGrades_dtype, MPI_ANY_SOURCE, c + total_students, MPI_COMM_WORLD, &status);
            printf("[%d] Recebido: source(ANY), tag(%5d)\n", myrank, c);
            printf("[%d] conteudo:\n");
            printf("\t\t(%d) min = %.2f max = %.2f med = %.2f avg = %.2f dev = %.2f sq_sum = %.2f\n", myrank, c);
        }
    
        if((myrank != 0)) {
            printf("esperando os MPI_Isend!\n");
            for (int c = 0; c < citiesInThisNode; c++) {
                if(allMyCities[c].destination != myself) {
                    MPI_Wait(&requests[2*c], &status);
                    printf("[%d] foi os reqs %d e ", myrank, 2*c);
                    MPI_Wait(&requests[2*c+1], &status);
                    printf("%d\n", 2*c+1);
                }
            }
        }*/
        
        /*if(myrank == 0) {
            printf("esperando os MPI_Isend!\n");
            //TODO wait nos requests nao utilizados?
            for (int p = 1; p < npes; p++) {
                MPI_Wait(&requests[p], &status);
            }
        }
        else
        {
        
        }*/
    
        free(cit_avg);
        free(cit_dev);
        free(cit_med);
        free(cit_max);
        free(cit_min);
    
        free(cit_sum);
        free(cit_sq_sum);
        if(NumberOfMainCityIHave > 0)
        {
            /*for (int c = 0; c < needToRecvFromOtherCities; ++c)
            {
                free(regionCities[c].grades);
            }*/
            free(regionCities);
        }
    }
    else if(aggloType == Cities) {
        printf("------------------------------------------\n"
                       "-------------NOT IMPLEMENTED!-------------\n"
                       "------------------------------------------\n");
    }
    
    free(main_city_tasks);
    
    if(myrank == 0)
    {
        for (int c = 0; c < total_cities; ++c)
        {
            free(allMyCities[c].grades);
        }
    }else
    {
        for (int c = 0; c < citiesInThisNode; ++c)
        {
            free(allMyCities[c].grades);
        }
    }
    
    free(allMyCities);
    free(main_city_tasks);
    
    if(myrank == 0) {
        free(cities_per_process);
    }
    
    free(requests);
    
    MPI_Finalize();
    
    printf("\n[%d] the end! [%d]\n", myrank, myrank);
    
    return 0;
}


