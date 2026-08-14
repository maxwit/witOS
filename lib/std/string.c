#include <types.h>
#include <malloc.h>
#include <string.h>

size_t strlen(const char *src)
{
	const char *iter;

	for (iter = src; *iter; iter++);

	return iter - src;
}

size_t strnlen(const char *src, size_t count)
{
	const char *iter;

	for (iter = src; *iter && count; iter++, count--);

	return iter - src;
}

char *strcpy(char *dst, const char *src)
{
	char *iter = dst;

	while ((*iter = *src) != '\0') {
		iter++;
		src++;
	}

	return dst;
}

char *strncpy(char *dst, const char *src, size_t count)
{
	char *iter = dst;
	size_t n = 0;

	while ((n < count) && (*iter = *src)) {
		iter++;
		src++;
		n++;
	}

	while (n < count) {
		*iter++ = '\0';
		n++;
	}

	return dst;
}

int strcmp (const char *pstr1, const char *pstr2)
{
	while (*pstr1 == *pstr2) {
		if ('\0' == *pstr1)
			return 0;

		pstr1++;
		pstr2++;
	}

	return *pstr1 - *pstr2;
}

int strncmp(const char *pstr1, const char *pstr2, size_t count)
{
	size_t n = 1;

	while (*pstr1 == *pstr2) {
		if (('\0' == *pstr1) || (n == count))
			return 0;

		pstr1++;
		pstr2++;
		n++;
	}

	return *pstr1 - *pstr2;
}

// FIXME!!
int strcasecmp (const char *pstr1, const char *pstr2)
{
	while (*pstr1 == *pstr2) {
		if ('\0' == *pstr1)
			return 0;

		pstr1++;
		pstr2++;
	}

	return *pstr1 - *pstr2;
}

char *strcat(char *dst, const char *src)
{
	char *iter;

	for (iter = dst; *iter; iter++);

	while ((*iter = *src) != '\0') {
		iter++;
		src++;
	}

	return dst;
}

char *strncat(char *dst, const char *src, size_t count)
{
	char *iter;
	size_t n = 0;

	for (iter = dst; *iter; iter++);

	while (n < count && (*iter = *src)) {
		iter++;
		src++;
		n++;
	}

	while (n < count) {
		*iter = '\0';
		iter++;
		n++;
	}

	return dst;
}

// why not use the KMP algo?
char *strstr(const char *haystack, const char *needle)
{
	const char *p, *q;

	p = haystack;
	q = needle;
	while (*p) {
		int i = 0;
		while (q[i] && q[i] == p[i]) i++;
		if (q[i] == '\0')
			return (char *)p;
		p++;
	}

	return NULL;
}

// FIXME
char *strcasestr(const char *haystack, const char *needle)
{
	return NULL;
}

char *strchr(const char *src, int c)
{
	const char *iter;

	for (iter = src; *iter; iter++) {
		if (*iter == c)
			return (char *)iter;
	}

	return NULL;
}

char *strrchr(const char *src, int c)
{
	const char *iter;

	for (iter = src; *iter; iter++);

	while (iter > src) {
		iter--;
		if (*iter == c)
			return (char *)iter;
	}

	return NULL;
}

char *strdup(const char *s)
{
	char *dst;
	size_t size = strlen(s) + 1;

	dst = malloc(size);
	if (dst)
		strcpy(dst, s);

	return dst;
}

void *memcpy(void *dst, const void *src, size_t count)
{
	__u8 *d;
	const __u8 *s;

	d = dst;
	s = src;

	while (count > 0) {
		*d++ = *s++;
		count--;
	}

	return dst;
}

void *memmove(void *dst, const void *src, size_t count)
{
	__u8 *d;
	const __u8 *s;

	if (dst < src) {
		d = dst;
		s = src;

		while (count > 0) {
			*d++ = *s++;
			count--;
		}
	} else {
		d = dst + count;
		s = src + count;

		while (count > 0) {
			*--d = *--s;
			count--;
		}
	}

	return dst;
}

void *memset(void *src, int c, size_t count)
{
	char *s = src;

	while (count) {
		*s = c;

		s++;
		count--;
	}

	return src;
}

int memcmp(const void* dst, const void* src, size_t count)
{
	const __u8 *s, *d;

	d = dst;
	s = src;

	while (count > 0) {
		if (*d != *s)
			return *d - *s;

		s++;
		d++;

		count--;
	}

	return 0;
}

