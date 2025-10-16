#include "shared.h"
#include <fcntl.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <unistd.h>

// Mapea la memoria compartida existente en el espacio de direcciones del proceso
Shared *map_shared_memory(const char *name, int flags, size_t size)
{
    int fd = shm_open(name, flags, 0666);
    if (fd == -1)
    {
        perror("shm_open: ¿Ejecutó el inicializador primero?");
        return NULL;
    }
    Shared *sh = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (sh == MAP_FAILED)
    {
        perror("mmap");
        close(fd);
        return NULL;
    }
    close(fd);
    return sh;
}

// Desmapea la memoria compartida
void unmap_shared_memory(Shared *sh, size_t size)
{
    if (munmap(sh, size) == -1)
    {
        perror("munmap");
    }
}

// Escribir en el buffer circular
bool ring_push(Shared *s, Slot entry)
{
    pthread_mutex_lock(&s->mtx);
    // ESTO NO ES BUSY-WAITING: El hilo NO consume CPU en este ciclo.
    // Llama a pthread_cond_wait, que pone al hilo en estado de *dormido*
    // y lo saca de la cola de planificación del SO hasta que se recibe
    // una señal en 'not_full'.
    while (atomic_load(&s->count) == s->cap && !atomic_load(&s->shutting_down))
    {
        pthread_cond_wait(&s->not_full, &s->mtx);
    }
    if (atomic_load(&s->shutting_down))
    {
        pthread_mutex_unlock(&s->mtx);
        return false;
    }
    uint32_t tail = atomic_load(&s->tail);
    s->buf[tail] = entry;
    atomic_store(&s->tail, (tail + 1) % s->cap);
    atomic_fetch_add(&s->count, 1);
    pthread_cond_signal(&s->not_empty);
    pthread_mutex_unlock(&s->mtx);
    return true;
}

// Leer del buffer circular
bool ring_pop(Shared *s, Slot *out)
{
    pthread_mutex_lock(&s->mtx);
    // ESTO NO ES BUSY-WAITING: El hilo NO consume CPU en este ciclo.
    // Llama a pthread_cond_wait, que pone al hilo en estado de *dormido*
    // y lo saca de la cola de planificación del SO hasta que se recibe
    // una señal en 'not_empty'.
    while (atomic_load(&s->count) == 0 && !atomic_load(&s->shutting_down))
    {
        pthread_cond_wait(&s->not_empty, &s->mtx);
    }
    if (atomic_load(&s->shutting_down) && atomic_load(&s->count) == 0)
    {
        pthread_mutex_unlock(&s->mtx);
        return false;
    }
    uint32_t head = atomic_load(&s->head);
    *out = s->buf[head];
    atomic_store(&s->head, (head + 1) % s->cap);
    atomic_fetch_sub(&s->count, 1);
    pthread_cond_signal(&s->not_full);
    pthread_mutex_unlock(&s->mtx);
    return true;
}

// Inicia un apagado elegante
void shutdown(Shared *s)
{
    pthread_mutex_lock(&s->mtx);
    atomic_store(&s->shutting_down, true);
    pthread_cond_broadcast(&s->not_empty); // Despierta a todos los receptores
    pthread_cond_broadcast(&s->not_full);  // Despierta a todos los emisores
    pthread_mutex_unlock(&s->mtx);
}