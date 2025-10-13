// emisor.c
#include "shared.h"

typedef struct
{
    Shared *sh;
    GPtrArray *data;
    uint32_t id;
} EmArgs;

static gpointer emitter_fn(gpointer p)
{
    EmArgs *a = (EmArgs *)p;
    g_atomic_int_inc(&a->sh->emitters_live);
    for (guint i = 0; i < a->data->len; i++)
    {
        if (a->sh->shutting_down)
            break;
        uint8_t ch = GPOINTER_TO_UINT(a->data->pdata[i]);
        Slot e = {
            .ch_enc = (uint8_t)(ch ^ a->sh->key),
            .index = g_atomic_int_add(&a->sh->total_written, 1),
            .ts_us = now_us(),
            .producer_id = a->id};
        if (!ring_push(a->sh, e))
            break;

        printf(ANSI_GREEN "[Emisor %u] idx=%u enc=%u ts=%" G_GINT64_FORMAT ANSI_RESET "\n",
               a->id, e.index, e.ch_enc, e.ts_us);

        if (!a->sh->automatic_mode)
        {
            getchar(); // modo manual: enter para continuar
        }
        else if (a->sh->interval_ms)
        {
            g_usleep(a->sh->interval_ms * 1000);
        }
    }
    g_atomic_int_dec_and_test(&a->sh->emitters_live);
    return NULL;
}

static GPtrArray *load_chars(const char *path)
{
    GPtrArray *arr = g_ptr_array_new();
    gchar *content = NULL;
    gsize len = 0;
    if (!g_file_get_contents(path, &content, &len, NULL))
    {
        fprintf(stderr, "No se pudo leer %s\n", path);
        return arr;
    }
    for (gsize i = 0; i < len; i++)
    {
        g_ptr_array_add(arr, GUINT_TO_POINTER((guint)(uint8_t)content[i]));
    }
    g_free(content);
    return arr;
}

int main(int argc, char **argv)
{
    if (argc < 5)
    {
        fprintf(stderr, "Uso: %s <cap> <auto|manual> <key> <archivo>\n", argv[0]);
        return 1;
    }
    Shared s;
    ring_init(&s, (uint32_t)g_ascii_strtoull(argv[1], NULL, 10));
    s.automatic_mode = g_strcmp0(argv[2], "auto") == 0;
    s.interval_ms = s.automatic_mode ? 150 : 0;
    s.key = (uint8_t)g_ascii_strtoull(argv[3], NULL, 10);

    GPtrArray *data = load_chars(argv[4]);
    g_atomic_int_inc(&s.emitters_total);

    EmArgs args = {.sh = &s, .data = data, .id = 1};
    GThread *th = g_thread_new("emisor", emitter_fn, &args);
    g_thread_join(th);

    g_ptr_array_free(data, TRUE);
    ring_destroy(&s);
    return 0;
}
