/*
  This is a scaffolding implementation of simple argument parsing.  Copy the file into
  your project, parse types supported out of the box, or implement your own types
  in-place.
*/

#ifndef __FLAG_H
#define __FLAG_H

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#ifndef FLAG_CAP
#define FLAG_CAP 64
#endif

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

struct flag *flag_info(void *value_ptr);
void flag_required(void *value_ptr);
void flag_parse(int argc, char **argv);
void flag_print_options(FILE *stream);




#ifdef FLAG_IMPLEMENTATION

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stddef.h>

struct flag_context {
  struct flag flags[FLAG_CAP];
  int flags_count;
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


static char *flag_shift_args(int argc[static 1], char **argv[static 1])
{
  assert(*argc > 0);
  char *result = **argv;
  *argv += 1;
  *argc -= 1;
  return result;
}


static void flag_parse_stripped_flag(int argc[static 1], char **argv[static 1], char *flag, struct flag *f)
{
  f->set_by_user = true;

  static_assert(FLAG_TYPE_COUNT == 3, "Handle all FLAG_TYPES.");
  switch (f->type) {
  case FLAG_TYPE_BOOL: {
    f->value.as_bool = true;
  } break;
  case FLAG_TYPE_STR: {
    if (*argc < 1) {
      fprintf(stderr, "Error parsing arguments. Expected string value to flag '%s'.\n", flag);
      exit(1);
    }
    char *str_val = flag_shift_args(argc, argv);
    f->value.as_str = str_val;
  } break;
  case FLAG_TYPE_INT64: {
    if (*argc < 1) {
      fprintf(stderr, "Error parsing arguments. Expected number value to flag '%s'.\n", flag);
      exit(1);
    }
    char *number_str = flag_shift_args(argc, argv);

    errno = 0;
    char *endptr;
    int64_t v = strtoll(number_str, &endptr, 0);
    if (errno != 0) {
      perror("strtol");
      exit(1);
    }

    if (endptr == number_str) {
      fprintf(stderr, "Error parsing the number '%s' of flag '%s'.\n", number_str, flag);
      exit(1);
    }

    f->value.as_int64 = v;
  } break;
  default: assert(0 && "unreachable");
  }
}

struct flag *flag_info(void *value_ptr)
{
  assert(value_ptr);
  return (struct flag *)(value_ptr - offsetof(struct flag, value));
}

void flag_required(void *value_ptr)
{
  assert(value_ptr);
  struct flag* f = flag_info(value_ptr);
  f->is_required = true;
}

void flag_parse(int argc, char **argv)
{
  flag_shift_args(&argc, &argv); // skip exe name

  while (argc > 0) {
    char *flag = flag_shift_args(&argc, &argv);
    bool found = false;

    // long names
    if (flag[0] == '-' && flag[1] == '-') {
      flag += 2; // skip dashes

      for (int i = 0; i < g_flag_ctx.flags_count && !found; i += 1) {
        struct flag *f = &g_flag_ctx.flags[i];

        assert(f->name);
        if (strcmp(flag, f->name) == 0) {
          found = true;
          flag_parse_stripped_flag(&argc, &argv, flag, f);
        }
      }
    }
    // short names
    else if (flag[0] == '-') {
      flag += 1; // skip dash
      for (int i = 0; i < g_flag_ctx.flags_count && !found; i += 1) {
        struct flag *f = &g_flag_ctx.flags[i];

        if (f->name_short && strcmp(flag, f->name_short) == 0) {
          found = true;
          flag_parse_stripped_flag(&argc, &argv, flag, f);
        }
      }
    }

    if (!found) {
      fprintf(stderr, "Warning: unexpected argument '%s'\n", flag);
    }
  }

  for (int i = 0; i < g_flag_ctx.flags_count; i += 1) {
    struct flag *f = &g_flag_ctx.flags[i];
    if (f->is_required && !f->set_by_user)
    {
      fprintf(stderr, "Error: Required flag '%s' not set.\n", f->name);
      exit(1);
    }
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

    default: assert(0 && "unreachable");
    }

    fprintf(stream, " ]\n");
  }
}

#endif // FLAG_IMPLEMENTATIONS

#endif // __FLAG_H
