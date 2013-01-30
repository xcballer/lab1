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
evaluateTree (command_t com);

int
command_status (command_t c)
{
  return c->status;
}

void
execute_command (command_t c, bool time_travel)
{
  /* I had to fork here because I need a way to reset the file descriptor
    tables for each complete command. Otherwise file descriptors will carry
    over and I/O will be wrong.*/
  if (!time_travel)
  {
    pid_t child = fork();
    if(child == -1)
      error(1,0,"Error creating child");
     else if(child == 0)
     {
      int status = evaluateTree(c);
       _exit(status);
     }
    else
    {
      int temp;
      wait(&temp);
      c->status = WEXITSTATUS(temp);
    }
  } 
}

int
evaluateTree (command_t com)
{
  int left;
  int right;
  int pipefd[2];
  pid_t pid;
  int out;
  int in;
  int returnStatus;

  switch (com->type)
  {
    case AND_COMMAND:// ------------------------------------------- AND_COMMAND
    {
      left = evaluateTree(com->u.command[0]);
      if(left == 0)
        return evaluateTree(com->u.command[1]);
      return left; 
    }

    case SEQUENCE_COMMAND: // ------------------------------- SEQUENCE_COMMAND
    {
      evaluateTree(com->u.command[0]);
      return evaluateTree(com->u.command[1]);
    }
    case OR_COMMAND:
    {
      left = evaluateTree(com->u.command[0]);
      if(left != 0)
        return evaluateTree(com->u.command[1]);
      return left;
    }
    case PIPE_COMMAND: // --------------------------------------- PIPE_COMMAND
    {
      if(pipe(pipefd) == -1)
        error(1,0,"Pipe Error");

      pid = fork();
      if(pid == -1)
        error(1,0,"Error during fork()");
      else if(pid == 0) /*In Child*/
      {
        if(dup2(pipefd[1],1) == -1)
          error(1,0,"Error using dup2()");
        if(close(pipefd[0]) == -1)
          error(1,0,"Error closing file descriptor");
        if(close(pipefd[1]) == -1)
          error(1,0,"Error closing file descriptor");
        left = evaluateTree(com->u.command[0]);
        _exit(left);
      }
      else /* In Parent */
      {
        if(dup2(pipefd[0],0) == -1)
          error(1,0,"Error using dup2()");
        if(close(pipefd[1]) == -1)
          error(1,0,"Error closing file descriptor");
        if(close(pipefd[0]) == -1)
          error(1,0,"Error closing file descriptor");
        right = evaluateTree(com->u.command[1]);
        return right;
      }
    }
    case SIMPLE_COMMAND: // ----------------------------------- SIMPLE_COMMAND
    case SUBSHELL_COMMAND:
    {
      pid = fork();
      if(pid == -1)
        error(1,0,"Error During fork()");
      // PARENT
      if (pid != 0)
      {
        wait(&returnStatus);
        return WEXITSTATUS(returnStatus);
      }

      // CHILD
      else
      {
        if (com->output)  // command contains a '>' redirect
        {
          if ((out = open(com->output, O_WRONLY | O_CREAT | O_TRUNC,
                          0x0666)) == -1)
            error(1, 0, "failed to open file");

          if(dup2(out, STDOUT_FILENO) == -1)
            error(1,0,"Error using dup2()");
          if(close(out) == -1)
            error(1,0,"Error closing file descriptor");
        }

        if (com->input)  // command contains a '<' redirect
        {
          if ((in = open(com->input, O_RDONLY)) == -1)
            error(1, 0, "failed to open file");

          if(dup2(in, STDIN_FILENO) == -1)
            error(1,0,"Error using dup2()");
          if(close(in) == -1)
            error(1,0,"Error closing file descriptor");
        }

        if(com->type == SIMPLE_COMMAND)
        {
          execvp(com->u.word[0], com->u.word);
          error(1, 0, "execvp returned");
        }
        else
        {
          int my_result = evaluateTree(com->u.subshell_command);
          _exit(my_result);
        }
      }
    }
    default:
      error(1, 0, "unrecognized command type");
  }
  return 1; // To avoid warning flag
}
