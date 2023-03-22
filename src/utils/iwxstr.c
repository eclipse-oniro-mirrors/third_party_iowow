#include "iwxstr.h"
#include "iwlog.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <errno.h>

// Default IWXSTR initial size
#ifndef IWXSTR_AUNIT
#define IWXSTR_AUNIT 16
#endif

struct _IWXSTR {
  char  *ptr;     /**< Data buffer */
  size_t size;    /**< Actual data size */
  size_t asize;   /**< Allocated buffer size */
  void   (*user_data_free_fn)(void*);
  void  *user_data;
};

IWXSTR* iwxstr_new2(size_t siz) {
  if (!siz) {
    siz = IWXSTR_AUNIT;
  }
  IWXSTR *xstr = malloc(sizeof(*xstr));
  if (!xstr) {
    return 0;
  }
  xstr->ptr = malloc(siz);
  if (!xstr->ptr) {
    free(xstr);
    return 0;
  }
  xstr->user_data = 0;
  xstr->user_data_free_fn = 0;
  xstr->size = 0;
  xstr->asize = siz;
  xstr->ptr[0] = '\0';
  return xstr;
}

IWXSTR* iwxstr_new(void) {
  return iwxstr_new2(IWXSTR_AUNIT);
}

IWXSTR* iwxstr_wrap(const char *buf, size_t size) {
  IWXSTR *xstr = iwxstr_new2(size + 1);
  if (!xstr) {
    return 0;
  }
  if (iwxstr_cat(xstr, buf, size)) {
    iwxstr_destroy(xstr);
    return 0;
  }
  return xstr;
}

void iwxstr_destroy(IWXSTR *xstr) {
  if (!xstr) {
    return;
  }
  if (xstr->user_data_free_fn) {
    xstr->user_data_free_fn(xstr->user_data);
  }
  free(xstr->ptr);
  free(xstr);
}

void iwxstr_destroy_keep_ptr(IWXSTR *xstr) {
  if (!xstr) {
    return;
  }
  if (xstr->user_data_free_fn) {
    xstr->user_data_free_fn(xstr->user_data);
  }
  free(xstr);
}

void iwxstr_clear(IWXSTR *xstr) {
  assert(xstr);
  xstr->size = 0;
  xstr->ptr[0] = '\0';
}

iwrc iwxstr_cat(IWXSTR *xstr, const void *buf, size_t size) {
  size_t nsize = xstr->size + size + 1;
  if (xstr->asize < nsize) {
    while (xstr->asize < nsize) {
      xstr->asize <<= 1;
      if (xstr->asize < nsize) {
        xstr->asize = nsize;
      }
    }
    char *ptr = realloc(xstr->ptr, xstr->asize);
    if (!ptr) {
      return IW_ERROR_ERRNO;
    }
    xstr->ptr = ptr;
  }
  memcpy(xstr->ptr + xstr->size, buf, size);
  xstr->size += size;
  xstr->ptr[xstr->size] = '\0';
  return IW_OK;
}

iwrc iwxstr_set_size(IWXSTR *xstr, size_t size) {
  size_t nsize = size + 1;
  if (xstr->asize < nsize) {
    while (xstr->asize < nsize) {
      xstr->asize <<= 1;
      if (xstr->asize < nsize) {
        xstr->asize = nsize;
      }
    }
    char *ptr = realloc(xstr->ptr, xstr->asize);
    if (!ptr) {
      return IW_ERROR_ERRNO;
    }
    xstr->ptr = ptr;
  }
  xstr->size = size;
  return IW_OK;
}

iwrc iwxstr_cat2(IWXSTR *xstr, const char *buf) {
  return buf ? iwxstr_cat(xstr, buf, strlen(buf)) : 0;
}

iwrc iwxstr_unshift(IWXSTR *xstr, const void *buf, size_t size) {
  size_t nsize = xstr->size + size + 1;
  if (xstr->asize < nsize) {
    while (xstr->asize < nsize) {
      xstr->asize <<= 1;
      if (xstr->asize < nsize) {
        xstr->asize = nsize;
      }
    }
    char *ptr = realloc(xstr->ptr, xstr->asize);
    if (!ptr) {
      return IW_ERROR_ERRNO;
    }
    xstr->ptr = ptr;
  }
  if (xstr->size) {
    // shift to right
    memmove(xstr->ptr + size, xstr->ptr, xstr->size);
  }
  memcpy(xstr->ptr, buf, size);
  xstr->size += size;
  xstr->ptr[xstr->size] = '\0';
  return IW_OK;
}

void iwxstr_shift(IWXSTR *xstr, size_t shift_size) {
  if (shift_size == 0) {
    return;
  }
  if (shift_size > xstr->size) {
    shift_size = xstr->size;
  }
  if (xstr->size > shift_size) {
    memmove(xstr->ptr, xstr->ptr + shift_size, xstr->size - shift_size);
  }
  xstr->size -= shift_size;
  xstr->ptr[xstr->size] = '\0';
}

void iwxstr_pop(IWXSTR *xstr, size_t pop_size) {
  if (pop_size == 0) {
    return;
  }
  if (pop_size > xstr->size) {
    pop_size = xstr->size;
  }
  xstr->size -= pop_size;
  xstr->ptr[xstr->size] = '\0';
}

static iwrc iwxstr_vaprintf(IWXSTR *xstr, const char *format, va_list va) {
  iwrc rc = 0;
  char buf[1024];
  va_list cva;
  va_copy(cva, va);
  char *wp = buf;
  int len = vsnprintf(wp, sizeof(buf), format, va);
  if (len >= sizeof(buf)) {
    RCA(wp = malloc(len + 1), finish);
    len = vsnprintf(wp, len + 1, format, cva);
  }
  rc = iwxstr_cat(xstr, wp, len);

finish:
  va_end(cva);
  if (wp != buf) {
    free(wp);
  }
  return rc;
}

iwrc iwxstr_printf(IWXSTR *xstr, const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  iwrc rc = iwxstr_vaprintf(xstr, format, ap);
  va_end(ap);
  return rc;
}

char* iwxstr_ptr(IWXSTR *xstr) {
  return xstr->ptr;
}

size_t iwxstr_size(IWXSTR *xstr) {
  return xstr->size;
}

size_t iwxstr_asize(IWXSTR *xstr) {
  return xstr->asize;
}

void iwxstr_user_data_set(IWXSTR *xstr, void *data, void (*free_fn)(void*)) {
  if (xstr->user_data_free_fn) {
    xstr->user_data_free_fn(xstr->user_data);
  }
  xstr->user_data = data;
  xstr->user_data_free_fn = free_fn;
}

void* iwxstr_user_data_get(IWXSTR *xstr) {
  return xstr->user_data;
}

void* iwxstr_user_data_detach(IWXSTR *xstr) {
  xstr->user_data_free_fn = 0;
  return xstr->user_data;
}
