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

É necessário realizar o cálculo de **6** estatísticas para o país, **6\*R** estatísticas para as regiões (**R** regiões no país) e **6\*R\*C** estatísticas para as cidades (**C** cidades por região). Cada cidade contém **A** estudantes (**A** estudantes por cidade).

Para o Brasil, tomando a cidade de São Paulo como exemplo (que é a cidade que mais tem alunos no Ensino Fundamental) no ano de 2018, houveram 1.383.779 matrículas no Ensino Fundamental como um todo ([Fonte: Cidades@, portal do IBGE com dados sobre cidades brasileiras](https://cidades.ibge.gov.br/brasil/sp/sao-paulo/panorama)). Sendo que o objetivo deste trabalho limita-se às notas dos alunos do 9o ano, supondo-se que essas matrículas são igualmente distribuídas entre todos os anos do Ensino Fundamental, temos por volta de 153 mil estudantes no 9o ano. Tendo esta quantidade de alunos em um cidade como exemplo, é interessante que ela seja dividida entre algumas tarefas para que sejam calculadas as estatísticas de maneira melhor paralelizada. Esta quantidade de tarfas dentro de uma cidade será denominada quantidade **B** de tarefas

Logo, a quantidade total de tarefas no problema será de **B\*C\*R**, sendo que dentre estas haverá as "tarefas principais" no nível dos blocos e, das cidades e das regiões seguindo a seguinte lógica:
- A tarefa principal entre os blocos calculará as estatísticas da respectiva cidade; 
- A tarefa principal entre as cidades calculará as estatísticas da respectiva região; e
- A tarefa principal entre as regiões calculará as estatísticas do país;

**Ex**: *Para um país com 3 (**R**) regiões, cada região com 5 (**C**) cidades e cada cidade com 12 (**A**) alunos, considerando um particionamento de 2 (**B**) blocos dentro de cada cidade, será um total de:*
 - *2 (=**B**) tarefas por cidade (1 tarefa principal dentre esses blocos), ou seja,*
 - *10 (=**B\*C**) tarefas por região (1 tarefa principal dentre essas cidades), ou seja,*
 - *30 (=**B\*C\*R**) tarefas no total para todo o país (1 tarefa principal entre as tarefas desse país).* 

###### Lista de tarefas dos blocos
Cada uma das **B\*C\*R** tarefas receberá uma parte (um bloco) de notas de cada cidade e realizará as seguintes funções:
- Ordenação do das notas bloco; 
- Indicação da menor nota do bloco;
- Indicação da maior nota do bloco;
- Somatória das notas do bloco; e
- Somatória do quadrado das notas do bloco.

Sabendo que a média aritmética (**avg**) é dada por:

![](.README_images/average.png)

E sabendo que o desvio padrão (**dev**) é dado por:

![](.README_images/standard_deviation.png)

Observa-se que é possível desatrelar o cáculo do desvio padrão da necessidade de se ter disponível a média aritmética dos dados. Além disso, as _somatória 1_ (também necessária para o cáculo da média aritmética) e _somatória 2_ são executáveis por cada uma das **B\*C\*R** tarefas sem haver dependência entre estas, assim como todas as tarefas da [lista de tarefas](#Lista-de-tarefas-dos-blocos).

A tarefa principal do nível de particionamento dos blocos dentro da cidade será a tarefa responsável por receber os resultados da [lista de tarefas](#Lista-de-tarefas-dos-blocos) de cada tarefa deste nível. Então cada uma destas **C\*R** tarefa realizará as seguintes tarefas:

- Ordenação do das notas da cidade; 
- Indicação da menor nota da cidade;
- Indicação da maior nota da cidade;
- Cálculo da média da cidade;
- Somatória das _somatória 2_ do quadrado das notas do bloco.

## Comunicação

Para a comunicação, é importante entender como se dão as dependências entre as tarefas. O Diagrama UML de classes abaixo ilustra (por meio de herença) a dependência entre as estatísticas a serem calculadas.


