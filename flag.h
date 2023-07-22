/*
  This is a scaffolding implementation of simple argument parsing.  Copy the file into
  your project, parse types supported out of the box, or implement your own types
  in-place.
*/

#ifndef __FLAG_H
#define __FLAG_H

#include <stdint.h>
#include <stdbool.h>

#ifndef FLAG_CAP
#define FLAG_CAP 64
#endif

#ifndef FLAG_ASSERT
#define FLAG_ASSERT(expr) assert((expr));
#define FLAG_STATIC_ASSERT(expr, msg) static_assert((expr), (msg));
#endif


enum FLAG_ERROR {
  FLAG_ERROR_NONE,
  FLAG_ERROR_MISSING_VALUE,
  FLAG_ERROR_MISSING_REQUIRED_FLAG,
  FLAG_ERROR_INVALID_INT64,
  FLAG_ERROR_OVERFLOW_INT64,
  FLAG_ERROR_UNDERFLOW_INT64,
  FLAG_ERROR_TOO_MANY_PARGS,
  FLAG_ERROR_COUNT,
};

struct flag_error {
  enum FLAG_ERROR code;
  const char *flag;
  union {
    struct { char *number_str; } int64;
  };
};

enum FLAG_TYPE {
  FLAG_TYPE_BOOL,
  FLAG_TYPE_STR,
  FLAG_TYPE_INT64,

  FLAG_TYPE_COUNT,
};

static_assert(FLAG_TYPE_COUNT == 3, "Handle all FLAG_TYPES");
union FLAG_VALUE {
  bool    as_bool;
  char   *as_str;
  int64_t as_int64;
};

struct flag {
  enum FLAG_TYPE type;
  const char *name;
  const char *name_short;
  const char *description;
  bool      is_required;
  bool      set_by_user;
  union FLAG_VALUE value;
  union FLAG_VALUE default_value;
};

bool    *flag_bool (const char *name, const char *name_short, bool default_value, const char *description);
char   **flag_str  (const char *name, const char *name_short, const char *default_value, const char *description);
int64_t *flag_int64(const char *name, const char *name_short, int64_t default_value, const char *description);

void flag_required(void *value_ptr);
struct flag *flag_info(void *value_ptr);

int   flag_pargs_n();    // Get the the number of arguments after parsing named arguments
char *flag_pargs(int i); // Get i'th argument after parsing named arguments (positional arguments)

bool flag_parse(int argc, char **argv);

#if __STDC_HOSTED__ == 1
void flag_print_options(FILE *stream);
void flag_print_error(FILE *stream);
#endif



#ifdef FLAG_IMPLEMENTATION

struct flag_context {
  struct flag flags[FLAG_CAP];
  int flags_count;

  char *positionals[FLAG_CAP];
  int positionals_count;

  struct flag_error error;
};

static struct flag_context g_flag_ctx = {0};

bool *flag_bool(const char *name, const char *name_short, bool default_value, const char *description)
{
  assert(g_flag_ctx.flags_count < FLAG_CAP);
  assert(sizeof(bool) <= sizeof(union FLAG_VALUE));
  struct flag *f = &g_flag_ctx.flags[g_flag_ctx.flags_count++];
  *f = (struct flag){
    .type = FLAG_TYPE_BOOL,
    .name = name,
    .name_short = name_short,
    .description = description,
    .default_value = (union FLAG_VALUE)default_value,
    .value         = (union FLAG_VALUE)default_value,
  };
  return &f->value.as_bool;
}

char **flag_str(const char *name, const char *name_short, const char *default_value, const char *description)
{
  assert(g_flag_ctx.flags_count < FLAG_CAP);
  assert(sizeof(char *) <= sizeof(union FLAG_VALUE));
  struct flag *f = &g_flag_ctx.flags[g_flag_ctx.flags_count++];
  *f = (struct flag){
    .type = FLAG_TYPE_STR,
    .name = name,
    .name_short = name_short,
    .description = description,
    .default_value = (union FLAG_VALUE)(char *)default_value,
    .value         = (union FLAG_VALUE)(char *)default_value,
  };
  return &f->value.as_str;
}

int64_t *flag_int64(const char *name, const char *name_short, int64_t default_value, const char *description)
{
  assert(g_flag_ctx.flags_count < FLAG_CAP);
  assert(sizeof(int64_t) <= sizeof(union FLAG_VALUE));
  struct flag *f = &g_flag_ctx.flags[g_flag_ctx.flags_count++];
  *f = (struct flag){
    .type = FLAG_TYPE_INT64,
    .name = name,
    .name_short = name_short,
    .description = description,
    .default_value = (union FLAG_VALUE)default_value,
    .value         = (union FLAG_VALUE)default_value,
  };
  return &f->value.as_int64;
}

// Return true on match
static bool flag_str_cmp(const char *a, const char *b) {
  assert(a && b);
  for (; *a && *b; a++, b++) {
    if (*a != *b) {
      return false;
    }
  }
  return true;
}

//
// Credit: https://github.com/skeeto/scratch/blob/master/parsers/strtonum.c
//
// Parse the buffer as a base 10 integer. The buffer may contain
// arbitrary leading whitespace ("\t\n\v\f\r "), one optional + or -,
// then any number of digits ("0123456789").
//
// If the result is < min,   returns FLAG_ERROR_UNDERFLOW_INT64,
// If the result is > max,   returns FLAG_ERROR_OVERFLOW_INT64,
// If the buffer is invalid, returns FLAG_ERROR_INVALID_INT64,
// Otherwise returns FLAG_ERROR_NONE and *out is set to the parsed value.
//
static enum FLAG_ERROR flag_str_to_int64(const char *buf, int64_t *out, int64_t min, int64_t max)
{
  assert(buf);
  assert(out);
  assert(min < max);

  // Skip any leading whitespace
  for (; (*buf >= 0x09 && *buf <= 0x0d) || (*buf == 0x20); buf++);

  int64_t mmax, mmin, n = 0;
  switch (*buf) {
  case 0x2b: // +
    buf++; // fallthrough
  case 0x30: case 0x31: case 0x32: case 0x33: case 0x34:
  case 0x35: case 0x36: case 0x37: case 0x38: case 0x39:
    // Accumulate in positive direction, watching for max bound
    mmax = max / 10;
    do {
      int v = *buf++ - 0x30;

      if (v < 0 || v > 9) {
        goto invalid;
      }

      if (n > mmax) {
        goto toolarge;
      }
      n *= 10;

      if (max - v < n) {
        goto toolarge;
      }
      n += v;
    } while (*buf);

    // Still need to check min bound
    if (n < min) {
      goto toosmall;
    }

    *out = n;
    return FLAG_ERROR_NONE;

  case 0x2d: // -
    buf++;
    // Accumulate in negative direction, watching for min bound
    mmin = min / 10;
    do {
      int v = *buf++ - 0x30;

      if (v < 0 || v > 9) {
        goto invalid;
      }

      if (n < mmin) {
        goto toosmall;
      }
      n *= 10;

      if (min + v > n) {
        goto toosmall;
      }
      n -= v;
    } while (*buf);

    // Still need to check max bound
    if (n > max) {
      goto toolarge;
    }

    *out = n;
    return FLAG_ERROR_NONE;
  }

 invalid:
  return FLAG_ERROR_INVALID_INT64;

 toolarge:
  // Skip remaining digits
  for (; *buf >= 0x30 && *buf <= 0x39; buf++);
  if (*buf) goto invalid;
  return FLAG_ERROR_OVERFLOW_INT64;

 toosmall:
  // Skip remaining digits
  for (; *buf >= 0x30 && *buf <= 0x39; buf++);
  if (*buf) goto invalid;
  return FLAG_ERROR_UNDERFLOW_INT64;
}

static char *flag_shift_args(int argc[static 1], char **argv[static 1])
{
  assert(*argc > 0);
  char *result = **argv;
  *argv += 1;
  *argc -= 1;
  return result;
}

static bool flag_parse_stripped_flag(int argc[static 1], char **argv[static 1], char *flag, struct flag *f)
{
  f->set_by_user = true;

  static_assert(FLAG_TYPE_COUNT == 3, "Handle all FLAG_TYPES.");
  switch (f->type) {
  case FLAG_TYPE_BOOL: {
    f->value.as_bool = true;
  } break;
  case FLAG_TYPE_STR: {
    if (*argc < 1) {
      g_flag_ctx.error = (struct flag_error) { .code = FLAG_ERROR_MISSING_VALUE, .flag = flag, };
      return false;
    }
    char *str_val = flag_shift_args(argc, argv);
    f->value.as_str = str_val;
  } break;
  case FLAG_TYPE_INT64: {
    if (*argc < 1) {
      g_flag_ctx.error = (struct flag_error) { .code = FLAG_ERROR_MISSING_VALUE, .flag = flag, };
      return false;
    }
    char *number_str = flag_shift_args(argc, argv);

    int64_t v = 0;
    enum FLAG_ERROR err = flag_str_to_int64(number_str, &v, INT64_MIN, INT64_MAX);
    if (err != FLAG_ERROR_NONE) {
      g_flag_ctx.error = (struct flag_error) { .code = err, .flag = flag, .int64.number_str = number_str, };
      return false;
    }

    f->value.as_int64 = v;
  } break;
  default: assert(0 && "unreachable");
  }

  return true;
}

struct flag *flag_info(void *value_ptr)
{
  assert(value_ptr);
  return (struct flag *)(value_ptr - __builtin_offsetof(struct flag, value));
}

void flag_required(void *value_ptr)
{
  assert(value_ptr);
  struct flag* f = flag_info(value_ptr);
  f->is_required = true;
}

char *flag_pargs(int i)
{
  assert(i < g_flag_ctx.positionals_count);
  return g_flag_ctx.positionals[i];
}

int flag_pargs_n()
{
  return g_flag_ctx.positionals_count;
}

bool flag_parse(int argc, char **argv)
{
  flag_shift_args(&argc, &argv); // skip exe name

  while (argc > 0) {
    char *const argument = flag_shift_args(&argc, &argv);
    char *flag = argument;
    bool found = false;

    // long names
    if (flag[0] == '-' && flag[1] == '-') {
      flag += 2; // skip dashes

      for (int i = 0; i < g_flag_ctx.flags_count && !found; i += 1) {
        struct flag *f = &g_flag_ctx.flags[i];

        assert(f->name);
        if (flag_str_cmp(flag, f->name)) {
          found = true;
          bool ok = flag_parse_stripped_flag(&argc, &argv, flag, f);
          if (!ok) { return false; }
        }
      }
    }
    // short names
    else if (!found && flag[0] == '-') {
      flag += 1; // skip dash
      for (int i = 0; i < g_flag_ctx.flags_count && !found; i += 1) {
        struct flag *f = &g_flag_ctx.flags[i];

        if (f->name_short && flag_str_cmp(flag, f->name_short)) {
          found = true;
          bool ok = flag_parse_stripped_flag(&argc, &argv, flag, f);
          if (!ok) { return false; }
        }
      }
    }

    if (!found) {
      // treat as positional argument
      if (g_flag_ctx.positionals_count < FLAG_CAP) {
        g_flag_ctx.positionals[g_flag_ctx.positionals_count++] = argument;
      }
      else {
        g_flag_ctx.error = (struct flag_error) { .code = FLAG_ERROR_TOO_MANY_PARGS, };
        return false;
      }
    }
  }

  for (int i = 0; i < g_flag_ctx.flags_count; i += 1) {
    struct flag *f = &g_flag_ctx.flags[i];
    if (f->is_required && !f->set_by_user) {
      g_flag_ctx.error = (struct flag_error) { .code = FLAG_ERROR_MISSING_REQUIRED_FLAG, .flag = f->name, };
      return false;
    }
  }

  return true;
}

#if __STDC_HOSTED__ == 1

void flag_print_error(FILE *stream)
{
  struct flag_error e = g_flag_ctx.error;

  static_assert(FLAG_ERROR_COUNT == 7, "Handle all FLAG_ERROR.");
  switch (e.code) {
  case FLAG_ERROR_NONE:
    {
      fprintf(stream, "Internal Error. The developer called an error handling procedure without there being any errors. Fire that guy.\n");
    } break;
  case FLAG_ERROR_MISSING_REQUIRED_FLAG:
    {
      fprintf(stream, "Error. Required flag '%s' not set.\n", e.flag);
    } break;
  case FLAG_ERROR_MISSING_VALUE:
    {
      fprintf(stream, "Error parsing arguments. Expected a value to flag '%s'.\n", e.flag);
    } break;
  case FLAG_ERROR_INVALID_INT64:
    {
      fprintf(stream, "Error parsing the number '%s' of flag '%s'.\n", e.int64.number_str, e.flag);
    } break;
  case FLAG_ERROR_UNDERFLOW_INT64:
    {
      fprintf(stream, "Error parsing the number '%s' of flag '%s'. Underflow.\n", e.int64.number_str, e.flag);
    } break;
  case FLAG_ERROR_OVERFLOW_INT64:
    {
      fprintf(stream, "Error parsing the number '%s' of flag '%s'. Overflow.\n", e.int64.number_str, e.flag);
    } break;
  case FLAG_ERROR_TOO_MANY_PARGS:
    {
      fprintf(stream, "Error: too many positional arguments given.\n");
    } break;
  case FLAG_ERROR_COUNT: assert(0 && "unreachable");
  }
}

void flag_print_options(FILE *stream)
{
  for (int i = 0; i < g_flag_ctx.flags_count; i += 1) {
    struct flag *f = &g_flag_ctx.flags[i];
    fprintf(stream, "  --%s, -%s     %s", f->name, f->name_short, f->description);

    fprintf(stream, "  [ ");
    static_assert(FLAG_TYPE_COUNT == 3, "Handle all FLAG_TYPES.");
    switch (f->type) {
    case FLAG_TYPE_BOOL:
      {
        bool v = f->default_value.as_bool;
        fprintf(stream, "default: %s", v ? "true" : "false");
      } break;
    case FLAG_TYPE_STR:
      {
        char *str = f->default_value.as_str;
        fprintf(stream, "default: '%s'", str);
      } break;
    case FLAG_TYPE_INT64:
      {
        int64_t v = f->default_value.as_int64;
        fprintf(stream, "default: %ld", v);
      } break;
    case FLAG_TYPE_COUNT: assert(0 && "unreachable");
    }
    fprintf(stream, " ]\n");
  }
}

#endif // __STDC_HOSTED__

#endif // FLAG_IMPLEMENTATIONS

#endif // __FLAG_H
