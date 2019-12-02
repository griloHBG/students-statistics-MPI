// para compilar: mpicc students.c -o students -Wall
// para rodar: mpirun --hostfile hostfile.txt hello

//compilar com EXAMPLE definido: ignora-se a entrada e utiliza-se o exemplo do PDF do trabalho
//compilar com INCREMENTAL definido: valores das notas são incrementais com máximo em 100 e mínimo em 0. após 100, retorna a 0 e continua incrementando.
//compilar com DECREMENTAL definido: valores das notas são decrementais com máximo em 100 e mínimo em 0. após 0, retorna a 100 e continua decrementando.

//regexp para inicialização e declaração (ou não) de variáveis
///^(\s+)?[A-Za-z_\d]+\s\*?[A-Za-z_\d]+(\[\d+\])?+(\s+?=.+)?;$

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
    unsigned int sq_sum;

} Stats;
//Quicksort adaptado de //https://www.geeksforgeeks.org/quick-sort/
int partition (int* arr, int low, int high, int C, int* sum, unsigned int* sq_sum)
{
    int i, j;
    int pivot,swap;

    // pivot (Element to be placed at right position)
    pivot = arr[high*C];

    i = (low - 1);  // Index of smaller element

    //aproveitando a ordenação para já calcular a média
    int calculate_sum = (sum != NULL);
    int calculate_sq_sum = (sq_sum != NULL);


    for (j = low; j <= high-1; j++) {

        if (calculate_sum)
            *sum += arr[j * C];
        if (calculate_sq_sum)
            *sq_sum += arr[j * C] * arr[j * C];

        // If current element is smaller than or
        // equal to pivot
        if (arr[j * C] <= pivot) {
            i++;    // increment index of smaller element

            // swap arr[i] and arr[j]
            swap = arr[i * C];
            arr[i * C] = arr[j * C];
            arr[j * C] = swap;
        }
    }

    if(calculate_sum)
        *sum += arr[j * C];
    if(calculate_sq_sum)
        *sq_sum += arr[j * C] * arr[j * C];

    //swap arr[i + 1] and arr[high]
    swap = arr[(i + 1)*C];
    arr[(i + 1)*C] = arr[high*C];
    arr[high*C] = swap;

    return (i + 1);

} // fim partition


/* low  --> Starting index,  high  --> Ending index */
void quicksort(int* arr, int low, int high, int C, int* sum, unsigned int* sq_sum)
{
    int pi;

    if (low < high)  {
        /* pi is partitioning index, arr[pi] is now
           at right place */
        pi = partition(arr, low, high, C, sum, sq_sum);

        quicksort(arr, low, pi - 1, C, NULL, NULL);  // Before pi
        quicksort(arr, pi + 1, high, C, NULL, NULL);  // Before pi// After pi
    }

} // fim quicksort

/* This function takes last element as pivot, places
   the pivot element at its correct position in sorted
    array, and places all smaller (smaller than pivot)
   to left of pivot and all greater elements to right
   of pivot
   https://www.geeksforgeeks.org/quick-sort/
*/

void quicksortArray(int* array, int length, int* sum, unsigned int* sq_sum)
{
    //manda o endereco do primeiro elemento da coluna, limites inf e sup e a largura da array
    quicksort(array, 0, length - 1, 1, sum, sq_sum);
}

void calculateMedian(int* array, int length, float* med)
{
    if(length % 2 == 0)
    {
        *med = 0.5 * (array[length / 2] + array[length / 2 - 1]);
    }
    else
    {
        *med = array[length / 2];
    }
}

void calculateStddev(int length, int sum, unsigned int sq_sum, float avg,  float* dev)
{
    *dev = sqrt((sq_sum - 2*avg*sum + length*avg*avg)/(1.0 * (length-1)));
#ifdef VERBOSE
    printf("sqrt((%d - 2*%f*%d + %d*%f^2)/(1.0 * (%d-1)))\n", sq_sum, avg, sum, length, avg, length);
#endif
}



//versão MPI_DataType do tipo de dado Stats (logo acima) para envio via MPI
MPI_Datatype Stats_dtype;

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
int *citiesPerProcess;

//guarda a quantidade de regioões que são de responsabilidade deste processo
//tamanho: depende da quantidade de processos e da dimensão do problema
//int numberOfMainCityIHave = 0;

//os índices das regiões que são de responsabilidade deste processo. no caso do "master", guardará a quantidade de regiões pelas quais ele é responsável no primeiro estágio do processo
//tamanho: 0 <= regionsIOwn <= regions
int *regionsIOwn;
int regionsIOwnSize;

//válido apenas no processo 0. este array guarda a quantidade de regiões pelas quais cada processo é responsável
//tamanho: quantidade de processos
int *numberOfRegionsEachProcOwns;

//quantidade de cidades que pertencem a processos de índice inferior que o índice deste processo
//valor: somatório (p= 0:myrank-1) em citiesPerProcess[p]
int citiesBeforeMe = 0;

//guarda as estatísticas das cidades deste processo
//tamanho: citiesPerProcess[myRank]
Stats *cityStats;
int cityStatsSize;

//guarda as estatísticas das regiões que pertencem a este processo
//tamanho: comprimento do regionsIOwn
Stats *regionStats;
int regionStatsSize;

//guarda as estatísticas do país
//tamanho: 1
Stats countryStats;

//o nome é auto explicativo: guarda os índice da cidade de melhor média dentro de cada região
//tamanho: regionsIOwnSize
int* bestCityInRegion;

int main(int argc, char *argv[]) {

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
#ifdef EXAMPLE
    regions     = example_regions;
    cities      = example_cities;
    students    = example_students;
    seed        = example_seed; //seed para valores aleatórios
#endif
#ifdef VERBOSE

    printf("-----------------------------------\nreg = %d\tcit = %d\tstu = %d\tseed = %d\n-----------------------------------\n", regions, cities, students, seed);
#endif

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
    int myRank;
    int npes;

    //começa a brincadeira
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    MPI_Comm_size(MPI_COMM_WORLD, &npes);

    //quantidade máxima de processos, dado que cidades não serão subdivididas em processos
    int maximumProcesses = cities * regions;

    //se houver mais processos que cidades, os sobressalentes retornarão
    int iCanRest = (myRank >= maximumProcesses);

    if(iCanRest) {
        //se houver mais nós que o necessário, deixá-los disponíveis (econimizar energia tbm :p).
        MPI_Finalize();
        exit(0);
    }

    //guarda os status das comunicações
    MPI_Status status;

    //usado para avaliar se envios não bloqueantes foram realizados
    //tamanho: max(npes-1,5)
    MPI_Request *requests;
    /*
    typedef struct stats_t
    {
        int min;
        int max;
        float med;
        float avg;
        float dev;
        unsigned int sq_sum;
    
    } Stats;
    */
    //tipo de dado para enviar
    MPI_Datatype type[3]    = { MPI_INT , MPI_FLOAT     , MPI_UNSIGNED                      };
    int blocklen[3]         = { 2       , 3             , 1                                 };
    MPI_Aint disp[3]        = { 0       , 2* sizeof(int), 2*sizeof(int) + 3*sizeof(float)   };
    MPI_Type_create_struct(3, blocklen, disp, type, &Stats_dtype);
    MPI_Type_commit(&Stats_dtype);

    //guarda quantidade de cidades que serão igualmente distribuidas
    int dataEqualDistributed;

    //guarda quantidade de cidades sobressalentes em uma divisão igualitária
    int dataRemaining;

    //indica se o processo em questão receberá mais cidades do que em uma divisão igualitária
    int moreData = 0;

    dataEqualDistributed = totalCities / npes;
    dataRemaining = totalCities % npes;

    citiesPerProcess = (int *) calloc(npes, sizeof(int));

    numberOfRegionsEachProcOwns = (int*) calloc(npes, sizeof(int));
    
    int cityResidue = 0;
    
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
#ifdef VERBOSE
        printf("cmsum %d\n", citiesBeforeMe);
#endif

        //cada processo calcula a quantidade de cidades que precisa enviar para o processo anterior ou receber do processo posterior
        if(p == myRank)
        {
            citiesInThisNode = citiesPerProcess[p];
            numberOfCitiesToSend = citiesBeforeMe % cities == 0 ? 0 : cities - citiesBeforeMe % cities;
            numberOfCitiesToReceive = (citiesBeforeMe + citiesInThisNode) % cities == 0 ? 0 : cities - (citiesBeforeMe + citiesInThisNode) % cities;
        }
        //todo está dando igual para todos os processos
        //cityResidue guarda quantas cidades do última regiao de cada processo estão no próximo processo
        cityResidue = (citiesPerProcess[p] + cityResidue) - (((citiesPerProcess[p] + cityResidue) / cities) + (((citiesPerProcess[p] + cityResidue) % cities) > 0)) * cities;

        numberOfRegionsEachProcOwns[p] = (citiesPerProcess[p] - cityResidue) / cities;
#ifdef VERBOSE
        printf("[%d] numberOfRegionsEachProcOwns[%d] = %d\n",myRank, p, numberOfRegionsEachProcOwns[p]);
        printf("[%d] cityResidue[%d] = %d\n",myRank, p, cityResidue);
#endif
    }

    //~le printada
#ifdef VERBOSE
    printf("[%d] numberOfCitiesToSend = %d , numberOfCitiesToReceive = %d\n", myRank, numberOfCitiesToSend, numberOfCitiesToReceive);
#endif

    if(myRank == 0)
    {
        //aloca o vetor que guarda os índices das regiões de responsabilidade deste processo
        regionsIOwnSize = (citiesInThisNode-numberOfCitiesToSend)/cities + ((citiesInThisNode-numberOfCitiesToSend) % cities > 0);
        regionsIOwn = (int*)calloc(regionsIOwnSize, sizeof(int));

        //aloca o vetor que guarda as estatísticas das regiões de responsabilidade deste processo
        regionStatsSize = regions;
        regionStats = (Stats*) calloc(regionStatsSize, sizeof(Stats));

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
        regionStatsSize = (citiesInThisNode - numberOfCitiesToSend) / cities + 1;
        regionStats = (Stats*) calloc(regionStatsSize, sizeof(Stats));

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
#ifdef VERBOSE
        printf("[%d] regionsIOwn[%d] = %d!\n", myRank, (c - citiesBeforeMe + numberOfCitiesToSend) / cities, regionsIOwn[(c - citiesBeforeMe + numberOfCitiesToSend) / cities]);
#endif
    }
#ifdef VERBOSE

    printf("[%d] number of: cit_stats %3d reg_stats %3d regionsIOwn %3d\n", myRank, cityStatsSize, regionStatsSize, regionsIOwnSize);

    //a imprimida na tela
    printf("[%d] citiesInThisNode = %d\n", myRank, citiesInThisNode);
#endif

    //aloca espaço para TODAS AS NOTAS QUE ESTE PROCESSO USARÁ EM SUA VIDA
    myGrades = (int*)calloc(myRank == 0 ?
                                    totalStudents :
                                    (citiesInThisNode + numberOfCitiesToReceive)*students,
                            sizeof(int));
#ifdef VERBOSE
    //printadela
    printf("[%d] number of grades %d * %d\n", myRank,
           myRank == 0 ?
                   totalStudents :
                   (citiesInThisNode + numberOfCitiesToReceive),
           students);
#endif
    //cria as notas
    if(myRank == 0)
    {
        for (int s = 0; s < totalStudents; ++s)
        {
#ifdef EXAMPLE
            myGrades[s] = example_matrix[s];
#else
    #ifdef DECREMENTAL
                    myGrades[s] = (totalStudents - s) % 101;
    #else
        #ifdef INCREMENTAL
                    myGrades[s] = s % 101;
        #else
                    myGrades[s] =  100 * (rand() / (1.0 * RAND_MAX));
        #endif
    #endif
#endif

#ifdef VERBOSE
            if(s % students == 0)
            {
                printf("\n");
            }
            if(s % studentsPerRegion == 0)
            {
                printf("\n");
            }
            printf("(%3d)", myGrades[s]);
#endif
        }
    }
#ifdef VERBOSE
    printf("\n");
    printf("\n");
#endif

    //cria uma quantidade ae qualquer de requests de MPI
    requests = (MPI_Request*) calloc(MAX(npes-1, 6), sizeof(MPI_Request));

    double time;

    //enviando pra galera fazer os cáculos do nível de cidades
    if(myRank == 0)
    {
        time = MPI_Wtime();

        //esse cara só serve para dar o "offset" entre um envio e outro
        int cummulativeSum = citiesPerProcess[0];

        for (int p = 1; p < npes; ++p)
        {
#ifdef VERBOSE
            printf("[%d] enviando para %d (cities %d, students %d, cmsum %d):\n", myRank, p, citiesPerProcess[p], students, cummulativeSum);
#endif
            //enviando meeeemo
            MPI_Isend(&myGrades[cummulativeSum * students], citiesPerProcess[p] * students, MPI_INT, p, 0, MPI_COMM_WORLD, &requests[p]);
            cummulativeSum += citiesPerProcess[p];
        }
        for (int p = 1; p < npes; ++p)
        {
            //um wait para os malotes enviados para cada processo
            MPI_Wait(&requests[p], &status);
        }
    }
    else
    {
#ifdef VERBOSE
        printf("[%d] esperando do master:\n", myRank);
#endif
        //recebendo meeeemo
        MPI_Recv(myGrades, citiesInThisNode * students, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

#ifdef VERBOSE
        printf("[%d] recebido do master!\n", myRank);
        printf("[%d] myGrades:\n", myRank);
        for (int s = 0; s < citiesInThisNode * students; ++s)
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
        }
#endif
    }
    
    /*if(myRank == 0)
    {
        //tirando os valores do master que não sao do master (para testes)
        for (int s = citiesPerProcess[0] * students; s < totalCities * students; ++s)
        {
            myGrades[s] = -1;
        }
    }*/

#ifdef VERBOSE
    if(myRank == 0)
    for (int s = 0; s < totalStudents; ++s)
    {
        printf("(%3d)", myGrades[s]);
    }
#endif

    /////////////////////////////////
    //TODO fazer contas nas cidades//
    /////////////////////////////////

    int sum;
#ifdef VERBOSE
    printf("calculando as %d cidades\n", citiesInThisNode);
#endif
    //para cada cidade que é deste processo, no nível das cidades, calcular as estatísticas
    for (int c = 0; c < citiesInThisNode; ++c)
    {
        sum = 0;
        cityStats[c].sq_sum = 0;
        //aproveito o quicksort para calcular o somatório e o somatório das notas ao quadrado
        quicksortArray(&myGrades[c * students], students, &sum, &cityStats[c].sq_sum);
        cityStats[c].min = myGrades[c * students];
        cityStats[c].max = myGrades[c * students + (students - 1)];
        calculateMedian(&myGrades[c * students], students, &cityStats[c].med);
        cityStats[c].avg = sum / (1.0*students);
        calculateStddev(students, sum, cityStats[c].sq_sum, cityStats[c].avg, &cityStats[c].dev);

#ifdef VERBOSE
        printf("[%d] (%3d) stats: min %d max %d med %5.2f avg %5.2f dev %5.2f sq_sum %u\n", myRank, c,
               cityStats[c].min, cityStats[c].max, cityStats[c].med, cityStats[c].avg, cityStats[c].dev, cityStats[c].sq_sum);
#endif
    }

#ifdef VERBOSE
    printf("\n\ntudo calculado!\n", citiesInThisNode);

    printf("\n[%d] contas das cidades feitas!\n\n", myRank);

    for (int s = 0; s < citiesInThisNode * students; ++s)
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
    }

    printf("[%d] enviando %d cidades para o proc [%d]!\n", myRank, numberOfCitiesToSend,myRank - 1);
#endif

    //fazendo envio para entrar no estágio de cáculos nas regiões. apenas os processos que precisam enviar cidades para completar região de outro processo envia
    if(numberOfCitiesToSend > 0)
    {
        MPI_Isend(&myGrades[0], numberOfCitiesToSend * students, MPI_INT, myRank - 1, 'g', MPI_COMM_WORLD, &requests[0]);

        MPI_Isend(&cityStats[0], numberOfCitiesToSend, Stats_dtype, myRank - 1, 's', MPI_COMM_WORLD, &requests[1]);
    }

    if(!iCanRest) {
#ifdef VERBOSE
        printf("[%d] recebendo %d cidades do proc [%d]!\n", myRank, numberOfCitiesToReceive, myRank + 1);
        printf("[%d] cidade que vai receber a galera: %d!\n", myRank, (citiesBeforeMe + citiesInThisNode));
#endif

        //fazendo recebimento para entrar no estágio de cáculos nas regiões
        if (numberOfCitiesToReceive > 0)
        {
            MPI_Recv(&myGrades[citiesInThisNode * students], numberOfCitiesToReceive * students, MPI_INT, myRank + 1,
                     'g', MPI_COMM_WORLD, &status);

            MPI_Recv(&cityStats[citiesInThisNode], numberOfCitiesToReceive, Stats_dtype, myRank + 1, 's',
                     MPI_COMM_WORLD, &status);
        }

        for (int c = numberOfCitiesToSend; c < citiesInThisNode + numberOfCitiesToReceive; ++c)
        {
#ifdef VERBOSE
            printf("[%d] (%3d) stats: min %d max %d med %5.2f avg %5.2f dev %5.2f sq_sum %u\n", myRank, c,
                   cityStats[c].min, cityStats[c].max, cityStats[c].med, cityStats[c].avg, cityStats[c].dev,
                   cityStats[c].sq_sum);
#endif
        }
#ifdef VERBOSE

        printf("[%d] regionsIOwn:%d\n", myRank, regionsIOwnSize);
        printf("[%d] quantidade de estudantes neste nível:%d x %d x %d\n", myRank, regionsIOwnSize, cities, students);

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
#endif

        ///////////////////////////////////
        ///TODO fazer contas nas regiões///
        ///////////////////////////////////
        
        if(myRank == 0)
        {
            bestCityInRegion = (int*)calloc(regions, sizeof(int));
        }
        else
        {
            bestCityInRegion = (int*)calloc(regionsIOwnSize, sizeof(int));
        }
        
        for (int r = 0; r < regionsIOwnSize; ++r)
        {
            bestCityInRegion[r] = numberOfCitiesToSend;
            //todo validar as estatisticas
            regionStats[r].avg = 0;
            regionStats[r].sq_sum = 0;
            quicksortArray(&myGrades[numberOfCitiesToSend * students], regionsIOwnSize * studentsPerRegion, NULL, NULL );
            
            for (int c = numberOfCitiesToSend + r * cities; c < numberOfCitiesToSend + (r+1) * cities; ++c)
            {
                regionStats[r].avg += cityStats[c].avg;
                regionStats[r].sq_sum += cityStats[c].sq_sum;
                bestCityInRegion[r] = cityStats[c].avg > cityStats[bestCityInRegion[r]].avg ? c : bestCityInRegion[r];
            }
    
            bestCityInRegion[r] += citiesBeforeMe;
#ifdef VERBOSE
            printf("melhor cidade da regiao %d: %d\n", r, bestCityInRegion[r]);
#endif
            
            regionStats[r].min = myGrades[numberOfCitiesToSend * students];
            regionStats[r].max = myGrades[numberOfCitiesToSend * students + regionsIOwnSize * studentsPerRegion - 1];
            calculateMedian(&myGrades[numberOfCitiesToSend * students], regionsIOwnSize * studentsPerRegion, &regionStats[r].med);
            regionStats[r].avg /= 1.0 * cities;
            calculateStddev(studentsPerRegion, regionStats[r].avg * studentsPerRegion, regionStats[r].sq_sum, regionStats[r].avg, &regionStats[r].dev);
        }
#ifdef VERBOSE
        printf("\n[%d] contas das regiões feitas!\n\n", myRank);

        if(myRank == 0)
        {

            printf("\n\n\t\tANTES de receber\n\n");

            for (int s = 0; s < totalStudents; ++s)
            {
                if (s % students == 0)
                {
                    printf("\n");
                }
                if (s % studentsPerRegion == 0)
                {
                    printf("-----------------------\n");
                }

                printf("(%4d)", myGrades[s]);
            }
        }

        printf("\n\n");
#endif
        
        if(numberOfCitiesToSend > 0)
        {
            MPI_Wait(&requests[0], &status);
            MPI_Wait(&requests[1], &status);
        }
        
        if (myRank == 0)
        {
            int cummulativeRegionSum = regionsIOwnSize;

            for (int p = 1; p < npes; ++p)
            {
#ifdef VERBOSE
                printf("[%d] MASTER REC do proc %d: %d regs, %d cits, %d stus | comeca cit %d e stu %d\n",
                       myRank, p, numberOfRegionsEachProcOwns[p], numberOfRegionsEachProcOwns[p] * cities,
                       numberOfRegionsEachProcOwns[p] * studentsPerRegion, (cummulativeRegionSum) * cities,
                       cummulativeRegionSum * studentsPerRegion);
#endif
                
                MPI_Recv(&myGrades[cummulativeRegionSum * studentsPerRegion],
                         numberOfRegionsEachProcOwns[p] * studentsPerRegion, MPI_INT, p,
                         cummulativeRegionSum * studentsPerRegion, MPI_COMM_WORLD, &status);
                MPI_Recv(&cityStats[cummulativeRegionSum * cities], numberOfRegionsEachProcOwns[p] * cities,
                         Stats_dtype, p, cummulativeRegionSum * cities + totalCities, MPI_COMM_WORLD, &status);
                MPI_Recv(&regionStats[cummulativeRegionSum], numberOfRegionsEachProcOwns[p], Stats_dtype, p,
                         cummulativeRegionSum + regions, MPI_COMM_WORLD, &status);
                
                MPI_Recv(&bestCityInRegion[cummulativeRegionSum], numberOfRegionsEachProcOwns[p], MPI_INT, p, totalStudents + p, MPI_COMM_WORLD, &status);
#ifdef VERBOSE
                printf("\n\nrecebi bestCityInRegion : %d\n\n", bestCityInRegion[cummulativeRegionSum]);
#endif
                cummulativeRegionSum += numberOfRegionsEachProcOwns[p];
            }
        }
        else
        {
#ifdef VERBOSE
            printf("[%d] citiesBeforeMe = %d numberOfCitiesToSend = %d 1o grade pra enviar = %d", myRank, citiesBeforeMe, numberOfCitiesToSend, myGrades[numberOfCitiesToSend * students]);
#endif
            MPI_Isend(&myGrades[numberOfCitiesToSend * students], regionsIOwnSize * studentsPerRegion, MPI_INT, 0,
                      (citiesBeforeMe + numberOfCitiesToSend) * students, MPI_COMM_WORLD, &requests[2]);
            MPI_Isend(&cityStats[numberOfCitiesToSend], regionsIOwnSize * cities, Stats_dtype, 0,
                      (citiesBeforeMe + numberOfCitiesToSend) + totalCities, MPI_COMM_WORLD, &requests[3]);
            MPI_Isend(regionStats, regionsIOwnSize, Stats_dtype, 0,
                      (citiesBeforeMe + numberOfCitiesToSend) / cities + regions, MPI_COMM_WORLD, &requests[4]);
            
            MPI_Isend(bestCityInRegion, regionsIOwnSize, MPI_INT, 0, totalStudents + myRank, MPI_COMM_WORLD, &requests[5]);
            
            MPI_Wait(&requests[2], &status);
            MPI_Wait(&requests[3], &status);
            MPI_Wait(&requests[4], &status);
            MPI_Wait(&requests[5], &status);
        }

#ifdef VERBOSE
        if(myRank == 0)
        {

            printf("\n\n\t\tDEPOIS de receber\n\n");

            for (int s = 0; s < totalStudents; ++s)
            {
                if (s % students == 0)
                {
                    printf("\n");
                }
                if (s % studentsPerRegion == 0)
                {
                    printf("-----------------------\n");
                }

                printf("(%4d)", myGrades[s]);
            }

            for (int r = 0; r < regions; ++r)
            {
                printf("[%d] (%3d) REGION stats: min %d max %d med %5.2f avg %5.2f dev %5.2f sq_sum %u\n", myRank, r,
                       regionStats[r].min, regionStats[r].max, regionStats[r].med, regionStats[r].avg, regionStats[r].dev, regionStats[r].sq_sum);
            }

            printf("\n");

            for (int c = 0; c < totalCities; ++c)
            {
                printf("[%d] (%3d) CITY stats: min %d max %d med %5.2f avg %5.2f dev %5.2f sq_sum %u\n", myRank, c,
                       cityStats[c].min, cityStats[c].max, cityStats[c].med, cityStats[c].avg, cityStats[c].dev, cityStats[c].sq_sum);
            }
        }

        printf("\n\n");

        printf("[%d] liberando as memórias!\n", myRank);
#endif
    }
    if(!iCanRest){}
    ///////////////////////////////////
    ///  TODO fazer contas no país  ///
    ///////////////////////////////////

    int bestCityInCountry = 0;
    int bestRegionInCountry = 0;
    
    if(myRank == 0)
    {
        countryStats.avg = 0;
        countryStats.sq_sum = 0;
        quicksortArray(myGrades, totalStudents, NULL, NULL);

        for (int r = 0; r < regions; ++r)
        {
            countryStats.avg += regionStats[r].avg;
            countryStats.sq_sum += regionStats[r].sq_sum;
            bestCityInCountry = cityStats[bestCityInRegion[r]].avg > cityStats[bestCityInCountry].avg ? bestCityInRegion[r] : bestCityInCountry;
            bestRegionInCountry = regionStats[r].avg > regionStats[bestRegionInCountry].avg ? r : bestRegionInCountry;
        }

        countryStats.min = myGrades[0];
        countryStats.max = myGrades[totalStudents - 1];
        calculateMedian(myGrades, totalStudents, &countryStats.med);
        countryStats.avg /= 1.0 * regions;
        calculateStddev(totalStudents, countryStats.avg * totalStudents, countryStats.sq_sum, countryStats.avg,
                        &countryStats.dev);

#ifdef VERBOSE
        printf("\n\ncountryStats.sq_sum = %d\n\n", countryStats.sq_sum);
#endif
    }

#ifdef VERBOSE
    printf("\n\nimprimindo tudo!\n\n");
#endif

    if(myRank == 0)
    {
        time = MPI_Wtime() - time;
        for (int c = 0; c < totalCities; ++c)
        {
            if (c % cities == 0 && c > 0)
            {
                printf("\n");
            }
            printf("Reg %d - Cid %d: menor: %d, maior: %d, mediana: %.2f, média: %.2f e DP: %.2f\n", c / cities,
                   c % cities,
                   cityStats[c].min, cityStats[c].max, cityStats[c].med, cityStats[c].avg, cityStats[c].dev);
        }

        printf("\n");

        for (int r = 0; r < regions; ++r)
        {
            printf("Reg %d: menor: %d, maior: %d, mediana: %.2f, média: %.2f e DP: %.2f\n", r,
                   regionStats[r].min, regionStats[r].max, regionStats[r].med, regionStats[r].avg, regionStats[r].dev);
        }

        printf("\n");

        printf("Brasil: menor: %d, maior: %d, mediana: %.2f, média: %.2f e DP: %.2f\n",
               countryStats.min, countryStats.max, countryStats.med, countryStats.avg, countryStats.dev);

        printf("\n");

        printf("Melhor região: Região %d\n", bestRegionInCountry);
        printf("Melhor cidade: Região %d, Cidade %d\n",bestCityInCountry / cities  ,bestCityInCountry % cities);

        printf("\n");

        printf("Tempo de resposta sem considerar E/S, em segundos: %.3lfs\n", time);
    }

    free(citiesPerProcess);
    free(numberOfRegionsEachProcOwns);
    free(regionsIOwn);
    free(regionStats);
    free(cityStats);
    free(myGrades);
    free(requests);
    
    if(regionsIOwnSize > 0)
    {
        free(bestCityInRegion);
    }

    //fechando o rolê
    MPI_Finalize();

#ifdef VERBOSE
    printf("\n[%d] the end! [%d]\n", myRank, myRank);
#endif

    return 0;
}
