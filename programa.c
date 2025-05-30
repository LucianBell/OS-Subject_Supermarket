#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>

#define MAX_BUFFER_SIZE 10000

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_output = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_caminhao = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_funcionario = PTHREAD_COND_INITIALIZER;

int n_caix_areacarga = 0;
int n_caix_entregues = 0;
int n_caix_estoque = 0;
int N, B, C, F, T, L;
bool finalizado = false;
bool criar_caminhoes = true; // Controla a criação de novos caminhões

char output_buffer[MAX_BUFFER_SIZE];

const char *funcionarios[] = {"Bilguêiti", "Torvalino", "Ólanmusgo"};

void log_buffer(const char *fmt, ...) {
    pthread_mutex_lock(&mutex_output);
    va_list args;
    va_start(args, fmt);
    int len = strlen(output_buffer);
    vsnprintf(output_buffer + len, MAX_BUFFER_SIZE - len, fmt, args);
    va_end(args);
    pthread_mutex_unlock(&mutex_output);
}

void *caminhao(void *arg) {
    int id = *((int *)arg);
    free(arg);

    // Determinar número de caixas para este caminhão
    pthread_mutex_lock(&mutex);
    if (n_caix_entregues >= N) {
        pthread_mutex_unlock(&mutex);
        log_buffer("Caminhão %d não entregará caixas, limite atingido\n", id);
        return NULL;
    }
    int caixas_pedido = (rand() % C) + 1;
    if (n_caix_entregues + caixas_pedido > N) {
        caixas_pedido = N - n_caix_entregues;
    }
    pthread_mutex_unlock(&mutex);

    log_buffer("Caminhão %d, vai entregar %d caixa(s) na área de carga\n", id, caixas_pedido);

    int caixas_restantes = caixas_pedido;
    while (caixas_restantes > 0) {
        pthread_mutex_lock(&mutex);
        // Verificar espaço na área de carga
        while (n_caix_areacarga >= B && !finalizado) {
            pthread_cond_wait(&cond_caminhao, &mutex);
        }
        if (finalizado) {
            pthread_mutex_unlock(&mutex);
            break;
        }
        // Descarregar o máximo possível de caixas
        int caixas_descarga = B - n_caix_areacarga;
        if (caixas_descarga > caixas_restantes) {
            caixas_descarga = caixas_restantes;
        }
        n_caix_areacarga += caixas_descarga;
        n_caix_entregues += caixas_descarga;
        caixas_restantes -= caixas_descarga;

        log_buffer("Caminhão %d, descarregou %d caixa(s) na área de carga\n", id, caixas_descarga);

        // Sinalizar funcionários
        pthread_cond_broadcast(&cond_funcionario);
        pthread_mutex_unlock(&mutex);

        // Simular tempo de espera entre descarregamentos
        if (caixas_restantes > 0) {
            usleep(100000); // 100ms
        }
    }

    log_buffer("Caminhão %d concluiu a entrega\n", id);
    return NULL;
}

void *funcionario(void *arg) {
    int id = *((int *)arg);
    free(arg);
    const char *nome = funcionarios[id];

    log_buffer("Criada thread que simula o funcionário %s\n", nome);

    while (true) {
        pthread_mutex_lock(&mutex);
        while (n_caix_areacarga == 0 && !finalizado) {
            pthread_cond_wait(&cond_funcionario, &mutex);
        }
        if (finalizado && n_caix_areacarga == 0) {
            pthread_mutex_unlock(&mutex);
            break;
        }
        if (n_caix_areacarga > 0) {
            n_caix_areacarga--;
            n_caix_estoque++;
            log_buffer("Funcionário %s, levou uma caixa ao estoque\n", nome);
            pthread_cond_signal(&cond_caminhao);
        }
        pthread_mutex_unlock(&mutex);

        // Simular tempo de transporte
        int tempo = (rand() % L + 1) * 100000;
        usleep(tempo);
    }

    log_buffer("Trabalho do funcionário %s acabou\n", nome);
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

    log_buffer("Iniciando simulação com N=%d, B=%d, C=%d, F=%d, T=%d, L=%d\n", N, B, C, F, T, L);

    pthread_t threads_funcionarios[F];
    log_buffer("Criada thread que simula chegada de caminhões\n");

    // Criar threads dos funcionários
    for (int i = 0; i < F; i++) {
        int *id = malloc(sizeof(int));
        *id = i;
        pthread_create(&threads_funcionarios[i], NULL, funcionario, id);
    }

    // Criar threads de caminhões
    int id_caminhao = 1;
    pthread_t *threads_caminhoes = malloc(N * sizeof(pthread_t)); // Máximo N caminhões
    int num_caminhoes = 0;

    while (n_caix_entregues < N && criar_caminhoes) {
        pthread_mutex_lock(&mutex);
        if (n_caix_entregues >= N) {
            criar_caminhoes = false;
            pthread_mutex_unlock(&mutex);
            break;
        }
        pthread_mutex_unlock(&mutex);

        int *id = malloc(sizeof(int));
        *id = id_caminhao++;
        pthread_create(&threads_caminhoes[num_caminhoes++], NULL, caminhao, id);

        // Simular intervalo de chegada
        int tempo_chegada = (rand() % T + 1) * 1000000; // T em microssegundos
        usleep(tempo_chegada);
    }

    // Sinalizar finalização
    pthread_mutex_lock(&mutex);
    criar_caminhoes = false;
    finalizado = true;
    pthread_cond_broadcast(&cond_funcionario);
    pthread_mutex_unlock(&mutex);

    // Esperar funcionários terminarem
    for (int i = 0; i < F; i++) {
        pthread_join(threads_funcionarios[i], NULL);
    }

    // Esperar caminhões terminarem
    for (int i = 0; i < num_caminhoes; i++) {
        pthread_join(threads_caminhoes[i], NULL);
    }
    free(threads_caminhoes);

    log_buffer("Encerrou thread caminhão\n");
    log_buffer("Simulação concluída. Caixas no estoque: %d\n", n_caix_estoque);

    printf("%s", output_buffer);
    return 0;
}