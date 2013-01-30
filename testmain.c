// UCLA CS 111 Lab 1 main program

#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <stdio.h>

#include "command.h"
#include "command-internals.h"

static char const *program_name;
static char const *script_name;

static int
get_next_byte (void *stream)
{
  return getc (stream);
}

int
main (int argc, char **argv)
{
  list_t listy = list_new();

  list_push(listy, 4);
  printf("peeked %d\n", list_peek(listy));
  list_push(listy, 5);
  printf("peeked %d\n", list_peek(listy));
  list_push(listy, 6);
  printf("peeked %d\n", list_peek(listy));
  list_push(listy, 7);
  printf("peeked %d\n", list_peek(listy));
  list_push(listy, 8);
  printf("peeked %d\n", list_peek(listy));
  list_mark_exec(listy);
  printf("peeked %d\n", list_peek(listy));


  list_remove(listy, 6);
  list_remove(listy, 8);
  list_remove(listy, 10);

  while (*listy)
  {
    printf("%d\n", (*listy)->val);
    *listy = (*listy)->next;
  }
}
