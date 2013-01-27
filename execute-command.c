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

  if (!time_travel)
    c->status = evaluateTree(c); 
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
    case AND_COMMAND:
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
    case PIPE_COMMAND:
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
        left = evaluateTree(com->u.command[0]);
        if(close(pipefd[1]) == -1)
		  error(1,0,"Error closing file descriptor");
        _exit(left);
      }
      else /* In Parent */
      {
        if(dup2(pipefd[0],0) == -1)
		  error(1,0,"Error using dup2()");
        if(close(pipefd[1]) == -1)
		  error(1,0,"Error closing file descriptor");
        right = evaluateTree(com->u.command[1]);
        if(close(pipefd[0]) == -1)
		  error(1,0,"Error closing file descriptor");
        return right;
      }	
    }
    case SIMPLE_COMMAND: // ----------------------------------- SIMPLE_COMMAND
    {
      pid = fork();
      if(pid == -1)
        error(1,0,"Error During fork()");
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
		  close(out);
        }

        if (com->input)  // command contains a '<' redirect
        {
          if ((in = open(com->input, O_RDONLY)) == -1)
            error(1, 0, "failed to open file");

          dup2(in, STDIN_FILENO);
		  close(in);
        }

        execvp(com->u.word[0], com->u.word);

        error(1, 0, "execvp returned");
      }
    }
    case SUBSHELL_COMMAND: // ------------------------------ SUBSHELL_COMMAND
    {
      if (com->output)  // subshell command contains a '>' redirect
      {
        if ((out = open(com->output, O_WRONLY | O_CREAT)) == -1)
          error(1, 0, "failed to open file");
      }

      if (com->input)   // subshell command contains a '<' redirect
      {
        if ((in = open(com->input, O_RDONLY)) == -1)
          error(1, 0, "failed to open file");
      }

      return evaluateTree(com->u.subshell_command);
    }
    default:
      error(1, 0, "unrecognized command type");
  }
  return 1; // To avoid warning flag
}
