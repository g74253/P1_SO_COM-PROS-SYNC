#include "shared.h"
#include <signal.h>

static Shared s;

static void on_sig(int signo)
{
    (void)signo;
    printf(ANSI_BOLD ANSI_CYAN "\n[Finalizador] Señal recibida: cierre\n" ANSI_RESET);
    shutdown(&s);
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Uso: %s <cap>\n", argv[0]);
        return 1;
    }
    ring_init(&s, (uint32_t)g_ascii_strtoull(argv[1], NULL, 10));

    signal(SIGINT, on_sig); // Ctrl+C simula el botón
    puts("[Finalizador] Presiona Ctrl+C para finalizar procesos (demo).");

    while (!s.shutting_down)
        g_usleep(200000);

    // Estadísticas
    printf(ANSI_BOLD "\n--- Estadísticas ---\n" ANSI_RESET);
    printf("Transferidos: %d\n", g_atomic_int_get(&s.total_read));
    printf("En buffer: %u\n", s.count);
    printf("Emisores vivos/total: %d/%d\n", g_atomic_int_get(&s.emitters_live), g_atomic_int_get(&s.emitters_total));
    printf("Receptores vivos/total: %d/%d\n", g_atomic_int_get(&s.receivers_live), g_atomic_int_get(&s.receivers_total));
    printf("Memoria usada: %zu bytes\n", sizeof(Slot) * s.cap);

    ring_destroy(&s);
    return 0;
}
