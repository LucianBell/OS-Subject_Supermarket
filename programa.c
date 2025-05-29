#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>

#define MAX_BUFFER_SIZE 10000  // Tamanho máximo do buffer de saída

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_output = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_caminhao = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_funcionario = PTHREAD_COND_INITIALIZER;

int n_caix_areacarga = 0;  // Caixas na área de carga
int n_caix_entregues = 0;  // Caixas entregues pelos caminhões
int n_caix_estoque = 0;    // Caixas movidas para o estoque
int N, B, C, F, T, L;
bool finalizado = false;

char output_buffer[MAX_BUFFER_SIZE];  // Buffer de saída para armazenar logs

const char *funcionarios[] = {"Bilguêiti", "Torvalino", "Ólanmusgo"};

// Função protegida por mutex que adiciona texto formatado ao buffer de saída
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
    int caixas_total = 0;  // Contador total de caixas entregues
    int id_caminhao = 1;   // Identificador incremental para cada caminhão

    while (caixas_total < N) {
        // Simula tempo aleatório entre a chegada de caminhões (em microsegundos)
        int tempo_chegada = (rand() % T + 1) * 100000;
        usleep(tempo_chegada);

        // Gera quantidade aleatória de caixas no pedido (entre 1 e C)
        int pedido = rand() % C + 1;

        // Garante que o total de caixas não ultrapasse N
        if (caixas_total + pedido > N) {
            pedido = N - caixas_total;
        }

        log_buffer("Caminhão %d, vai entregar %d caixa(s) na área de carga\n", id_caminhao, pedido);

        // Tenta descarregar todas as caixas do pedido de uma vez
        pthread_mutex_lock(&mutex);
        while (n_caix_areacarga + pedido > B) {
            // Se não houver espaço suficiente, espera
            pthread_cond_wait(&cond_caminhao, &mutex);
        }

        // Descarrega todas as caixas do pedido
        n_caix_areacarga += pedido;
        n_caix_entregues += pedido;
        caixas_total += pedido;

        log_buffer("Caminhão %d, descarregou %d caixa(s) na área de carga\n", id_caminhao, pedido);

        // Acorda funcionários que estejam esperando
        pthread_cond_broadcast(&cond_funcionario);
        pthread_mutex_unlock(&mutex);

        id_caminhao++; // Próximo caminhão
    }

    // Marca que não haverá mais caminhões
    pthread_mutex_lock(&mutex);
    finalizado = true;
    pthread_cond_broadcast(&cond_funcionario); // Acorda todos os funcionários
    pthread_mutex_unlock(&mutex);

    // Aguarda todas as caixas serem movidas para o estoque
    pthread_mutex_lock(&mutex);
    while (n_caix_estoque < N) {
        pthread_cond_wait(&cond_caminhao, &mutex);
    }
    pthread_mutex_unlock(&mutex);

    log_buffer("Encerrou thread caminhão\n");
    return NULL;
}

void *funcionario(void *arg) {
    int id = *((int *)arg);
    free(arg);
    const char *nome = funcionarios[id];

    log_buffer("Criada thread que simula o funcionário %s\n", nome);

    while (true) {
        pthread_mutex_lock(&mutex);

        // Aguarda até que haja caixas ou a simulação tenha finalizado
        while (n_caix_areacarga == 0 && !finalizado) {
            pthread_cond_wait(&cond_funcionario, &mutex);
        }

        // Se todas as caixas foram entregues e movidas, encerra
        if (finalizado && n_caix_areacarga == 0) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        if (n_caix_areacarga > 0) {
            n_caix_areacarga--;
            n_caix_estoque++;
            log_buffer("Funcionário %s, levou uma caixa ao estoque\n", nome);

            // Acorda caminhão que esteja esperando espaço
            pthread_cond_signal(&cond_caminhao);
        }

        pthread_mutex_unlock(&mutex);

        // Simula o tempo de transporte (1 a L décimos de segundo)
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
    pthread_t thread_caminhao;

    // Cria thread do caminhão
    log_buffer("Criada thread que simula chegada de caminhões\n");
    pthread_create(&thread_caminhao, NULL, caminhao, NULL);

    // Cria threads dos funcionários
    for (int i = 0; i < F; i++) {
        int *id = malloc(sizeof(int));
        *id = i;
        pthread_create(&threads_funcionarios[i], NULL, funcionario, id);
    }

    // Espera todas as threads de funcionários terminarem
    for (int i = 0; i < F; i++) {
        pthread_join(threads_funcionarios[i], NULL);
    }

    // Espera a thread do caminhão terminar
    pthread_join(thread_caminhao, NULL);

    log_buffer("Simulação concluída. Caixas no estoque: %d\n", n_caix_estoque);

    // Imprime todo o conteúdo armazenado no buffer de saída
    printf("%s", output_buffer);

    return 0;
}
