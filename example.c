#include "stdbool.h"
#include "stdio.h"
#include "limits.h"
#include <assert.h>

#define FLAG_IMPLEMENTATION
#include "flag.h"

void usage(FILE *stream) {
  fprintf(stream, "Usage: parse [OPTIONS]\n");
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

  putchar('\n');

  return 0;
}
