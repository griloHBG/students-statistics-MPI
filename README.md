# Students Statistics

Objetivo: calcular métricas (estatísticas) nacionais, regionais e
municipais (3 níveis) a partir de notas (de 0 a 100) hipotéticas de
alunos do 9o ano do ensino fundamental.

As estatísticas são, **para cada nível**:

- Menor nota,
- Maior nota,
- Mediana,
- Média aritimética simples e,
- Desvio padrão.

***Serão premiadas a cidade e a região com as maiores médias aritiméticas simples.***

## Formato de entrada

Matricial:

- Cada elemento: nota de um aluno (geradas pseudo-aleatoriamente, com ***seed*** proveniente de argumento de entrada)
- Cada linha: uma cidade
- Cada parte de matriz: uma região

As notas dos alunos (inteiros de 0 a 100) devem ser geradas na seguinte ordem:

1. crescente por aluno
1. crescente por cidade
1. crescente por região

## Formato de saída

Deve ser exibida a menor e a maior nota, a mediana, a média e o desvio padrão para cada cidade, cada região e, por fim, para o Brasil.

A numeração dos alunos, cidades e regiões serão iniciadas em zero, sendo que na saída, as estatísticas devem estar apresentadas pelos índices em ordem crescente.

Após, devem ser indicadas a região e a cidade que receberão os prêmios

## Execução do programa

Para executar:

```
mpirun -np <N> --hostfile <hostfilehostfilename> statspar <R> <C> <A> <SEED>
```

Onde:

- N é a quantidade de de execuções de `statspar` que serão iniciadas
- hostfilename é o arquivo *hostfile* de mapeamento de carga de processamento
- R é a quantidade de regiões
- C é a quantidade de cidades por região
- A é a quantidade de alunos por cidade


## Desenvolvimento

Foi utilizado a metodologia **PCAM** para desenvolvimento, baseando-se em uma máquina MIMD de memória distribuida. 