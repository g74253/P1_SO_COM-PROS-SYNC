// inicializador.c
#include "shared.h"

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        fprintf(stderr, "Uso: %s <capacidad> <modo:auto|manual> <llave_xor(0-255)>\n", argv[0]);
        return 1;
    }
    uint32_t cap = (uint32_t)g_ascii_strtoull(argv[1], NULL, 10);
    gboolean automatic = g_strcmp0(argv[2], "auto") == 0;
    uint8_t key = (uint8_t)g_ascii_strtoull(argv[3], NULL, 10);

    Shared s;
    ring_init(&s, cap);
    s.automatic_mode = automatic;
    s.interval_ms = automatic ? 150 : 0;
    s.key = key;

    printf(ANSI_BOLD ANSI_CYAN "[Inicializador] " ANSI_RESET
                               "Cap=%u, Modo=%s, Key=%u. Memoria compartida lista.\n",
           cap, automatic ? "auto" : "manual", key);

    ring_destroy(&s);
    return 0;
}
