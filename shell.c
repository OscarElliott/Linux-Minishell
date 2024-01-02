#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>


#define NV 20			/* max number of command tokens */
#define NL 100			/* input buffer size */
char    line[NL];	/* command input buffer */


/*
	shell prompt
 */

void prompt(void)
{
  // declare variable to sotre directory
  char cwd[1024];
  if (getcwd(cwd, sizeof(cwd)) != NULL)
  {
    //fprintf(stdout, "\n%s msh> ", cwd);
  }
  else
  {
    perror("getcwd");
    printf("\nmsh> ");
  }
  fflush(stdout);
}

int num_jobs = 0; // counter for number of jobs running
int jobs[32] = {0}; // this is meant to represent the list of process running in the terminal 32 is a arbtriary limit I selected, PID are stored in this array in the order they are called
char* jobsnames[32] = {NULL}; // stores the commands correlating to the PID

// method that checks if a command should be run in the background and if so flags it
int is_background(char **tokens, int num_tokens)
{
  if (num_tokens > 0)
  {
    if (strcmp(tokens[num_tokens-1], "&") == 0)
    {
      tokens[num_tokens-1] = NULL; // remove the & token
      return 1; // Background execution
    }
  }
  return 0; // normal execution
}

// changes directory using current path and user input
void change_directory(char *dir)
{
  char cwd[1024];
  char new_path[1024];
  if (dir == NULL)
  {
    fprintf(stderr, "cd: missing argument\n");
  }
  else if (getcwd(cwd, sizeof(cwd)) != NULL)
  {
    snprintf(new_path, sizeof(new_path), "%.*s/%s", (int)(sizeof(new_path) - strlen(cwd) - 1), cwd, dir);;
    if(chdir(new_path) != 0)
    {
      perror("cd"); // print error message if chrdir fails
    }
  }
}

void delete_file(const char *filename) 
{
    // Remove the first and last characters (known to be double quotes)
    size_t len = strlen(filename);
    if (len >= 2 && filename[0] == '"' && filename[len - 1] == '"') {
        char modified_filename[len - 1];
        memcpy(modified_filename, filename + 1, len - 2);
        modified_filename[len - 2] = '\0';

        pid_t child_pid;
        int status;

        child_pid = fork();

        if (child_pid == -1) 
        {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (child_pid == 0) 
        {
            // Child process
            execlp("rm", "rm", "-r", modified_filename, (char *)NULL);
            perror("execlp");
            exit(EXIT_FAILURE);
        } 
        else 
        {
            // Parent process
            if (waitpid(child_pid, &status, 0) == -1) 
            {
                perror("waitpid");
                exit(EXIT_FAILURE);
            }

            if (WIFEXITED(status) && WEXITSTATUS(status) == 0) 
            {
                printf("File or directory '%s' deleted successfully.\n", modified_filename);
            } 
            else 
            {
                fprintf(stderr, "Error deleting file or directory '%s'.\n", modified_filename);
            }
        }
    } 
    else 
    {
        fprintf(stderr, "Invalid filename format: %s\n", filename);
    }
}

void rename_file(const char *srcfile, const char *dstfile) {
    // Check if the source file exists
    if (access(srcfile, F_OK) != 0) 
    {
        fprintf(stderr, "The source file %s does not exist.\n", srcfile);
        return;
    }

    // Check if source and destination files are the same
    if (strcmp(srcfile, dstfile) == 0) 
    {
        fprintf(stderr, "The source file %s and the destination file %s cannot be the same.\n", srcfile, dstfile);
        return;
    }

    // Perform the file rename
    if (rename(srcfile, dstfile) != 0) 
    {
        perror("rename");
        fprintf(stderr, "Error renaming file %s to %s.\n", srcfile, dstfile);
    } 
    else 
    {
        printf("File %s renamed to %s successfully.\n", srcfile, dstfile);
    }
}

void copy_file(const char *srcfile, const char *dstfile) 
{
    pid_t child_pid;
    int status;

    child_pid = fork();

    if (child_pid == -1) 
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0) 
    {
        // Child process
        execlp("cp", "cp", "-r", srcfile, dstfile, (char *)NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    } 
    else 
    {
        // Parent process
        if (waitpid(child_pid, &status, 0) == -1) 
        {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) 
        {
            printf("'%s' copied to '%s' successfully.\n", srcfile, dstfile);
        } 
        else 
        {
            fprintf(stderr, "Error copying '%s' to '%s'.\n", srcfile, dstfile);
        }
    }
}

// Function to check if a string starts with a double quote
int startsWithQuote(const char *str) 
{
    return (str != NULL && str[0] == '"');
}

int endWithQuote(const char *str)
{
    return (str != NULL && str[strlen(str) - 1] == '"');
}

// Function to remove quotes from the beginning and end of a string
void removeQuotes(char *str) 
{
    if (startsWithQuote(str)) 
    {
        // Remove the starting quote
        memmove(str, str + 1, strlen(str));
    }
    if (str[strlen(str) - 1] == '"') 
    {
        // Remove the trailing quote
        str[strlen(str) - 1] = '\0';
    }
}


int main(int argk, char *argv[], char *envp[])
{
  int             frkRtnVal;	/* value returned by fork sys call */
  int             wpid;		/* value returned by wait */
  char           *v[NV];	/* array of pointers to command line tokens */
  char           *sep = " \t\n";/* command line token separators    */
  int             i;		/* parse index */

  printf("----------------------------------------------\n");
  printf("             Welcome to GCC Shell             \n");
  printf("----------------------------------------------\n");
  /* prompt for and process one command line at a time  */

  while (1)  /* do Forever */
  {			
    prompt();
    fgets(line, NL, stdin);
    fflush(stdin);
    char* line_copy = strdup(line); // make a copy of the command to store in the jobs list later

    // check for end of file situation, if true terminates after printing process id and error
    if (feof(stdin)) /* non-zero on EOF  */
    {		
      /* fprintf(stderr, "EOF pid %d feof %d ferror %d\n", getpid(),
	      feof(stdin), ferror(stdin)); */
      exit(0); // as we already have an error here no perror needed
    }
    // check for comment, whitespace or new line
    if (line[0] == '#' || line[0] == '\n' || line[0] == '\000')
      continue;			/* to prompt */
    // seperate line into tokens based on "seperators" and store them in v array (Tokenization)
    v[0] = strtok(line, sep);
    for (i = 1; i < NV; i++) 
    {
      v[i] = strtok(NULL, sep);
      if (v[i] == NULL)
	      break;
    }
    /* assert i is number of tokens + 1 */

    // Check if the command should run in the background
    int background = is_background(v,i);

    if (strcmp(v[0], "rm") == 0) 
    {
      if (v[1] != NULL) 
      {
          char filename[1024];
          filename[0] = '\0'; 
          for (int i = 1; v[i] != NULL; i++) 
          {
              strcat(filename, v[i]);
              strcat(filename, " ");
          }
          filename[strlen(filename) - 1] = '\0'; // Remove the trailing space

          delete_file(filename);
      } 
      else 
      {
          fprintf(stderr, "Usage: delete <filename>\n");
      }
      continue;
    }

    if (strcmp(v[0], "mv") == 0) 
    {
        if (v[1] != NULL && v[2] != NULL) 
        {
            char srcfile[1024] = "";
            char dstfile[1024] = "";

            

            // Concatenate tokens for source file
            int i = 2;
            strcat(srcfile, v[1]);
            strcat(srcfile, " ");

            if (startsWithQuote(v[1]) && !endWithQuote(v[1]))
            {
              for (i = 2; v[i] != NULL && !endWithQuote(v[i-1]); i++) 
              {
                  strcat(srcfile, v[i]);
                  strcat(srcfile, " ");
              }
            }
            srcfile[strlen(srcfile) - 1] = '\0'; // Remove the trailing space
            removeQuotes(srcfile);

            // Concatenate tokens for destination file
            for (int j = i; v[j] != NULL; j++) 
            {
                strcat(dstfile, v[j]);
                strcat(dstfile, " ");
            }
            dstfile[strlen(dstfile) - 1] = '\0'; // Remove the trailing space

            removeQuotes(dstfile);

            printf("File '%s' to be renamed '%s'.\n", srcfile, dstfile);
            rename_file(srcfile, dstfile);
        } 
        else 
        {
            fprintf(stderr, "Usage: mv \"<srcfile>\" \"<dstfile>\"\n");
        }
        continue;
    }

    if (strcmp(v[0], "cp") == 0) 
    {
        if (v[1] != NULL && v[2] != NULL) 
        {
            char srcfile[1024] = "";
            char dstfile[1024] = "";

            // Concatenate tokens for source file
            int i = 2;
            strcat(srcfile, v[1]);
            strcat(srcfile, " ");

            if (startsWithQuote(v[1]) && !endWithQuote(v[1]))
            {
              for (i = 2; v[i] != NULL && !endWithQuote(v[i-1]); i++) 
              {
                  strcat(srcfile, v[i]);
                  strcat(srcfile, " ");
              }
            }

            srcfile[strlen(srcfile) - 1] = '\0'; // Remove the trailing space
            removeQuotes(srcfile);

            // Concatenate tokens for destination file
            for (int j = i; v[j] != NULL; j++) 
            {
                strcat(dstfile, v[j]);
                strcat(dstfile, " ");
            }
            dstfile[strlen(dstfile) - 1] = '\0'; // Remove the trailing space
            removeQuotes(dstfile);

            printf("File '%s' to be copied '%s'.\n", srcfile, dstfile);
            copy_file(srcfile, dstfile);
        } 
        else 
        {
            fprintf(stderr, "Usage: mv \"<srcfile>\" \"<dstfile>\"\n");
        }
        continue;
    }

    if (strcmp(v[0], "cd") == 0)
    {
      if (background)
      {
        num_jobs++;
        for (int i = 0; i < 32; i++)
        {
          if (jobs[i] == 0)
          {
            jobs[i] = frkRtnVal;
            char* ampersand = strrchr(line_copy, '&'); // this is to remove the ampersand from background tasks before they are stored in the jobs list
            if (ampersand != NULL)
            {
              *ampersand = '\0';
            }
            jobsnames[i] = line_copy;
            break;
          }  
        }
        printf("[%d] %d\n", num_jobs, frkRtnVal); // prints out job number and PID to terminal
      }
      change_directory(v[1]);
    }
    else
    {
      /* fork a child process to exec the command in v[0] */
      switch (frkRtnVal = fork()) 
      {
        case -1:			/* fork returns error to parent process */
          {
            break;
          }
        case 0:			/* code executed only by child process */
          {
            execvp(v[0], v);
            perror("execvp"); // this will print out an error message if execvp fails
            exit(1);
          }
        default:			/* code executed only by parent process */
          {
            if (background)
            {
              for (int i = 0; i < 32; i++)
              {
                if (jobs[i] == 0)
                {
                  jobs[i] = frkRtnVal;
                  char* ampersand = strrchr(line_copy, '&');
                  if (ampersand != NULL)
                  {
                    *ampersand = '\0';
                  }
                  jobsnames[i] = line_copy;
                  break;
                }  
              }
              num_jobs++;
              printf("[%d] %d\n", num_jobs, frkRtnVal); // prints out job number and PID to terminal
            }
            // if the task is not a background task wait for it to complete
            if (!background)
            {
              waitpid(frkRtnVal, NULL, 0); // Wait for the child process to finish
              

              // when the program runs a job in the forground it will check if the backgound process have finished
              // if so they will print and the number of jobs will be reduced.
              int status;
              // prints out the jobs number and the command associated upon completion but only once a forground command finishes
              while ((wpid = waitpid(-1, &status, WNOHANG)) > 0)
              {
                if (WIFEXITED(status))
                {
                  for (int i = 0; i < 32; i++)
                  {
                    if (jobs[i] == wpid)
                    {
                      jobs[i] = 0;
                      char* name = jobsnames[i];
                      printf("[%d]+ Done             %s\n", i+1, name); 
                      num_jobs--;
                      break;
                    }
                  }
                }
              }
            }

            // told to comment this line out on piazza
            //printf("%s done \n", v[0]);
            break;
          }
      } /* switch */
    } /* else */
  }				/* while */
}				/* main */
