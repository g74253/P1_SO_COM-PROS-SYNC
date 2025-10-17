#ifndef SHARED_H
#define SHARED_H

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

#define ANSI_RESET "\x1b[0m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_CYAN "\x1b[36m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_BOLD "\x1b[1m"

#define MAX_CAP 256
#define MAX_FILE_SIZE (10 * 1024)

typedef struct
{
    uint8_t ch_enc;
    uint32_t index;
    int64_t ts_us;
    uint32_t producer_id;
} Slot;

typedef struct
{
    // Buffer circular
    Slot buf[MAX_CAP];
    uint32_t cap;
    atomic_uint head;
    atomic_uint tail;
    atomic_uint count;

    // Sincronización entre procesos
    pthread_mutex_t mtx;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    pthread_mutex_t file_mtx;

    // Control global
    atomic_bool shutting_down;

    // Métricas
    atomic_int total_written;
    atomic_int total_read;

    // Conteo de procesos vivos/totales
    atomic_int emitters_live;
    atomic_int emitters_total;
    atomic_int receivers_live;
    atomic_int receivers_total;

    // Modo y temporizado
    bool automatic_mode;
    unsigned int interval_ms;

    // Buffer para el contenido del archivo de entrada
    size_t data_len;
    char data_buffer[MAX_FILE_SIZE];
} Shared;

// Funciones para manejar la memoria compartida
Shared *map_shared_memory(const char *name, int flags, size_t size);
void unmap_shared_memory(Shared *sh, size_t size);

// Operaciones del buffer circular
bool ring_push(Shared *s, Slot entry);
bool ring_pop(Shared *s, Slot *out);
void shutdown(Shared *s);

#endif // SHARED_H