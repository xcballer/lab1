// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <errno.h>
#include <string.h>
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
  // --------------------------------------------------------- TIME TRAVEL MODE
  else
  {
    // If this is the first command we're executing, get ready to catch 
    //  SIGCHLD signals
    if (c->index == 0)
    {
      if (signal(SIGCHLD, handleChildExit) == SIG_ERR)
        error(1,0,"%s",strerror(errno));
    }

    // if we can actually start this command running, do it
    if (list_peek(gcs->depLists[c->index]) == READY_TO_RUN)
    {
      forkCommandProcess(c);
    }

    // If we're "executing" the last command...
    if (c->index == (gcs->numCommands-1))
    {
      // MASTER LOOP .................... wait for everything to run //
      int ii;
      int jj;
      bool done;
      while(1)
      {
        done = true;
        for (ii = 0; ii < gcs->numCommands; ii++)
        {
          // for any child command that exited within the last iteration of the
          //  master loop, remove it from dependency lists.  (removal is not
          //  safe to do inside signal handler)
          if (gcs->pidList[ii] == JUST_EXITED)
          {
            // Note: commands can only depend on a command BEFORE it
            for (jj = ii+1; jj < gcs->numCommands; jj++)
            {
              list_remove(gcs->depLists[jj], ii);
            }
            gcs->pidList[ii] = EXITED_REMOVED_FROM_DEPLISTS;
          }
          else if (gcs->pidList[ii] != EXITED_REMOVED_FROM_DEPLISTS)
          {
            done = false;
          }

          // if any child is ready to run, run it!
          if (list_peek(gcs->depLists[ii]) == READY_TO_RUN)
          {
            forkCommandProcess(gcs->commands[ii]);
          }

        }
        // if everything has exited, we're done!
        if (done)
          return;
      }
    }
    return;
  }
}

void
handleChildExit(int signum)
{
  if (signum != SIGCHLD)
    error(1,0,"Wrong signal. This should never happen.");

  int pid;
  int status;

  if ((pid = wait(&status)) == -1)
    error(1,0,"%s",strerror(errno));


  if (WIFEXITED(status))
  {
    int comIndex;

    // find this pid in the pidLists
    for (comIndex = 0; comIndex < gcs->numCommands; comIndex++)
    {
      if (gcs->pidList[comIndex] == pid)
      {
        // now we've found the exited process, mark it as exited
        gcs->pidList[comIndex] = JUST_EXITED;
        break;
      }
    }

    // save the exit status
    gcs->commands[comIndex]->status = WEXITSTATUS(status);
  }

  return;
}
    





void
forkCommandProcess(command_t c)
{
  pid_t child = fork();

  if(child == -1)
  {
    error(1,0,"Error creating child");
  }
  else if(child == 0)  // in child
  {
    int status = evaluateTree(c);
    _exit(status);
  }
  else                 // in parent
  { 
    // add child's pid to the pidList
    gcs->pidList[c->index] = child;
    list_push(gcs->depLists[c->index], EXECUTION_STARTED);
    // parent's work is done here.  child will remain until finished
    return;
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
