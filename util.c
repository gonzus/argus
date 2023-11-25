#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "log.h"
#include "util.h"

int parse_boolean(const char* str,  int len, int deflt) {
  if (!str) {
    return deflt;
  }
  if (len == 0) {
    len = strlen(str);
  }
  if (len == 0) {
    return deflt;
  }

  return (0
      || str[0] == 't'
      || str[0] == 'T'
      || str[0] == 'y'
      || str[0] == 'Y'
      || str[0] == '1'
      );
}

int parse_int(const char* str, int len, int deflt) {
  if (!str) {
    return deflt;
  }
  if (len == 0) {
    len = strlen(str);
  }
  if (len == 0) {
    return deflt;
  }

  char* after = 0;
  int val = (int) strtol(str, &after, 10);
  int size = after - str;
  if (size == 0 || size > len) {
    LOG_INFO("Cannot parse [%s] as integer, using default %d", str, deflt);
    val = deflt;
  }
  return val;
}

double parse_double(const char* str, int len, double deflt) {
  if (!str) {
    return deflt;
  }
  if (len == 0) {
    len = strlen(str);
  }
  if (len == 0) {
    return deflt;
  }

  char* after = 0;
  double val = (double) strtod(str, &after);
  int size = after - str;
  if (size == 0 || size > len) {
    LOG_INFO("Cannot parse [%s] as double, using default %f", str, deflt);
    val = deflt;
  }
  return val;
}

uint64_t stamp_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  uint64_t stamp = (uint64_t) ts.tv_sec * NS_PER_SEC + (uint64_t) ts.tv_nsec;
  return stamp;
}

uint64_t stamp_us(void) {
  uint64_t stamp = stamp_ns() / NS_PER_US;
  return stamp;
}

uint32_t stamp_ms(void) {
  uint32_t stamp = stamp_ns() / NS_PER_MS;
  return stamp;
}

void pause_us(uint32_t us) {
  struct timespec wanted = {0, 0};
  if (us >= US_PER_SEC) {
    wanted.tv_sec = us / US_PER_SEC;
    wanted.tv_nsec = us % US_PER_SEC;
  } else {
    wanted.tv_nsec = us;
  }
  wanted.tv_nsec *= NS_PER_US;
  while (1) {
    struct timespec left = {0};
    int rc = nanosleep(&wanted, &left);
    if (rc == 0) break;
    if (errno != EINTR) break;
    wanted = left;
  }
}

unsigned long now(int* Y, int* M, int* D, int* h, int* m, int* s) {
  time_t seconds = time(0);
  struct tm* local = localtime(&seconds);

  if (Y) *Y = local->tm_year + 1900;
  if (M) *M = local->tm_mon  + 1;
  if (D) *D = local->tm_mday;

  if (h) *h = local->tm_hour;
  if (m) *m = local->tm_min;
  if (s) *s = local->tm_sec;

  return seconds;
}

int ensure_dir(const char* path) {
  int ok = 0;
  do {
    struct stat st;
    int rc = stat(path, &st);
    if (rc < 0) {
      if (errno != ENOENT) {
        // stat failed but path apparently exists
        LOG_WARN("Could not determine existence of directory [%s]", path);
      } else {
        // stat failed because path does not exist
        mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
        rc = mkdir(path, mode);
        if (rc < 0) {
          LOG_WARN("Could not create directory [%s]", path);
        } else {
          ok = 1;
          LOG_INFO("Created missing directory [%s]", path);
        }
      }
    } else {
      if (st.st_mode & S_IFDIR) {
        ok = 1;
        LOG_INFO("Directory [%s] exists and is a directory", path);
      } else {
        LOG_WARN("Directory [%s] exists but is not a directory", path);
      }
    }
  } while (0);
  return ok;
}
