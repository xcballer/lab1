// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>


int
evaluateTree (command_t com, int writeto, int readfrom);

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

int
command_status (command_t c)
{
  return c->status;
}

void
execute_command (command_t c, bool time_travel)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */

  if (!time_travel)
  {
    c->status = evaluateTree(c, STDOUT_FILENO, STDOUT_FILENO); 
  }
}

int
evaluateTree (command_t com, int writeto, int readfrom)
{
  int pid;
  int out;
  int in;
  int returnStatus;

  switch (com->type)
  {
    case AND_COMMAND:
      break;

    case SEQUENCE_COMMAND: // --------------------------------- SEQUENCE_COMMAND

      evaluateTree(com->u.command[0], writeto, readfrom);
      return evaluateTree(com->u.command[1], writeto, readfrom);
      break;

    case OR_COMMAND:
      break;
    case PIPE_COMMAND:
      break;

    case SIMPLE_COMMAND: // ------------------------------------- SIMPLE_COMMAND
      pid = fork();

      // PARENT
      if (pid != 0)
      {
        wait(&returnStatus);
        return returnStatus;
      }

      // CHILD
      else
      {
        if (com->output)  // command contains a '>' redirect
        {
          if ((out = open(com->output, O_WRONLY | O_CREAT)) == -1)
            error(1, 0, "failed to open file");

          dup2(out, STDOUT_FILENO);
        }
        else              // command does not contain a '>' redirect
        {
          dup2(writeto, STDOUT_FILENO);
        }

        if (com->input)  // command contains a '<' redirect
        {
          if ((in = open(com->input, O_RDONLY)) == -1)
            error(1, 0, "failed to open file");

          dup2(in, STDIN_FILENO);
        }
        else              // command does not contain a '<' redirect
        {
          dup2(readfrom, STDIN_FILENO);
        }

        execvp(com->u.word[0], com->u.word);

        error(1, 0, "execvp returned");
      }
      break;

    case SUBSHELL_COMMAND: // --------------------------------- SUBSHELL_COMMAND

      if (com->output)  // subshell command contains a '>' redirect
      {
        if ((out = open(com->output, O_WRONLY | O_CREAT)) == -1)
          error(1, 0, "failed to open file");
      }
      else              // subshell command does not contain a '>' redirect
      {
        out = writeto;
      }

      if (com->input)   // subshell command contains a '<' redirect
      {
        if ((in = open(com->input, O_RDONLY)) == -1)
          error(1, 0, "failed to open file");
      }
      else              // subshellcommand does not contain a '<' redirect
      {
        in = readfrom;
      }

      return evaluateTree(com->u.subshell_command, out, in);
      break;

    default:
      error(1, 0, "unrecognized command type");
  }

}
