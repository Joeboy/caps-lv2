#include <stddef.h>
#include <string.h>

extern "C" {
void *__dso_handle __attribute__((visibility("hidden"))) = 0;
extern void *picolv2_alloc(size_t size, size_t align);
extern void picolv2_dealloc(void *ptr, size_t size, size_t align);
extern void picolv2_log_write(const char *ptr, int len);

int _write(int file, char *ptr, int len) {
    (void)file;
    picolv2_log_write(ptr, len);
    return len;
}
}

#define PICO_ALLOC_ALIGN (sizeof(max_align_t))

typedef struct {
    size_t size;
} pico_alloc_header;

extern "C" void *malloc(size_t size) {
    if (size == 0) {
        size = 1;
    }
    size_t total = size + sizeof(pico_alloc_header);
    if (total < size) {
        return NULL;
    }
    void *raw = picolv2_alloc(total, PICO_ALLOC_ALIGN);
    if (!raw) {
        return NULL;
    }
    ((pico_alloc_header *)raw)->size = total;
    return (char *)raw + sizeof(pico_alloc_header);
}

extern "C" void free(void *ptr) {
    if (!ptr) {
        return;
    }
    pico_alloc_header *header = (pico_alloc_header *)((char *)ptr - sizeof(pico_alloc_header));
    picolv2_dealloc(header, header->size, PICO_ALLOC_ALIGN);
}

extern "C" void *calloc(size_t count, size_t size) {
    if (count != 0 && size > (size_t)-1 / count) {
        return NULL;
    }
    size_t total = count * size;
    void *ptr = malloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

extern "C" void *realloc(void *ptr, size_t size) {
    if (!ptr) {
        return malloc(size);
    }
    if (size == 0) {
        free(ptr);
        return NULL;
    }
    pico_alloc_header *header = (pico_alloc_header *)((char *)ptr - sizeof(pico_alloc_header));
    size_t old_size = header->size - sizeof(pico_alloc_header);
    void *new_ptr = malloc(size);
    if (new_ptr) {
        memcpy(new_ptr, ptr, old_size < size ? old_size : size);
        free(ptr);
    }
    return new_ptr;
}

void *operator new(unsigned int size) {
    return malloc(size);
}

void operator delete(void *pointer) noexcept {
    free(pointer);
}

void operator delete(void *pointer, unsigned int size) noexcept {
    (void)size;
    free(pointer);
}

void *operator new[](unsigned int size) {
    return malloc(size);
}

void operator delete[](void *pointer) noexcept {
    free(pointer);
}

void operator delete[](void *pointer, unsigned int size) noexcept {
    (void)size;
    free(pointer);
}

extern "C" {
int __cxa_guard_acquire(unsigned char *guard) {
    return *guard == 0;
}

void __cxa_guard_release(unsigned char *guard) {
    *guard = 1;
}

int atexit(void (*callback)()) {
    (void)callback;
    return 0;
}

void _fini() {}

void _ZSt20__throw_length_errorPKc(const char *message) {
    (void)message;
    __builtin_trap();
}
}
