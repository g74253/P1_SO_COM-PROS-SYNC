// shared.h
#ifndef SHARED_H
#define SHARED_H

#include <glib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define ANSI_RESET "\x1b[0m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_CYAN "\x1b[36m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_BOLD "\x1b[1m"

typedef struct
{
    uint8_t ch_enc;       // caracter codificado (XOR)
    uint32_t index;       // índice de inserción
    gint64 ts_us;         // timestamp (microsegundos)
    uint32_t producer_id; // id de emisor
} Slot;

typedef struct
{
    // buffer circular
    Slot *buf;
    uint32_t cap;
    uint32_t head; // próxima lectura
    uint32_t tail; // próxima escritura
    uint32_t count;

    // sync
    GMutex mtx;
    GCond not_empty;
    GCond not_full;

    // control global
    volatile gboolean shutting_down;

    // métricas
    gint total_written;
    gint total_read;

    // vivos/totales (cuantos hay y hubo)
    gint emitters_live;
    gint emitters_total;
    gint receivers_live;
    gint receivers_total;

    // modo y temporizado
    gboolean automatic_mode;
    guint interval_ms; // para automático

    uint8_t key; // llave XOR (8 bits)
} Shared;

// microsegs
static inline gint64 now_us(void)
{
    GDateTime *dt = g_date_time_new_now_local();
    gint64 us = g_date_time_to_unix(dt) * 1000000 + g_date_time_get_microsecond(dt);
    g_date_time_unref(dt);
    return us;
}

// creacion del aro
static inline void ring_init(Shared *s, uint32_t cap)
{
    s->buf = g_new0(Slot, cap);
    s->cap = cap;
    s->head = s->tail = s->count = 0;
    g_mutex_init(&s->mtx);
    g_cond_init(&s->not_empty);
    g_cond_init(&s->not_full);
    s->shutting_down = FALSE;
    g_atomic_int_set(&s->total_written, 0);
    g_atomic_int_set(&s->total_read, 0);
    g_atomic_int_set(&s->emitters_live, 0);
    g_atomic_int_set(&s->emitters_total, 0);
    g_atomic_int_set(&s->receivers_live, 0);
    g_atomic_int_set(&s->receivers_total, 0);
}

// Elimina aro
static inline void ring_destroy(Shared *s)
{
    g_free(s->buf);
    g_mutex_clear(&s->mtx);
    g_cond_clear(&s->not_empty);
    g_cond_clear(&s->not_full);
}

// escribir en aro
static inline gboolean ring_push(Shared *s, Slot entry)
{
    g_mutex_lock(&s->mtx);
    while (s->count == s->cap && !s->shutting_down)
    {
        g_cond_wait(&s->not_full, &s->mtx);
    }
    if (s->shutting_down)
    {
        g_mutex_unlock(&s->mtx);
        return FALSE;
    }
    s->buf[s->tail] = entry;
    s->tail = (s->tail + 1) % s->cap;
    s->count++;
    g_cond_signal(&s->not_empty);
    g_mutex_unlock(&s->mtx);
    return TRUE;
}

// elimina en aro
static inline gboolean ring_pop(Shared *s, Slot *out)
{
    g_mutex_lock(&s->mtx);
    while (s->count == 0 && !s->shutting_down)
    {
        g_cond_wait(&s->not_empty, &s->mtx);
    }
    if (s->shutting_down && s->count == 0)
    {
        g_mutex_unlock(&s->mtx);
        return FALSE;
    }
    *out = s->buf[s->head];
    s->head = (s->head + 1) % s->cap;
    s->count--;
    g_cond_signal(&s->not_full);
    g_mutex_unlock(&s->mtx);
    return TRUE;
}

// apaga todo
static inline void shutdown(Shared *s)
{
    g_mutex_lock(&s->mtx);
    s->shutting_down = TRUE;
    g_cond_broadcast(&s->not_empty);
    g_cond_broadcast(&s->not_full);
    g_mutex_unlock(&s->mtx);
}

#endif // SHARED_H
