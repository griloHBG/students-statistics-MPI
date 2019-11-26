# Students Statistics

Objetivo: calcular métricas (estatísticas) nacionais, regionais e
municipais (3 níveis) a partir de notas (de 0 a 100) hipotéticas de
alunos do 9o ano do ensino fundamental.

As estatísticas são, **para cada nível**:

- Menor nota (***min***),
- Maior nota (***max***),
- Mediana (***med***),
- Média aritimética simples (***avg***) e,
- Desvio padrão (***dev***).

***Serão premiadas a cidade e a região com as maiores médias aritiméticas simples.***

## Formato de entrada

Matricial:

- Cada elemento: nota de um aluno (geradas pseudo-aleatoriamente, com ***seed*** proveniente do último argumento de entrada por linha de comando)
- Cada linha: uma cidade
- Cada parte de matriz: uma região

As notas dos alunos (inteiros de 0 a 100) devem ser geradas na seguinte ordem:

1. crescente por aluno
1. crescente por cidade
1. crescente por região

## Formato de saída

Deve ser exibida a menor (***min***) e a maior nota (***max***), a mediana (***med***), a média (***avg***) e o desvio padrão (***dev***) para cada cidade, cada região e, por fim, para o Brasil.

A numeração dos alunos, cidades e regiões serão iniciadas em zero, sendo que na saída, as estatísticas devem estar apresentadas pelos índices em ordem crescente.

Após, devem ser indicadas a região e a cidade que receberão os prêmios

## Compilando

### Paralelo

Para compilar o paralelo (`studentspar`), é necessário vincular com *math* (por causa do `sqrt()`) e compilar com OpenMP (por causa do `omp_get_wtime()`):

```
mpicc studentspar/studentspar -o studentspar -Wall -lm -fopenmp
``` 

### Sequencial

Para compilar o sequencial (`studentsseq`), é necessário vincular com *math* (por causa do `sqrt()`) e compilar com OpenMP (por causa do `omp_get_wtime()`):

```
gcc studentsseq/studentsseq -o studentsseq -Wall -lm -fopenmp
``` 

## Execução do programa

Para executar o programa paralelo `studentspar` (que está na pasta [studentspar](./studentspar)) :

```
mpirun -np <N> --hostfile <hostfilehostfilename> studentspar/studentspar <R> <C> <A> <SEED>
```

Onde:

- N é a quantidade de de execuções de `studentspar` que serão iniciadas
- hostfilename é o arquivo *hostfile* de mapeamento de carga de processamento
- R é a quantidade de regiões
- C é a quantidade de cidades por região
- A é a quantidade de alunos por cidade
- SEED é a semente utilizada na geração dos números aleatórios

Para executar o programa sequencial (que está na pasta [studentsseq](./studentsseq)) :

```
./studentsseq/studentsseq <R> <C> <A> <SEED>
```

Onde:

- R é a quantidade de regiões
- C é a quantidade de cidades por região
- A é a quantidade de alunos por cidade
- SEED é a semente utilizada na geração dos números aleatórios

## Desenvolvimento

Foi utilizado a metodologia **PCAM** para desenvolvimento (***particionamento***, ***comunicação***, ***aglomeração*** e ***mapeamento***), baseando-se em uma máquina MIMD de memória distribuida.

## Objetivo do desenvolvimento

Minimizar o tempo de resposta da aplicação, sendo que não será considerado o tempo de entrada de dados, alocação de memória, geração de números aleatórios nem impressão de resultados na tela. 

# Desenvolvimento dos estágios da metodologia PCAM

### Particionamento

**O particionamento será feito por DADOS.**

É necessário realizar o cálculo de **5** estatísticas para o país, **5\*R** estatísticas para as regiões (**R** regiões no país) e **5\*R\*C** estatísticas para as cidades (**C** cidades por região). Cada cidade contém **A** estudantes (**A** estudantes por cidade).

Para o Brasil, tomando a cidade de São Paulo como exemplo (que é a cidade que mais tem alunos no Ensino Fundamental) no ano de 2018, houveram 1.383.779 matrículas no Ensino Fundamental como um todo ([Fonte: Cidades@, portal do IBGE com dados sobre cidades brasileiras](https://cidades.ibge.gov.br/brasil/sp/sao-paulo/panorama)). Sendo que o objetivo deste trabalho limita-se às notas dos alunos do 9o ano, supondo-se que essas matrículas são igualmente distribuídas entre todos os anos do Ensino Fundamental, temos por volta de 153 mil estudantes no 9o ano.

Tendo esta quantidade de alunos em cada cidade como exemplo, é interessante que as notas dos alunos sejam divididas em alguns blocos, dentro de uma mesma cidade, assim cada tarefas operará, em um primeiro momento, em seu próprio bloco. Esta quantidade de tarefas **dentro de uma cidade** (cada uma com seu bloco de dados) será denominada quantidade **B** de tarefas.

Logo, a quantidade total de tarefas no problema será de **B\*C\*R**, sendo que dentre estas haverá as "tarefas principais" no nível dos blocos e, das cidades e das regiões seguindo a seguinte lógica:
- A tarefa principal entre os blocos calculará as estatísticas da respectiva cidade; 
- A tarefa principal entre as cidades calculará as estatísticas da respectiva região; e
- A tarefa principal entre as regiões calculará as estatísticas do país;

**Ex**: *Para um país com 3 (**R**) regiões, cada região com 5 (**C**) cidades e cada cidade com 12 (**A**) alunos, considerando um particionamento de 2 (**B**) blocos dentro de cada cidade, será um total de:*
 - *2 (=**B**) tarefas por cidade (1 tarefa principal dentre esses blocos), ou seja,*
 - *10 (=**B\*C**) tarefas por região (1 tarefa principal dentre essas cidades), ou seja,*
 - *30 (=**B\*C\*R**) tarefas no total para todo o país (1 tarefa principal entre as tarefas desse país).* 

###### Lista de funções das tarefas dos blocos
Cada uma das **B\*C\*R** tarefas receberá uma parte (um bloco) de notas de cada cidade e realizará as seguintes funções:
- Ordenação do das notas bloco;
- Somatória das notas do bloco; e
- Somatória do quadrado das notas do bloco.

Cada uma dessas funções será uma tarefa

Sabendo que a média aritmética (**avg**) é dada por:

![](.README_images/average.png)

E sabendo que o desvio padrão (**dev**) é dado por:

![](.README_images/standard_deviation.png)

Observa-se que é possível desatrelar o cáculo do desvio padrão da necessidade de se ter disponível a média aritmética dos dados. Além disso, as _somatória 1_ (também necessária para o cáculo da média aritmética) e _somatória 2_ são executáveis por cada uma das **B\*C\*R** tarefas sem haver dependência entre estas, assim como todas as tarefas da [lista de tarefas dos blocos](#Lista-de-tarefas-dos-blocos).

###### Lista de funções das tarefas principais dentre os blocos
As tarefas principais do nível de particionamento dos blocos dentro da cidade será a tarefa responsável por receber os resultados da [lista de tarefas dos blocos](#Lista-de-tarefas-dos-blocos) de cada tarefa deste nível. Então cada uma destas **C\*R** tarefas realizará as seguintes funções:

- Ordenação do das notas da cidade;
- Cálculo da nota mediana da cidade (**cit_med**);
- Indicação da menor nota da cidade (**cit_min**);
- Indicação da maior nota da cidade (**cit_max**);
- Cálculo da nota média da cidade (**cit_avg**);
- Somatória das _somatória 2_ provenientes das tarefas dos blocos; e
- Cálculo do desvio padrão das notas da cidade (**cit_dev**);

###### Lista de funções das tarefas principais dentre as cidades
As tarefas principais do nível de particionamento das cidades dentro da região serão as tarefas responsáveis por receberem os resultados da [Lista de funções das tarefas principais dentre os blocos](#Lista-de-funções-das-tarefas-principais-dentre-os-blocos) de cada tarefa deste nível. Então cada uma destas **R** tarefas realizará as seguintes funções:

- Ordenação das notas da região;
- Cálculo da nota mediana da região (**reg_med**);
- Indicação da menor nota da região (**reg_min**);
- Indicação da maior nota da região (**reg_max**;
- Cálculo da nota média da região (**reg_avg**);
- Somatória das _somatória 2_ provenientes das tarefas das cidades;
- Cálculo do desvio padrão das notas da região (**reg_dev**); e
- Indicação da cidade com maior nota média;

###### Lista de funções das tarefas principais dentre as regiões
A tarefa principal do nível de particionamento das regiões dentro do do país será a tarefa responsável por receber os resultados da [Lista de funções das tarefas principais dentre as cidades](#Lista-de-funções-das-tarefas-principais-dentre-as-cidades) de cada tarefa deste nível. Então esta única tarefa realizará as seguintes funções:

- Ordenação das notas do país;
- Cálculo da nota mediana do país (**cou_med**);
- Indicação da menor nota do país (**cou_min**);
- Indicação da maior nota do país (**cou_max**;
- Cálculo da nota média do país (**cou_avg**);
- Somatória das _somatória 2_ provenientes das tarefas das regiões;
- Cálculo do desvio padrão das notas do país (**cou_dev**); e
- Indicação da região com maior nota média;

## Comunicação

Para a comunicação, é importante entender como se dão as dependências entre as tarefas. O diagrama abaixo ilustra a dependência entre as tarefas. 

[](.README_images/tasks_partitioning.png) 

As tarefas dos blocos enviam para as tarefas principais dos blocos:
- somatória 1;
- somatória 2;
- notas ordenadas;

As tarefas principais dos blocos enviam para as tarefas principais das cidades:
- cit_avg;
- somatória 2;
- notas ordenadas;
- cit_med;
- cit_max;
- cit_min;
- cit_dev;

As tarefas principais das cidades enviam para a tarefa principal das regiões:
- max_cit_avg;
- reg_avg;
- somatória 2;
- notas ordenadas;
- reg_med;
- reg_max;
- reg_min;
- reg_dev;
- informações que vieram das tarefas principais dos blocos

São esses dados que precisam ser comunicados entre as tarefas.

## Aglomeração

Sendo que, dada a dimensão do problema não há como atribuir cada bloco de cada cidade de cada região do país a um nó de uma máquina MIMD de memória distribuída (cluster de computadores), é necessário realizar a aglomeração das tarefas. A aglomeração deve suceder enquanto houverem mais tarefas que nós disponíveis.

A aglomeração pode ser tanto realizada na "direção" das cidades quanto na direção dos blocos, a depender das quantidade de estudantes e quantidade (total) de cidades. 

    Aglomeração na direção das cidades:
    Aglomeração é realizada dentre as cidades de uma mesma região.    
    Analogamente, o próximo nível de aglomeração é dentre as cidades de uma mesma região  e, por fim, entre as regiões do país.
    
    Aglomeração na direção dos blocos:
    Aglomeração é realizada dentre os blocos de uma mesma cidade, assim evitando a comunicação entre as tarefas dos blocos e a tarefa principais da respectiva cidade.


## Mapeamento

Os processos serão distribuídos, via ambiente MPI, de forma igualitária entre os nós da máquina. Quando isto não for possível, a quantidade sobressalente de processos serão distribuídas entre os primeiros nós.
