#include "ipc_utils.c"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

Shared *g_sh = NULL;

void handle_sigint(int sig)
{
    (void)sig;
    if (g_sh)
    {
        printf(ANSI_CYAN "\n[Receptor] Saliendo...\n" ANSI_RESET);
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

    FILE *archivo = fopen("output.txt", "r+");
    if (archivo == NULL)
    {
        perror("fopen: no se pudo abrir output.txt");
        return 1;
    }

    Shared *sh = map_shared_memory(shm_name, O_RDWR, sizeof(Shared));
    if (!sh)
        return 1;
    g_sh = sh;
    signal(SIGINT, handle_sigint);

    uint32_t id = atomic_fetch_add(&sh->receivers_total, 1) + 1;
    atomic_fetch_add(&sh->receivers_live, 1);

    Slot r;
    while (ring_pop(sh, &r))
    {
        uint8_t plain = r.ch_enc ^ key;
        atomic_fetch_add(&sh->total_read, 1);

        printf(ANSI_YELLOW "[Receptor %u] idx=%u plain='%c'\n" ANSI_RESET, id, r.index, plain);

        pthread_mutex_lock(&sh->file_mtx);

        fseek(archivo, r.index, SEEK_SET);
        fputc(plain, archivo);
        fflush(archivo);

        pthread_mutex_unlock(&sh->file_mtx);

        if (!automatic_mode)
        {
            getchar();
        }
        else if (interval_ms > 0)
        {
            usleep(interval_ms * 1000);
        }
    }

    atomic_fetch_sub(&sh->receivers_live, 1);
    unmap_shared_memory(sh, sizeof(Shared));
    return 0;
}