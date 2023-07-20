#include "stdbool.h"
#include "stdio.h"
#include "limits.h"
#include <assert.h>

#define FLAG_IMPLEMENTATION
#include "../flag.h"

#include "./argv-fuzz-inl.h"

void usage(FILE *stream)
{
  fprintf(stream, "Usage: example [OPTIONS]\n");
  fprintf(stream, "Options:\n");
  flag_print_options(stream);
  printf("\n");
}

int main(int argc, char *argv[])
{
  AFL_INIT_ARGV();

  bool     *bool_flag = flag_bool("bool", "b", false, "This is a boolean flag of the form '--bool' or '-b'");
  bool     *bool_null_flag = flag_bool("bool_nul", 0, true, "desc");
  char    **str_flag  = flag_str ("str",  "s", "default string",     "This is a string flag of the form '--str <str>' or '-s <str>'");
  char    **str_nul_flag  = flag_str ("str_nul", 0, 0,     "This is a string flag of the form '--str <str>' or '-s <str>'");
  int64_t  *int_flag  = flag_int64("int", "i", 69,    "This is a int flag of the form '--int <number>' or '-i <number>'");
  bool     *help      = flag_bool("help", "h", false, "Show help menu.");

  flag_parse(argc, argv);
  if (*help) {
    usage(stdout);
    exit(0);
  }

  // Read flag value directly by pointer
  if (*bool_flag) {
    printf("log: Got bool: '%s'\n", *bool_flag ? "true" : "false");
  }
  if (*str_flag) {
    printf("log: Got string: '%s'\n", *str_flag);
  }

  // Get additional information about the flag
  struct flag *int_flag_info = flag_info(int_flag);
  if (int_flag_info->set_by_user) {
    printf("log: argument to flag '%s' explicitly given by user: '%ld'\n", int_flag_info->name, *int_flag);
  }

  // Iterate over positional argument
  for (int i = 0; i < flag_pargs_n(); i += 1) {
    printf("log: positional %d is %s\n", i, flag_pargs(i));
  }

  putchar('\n');

  return 0;
}
