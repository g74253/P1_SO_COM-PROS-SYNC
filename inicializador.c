#include "shared.h"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

// Lee un archivo y lo carga en la memoria compartida
long load_file_into_shm(const char *path, Shared *sh)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return -1;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (len > MAX_FILE_SIZE)
    {
        fclose(f);
        return -1;
    }
    fread(sh->data_buffer, 1, len, f);
    sh->data_len = len;
    fclose(f);
    return len;
}

void initialize_shared_struct(Shared *s, uint32_t cap)
{
    memset(s, 0, offsetof(Shared, data_len));
    s->cap = cap > MAX_CAP ? MAX_CAP : cap;

    atomic_init(&s->head, 0);
    atomic_init(&s->tail, 0);
    atomic_init(&s->count, 0);
    atomic_init(&s->shutting_down, false);
    atomic_init(&s->total_written, 0);
    atomic_init(&s->total_read, 0);
    atomic_init(&s->emitters_live, 0);
    atomic_init(&s->emitters_total, 0);
    atomic_init(&s->receivers_live, 0);
    atomic_init(&s->receivers_total, 0);

    pthread_mutexattr_t mattr;
    pthread_condattr_t cattr;
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    pthread_condattr_init(&cattr);
    pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);

    pthread_mutex_init(&s->mtx, &mattr);
    pthread_mutex_init(&s->file_mtx, &mattr);
    pthread_cond_init(&s->not_empty, &cattr);
    pthread_cond_init(&s->not_full, &cattr);

    pthread_mutexattr_destroy(&mattr);
    pthread_condattr_destroy(&cattr);
}

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        fprintf(stderr, "Uso: %s <shm_id> <cap> <archivo>\n", argv[0]);
        fprintf(stderr, "Ej:  %s /mi_espacio 10 input.txt\n", argv[0]);
        return 1;
    }
    const char *shm_name = argv[1];
    uint32_t cap = (uint32_t)strtoul(argv[2], NULL, 10);
    const char *file = argv[3];

    FILE *f = fopen("output.txt", "w");
    if (f)
        fclose(f);

    shm_unlink(shm_name);
    int fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    ftruncate(fd, sizeof(Shared));
    Shared *sh = mmap(NULL, sizeof(Shared), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    initialize_shared_struct(sh, cap);

    if (load_file_into_shm(file, sh) < 0)
    {
        fprintf(stderr, "Error al cargar el archivo en memoria compartida.\n");
        munmap(sh, sizeof(Shared));
        shm_unlink(shm_name);
        return 2;
    }

    printf(ANSI_BOLD ANSI_GREEN "Memoria compartida '%s' inicializada.\n" ANSI_RESET, shm_name);
    munmap(sh, sizeof(Shared));
    return 0;
}