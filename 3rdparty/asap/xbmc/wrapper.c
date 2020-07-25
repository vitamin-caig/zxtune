#include <stdio.h>

void *__wrap_malloc(size_t size)
{
	void *dllmalloc(size_t size);
	return dllmalloc(size);
}

void *__wrap_realloc(void *ptr, size_t size)
{
	void *dllrealloc(void *ptr, size_t size);
	return dllrealloc(ptr, size);
}

void __wrap_free(void *ptr)
{
	void dllfree(void *ptr);
	dllfree(ptr);
}

FILE *__wrap_fopen(const char *path, const char *mode)
{
	FILE *dll_fopen(const char *path, const char *mode);
	return dll_fopen(path, mode);
}

size_t __wrap_fread(void *buf, size_t size, size_t count, FILE *stream)
{
	size_t dll_fread(void *buf, size_t size, size_t count, FILE *stream);
	return dll_fread(buf, size, count, stream);
}

int __wrap_fclose(FILE *stream)
{
	int dll_fclose(FILE *stream);
	return dll_fclose(stream);
}
