#include "ipc_utils.c"
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>

static Shared *g_shm_ptr = NULL;

static void on_sig(int signo)
{
    (void)signo;
    if (g_shm_ptr)
    {
        printf(ANSI_BOLD ANSI_CYAN "\n[Finalizador] Señal recibida: cierre\n" ANSI_RESET);
        shutdown(g_shm_ptr);
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Uso: %s <shm_id>\n", argv[0]);
        return 1;
    }

    const char *shm_name = argv[1];

    Shared *sh = map_shared_memory(shm_name, O_RDWR, sizeof(Shared));
    if (!sh)
        return 1;

    g_shm_ptr = sh;
    signal(SIGINT, on_sig);

    puts(ANSI_BOLD ANSI_CYAN "[Finalizador] Presiona Ctrl+C para finalizar procesos." ANSI_RESET);

    while (!atomic_load(&sh->shutting_down))
    {
        if (atomic_load(&sh->emitters_total) > 0)
        {
            // Condición 1: ¿Han terminado TODOS los emisores su trabajo?
            bool all_emitters_finished = (atomic_load(&sh->emitters_live) == 0);

            if (all_emitters_finished)
            {
                // Condición 2: ¿Está el buffer vacío?
                bool buffer_is_empty = (atomic_load(&sh->count) == 0);

                if (buffer_is_empty)
                {
                    printf("[Finalizador] Tarea completada: Todos los emisores terminaron y el buffer está vacío. Iniciando cierre...\n");
                    shutdown(sh);
                    break; // Salir del bucle para la limpieza final
                }
            }
        }
        // ESTO NO ES BUSY-WAITING: El proceso cede el control del CPU
        // a otros procesos. El Finalizador está diseñado para ser reactivo a un
        // evento de cierre O para verificar periódicamente la condición de "trabajo terminado".
        usleep(200000);
    }

    // Espera a que todos los procesos terminen
    // ESTO NO ES BUSY-WAITING: El proceso cede el control del CPU
    // a otros procesos. La espera es necesaria para que los procesos emisores
    // y receptores tengan tiempo de recibir la señal de 'shutting_down' y terminen su propia limpieza.
    while (atomic_load(&sh->emitters_live) > 0 || atomic_load(&sh->receivers_live) > 0)
    {
        printf("[Finalizador] Esperando... Emisores vivos: %d, Receptores vivos: %d\n",
               atomic_load(&sh->emitters_live), atomic_load(&sh->receivers_live));
        sleep(1);
    }

    printf(ANSI_BOLD "\n==== ESTADÍSTICAS FINALES ====\n" ANSI_RESET);
    printf("Transferidos (leídos): %d\n", atomic_load(&sh->total_read));
    printf("En memoria compartida: %u\n", atomic_load(&sh->count));
    printf("Emisores total: %d\n", atomic_load(&sh->emitters_total));
    printf("Receptores total: %d\n", atomic_load(&sh->receivers_total));

    // Destruir mutex y conds
    pthread_mutex_destroy(&sh->mtx);
    pthread_mutex_destroy(&sh->file_mtx);
    pthread_cond_destroy(&sh->not_empty);
    pthread_cond_destroy(&sh->not_full);

    // Desmapear y eliminar la memoria compartida
    unmap_shared_memory(sh, sizeof(Shared));
    shm_unlink(shm_name);
    printf(ANSI_BOLD ANSI_GREEN "Memoria compartida '%s' liberada.\n" ANSI_RESET, shm_name);

    return 0;
}