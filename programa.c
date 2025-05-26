#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_caminhao = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_funcionario = PTHREAD_COND_INITIALIZER;

int n_caix_areacarga = 0;
int n_caix_entregues = 0;
int n_caix_estoque = 0;
int N, B, C, F, T, L;
bool finalizado = false;

const char *funcionarios[] = {"Bilguêiti", "Torvalino", "Ólanmusgo"};

void *caminhao(void *arg) {
    int caixas_total = 0;  // Contador total de caixas entregues por todos os caminhões
    int id_caminhao = 1;   // Identificador incremental para cada caminhão

    while (caixas_total < N) {
        // Simula tempo aleatório entre a chegada de caminhões, em microsegundos
        int tempo_chegada = (rand() % T + 1) * 100000;
        usleep(tempo_chegada);

        // Gera quantidade aleatória de caixas no pedido (entre 1 e C)
        int pedido = rand() % C + 1;

        // Garante que o total de caixas não ultrapasse N
        if (caixas_total + pedido > N) {
            pedido = N - caixas_total;
        }

        printf("Caminhão %d, vai entregar %d caixa(s) na área de carga\n", id_caminhao, pedido);

        // Entrega das caixas (uma por uma, respeitando limite da área de carga B)
        for (int i = 0; i < pedido; i++) {
            pthread_mutex_lock(&mutex);

            // Espera espaço na área de carga
            while (n_caix_areacarga >= B) {
                pthread_cond_wait(&cond_caminhao, &mutex);
            }

            // Entrega uma caixa
            n_caix_areacarga++;
            caixas_total++;

            printf("Caminhão %d, descarregou 1 caixa na área de carga\n", id_caminhao);

            // Acorda algum funcionário que esteja esperando por caixas
            pthread_cond_signal(&cond_funcionario);
            pthread_mutex_unlock(&mutex);
        }

        id_caminhao++; // Próximo caminhão
    }

    // Indica que nenhum caminhão mais virá (fim das entregas)
    pthread_mutex_lock(&mutex);
    finalizado = true;
    pthread_cond_broadcast(&cond_funcionario); // Acorda todos os funcionários que estejam esperando
    pthread_mutex_unlock(&mutex);

    printf("Encerrou thread caminhão\n");
    return NULL;
}

void *funcionario(void *arg) {
    //obtem id do funcionário convertendo o argumento da thread(void *) para int *
    int id = *((int *)arg);
    free(arg);

    const char *nome = funcionarios[id];

    printf("Criada a thread que simula o funcionário %s\n", nome);

    while(true){
        pthread_mutex_lock(&mutex);

        //aguarda até que haja caixas para transportar ou que todas tenham sido processadas
        while(n_caix_areacarga == 0 && (!finalizado || n_caix_estoque < n_caix_entregues)){
            pthread_cond_wait(&cond_funcionario, &mutex);
        }

        //Se todas as caixas foram entregues e levadas ao estoque, quebra o loop
        if (finalizado && n_caix_areacarga == 0 && n_caix_estoque == n_caix_entregues){
            pthread_mutex_unlock(&mutex);
            break;
        }

        if(n_caix_areacarga > 0){
            n_caix_areacarga --;
            n_caix_entregues ++;
            printf("Funcionário %s, levou uma caixa ao estoque\n", nome);

            //Acorda caminhões aguardando espaço na área de carga
            pthread_cond_signal(&cond_caminhao);
        }

        pthread_mutex_unlock(&mutex);

        //Simula o tempo de transporte da caixa (1 até L décimos de segundo)
        int tempo = (rand() % L + 1) * 100000;
        usleep(tempo);
    }

    printf("Trabalho do funcionário %s acabou\n", nome);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 7) {
        printf("Uso: %s N B C F T L\n", argv[0]);
        printf("Onde:\n");
        printf("N = quantidade mínima de caixas (1 < N < 100)\n");
        printf("B = capacidade da área de carga (1 < B < 10)\n");
        printf("C = máximo de caixas por pedido (1 < C < 5)\n");
        printf("F = número de funcionários (1 < F < 3)\n");
        printf("T = intervalo máximo entre chegadas (1 < T < 20)\n");
        printf("L = tempo máximo de transporte (1 < L < 15)\n");
        return 1;
    }
    
    N = atoi(argv[1]);
    B = atoi(argv[2]);
    C = atoi(argv[3]);
    F = atoi(argv[4]);
    T = atoi(argv[5]);
    L = atoi(argv[6]);
    
    if (N <= 1 || N >= 100 || B <= 1 || B >= 10 || C <= 1 || C >= 5 || 
        F <= 1 || F >= 3 || T <= 1 || T >= 20 || L <= 1 || L >= 15) {
        printf("Parâmetros fora dos limites especificados.\n");
        return 1;
    }
    
    srand(time(NULL));
    
    printf("Iniciando simulação com N=%d, B=%d, C=%d, F=%d, T=%d, L=%d\n", N, B, C, F, T, L);
    
    pthread_t threads_funcionarios[F];
    
    for (int i = 0; i < F; i++) {
        int *id = malloc(sizeof(int));
        *id = i;
        pthread_create(&threads_funcionarios[i], NULL, funcionario, id);
    }

    // Cria thread do caminhão
    pthread_t thread_caminhao;
    printf("Criada thread que simula chegada de caminhões\n");
    pthread_create(&thread_caminhao, NULL, caminhao, NULL);

    // Espera a thread do caminhão terminar
    pthread_join(thread_caminhao, NULL);

    // Espera todas as threads de funcionários terminarem
    for (int i = 0; i < F; i++) {
        pthread_join(threads_funcionarios[i], NULL);
    }

    printf("Simulação concluída. Caixas no estoque: %d\n", n_caix_estoque);
    return 0;
}


//execução com gcc programa.c -o programa -Wall -lpthread
//./programa (casos teste)