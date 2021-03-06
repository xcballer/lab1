// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"
#include "alloc.h"

#include <error.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

bool is_token(char c)
{
  /*This function checks for tokens that are not alphanumeric*/
  
  switch(c)
  {
    case '!': return true;
    case '%': return true;
    case '+': return true;
    case ',': return true;
    case '-': return true;
    case '.': return true;
    case '/': return true;
    case ':': return true;
    case '@': return true;
    case '^': return true;
    case '_': return true;
    case '#': return true;
  }
  return false;
}
   
bool is_special(char c)
{
  /* This function checks for special tokens that deal with
    commands, piping, or redirection.*/
  switch (c)
  {
   case '|': return true;
   case '&': return true;
   case '(': return true;
   case ')': return true;
   case '<': return true;
   case '>': return true;
  }
  return false;
}

bool is_illegal(char c)
{
  /* This function makes sure that the shell contains tokens that fall
    within the allowable subset. This doesn't apply to comments*/
    
  if((isalnum((unsigned char)c) == 0) && (!is_special(c)) && (c != ';')
    && (!is_token(c)) && (isspace((unsigned char)c) == 0))
    return true;
  return false;
}

void add_char(char** list, int curr, char c)
{
  /* This function adds a character to a current complete command"*/
  
  char* str = list[curr];
  char* tmp = NULL;
  int length = 0;
  if(str == NULL)
  {
    tmp = (char*)checked_realloc(str,sizeof(char)*2);
    tmp[0] = c;
    tmp[1] = '\0';
    list[curr] = tmp;
    return;
  }
  length = strlen(str);
  tmp = (char*)checked_realloc(str,sizeof(char)*(2+length));
  tmp[length] = c;
  tmp[length+1] = '\0';
  list[curr] = tmp;
  return;
}


/* Moved this to command-internals.h so that execute-command.c has access to it
struct command_stream
{
  command_t *commands;

  int curCommand;

  int numCommands;

  list_t *depLists;

  pid_t *pidList;
};
*/

void
make_dep_lists(command_stream_t s);

command_t
read_command_stream (command_stream_t s)
{
  if(s->numCommands == 0)
    return 0; // Or Error?
  if(s->curCommand == s->numCommands)
    return 0;
  int index = s->curCommand;
  s->curCommand += 1;
  return s->commands[index];
}


command_stream_t
make_command_stream (int (*get_next_byte) (void *),
             void *get_next_byte_argument)
{
  char** commandStrings = (char**)checked_malloc(sizeof(char*));
  commandStrings[0] = NULL;
  char** temp = NULL;
  char c;
  char effective_prev = '\0';
  char prev ='\0';
  int count = 1; // Keeps count of complete commands
  bool sub = false; //To see if we are within a subshell
  int open_par = 0;
  int closed_par = 0;
  int curr_line = 1;
  int* line_nums = (int*)checked_malloc(sizeof(int));
  line_nums[0] = 1;
  
  while((c = get_next_byte(get_next_byte_argument)) != EOF)
  {
    if(is_illegal(c))  // Check for illegal tokens
    {
      error(1,0,"%d: Invalid token '%c'.",curr_line,c);
      return 0;
    }
    if((c == ';') && (!sub))
    {
    if(effective_prev =='\n' || effective_prev =='\0')
      error(1,0, "%d: Can't start line with '%c'", curr_line, c);

    if(effective_prev == ';')
      error(1,0, "%d: Too many '%c' tokens", curr_line, c);

      count++;
      temp = (char**)checked_realloc(commandStrings,sizeof(char*)*count);
      commandStrings = temp;
      commandStrings[count-1] = NULL;
      int* temp_int = (int*)checked_realloc(line_nums,sizeof(int)*count);
      line_nums = temp_int;
      prev = c;
      effective_prev = c;
      continue;
    }
    if(c == '#') // Take care of comments be ignoring everything up to newline
    {
      if((is_special(prev)) || (isblank((unsigned char)prev) != 0)  // changed from unsigned char type to char
        || (prev == '\n') ||(prev == '\0'))
      {
        while((c = get_next_byte(get_next_byte_argument)) != EOF)
          if( c == '\n')
            break;
        if(c == EOF)
          break;
        if((prev == '\0') && (c == '\n')) // Case if script starts with #
        {                             //If I don't include this an empty string is made
          prev = c;
          if(isblank((unsigned char)c) == 0)
            effective_prev = c;
          curr_line++;
          continue;
        }
      }
      else
      {
        error(1,0,"Line %d: Invalid use of '#' token.",curr_line);
        return 0;
      }
    }
    if(c != '\n')
    {
      if((isblank((unsigned char)c) != 0) && (effective_prev == '\0'))
        continue;
      // Check that that only paranthesis are preceded by newline(and words of course)
      if(effective_prev == '\n')
      {
        if((c == '|') || (c == '&') || (c == '>') || (c == '<'))
        {
          error(1,0,"%d: Unexpected newline before '%c' token",curr_line,c);
          return 0;
        }
      }
      if(c == '(')
      {
        sub = true;
        open_par++;
      }
      else if(c == ')')
      {
        closed_par++;
        if(open_par == closed_par)
        {
          sub = false;
          open_par = 0;
          closed_par = 0;
        }
      }
      if(((c == '|') && (effective_prev == '&')) ||
        ((c == '&') && (effective_prev == '|')))
      {
        error(1,0,"%d: Invalid AND-OR.",curr_line);
        return 0;
      }
      if(commandStrings[count-1] == NULL)
      {
        line_nums[count-1] = curr_line;
      }
      add_char(commandStrings,count-1,c);
    }
    else
    {
      if((effective_prev == '|') || (effective_prev == '&') || sub)
      {
        prev = c;
        if(isblank((unsigned char)c) == 0)
          effective_prev = c;
        if(commandStrings[count-1] == NULL)
        {
          line_nums[count-1] = curr_line;
        }
        add_char(commandStrings,count-1,c);        
        curr_line++;
        continue;
      }      
      if(prev == '\n' || effective_prev == '\0')
      {
        prev = c;
        if(isblank((unsigned char)c) == 0)
          effective_prev = c;
        curr_line++;
        continue;
      }
      // Gotta also check for < or > cases
      
      if((effective_prev == '<') || (effective_prev == '>'))
      {
        error(1,0,"%d: Cannot end line in < or >",curr_line);
        return 0;
      }
      if(!sub && (prev != ';'))
      {
        count++;
        temp = (char**)checked_realloc(commandStrings,sizeof(char*)*count);
        commandStrings = temp;
        commandStrings[count-1] = NULL;
        int* temp_int = (int*)checked_realloc(line_nums,sizeof(int)*count);
        line_nums = temp_int;
      }
      curr_line++;
    }
    prev = c;
    if(isblank((unsigned char)c) == 0)
      effective_prev = c;
  }
  if((commandStrings[0] == NULL) && count == 1)
    count = 0;
  if(open_par != closed_par)     // Check to see if paranthesis are all good
  {
    error(1,0,"%d: Missing a paranthesis",curr_line);
    return 0;
  }

  if(prev == '\n' && (commandStrings[count-1] == NULL))
    count--;

  command_stream_t stream = stringsToStream(commandStrings, line_nums, count);

  /* Free Data Allocated Dynamically 
  int j;
  for(j = 0; j < count; j++)
    free(commandStrings[j]);
  free(commandStrings);
  free(line_nums); */

  // BUILD READ/WRITE LISTS (add as a member of command_stream_t)
 
  // BUILD DEPENDENCY LISTS (lists of integer indices of commands that must 
  //  finish before this one does) 
  
  stream->pidList = checked_malloc(count * sizeof(pid_t));

  // 0 out all pid's.  This will give them all integer values of NOT_YET_RUN.
  memset(stream->pidList, 0, count * sizeof(pid_t));
  
  make_dep_lists(stream);

  return stream;
}



command_stream_t
stringsToStream(char **commandStrings, int *line_nums, int count)
{
  int ii = 0;


  command_t *commandArray = checked_malloc(sizeof(command_t) * count);


  for (ii = 0; ii < count; ii++) 
  {
    if (commandStrings[ii] == 0)
      continue;
    commandArray[ii] = parse_command_string(commandStrings[ii], line_nums[ii]);
  }

  command_stream_t commandStream = (command_stream_t) checked_malloc(sizeof(struct command_stream));

  commandStream->commands = commandArray;
  commandStream->curCommand = 0;
  commandStream->numCommands = count;

  return commandStream;
}

command_t construct_command (enum command_type type, char *input, char *output,
    void *u0, void *u1)
{
  command_t ptr = (command_t) checked_malloc(sizeof(struct command));

  ptr->type = type;
  ptr->input = input;
  ptr->output = output;

  switch (type)
  {
    case AND_COMMAND:
    case SEQUENCE_COMMAND:
    case OR_COMMAND:
    case PIPE_COMMAND:
      ptr->u.command[0] = u0;
      ptr->u.command[1] = u1;
      break;

    case SIMPLE_COMMAND:
      ptr->u.word = u0;
      break;

    case SUBSHELL_COMMAND:
      ptr->u.subshell_command = u0;
      break;
  }
  return ptr;
}

void dump_command (command_t com)
{
  printf("COMMAND_T at 0x%p --------------\n", com);

  switch (com->type)
  {
    case     AND_COMMAND:
      printf("type: AND\n");
      printf("status: %d\n", com->status);
      printf("input: %s\n", com->input);
      printf("output: %s\n", com->output);
      printf("commands at: 0x%p and 0x%p\n", com->u.command[0], com->u.command[1]);
      break;

    case     SEQUENCE_COMMAND:
      printf("type: SEQUENCE\n");
      printf("status: %d\n", com->status);
      printf("input: %s\n", com->input);
      printf("output: %s\n", com->output);
      printf("commands at: 0x%p and 0x%p\n", com->u.command[0], com->u.command[1]);
      break;

    case     OR_COMMAND:
      printf("type: OR\n");
      printf("status: %d\n", com->status);
      printf("input: %s\n", com->input);
      printf("output: %s\n", com->output);
      printf("commands at: 0x%p and 0x%p\n", com->u.command[0], com->u.command[1]);
      break;

    case     PIPE_COMMAND:
      printf("type: PIPE\n");
      printf("status: %d\n", com->status);
      printf("input: %s\n", com->input);
      printf("output: %s\n", com->output);
      printf("commands at: 0x%p and 0x%p\n", com->u.command[0], com->u.command[1]);
      break;

    case     SIMPLE_COMMAND:
      printf("type: SIMPLE\n");
      printf("status: %d\n", com->status);
      printf("input: %s\n", com->input);
      printf("output: %s\n", com->output);
      printf("words are:\n");
      print_words((char **) com->u.word);
      break;

    case     SUBSHELL_COMMAND:
      printf("type: SUBSHELL\n");
      printf("status: %d\n", com->status);
      printf("input: %s\n", com->input);
      printf("output: %s\n", com->output);
      printf("command at: 0x%p\n", com->u.subshell_command);
      break;
  }
  printf("\n");
}

void print_words(char **words)
{
  while (*words != NULL)
  {
    fprintf(stderr, "  %s\n", *words);
    words++;
  }
}

command_t parse_command_string (char *comString, int line)
{
  char *ii = comString;
  int endln = line;


  while (*ii)
  {
    if (*ii == '\n')
      endln++;
    ii++;
  }

  com_text_t comText = 
  {
    .start = comString,
    .end = ii-1,
    .startln = line,
    .endln = endln
  };

  return parse_ao_string(*strip_text(&comText));
}

command_t parse_ao_string (com_text_t comText)
{
  char *ii = comText.end;
  bool haveWord = false;
  int currln = comText.endln;
  int subDepth = 0; 

  for ( ; ii >= comText.start; ii--)
  {
//  fprintf(stderr, "GOT HERE!");
    if (*ii == '\n') currln--;

    if (*ii == ')')
    {
      subDepth++;
      haveWord = true;
    }
    if (*ii == '(')
    {
      subDepth--;
      if (subDepth < 0)
      {
        error(1, 0, "%d: unmatched '('", currln);
      }
    }
    if (subDepth != 0) continue;

    if (*ii == '&' && *(ii+1) == '&')
    {
      if (!haveWord) 
        error(1, 0, "%d: expected command after '&&'", currln);

      *ii = 0;

      if (!anything_before(ii-1,comText.start)) 
        error(1, 0, "%d: expected command before '&&'", currln);

      

      com_text_t subComms[2]= {
        {
          .start = comText.start,
          .end = ii-1,
          .startln = comText.startln,
          .endln = currln 
        },
        {
          .start = ii+2,
          .end = comText.end,
          .startln = currln,
          .endln = comText.endln
        }
      };

      return construct_command(AND_COMMAND, NULL, NULL, 
         parse_ao_string(*strip_text(&subComms[0])), 
         parse_pipe_string(*strip_text(&subComms[1])));
    }

    if (*ii == '|' && *(ii+1) == '|')
    {
  //fprintf(stderr, "GOT HERE!");
      if (!haveWord) error(1, 0, "%d: expected command after '||'", currln);
      *ii = 0;

      if (!anything_before(ii-1,comText.start)) 
        error(1, 0, "%d: expected command before '||'", currln);

      com_text_t subComms[2]= {
        {
          .start = comText.start,
          .end = ii-1,
          .startln = comText.startln,
          .endln = currln 
        },
        {
          .start = ii+2,
          .end = comText.end,
          .startln = currln,
          .endln = comText.endln
        }
      };

      return construct_command(OR_COMMAND, NULL, NULL, 
         parse_ao_string(*strip_text(&subComms[0])), 
         parse_pipe_string(*strip_text(&subComms[1])));
    }
    
    if (!haveWord && isWordChar(*ii))
    {
      haveWord = true;
      continue;
    }
  }

  return parse_pipe_string(comText);
}

command_t parse_pipe_string(com_text_t comText)
{
  char *ii = comText.end;
  bool haveWord = false;
  int currln = comText.endln;
  int subDepth = 0; 

  for ( ; ii >= comText.start; ii--)
  {
    if (*ii == '\n') currln--;

    if (*ii == ')')
    {
      subDepth++;
      haveWord = true;
    }
    if (*ii == '(')
    {
      subDepth--;
      if (subDepth < 0)
      {
        error(1, 0, "%d: unmatched '('", currln);
      }
    }
    if (subDepth != 0) continue;

    if (!haveWord && isWordChar(*ii))
    {
      haveWord = true;
      continue;
    }

    if (*ii == '|') 
    {
      if (!haveWord) error(1, 0, "%d: expected command after '|'", currln);
      *ii = 0;

      if (!anything_before(ii-1,comText.start)) 
        error(1, 0, "%d: expected command before '|'", currln);

      com_text_t subComms[2]= {
        {
          .start = comText.start,
          .end = ii-1,
          .startln = comText.startln,
          .endln = currln 
        },
        {
          .start = ii+1,
          .end = comText.end,
          .startln = currln,
          .endln = comText.endln
        }
      };

      return construct_command(PIPE_COMMAND, NULL, NULL, 
         parse_pipe_string(*strip_text(&subComms[0])),
         parse_io_string(*strip_text(&subComms[1])));
    }
  }

  return parse_io_string(comText);
}

command_t parse_io_string(com_text_t comText)
{
  char *ii = comText.end;
  char *pastLastWord = comText.end+1;
  char *output = NULL;
  char *input = NULL;
  int currln = comText.endln;
  bool inWord = false;
  int numWords = 0;

  for ( ; ii >= comText.start; ii--)
  {
    if (*ii == '\n') currln--;

    if (*ii == ')')
    {
      *ii = 0;

      com_text_t subComm =
        {
          .start = comText.start,
          .end = ii-1,
          .startln = currln,
          .endln = comText.endln
        };

      return sub_command_build(*strip_text(&subComm), input, output);
    }

    if (isWordChar(*ii))
    {
      if (!inWord) numWords++;
      inWord = true;
      continue;
    }

    if (*ii == '>')
    {
      if (numWords != 1) 
        error(1,0, "%d: no words or too many words found after '>'", currln);

      if (!anything_before(ii-1,comText.start)) 
        error(1, 0, "%d: expected command before '>'", currln);

      output = strip_string(ii+1);
      *ii = 0;
      pastLastWord = ii;
      numWords = 0;
      inWord = false;
      continue;
    }

    if (*ii == '<')
    {
      if (numWords != 1) 
        error(1,0, "%d: no words or too many words found after '<'", currln);

      if (!anything_before(ii-1,comText.start)) 
        error(1, 0, "%d: expected command before '>'", currln);

      input = strip_string(ii+1);
      *ii = 0;
      pastLastWord = ii;
      numWords = 0;
      inWord = false;
      break;
    }
  }

  com_text_t otherComm = 
    {
      .start = comText.start,
      .end = pastLastWord-1,
      .startln = currln,
      .endln = comText.endln 
    };

  return simple_command_build(*strip_text(&otherComm), input, output);
}

command_t sub_command_build(com_text_t comText, char *input, char *output)
{
  if (*comText.start != '(')
  {
    error(1, 0, "%d: need '('", comText.startln);
  }

  if (!anything_before(comText.end,comText.start+1)) 
    error(1, 0, "%d: no command in subshell", comText.startln);

  com_text_t subComm =
    {
      .start = comText.start + 1,
      .end = comText.end,
      .startln = comText.startln,
      .endln = comText.endln
    };

  return construct_command(SUBSHELL_COMMAND, input, output, 
      parse_seq_string(*strip_text(&subComm)), NULL);
}

command_t parse_seq_string(com_text_t comText)
{
  char *ii = comText.start;
  bool haveSpecial = false;
  int currln = comText.startln;

  for( ; ii <= comText.end; ii++)
  {
    if (*ii == '\n') currln++;

    if (*ii == ';')
    {
      *ii = 0;

      if (!anything_before(ii-1,comText.start)) 
        error(1, 0, "%d: expected command before '>'", currln);

      com_text_t subComms[2]= {
        {
          .start = comText.start,
          .end = ii-1,
          .startln = comText.startln,
          .endln = currln 
        },
        {
          .start = ii+1,
          .end = comText.end,
          .startln = currln,
          .endln = comText.endln
        }
      };

      return construct_command(SEQUENCE_COMMAND, NULL, NULL, 
         parse_ao_string(*strip_text(&subComms[0])),
         parse_seq_string(*strip_text(&subComms[1])));
    }

    if (*ii == '|') 
    {
      haveSpecial = true; 
      continue;
    }

    if (*ii == '&' && *(ii+1) == '&')
    {
      haveSpecial = true;
      ii++;
      continue;
    }

    if (isWordChar(*ii))
    {
      haveSpecial = false;
      continue;
    }

    if (*ii == '\n' && !haveSpecial)
    {
      *ii = 0;
      com_text_t subComms[2]= {
        {
          .start = comText.start,
          .end = ii-1,
          .startln = comText.startln,
          .endln = currln 
        },
        {
          .start = ii+1,
          .end = comText.end,
          .startln = currln,
          .endln = comText.endln
        }
      };

      return construct_command(SEQUENCE_COMMAND, NULL, NULL, 
         parse_pipe_string(*strip_text(&subComms[0])),
         parse_io_string(*strip_text(&subComms[1])));
    }
  }

  return parse_ao_string(comText);

}



command_t simple_command_build(com_text_t comText, 
    char *input, char *output)
{
  return construct_command(SIMPLE_COMMAND, input, output,
     get_word_array(comText.start), NULL);
}

char *drop_trailing_nulls(char *endbyte)
{
  while (*(endbyte - 1) == 0)
  {
    endbyte--;
  }
  return endbyte;
}

char **get_word_array(char *sentence)
{
  int words = 0;
  char *end;
  bool inWord = false;

  sentence = strip_string(sentence);  //remove white space on either side

  char *ii;
  for (ii = sentence; *ii != 0; ii++)
  {
    if (isWhite(*ii))
    {
      *ii = 0;
      if (inWord)
      {
        words++;
        inWord = false;
      }
      continue;
    }
    if (!isWordChar(*ii))
    {
      error(1, 0, "unexpected '%c' token found", *ii);
    }
    else
    {
      inWord = true;
    }
  }
  if (inWord) words++;

  end = ii;

  char **wordsArray = checked_malloc((words+1) * sizeof(char *));

  inWord = false;
  words = 0;
  for (ii = sentence; ii <= end; ii++)
  {
    if (*ii)
    {
      if (!inWord) 
      {
        wordsArray[words] = ii;
        words++;
        inWord = true;
      }
    }
    else
    {
      inWord = false;
    }
  }
  wordsArray[words] = NULL;

  return wordsArray;
}




bool isWordChar(char c)
{
  return (isalpha((unsigned char) c) || isdigit((unsigned char) c) || 
      c=='!' || c=='%' ||
      c=='+' || c==',' || c=='-' || c=='.' ||
      c=='/' || c==':' || c=='@' || c=='^' ||
      c=='_');
}

bool isWhite(char c)
{
  return (c=='\n' || c=='\t' || c==' ');
}

com_text_t *strip_text(com_text_t *pComText)
{
  char *ii = pComText->start;

  for( ; *ii != 0; ii++)
  {
    if (*ii == '\n') 
    {
      pComText->start++;
      pComText->startln++;
      continue;
    }
    if (isWhite(*ii)) 
    {
      pComText->start++;
    }
    else break;
  }

  ii = pComText->end;
  for( ; ii >= pComText->start; ii--)
  {
    if (*ii == '\n') 
    {
      *ii = 0;
      pComText->end--;
      pComText->endln--;
      continue;
    }
    if (isWhite(*ii)) 
    {
      *ii = 0;
      pComText->end--;
    }
    else break;
  }

  return pComText;
}


char *strip_string(char *start)
{
  char *ii = start;
  for ( ; isWhite(*ii); ii++) 
  {
    if(!*ii) return ii;
    *ii = 0;
  }

  char *newStart = ii;

  for ( ; *ii; ii++) ;

  ii--;

  for ( ; isWhite(*ii); ii--)
  {
    *ii = 0;
  }
  
  return newStart;
}
    

bool anything_before(char *last, char *first)
{
  while (last >= first)
  {
    if (isWordChar(*last) || *last == ')')
    {
      return true;
    }
    last--;
  }
  return false;
}


// list_t functions
list_t list_new()
{
  list_t reference = checked_malloc(sizeof(struct list_node *));
  *reference = checked_malloc(sizeof(struct list_node));

  (*reference)->val = READY_TO_RUN;
  (*reference)->next = NULL;

  return reference;
}

void list_push(list_t list, int num)
{
  struct list_node *first = *list;
  struct list_node *new = malloc(sizeof(struct list_node));
  new->val = num;
  new->next = first;
  *list = new;
}

void list_remove(list_t list, int num)
{
  struct list_node *ii = *list;
  struct list_node **locprev = list;
  while (ii)
  {
    if (ii->val == num)
    {
      *locprev = ii->next;
      free(ii);
      return;
    }
    locprev = &(ii->next);
    ii = ii->next;
  }
}

void list_mark_exec(list_t list)
{
  (*list)->val = EXECUTION_STARTED;
}

int list_peek(list_t list)
{
  return (*list)->val;
}

void
add_inout(char * src, char *** dest)
{
  int length = 0;
  int i;
  for(i = 0; dest[0][i] != NULL; i++)
    length++;
  char** tmp = (char**)checked_realloc(dest[0],sizeof(char*)*(2+length));
  tmp[length] = src;
  tmp[length+1] = NULL;
  dest[0] = tmp;
}

void
add_words(char ** src, char **** dest)
{
  int length = 0;
  int i;
  for(i = 0; dest[0][i] != NULL; i++)
    length++;
  char*** tmp = (char***)checked_realloc(dest[0],sizeof(char**)*(2+length));
  tmp[length] = src;
  tmp[length+1] = NULL;
  dest[0] = tmp;
}

void
make_rw_list(command_t com, char **** words_read, char *** input_read,
  char *** output_write)
{
  switch(com->type)
  {
    case AND_COMMAND:
    case SEQUENCE_COMMAND:
    case OR_COMMAND:
    case PIPE_COMMAND:
    {
      make_rw_list(com->u.command[0],words_read,input_read,output_write);
      make_rw_list(com->u.command[1],words_read,input_read,output_write);
      break;
    }
    case SIMPLE_COMMAND:
    {
      add_words(com->u.word, words_read);
      break;
    }
    case SUBSHELL_COMMAND:
    {
      make_rw_list(com->u.subshell_command,words_read,input_read,output_write);
      break;
    }
  }
  /*Add redirections if any*/
  if(com->input) 
    add_inout(com->input, input_read);
  if(com->output)
    add_inout(com->output, output_write);
  return;
}

bool
does_depend(char **** words_read, char *** input_read, char *** output_write,
  int prev_com, int curr_com)
{
  char *** curr_word = words_read[curr_com];
  char ** curr_in = input_read[curr_com];
  char ** curr_out = output_write[curr_com];
  char *** pre_word = words_read[prev_com];
  char ** pre_in = input_read[prev_com];
  char ** pre_out = output_write[prev_com];
  
  int i;
  int j;
  /*Can't write to same file*/
  for(i = 0; (curr_out[i] != NULL) && (pre_out[i] != NULL); i++)
    if(strcmp(curr_out[i],pre_out[i]) == 0)
      return true;
  
  /*Can't read from a file being written to*/
  for(i = 0; (curr_in[i] != NULL) && (pre_out[i] != NULL); i++)
    if(strcmp(curr_in[i],pre_out[i]) == 0)
      return true;
  for(i = 0; (curr_word[i] != NULL) && (pre_out[i] != NULL); i++)
    for(j = 0; curr_word[i][j] != NULL; j++)
      if(strcmp(curr_word[i][j],pre_out[i]) == 0)
        return true;
  
  /*Can't write to a file being read from*/
  for(i = 0; (curr_out[i] != NULL) && (pre_in[i] != NULL); i++)
    if(strcmp(curr_out[i],pre_in[i]) == 0)
      return true;
  for(i = 0; (curr_out[i] != NULL) && (pre_word[i] != NULL); i++)
    for(j = 0; pre_word[i][j] != NULL; j++)
      if(strcmp(curr_out[i], pre_word[i][j]) == 0)
        return true;
  return false;
}

void
make_dep_lists(command_stream_t s)
{
  /*Initialize Read/Write Lists*/
  char *** words_read[s->numCommands];
  char ** input_read[s->numCommands];
  char ** output_write[s->numCommands];
  s->depLists = checked_malloc(sizeof(struct list_node **)*s->numCommands);
  
  int i;
  int j;
  
  /*Set index field for each command starting at 0 and
      fill the Read/Write Lists*/
  for(i=0; i < s->numCommands; i++)
  {
    (s->commands[i])->index = i;
    
    /* Set some NULL's*/
    words_read[i] = (char***)checked_malloc(sizeof(char **));
    words_read[i][0] = NULL;
    input_read[i] = (char**)checked_malloc(sizeof(char*));
    input_read[i][0] = NULL;
    output_write[i] = (char**)checked_malloc(sizeof(char*));
    output_write[i][0] = NULL;
    
    make_rw_list(s->commands[i],&words_read[i],&input_read[i],&output_write[i]);
  }
  
  /*Build the Dependency Lists*/
  for(i = 0; i < s->numCommands; i++)
  {
    s->depLists[i] = list_new();
    for(j = 0; j < i; j++)
      if(does_depend(words_read,input_read,output_write,j,i))
        list_push(s->depLists[i],j);
  }
  
  /*Debugging Stuff*/
  /*int k;
  for(i = 0; i < s->numCommands; i++)
  {
    printf("Command %d reads from:\n",i);
    for(j = 0; input_read[i][j] != NULL; j++)
    {
      printf(input_read[i][j]);
      printf("\n");
    }
    for(j = 0; words_read[i][j] != NULL; j++)
      for(k = 0; words_read[i][j][k] != NULL; k++)
      {
        printf(words_read[i][j][k]);
        printf("\n");
      }
    printf("Command %d writes to:\n",i);
    for(j = 0; output_write[i][j] != NULL; j++)
    {
      printf(output_write[i][j]);
      printf("\n");
    }
  }
  for(i = 0; i < s->numCommands; i++)
  {
    printf("Command %d depends on the following:\n",i);
    list_t listy = s->depLists[i];
    while(*listy)
    {
     printf("%d\n", (*listy)->val);
     *listy = (*listy)->next;   
    }
  }
  printf("Done Debugging\n");*/
  
  /*Free my allocated data*/
  for(i = 0; i < s->numCommands; i++)
  {
    free(words_read[i]);
    free(input_read[i]);
    free(output_write[i]);
  }
}
  
