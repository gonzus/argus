#ifndef UTIL_H_
#define UTIL_H_

/*
 * Utility functions.
 */

#include <stdint.h>
#include <string.h>

// Return length of a C array.
#define ALEN(a) (int) ((sizeof(a) / sizeof((a)[0])))

// Return true if two C strings are equal.
#define STREQ(s1, s2) (strcmp(s1, s2) == 0)

// Avoid compilation warnings for unused arguments.
#define UNUSED_ARG(x) (void) x

// Used to print a boolean value
#define BS(b) ((b) != 0 ? "true" : "false")

#define ENSURE_ARRAY(T, p, m, n, i, f) \
do { \
  if (p->n##len >= p->n##cap) { \
    int size = p->n##cap == 0 ? i : p->n##cap * f; \
    T** next = realloc(p->m, sizeof(T*) * size); \
    if (!next) { \
      LOG_FATAL("Could not reallocate array of pointers to %s", #T); \
    } \
    LOG_DEBUG("Engine grow %s %p: %d -> %d", #T, p->n##cap, size); \
    p->m = next; \
    p->n##cap = size; \
  } \
} while (0)


#ifdef __cplusplus
extern "C" {
#endif

// Conversions between units of time.
enum {
  NS_PER_US = 1000,
  US_PER_MS = 1000,
  MS_PER_SEC = 1000,
  US_PER_SEC = US_PER_MS * MS_PER_SEC,
  NS_PER_MS = NS_PER_US * US_PER_MS,
  NS_PER_SEC = NS_PER_MS * MS_PER_SEC,
};

// Parse a boolean string into an integer
int parse_boolean(const char* str,  int len, int deflt);

// Parse an integer string into an integer
int parse_int(const char* str, int len, int deflt);

// Parse a double string into an integer
double parse_double(const char* str, int len, double deflt);

// Get current timestamps in the desired unit.
// Guaranteed to be monotonically increasing, NTP notwithstanding.
uint64_t stamp_ns(void);
uint64_t stamp_us(void);
uint32_t stamp_ms(void);

// Pause (sleep) the execution of the current thread for some us.
void pause_us(uint32_t us);

// Get localtime()'s idea of current year / month / day / hour / minute / second.
// If any of Y, M, D, h, m, s are 0, don't assign that part.
// Return the epoch as returned by time().
unsigned long now(int* Y, int* M, int* D, int* h, int* m, int* s);

// Check and create a given directory.
// Return 1 if directory exists after calling the function.
int ensure_dir(const char* path);

#ifdef __cplusplus
}
#endif

#endif
