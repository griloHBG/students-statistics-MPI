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

typedef struct stats_t
{
    int min;
    int max;
    float med;
    float avg;
    float dev;
    long int sq_sum;
    
} Stats;

int myself = -1;

//todas as notas que este processo carregará na sua vida
//tamanho: cidades que vou receber * estudantes por cidade
int *myGrades;

//guarda a quantidade de cidades que este processo guardará durante o estágio da computação das cidades (pois a divisão é potencialmente desigual)
int citiesInThisNode;

//guarda quantas das cidades deste processo serão enviadas para o processo de índice imediatamente inferior
//valor: 0 <= numberOfCitiesToSend < cities
int numberOfCitiesToSend;

//guarda quantas das cidades que este processo receberá do processo de índice imediatamente superior
//valor: 0 <= numberOfCitiesToSend < cities
int numberOfCitiesToReceive;

//todos os processos sabem quantas cidades cada processo recebe
//tamanho: quantidade de processos
int* citiesPerProcess;

//guarda a quantidade de regioões que são de responsabilidade deste processo
//tamanho: depende da quantidade de processos e da dimensão do problema
int numberOfMainCityIHave = 0;

//os índices das regiões que são de responsabilidade deste processo. no caso do "master", guardará a quantidade de regiões pelas quais ele é responsável no primeiro estágio do processo
//tamanho: 0 <= regionsIOwn <= regions
int* regionsIOwn;
int regionsIOwnSize;

//quantidade de cidades que pertencem a processos de índice inferior que o índice deste processo
//valor: somatório (p= 0:myrank-1) em citiesPerProcess[p]
int citiesBeforeMe = 0;

//guarda as estatísticas das cidades deste processo
//tamanho: citiesPerProcess[myRank]
Stats* cityStats;
int cityStatsSize;

//guarda as estatísticas das regiões que pertencem a este processo
//tamanho: comprimento do regionsIOwn
Stats* regionStats;
int regionsStatsSize;

//guarda as estatísticas do país
//tamanho: 1
Stats country;

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

    //guardando e convertendo argumentos de linha de comando
    int regions     = atoi(argv[1]);
    int cities      = atoi(argv[2]);
    int students    = atoi(argv[3]);
    int seed        = atoi(argv[4]); //seed para valores aleatórios
    
    printf("-----------------------------------\nreg = %d\tcit = %d\tstu = %d\tseed = %d\n-----------------------------------\n", regions, cities, students, seed);
    
    srand(seed);
    
    //quantidade de  cidades no país
    int totalCities        = regions * cities;
    //quantidade de estudantes por região
    int studentsPerRegion = cities * students;
    //quantidade de estudantes no país
    int totalStudents      = regions * cities * students;
    
    //conversão de índice global para índice da região da nota
#define IDX2REG(I) I / students_per_region
    //conversão de global geral para índice da cidade da nota
#define IDX2CIT(I) I / students
    //conversão de indice global de cidade para índice da região da nota
#define CIT2REG(I) I / cities
    //conversão de índice da região, cidade e estudante da nota para índice global
#define RCS2IDX(R,C,S) R*students_per_region+C*students+S
    
    //meu índice de processo e quantos processos tem na brincadeira
    int myRank, npes;
    
    //começa a brincadeira
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    MPI_Comm_size(MPI_COMM_WORLD, &npes);
    
    //quantidade máxima de processos, dado que cidades não serão subdivididas em processos
    int maximumProcesses = cities * regions;

    //se houver mais processos que cidades, os sobressalentes retornarão
    int I_can_rest = (myRank >= maximumProcesses);

    if(I_can_rest) {
        //se houver mais nós que o necessário, deixá-los disponíveis (econimizar energia tbm :p).
        MPI_Finalize();
        exit(0);
    }
    
    //guarda os status das comunicações
    MPI_Status status;
    
    //usado para avaliar se envios não bloqueantes foram realizados
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
    //tipo de dado para enviar
    MPI_Datatype CityGradesHeader_dtype;
    MPI_Datatype type[4]    = { MPI_INT, 0, 0, 0};
    int blocklen[4]         = { 4, 0, 0, 0};
    MPI_Aint disp[4]        = { 0, 0, 0, 0};
    MPI_Type_create_struct(1, blocklen, disp, type, &CityGradesHeader_dtype);
    MPI_Type_commit(&CityGradesHeader_dtype);
    
    //guarda quantidade de cidades que serão igualmente distribuidas
    int dataEqualDistributed;
    
    //guarda quantidade de cidades sobressalentes em uma divisão igualitária
    int dataRemaining;
    
    //indica se o processo em questão receberá mais cidades do que em uma divisão igualitária
    int moreData = 0;
    
    dataEqualDistributed = totalCities / npes;
    dataRemaining = totalCities % npes;
    
    citiesPerProcess = (int *) calloc(npes, sizeof(int));
    
    //definindo valores interessantes para cada processo. todos os processos têm esses valores
    for (int p = 0; p < npes; ++p) {
        //se for nó inicial (com índices menores), receberá mais dados que os outros nós
        moreData = (p <= (dataRemaining - 1));
        
        //calculando qunatas cidades cada nó terá, mesmo para uma divisão desigual
        citiesPerProcess[p] = dataEqualDistributed + moreData;
        
        
        
        //realiza a soma acumulada de todas as cidades anteriores a este processo para facilitar o cálculo do índice das regiões de sua responsabilidade
        if(p < myRank)
        {
            citiesBeforeMe += citiesPerProcess[p];
        }
        
        //aquela printada de levs
        printf("cmsum %d\n", citiesBeforeMe);
        
        //cada processo calcula a quantidade de cidades que precisa enviar para o processo anterior ou receber do processo posterior
        if(p == myRank)
        {
            citiesInThisNode = citiesPerProcess[p];
            numberOfCitiesToSend = citiesBeforeMe % cities == 0 ? 0 : cities - citiesBeforeMe % cities;
            numberOfCitiesToReceive = (citiesBeforeMe + citiesInThisNode) % cities == 0 ? 0 : cities - (citiesBeforeMe + citiesInThisNode) % cities;
        }
    }
    
    //~le printada
    printf("[%d] numberOfCitiesToSend = %d , numberOfCitiesToReceive = %d\n", myRank, numberOfCitiesToSend, numberOfCitiesToReceive);
    
    if(myRank == 0)
    {
        //aloca o vetor que guarda os índices das regiões de responsabilidade deste processo
        regionsIOwnSize = (citiesInThisNode-numberOfCitiesToSend)/cities + ((citiesInThisNode-numberOfCitiesToSend) % cities > 0);
        regionsIOwn = (int*)calloc(regionsIOwnSize, sizeof(int));
    
        //aloca o vetor que guarda as estatísticas das regiões de responsabilidade deste processo
        regionsStatsSize = regions;
        regionStats = (Stats*) calloc(regionsStatsSize, sizeof(Stats));

        //aloca o vetor que guarda as estatísticas das regiões de responsabilidade deste processo
        cityStatsSize = totalCities;
        cityStats = (Stats*) calloc(cityStatsSize, sizeof(Stats));
    }
    else
    {
        //aloca o vetor que guarda os índices das regiões de responsabilidade deste processo
        regionsIOwnSize = (citiesInThisNode-numberOfCitiesToSend)/cities + ((citiesInThisNode-numberOfCitiesToSend) % cities > 0);
        regionsIOwn = (int*)calloc(regionsIOwnSize, sizeof(int));
    
        //aloca o vetor que guarda as estatísticas das regiões de responsabilidade deste processo
        regionsStatsSize = (citiesInThisNode-numberOfCitiesToSend)/cities + 1;
        regionStats = (Stats*) calloc(regionsStatsSize, sizeof(Stats));
    
        //aloca o vetor que guarda as estatísticas das regiões de responsabilidade deste processo
        cityStatsSize = citiesInThisNode+numberOfCitiesToReceive;
        cityStats = (Stats*) calloc(cityStatsSize, sizeof(Stats));
    }
    
    //itera da primeira cidade principal deste processo até a última cidade principal deste processo
    for (int c = citiesBeforeMe + numberOfCitiesToSend; c < citiesBeforeMe + citiesInThisNode; c = c + cities)
    {
        //calcula a região à qual a cidade principal pertence
        regionsIOwn[(c - citiesBeforeMe + numberOfCitiesToSend) / cities] = c / cities;
        //printadinha do sucesso
        printf("[%d] regionsIOwn[%d] = %d!\n", myRank, (c - citiesBeforeMe + numberOfCitiesToSend) / cities, regionsIOwn[(c - citiesBeforeMe + numberOfCitiesToSend) / cities]);
    }

    printf("[%d] number of: cit_stats %3d reg_stats %3d regionsIOwn %3d\n", myRank, cityStatsSize, regionsStatsSize, regionsIOwnSize);
    
    //só uma atalho. nem sei se é tão útil assim tê-lo
    citiesInThisNode = citiesInThisNode;
    
    //a imprimida na tela
    printf("[%d] citiesInThisNode = %d\n", myRank, citiesInThisNode);
    
    //aloca espaço para TODAS AS NOTAS QUE ESTE PROCESSO USARÁ EM SUA VIDA
    myGrades = (int*)calloc(myRank == 0 ?
                                     totalStudents :
                                     (citiesInThisNode + numberOfCitiesToReceive)*students,
                                     sizeof(int));
    //printadela
    printf("[%d] number of grades %d * %d\n", myRank,
                                         myRank == 0 ?
                                         totalStudents :
                                         (citiesInThisNode + numberOfCitiesToReceive),
                                         students);
    //cria as notas
    if(myRank == 0)
    {
        for (int s = 0; s < totalStudents; ++s)
        {
            myGrades[s] = s % 101;
            /*
            if(s % students == 0)
            {
                printf("\n");
            }
            if(s % studentsPerRegion == 0)
            {
                printf("\n");
            }
            printf("(%3d)", myGrades[s]);*/
        }
    }
    printf("\n");
    printf("\n");
    
    requests = (MPI_Request*) calloc(citiesInThisNode + numberOfCitiesToReceive, sizeof(MPI_Request));
    
    //enviando pra galera
    if(myRank == 0)
    {
        int cummulativeSum = citiesPerProcess[0];
    
        for (int p = 1; p < npes; ++p)
        {
            printf("[%d] enviando para %d (cities %d, students %d, cmsum %d):\n", myRank, p, citiesPerProcess[p], students, cummulativeSum);
            MPI_Isend(&myGrades[cummulativeSum * students], citiesPerProcess[p] * students, MPI_INT, p, 0, MPI_COMM_WORLD, &requests[p]);
            cummulativeSum += citiesPerProcess[p];
        }
    }
    else
    {
        printf("[%d] esperando do master:\n", myRank);
        
        MPI_Recv(myGrades, citiesInThisNode * students, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        printf("[%d] recebido do master!\n", myRank);
        //printf("[%d] myGrades:\n", myRank);
        /*for (int s = 0; s < citiesInThisNode * students; ++s)
        {
            if(s % students == 0)
            {
                printf("\n");
            }
            printf("(%4d)", myGrades[s]);
            if(s % studentsPerRegion == 0)
            {
                printf("\n");
            }
        }*/
    }

    if(myRank == 0)
    {
        //tirando os valores do master que não sao do master (para testes)
        for (int s = citiesPerProcess[0] * students; s < totalCities * students; ++s)
        {
            myGrades[s] = -1;
        }
    }

    /*if(myRank == 0)
    for (int s = 0; s < totalStudents; ++s)
    {
        printf("(%3d)", myGrades[s]);
    }
    */
    /////////////////////////////////
    //TODO fazer contas nas cidades//
    /////////////////////////////////

    printf("\n[%d] contas das cidades feitas!\n\n", myRank);

    printf("[%d] enviando %d cidades para o proc [%d]!\n", myRank, numberOfCitiesToSend,myRank - 1);
    //fazendo envio para entrar no estágio de cáculos nas regiões
    if(numberOfCitiesToSend > 0)
    {
        MPI_Isend(&myGrades[0], numberOfCitiesToSend * students, MPI_INT, myRank - 1, 0, MPI_COMM_WORLD, &requests[0]);
    }

    printf("[%d] recebendo %d cidades do proc [%d]!\n", myRank, numberOfCitiesToReceive, myRank + 1);
    printf("[%d] cidade que vai receber a galera: %d!\n", myRank, (citiesBeforeMe + citiesInThisNode));

    //fazendo recebimento para entrar no estágio de cáculos nas regiões
    if(numberOfCitiesToReceive > 0)
    {
        MPI_Recv(&myGrades[citiesInThisNode * students], numberOfCitiesToReceive * students, MPI_INT, myRank + 1, 0, MPI_COMM_WORLD, &status);
    }

    if(myRank == 1)
    {
        myGrades[16*students - 1] = -50;
    }

    printf("[%d] regionsIOwn:%d\n", myRank, regionsIOwnSize);
    printf("[%d] students in this level:%d x %d x %d\n", myRank, regionsIOwnSize, cities, students);


    for (int s = numberOfCitiesToSend * students; s < (citiesInThisNode + numberOfCitiesToReceive) * students; ++s)
    {
        if((s - numberOfCitiesToSend * students) % students == 0)
        {
            printf("\n");
        }
        printf("(%4d)", myGrades[s]);
        if((s - numberOfCitiesToSend * students) % studentsPerRegion == 0)
        {
            printf("\n");
        }
    }

    /////////////////////////////////
    //TODO fazer contas nas regiões//
    /////////////////////////////////
    printf("\n[%d] contas das regiões feitas!\n\n", myRank);


    //fechando o rolê
    MPI_Finalize();
    
    printf("\n[%d] the end! [%d]\n", myRank, myRank);
    
    return 0;
}


