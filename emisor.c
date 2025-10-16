#include "ipc_utils.c"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

Shared *g_sh = NULL; // Puntero global para el manejador de señales

// Obtiene el tiempo actual en microsegundos
int64_t now_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

void handle_sigint(int sig)
{
    (void)sig;
    if (g_sh)
    {
        printf(ANSI_CYAN "\n[Emisor] Saliendo y decrementando contador de vivos...\n" ANSI_RESET);
        atomic_fetch_sub(&g_sh->emitters_live, 1);
    }
    exit(0);
}

int main(int argc, char **argv)
{
    if (argc < 5)
    {
        fprintf(stderr, "Uso: %s <shm_id> <auto|manual> <interval_ms> <key>\n", argv[0]);
        return 1;
    }
    const char *shm_name = argv[1];
    bool automatic_mode = strcmp(argv[2], "auto") == 0;
    unsigned int interval_ms = (unsigned int)strtoul(argv[3], NULL, 10);
    uint8_t key = (uint8_t)strtoul(argv[4], NULL, 10);

    Shared *sh = map_shared_memory(shm_name, O_RDWR, sizeof(Shared));
    if (!sh)
        return 1;
    g_sh = sh;
    signal(SIGINT, handle_sigint);

    // Asignación de ID atómica y segura
    uint32_t id = atomic_fetch_add(&sh->emitters_total, 1) + 1;
    atomic_fetch_add(&sh->emitters_live, 1);

    int total_emitters_snapshot = atomic_load(&sh->emitters_total);

    for (size_t i = id - 1; i < sh->data_len; i += total_emitters_snapshot)
    {
        if (atomic_load(&sh->shutting_down))
            break;

        uint8_t ch = (uint8_t)sh->data_buffer[i];
        Slot e = {
            .ch_enc = (uint8_t)(ch ^ key),
            .index = (uint32_t)i,
            .ts_us = now_us(),
            .producer_id = id};
        atomic_fetch_add(&sh->total_written, 1);

        if (!ring_push(sh, e))
            break;

        printf(ANSI_GREEN "[Emisor %u] idx=%u enc=%u ts=%" PRId64 ANSI_RESET "\n", id, e.index, e.ch_enc, e.ts_us);

        if (!automatic_mode)
        {
            printf(ANSI_CYAN "(Enter para continuar emisor %u)\n" ANSI_RESET, id);
            getchar();
        }
        else if (interval_ms > 0)
        {
            usleep(interval_ms * 1000);
        }
    }

    atomic_fetch_sub(&sh->emitters_live, 1);
    unmap_shared_memory(sh, sizeof(Shared));
    return 0;
}