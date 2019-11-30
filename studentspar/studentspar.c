// para compilar: mpicc hello.c -o hello -Wall
// para rodar: mpirun -np 2 hello
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    //int index;
    int grade;
} GradeIndex;

typedef struct stats_t
{
    int min;
    int max;
    float med;
    float avg;
    float dev;
    long int sq_sum;
    
} Stats;

typedef struct CityGrades_t
{
    int city;
    int region;
    int is_main_city;
    int destination;
    
    Stats stats;
    
    GradeIndex *grades;
    
} CityGrades;

typedef struct RegionGrades_t
{
    int region;
    
    Stats stats;
    
    Stats *cit_stats;
    
    GradeIndex *grades;
    
} RegionGrades;

typedef struct CountryGrades_t
{
    Stats stats;
    
    Stats *reg_stats;
    
    Stats *cit_stats;
    
    GradeIndex *grades;
    
} CountryGrades;

int myself = -1;

int main(int argc, char *argv[]) {
    /*printf("sizeof(GradeIndex*) = %d\n", sizeof(GradeIndex*));
    printf("sizeof(GradeIndex) = %d\n\n", sizeof(GradeIndex));
    printf("sizeof(CityGrades*) = %d\n", sizeof(CityGrades*));
    printf("sizeof(CityGrades) = %d\n", sizeof(CityGrades));
    
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
    
    CityGrades* allMyCities;

    MPI_Request *requests;
    /*
    typedef struct CityGrades_t
    {
        int city;
        int region;
        int is_main_city;
        int destination;
        
        Stats stats;
        
        long int sq_sum;
        GradeIndex *grades;
        
    } CityGrades;
    */
    MPI_Datatype CityGradesHeader_dtype;
    MPI_Datatype type[4]    = { MPI_INT, 0, 0, 0};
    int blocklen[4]         = { 4, 0, 0, 0};
    MPI_Aint disp[4]        = { 0, 0, 0, 0};
    MPI_Type_create_struct(1, blocklen, disp, type, &CityGradesHeader_dtype);
    MPI_Type_commit(&CityGradesHeader_dtype);
    
    MPI_Datatype Stats_dtype;
    type[0]    = MPI_INT;
    type[1]    = MPI_FLOAT;
    type[2]    = MPI_LONG_INT;
    blocklen[0] = 2;
    blocklen[1] = 3;
    blocklen[2] = 1;
    disp[0] = 0;
    disp[1] = 2*sizeof(int);
    disp[2] = 2*sizeof(int)+3*sizeof(float);
    MPI_Type_create_struct(3, blocklen, disp, type, &Stats_dtype);
    MPI_Type_commit(&Stats_dtype);
    
    MPI_Datatype GradeIndex_dtype;
    type[0] =  MPI_INT;
    blocklen[0] = 1;
    disp[0] = 0;
    MPI_Type_create_struct(1, blocklen, disp, type, &GradeIndex_dtype);
    MPI_Type_commit(&GradeIndex_dtype);
    
    int data_equal_distributed;
    int data_remaining;
    int more_data = 0;

    int* cities_per_process;

    int citiesInThisNode;
    
    int dontNeedToRecvFromOtherCities;
    int thisNodelastCityIndex = -1;
    
    RegionGrades *regionCities;
    
    CountryGrades *countryGrades;
    
    int *myRegions;
    
    if(aggloType == Blocks) {
    
        data_equal_distributed = total_cities / npes;
        data_remaining = total_cities % npes;
    
        if (myrank == 0) {
            cities_per_process = (int *) calloc(npes, sizeof(int));
        
            for (int r = 0; r < npes; ++r) {
                //se for nó inicial (com índices menores), receberá mais dados que os outros nós
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
    
            allMyCities = (CityGrades*)calloc(total_cities, sizeof(CityGrades));
            
            for (int r = 0; r < regions; ++r) {
                is_other_process = 0;
                for (int c = 0; c < cities; ++c) {
                    allMyCities[r*cities + c].grades = (GradeIndex *) calloc(students, sizeof(GradeIndex));
    
                    for (int s = 0; s < students; ++s) {
                        //allMyCities[r*cities + c].grades[s].index = r * cities * students + c * students + s;
#ifdef EXAMPLE
                        grades[c].grade = example_matrix[c];
        aux_grades[c] = example_matrix[c];
#else
                        //grades[c].grade = 100 * (rand() / (1.0 * RAND_MAX));
                        //for testing porpouses (grades go increasing by 1 from the first to the last student)
                        allMyCities[r*cities + c].grades[s].grade = (r * cities * students + c * students + s) % 101;
    
                        //printf("(%3d,%3d) ", allMyCities[r*cities + c].grades[s].index, allMyCities[r*cities + c].grades[s].grade);
                        //printf("(%3d) ", allMyCities[r*cities + c].grades[s].grade);
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
    
        for (int r = 0; r < regions; ++r)
        {
            printf("r%3d\n", allMyCities[r*cities].region);
            for (int c = 0; c < cities; ++c)
            {
                printf("c%3d - dest %3d", allMyCities[r * cities + c].city, allMyCities[r * cities + c].destination);
                for (int s = 0; s < students; ++s)
                {
                    printf("%3d  ", allMyCities[r * cities + c].grades[s].grade);
                }
                printf("\n");
            }
        }
        
        int numberOfMainCityIHave = 0;
        int myFirstMainCityIndex = -1;
        
        if (myrank == 0) {
            numberOfMainCityIHave = (citiesInThisNode + cities) / cities;
            myFirstMainCityIndex = 0;
            requests = (MPI_Request *) calloc(npes, sizeof(MPI_Request));

            int offset = 0;
            for (int p = 1; p < npes; p++) {
                offset += cities_per_process[p - 1];
                for (int c = 0; c < cities_per_process[p]; ++c) {
                    MPI_Isend(&(allMyCities[offset]), 1, CityGradesHeader_dtype, p, 0, MPI_COMM_WORLD,
                              &requests[p]);
                    MPI_Isend(allMyCities[offset + c].grades, students, GradeIndex_dtype, p, 0, MPI_COMM_WORLD,
                              &requests[p]);
                }
            }
        }
        else
        {

            //printf("[%d] citiesInThisNode = %d\n",myrank, citiesInThisNode);


            allMyCities = (CityGrades *) calloc(citiesInThisNode, sizeof(CityGrades));

            requests = (MPI_Request *) calloc(2*citiesInThisNode, sizeof(MPI_Request));

            myRegions = (int*) calloc(numberOfMainCityIHave, sizeof(int));
            
            for (int c = 0; c < citiesInThisNode; ++c) {
    
                printf("[%d] waiting ...\n", myrank);
    
                MPI_Recv(allMyCities, 1, CityGradesHeader_dtype, 0, 0, MPI_COMM_WORLD, &status);
                MPI_Recv(allMyCities[c].grades, students, GradeIndex_dtype, 0, 0, MPI_COMM_WORLD, &status);
    
                printf("[%d] received!\n", myrank);
                
                CityGrades* amc = &(allMyCities[c]);
                printf("[%d] received: cit %d , reg %d , imc %d , dest %d\n", myrank, amc->city, amc->region, amc->is_main_city, amc->destination);
                if(allMyCities[c].city % cities == 0)
                {
                    numberOfMainCityIHave += 1;
                    if(myFirstMainCityIndex == -1)
                    {
                        myFirstMainCityIndex = allMyCities[c].city;
                    }
                    
                    myRegions[c] = allMyCities[c].region;
                }
                allMyCities[c].grades = (GradeIndex *) calloc(students, sizeof(GradeIndex));
            }
    

            for (int c = 0; c < citiesInThisNode; ++c) {
                for (int s = 0; s < students; ++s) {
                    printf("(%3d) ", allMyCities[c].grades[s].grade);
                }
            }
        }
        /*
        float *cit_avg = (float*)calloc(citiesInThisNode, sizeof(float));
        float *cit_dev = (float*)calloc(citiesInThisNode, sizeof(float));
        float *cit_med = (float*)calloc(citiesInThisNode, sizeof(float));
        float *cit_max = (float*)calloc(citiesInThisNode, sizeof(float));
        float *cit_min = (float*)calloc(citiesInThisNode, sizeof(float));
        */
        float *cit_sum    = (float*)calloc(citiesInThisNode, sizeof(float));
        /*
        float *cit_sq_sum = (float*)calloc(citiesInThisNode, sizeof(float));
        */
        printf("[%d] bora fazer conta\n", myrank);
        
        dontNeedToRecvFromOtherCities = MIN(cities, citiesInThisNode);
        thisNodelastCityIndex = -1;
        
        for (int c = 0; c < citiesInThisNode; ++c) {
            if(numberOfMainCityIHave > 0)
            {
                printf("numberOfMainCityIHave = %d\n", numberOfMainCityIHave);
                if(allMyCities[c].city % cities == 0)
                {
                    dontNeedToRecvFromOtherCities = 0;
                    thisNodelastCityIndex = allMyCities[c].city - 1;
                }
            }
    
            dontNeedToRecvFromOtherCities +=  (allMyCities[c].destination == myself);
            
            cit_sum[c] = 0;
            allMyCities[c].stats.sq_sum = 0;
            
            for (int s = 0; s < students; ++s) {
                cit_sum[c] += allMyCities[c].grades[s].grade;
                allMyCities[c].stats.sq_sum += allMyCities[c].grades[s].grade * allMyCities[c].grades[s].grade;
            }
    
            allMyCities[c].stats.avg = cit_sum[c] / (1.0 * students);
            allMyCities[c].stats.dev = sqrt((allMyCities[c].stats.sq_sum - 2*allMyCities[c].stats.avg*cit_sum[c] + students * allMyCities[c].stats.avg * allMyCities[c].stats.avg)/(1.0*(students - 1)));
            
            thisNodelastCityIndex += (numberOfMainCityIHave > 0);
        }
        
        int needToRecvFromOtherCities = numberOfMainCityIHave > 0 ? cities - dontNeedToRecvFromOtherCities : 0;
        
        printf("[%d] needToRecvFromOtherCities = %d\n", myrank, needToRecvFromOtherCities);
        
        //enviando para as tarefas_principais das cidades
        int city_offset = 0;
        int rank_destination;
        //float cityStatsSend[6];
        
        for (int c = 0; c < citiesInThisNode; ++c) {
            //printf("[%d] city = %6d ||  dest = %6d\n", myrank, allMyCities[c].city, allMyCities[c].destination);
            if(allMyCities[c].destination != myself) {
                printf("[%d] enviando city %d para proc %d\n", myrank, allMyCities[c].city, allMyCities[c].destination);
                /*
                cityStatsSend[0] = cit_min[c];
                cityStatsSend[1] = cit_max[c];
                cityStatsSend[2] = cit_med[c];
                cityStatsSend[3] = cit_avg[c];
                cityStatsSend[4] = cit_dev[c];
                cityStatsSend[5] = cit_sq_sum[c];
                */
                MPI_Isend(&allMyCities[c], 1, CityGradesHeader_dtype, allMyCities[c].destination, allMyCities[c].city, MPI_COMM_WORLD, &requests[2 * c]);
                MPI_Isend(allMyCities[c].grades, students, GradeIndex_dtype, allMyCities[c].destination, allMyCities[c].city+total_cities, MPI_COMM_WORLD, &requests[2*c+1]);
                printf("[%d] Isend: dest(%d), tag(%5d)\n", myrank, allMyCities[c].destination, allMyCities[c].city);
            }
        }
    
        if(myrank != 0) {
            printf("esperando os MPI_Isend!\n");
            for (int c = 0; c < citiesInThisNode; c++) {
                if(allMyCities[c].destination != myself) {
                    printf("esperando req %d", 2*c);
                    MPI_Wait(&requests[2*c], &status);
                    printf("[%d] foi o req %d\n", myrank, 2*c);
                    printf("esperando req %d", 2*c+1);
                    MPI_Wait(&requests[2*c+1], &status);
                    printf("[%d] foi o req %d\n", myrank, 2*c+1);
                }
            }
        }
        
        if(numberOfMainCityIHave > 0)
        {
            regionCities = (RegionGrades*) calloc(numberOfMainCityIHave, sizeof(RegionGrades));
    
            int myLastRegion = numberOfMainCityIHave - 1;
            
            for (int r = 0; r < numberOfMainCityIHave; ++r)
            {
                regionCities[r].grades = (GradeIndex *) calloc(cities * students, sizeof(GradeIndex));
                
                regionCities[r].cit_stats = (Stats *) calloc(cities, sizeof(Stats));
                
                for (int c = (citiesInThisNode+needToRecvFromOtherCities) - (numberOfMainCityIHave*cities); c < citiesInThisNode; ++c)
                {
                    memcpy(&(regionCities[r].grades[c]), allMyCities[c].grades, students * sizeof(CityGrades));
                    memcpy(regionCities[r].cit_stats, &(allMyCities[c].stats), sizeof(Stats));
                }
            }
            
            CityGrades cityStatsRcvBuffer;
            
            for (int c = thisNodelastCityIndex + 1; c < thisNodelastCityIndex + 1 + needToRecvFromOtherCities; ++c)
            {
                int cityIndex = (c - (numberOfMainCityIHave - 1) * cities);
                int regionIndex = numberOfMainCityIHave - 1;
                //MPI_Recv(&citiesStats[cityIndex], 1, CityGradesHeader_dtype, MPI_ANY_SOURCE, c, MPI_COMM_WORLD, &status);
                MPI_Recv(regionCities[regionIndex].cit_stats, 1, Stats_dtype, MPI_ANY_SOURCE, c, MPI_COMM_WORLD, &status);
                
                printf("[%d] Recebido: source(ANY), tag(%5d)\n", myrank, c);
                printf("[%d] conteudo:\n", myrank);
                printf("\t\t(%d) min = %.2f max = %.2f med = %.2f avg = %.2f dev = %.2f sq_sum = %.2f\n", c,
                       regionCities[regionIndex].cit_stats[c].max,
                       regionCities[regionIndex].cit_stats[c].min,
                       regionCities[regionIndex].cit_stats[c].med,
                       regionCities[regionIndex].cit_stats[c].avg,
                       regionCities[regionIndex].cit_stats[c].dev,
                       regionCities[regionIndex].cit_stats[c].sq_sum);
                MPI_Recv(&regionCities[myLastRegion].grades[cityIndex*students], students, GradeIndex_dtype, MPI_ANY_SOURCE, c + total_cities, MPI_COMM_WORLD, &status);
                printf("[%d] recebi grades da city %d!\n", myrank, c);
            }
            printf("[%d] recebi tudo!\n", myrank);
            for (int r = 0; r < numberOfMainCityIHave; ++r)
            {
                printf("[%d] r=%4d grades:\n", myrank, r);
                for (int c = 0; c < cities; ++c)
                {
                    for (int s = 0; s < students; ++s)
                    {
                        printf("r%3d c%3d s%3d", r, c, s);
                        printf("  (%3d)\n", regionCities[r].grades[r * students_per_region + c * students + s].grade);
                    }
                    printf("\n");
                }
            }
        }
        
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
    /*
        free(cit_avg);
        free(cit_dev);
        free(cit_med);
        free(cit_max);
        free(cit_min);
    */
        free(cit_sum);
        //free(cit_sq_sum);
        if(numberOfMainCityIHave > 0)
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
    
    if(myrank == 0) {
        free(cities_per_process);
    }
    
    free(requests);
    
    MPI_Finalize();
    
    printf("\n[%d] the end! [%d]\n", myrank, myrank);
    
    return 0;
}


