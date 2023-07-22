#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define FLAG_IMPLEMENTATION
#include "flag.h"

void usage(FILE *stream)
{
  fprintf(stream, "Usage: example [OPTIONS] FILE\n");
  fprintf(stream, "Options:\n");
  flag_print_options(stream);
  printf("\n");
}

int main(int argc, char *argv[])
{
  //                              name, short, default, description
  bool     *bool_flag = flag_bool("bool", "b", false, "This is a boolean flag of the form '--bool' or '-b'");
  char    **str_flag  = flag_str ("str",  "s", 0,     "This is a string flag of the form '--str <str>' or '-s <str>'");
  int64_t  *int_flag  = flag_int64("int", "i", 69,    "This is a int flag of the form '--int <number>' or '-i <number>'");
  bool     *help      = flag_bool("help", "h", false, "Show help menu.");

  bool ok = flag_parse(argc, argv);
  if (!ok) {
    flag_print_error(stderr);
    exit(1);
  }

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

  // Handle positional argument
  if (flag_pargs_n() < 1) {
    fprintf(stderr, "Error: Expected FILE argument.\n");
    exit(1);
  }
  char *file_path = flag_pargs(0);
  printf("log: positional FILE is is %s\n", file_path);

  for (int i = 1; i < flag_pargs_n(); i += 1) {
    printf("log: positional argument %d is %s\n", i, flag_pargs(i));
  }

  putchar('\n');

  return 0;
}
