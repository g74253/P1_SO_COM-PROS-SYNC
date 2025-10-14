#include "shared.h"

typedef struct
{
    Shared *sh;
    uint32_t id;
} RxArgs;

static gpointer receiver_fn(gpointer p)
{
    RxArgs *a = (RxArgs *)p;
    g_atomic_int_inc(&a->sh->receivers_live);
    GString *out = g_string_new(NULL);
    Slot r;
    while (ring_pop(a->sh, &r))
    {
        uint8_t plain = r.ch_enc ^ a->sh->key;
        g_string_append_c(out, (char)plain);
        g_atomic_int_inc(&a->sh->total_read);

        printf(ANSI_YELLOW "[Receptor %u] idx=%u plain=%u('%c') ts=%" G_GINT64_FORMAT ANSI_RESET "\n",
               a->id, r.index, plain, (plain >= 32 && plain < 127) ? plain : '.', r.ts_us);

        if (!a->sh->automatic_mode)
            getchar();
        else if (a->sh->interval_ms)
            g_usleep(a->sh->interval_ms * 1000);
    }
    printf(ANSI_BOLD ANSI_MAGENTA "\n[Receptor %u] Texto reconstruido:\n%s\n" ANSI_RESET, a->id, out->str);
    g_string_free(out, TRUE);
    g_atomic_int_dec_and_test(&a->sh->receivers_live);
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        fprintf(stderr, "Uso: %s <cap> <auto|manual> <key>\n", argv[0]);
        return 1;
    }
    Shared s;
    ring_init(&s, (uint32_t)g_ascii_strtoull(argv[1], NULL, 10));
    s.automatic_mode = g_strcmp0(argv[2], "auto") == 0;
    s.interval_ms = s.automatic_mode ? 150 : 0;
    s.key = (uint8_t)g_ascii_strtoull(argv[3], NULL, 10);

    g_atomic_int_inc(&s.receivers_total);
    RxArgs args = {.sh = &s, .id = 1};
    GThread *th = g_thread_new("receptor", receiver_fn, &args);
    g_thread_join(th);

    ring_destroy(&s);
    return 0;
}
