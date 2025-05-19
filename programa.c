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
    // Fiz e me buguei tudo, deixei limpo pra não confundir
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
            printf("Funcionário %s, levou uma caixa ao estoque", nome);

            //Acorda caminhões aguardando espaço na área de carga
            pthread_cond_signal(&cond_caminhao);
        }

        pthread_mutex_unlock(&mutex);

        //Simula o tempo de transporte da caixa (1 até L décimos de segundo)
        int tempo = (rand() % L + 1) * 100000;
        usleep(tempo);
    }

    print("Trabalho do funcionário %s acabou\n", nome);
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
    
    printf("Criada thread que simula chegada de caminhões\n");
    
    // IMPLEMENTAR A LÓGICA DE CRIAÇÃO DOS CAMINHÕES

    printf("Encerrou thread caminhão\n");
    
    // IPLEMENTAR A FINALIZAÇÃO
    
    for (int i = 0; i < F; i++) {
        pthread_join(threads_funcionarios[i], NULL);
    }
    
    printf("Simulação concluída. Caixas no estoque: %d\n", n_caix_estoque);
    
    return 0;
}