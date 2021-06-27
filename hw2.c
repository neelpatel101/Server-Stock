#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

#define SH_TOK_BUFSIZE 64
#define SH_TOK_DELIM " \t\r\n\a"

struct Job
{
    int jid;
    int pid;
    char status[20];
    char command[80];
};

int isFG = 0;
int fgpid = 0;
int counter = 0;
struct Job allJobs[5];

char *sh_read_line(void)
{
    char *line = NULL;
    size_t bufsize = 0;  // Have getline allocate a buffer for us

    if (getline(&line, &bufsize, stdin) == -1) 
    {
        if (feof(stdin))  // EOF
        {
            fprintf(stderr, "EOF\n");
            exit(EXIT_SUCCESS);
        }

        else 
        {
            fprintf(stderr, "Value of errno: %d\n", errno);
            exit(EXIT_FAILURE);
        }
    }
    return line;
}

char **sh_split_line(char *line)
{
    int bufsize = SH_TOK_BUFSIZE;
    int position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token, **tokens_backup;

    if (!tokens) 
    {
        fprintf(stderr, "sh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, SH_TOK_DELIM);
    while (token != NULL) 
    {
        tokens[position] = token;
        position++;

        if (position >= bufsize) 
        {
            bufsize += SH_TOK_BUFSIZE;
            tokens_backup = tokens;
            tokens = realloc(tokens, bufsize * sizeof(char *));

            if (!tokens) 
            {
                free(tokens_backup);
                fprintf(stderr, "sh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, SH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

int ampersand(char **args)
{
    int i;
    for (i = 1; args[i] != NULL; ++i);

    if (strcmp(args[i - 1], "&") == 0)
    {
        args[i - 1] = NULL;
        return 1;
    } 

    else
    {
        return 0;
    }
}

void sigint_handler(int sig) 
{
    kill(-fgpid,SIGINT);
    return;
}

void sigtstp_handler(int sig)
{
    kill(-fgpid, SIGTSTP);
    return;
}

void sigchld_handler(int sig) 
{
    int i;
    int status;
    pid_t pid;
    
    // Parent reaps children in no particular order
    while ((pid = waitpid(-1, &status, WNOHANG|WUNTRACED)) > 0)
    {
        isFG = 1;
        // Terminated normally
        if (WIFEXITED(status))
        {
            for (i = 0; i < counter; ++i)
            {
                if (allJobs[i].pid == pid)
                {
                    int j;

                    for (j = i; j < counter - 1; ++j)
                    {
                        allJobs[j].pid = allJobs[j + 1].pid;
                        strcpy(allJobs[j].status, allJobs[j + 1].status);
                        strcpy(allJobs[j].command, allJobs[j + 1].command);
                    }
                    counter--;
                }
            }
        }
        // Terminated by SIGINT
        else if (WIFSIGNALED(status))
        {
            for (i = 0; i < counter; ++i)
            {
                if (allJobs[i].pid == pid)
                {
                    int j;

                    for (j = i; j < counter - 1; ++j)
                    {
                        allJobs[j].pid = allJobs[j + 1].pid;
                        strcpy(allJobs[j].status, allJobs[j + 1].status);
                        strcpy(allJobs[j].command, allJobs[j + 1].command);
                    }
                    counter--;
                }
            }
        }
        // Suspended by SIGTSTP
        else if (WIFSTOPPED(status))
        {
            for (i = 0; i < counter; ++i)
            {
                if (allJobs[i].pid == pid)
                {
                    strcpy(allJobs[i].status, "Stopped");
                }
            }
        }
    }
    return;
}

int sh_launch(char **args)
{
    int pid = fork();
    int wait_pid = 0;
    mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO;

    int i;
    int in = 0;
    int out = 0;
    int no_ampersand = (ampersand(args) == 0);

    char *input;
    char *output;

    if (pid == 0) 
    {
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);

        for (i = 0; args[i] != NULL; ++i)
        {
            if (strcmp(args[i], "<") == 0)
            {
                args[i] = NULL;
                input = args[i + 1];
                int j;
                for (j = i; args[j - 1] != NULL; ++j)
                {
                    args[j] = args[j + 1];
                }
                in = 1;
            }

            if (strcmp(args[i], ">") == 0)
            {
                args[i] = NULL;
                output = args[i + 1];
                int j;
                for (j = i; args[j - 1] != NULL; ++j)
                {
                    args[j] = args[j + 1];
                }
                out = 1;
            }
        }

        if (in)
        {
            int fd = open(input, O_RDONLY, mode);
            if (fd < 0)
            {
                printf("Could not open input file");
                exit(0);
            }
            dup2(fd,STDIN_FILENO);
            close(fd);
            in = 0;
        }

        if (out)
        {
            int fd1 = open(output, O_CREAT | O_WRONLY | O_TRUNC, mode);
            if (fd1 < 0)
            {
                printf("Could not open output file");
                exit(0);
            }
            dup2(fd1,STDOUT_FILENO);
            close(fd1);
            out = 0;
        }

        execvp(*args, args);
        execv(*args, args);
        printf("Exec error\n");
        exit(0);
    }

    else
    {
        if (counter == 5)
        {
            printf("Cannot add more jobs\n");
            return 1;
        }
        
        allJobs[counter].jid = counter + 1;
        allJobs[counter].pid = pid;
        strcpy(allJobs[counter].status, "Running");
        strcpy(allJobs[counter].command, *args);
        counter++;
        
        if (no_ampersand)   //Foreground
        {
            fgpid = pid;
            strcpy(allJobs[counter - 1].status, "Foreground");

            while (!isFG)
            {   
                sleep(1);
            }
            isFG = 0;
        }

        else                //Background
        {
            setpgid(pid, 0);
        }
    }
    return 1;
}

int sh_execute(char **args)
{
    int i;
    int status;
    if (args[0] == NULL) 
    {
        return 1;  // An empty command was entered
    }

    if (strcmp(args[0], "quit") == 0)
    {
        exit(0);
    }

    else if (strcmp(args[0], "kill") == 0)
    {
        int killPid = 0;
        
        if (args[1][0] == '%') 
        {
            int inputJid = args[1][1] - '0';
            killPid = allJobs[inputJid - 1].pid;
        }

        else 
        {
            killPid = atoi(args[1]);
        }

        if (kill(killPid, SIGINT) == 0)
        {

            for (i = 0; i < counter; ++i)
            {
                if (killPid == allJobs[i].pid)
                {
                    int j;

                    for (j = i; j < counter - 1; ++j)
                    {
                        allJobs[j].pid = allJobs[j + 1].pid;
                        strcpy(allJobs[j].status, allJobs[j + 1].status);
                        strcpy(allJobs[j].command, allJobs[j + 1].command);
                    }
                    counter--;
                }
            }
        }
        return 1;
    }

    else if (strcmp(args[0], "jobs") == 0)
    {
        for (i = 0; i < counter; ++i)
        {
            printf("[%d] (%d) %s %s\n", allJobs[i].jid, allJobs[i].pid, allJobs[i].status, allJobs[i].command);
        }
        return 1;
    }

    else if (strcmp(args[0], "bg") == 0)
    {
        int bgPid = 0;
        int inputJid = 0;
        
        if (args[1][0] == '%') 
        {
            inputJid = args[1][1] - '0';
            bgPid = allJobs[inputJid - 1].pid;
        }
        
        else
        {
            bgPid = atoi(args[1]);
            for (i = 0; i < counter; ++i)
            {
                if (bgPid == allJobs[i].pid)
                {
                    inputJid = allJobs[i].jid;
                }
            }
        }

        if (strcmp(allJobs[inputJid - 1].status, "Stopped") == 0)
        {
            if (kill(bgPid, SIGCONT) < 0)
            {
                perror("kill(SIGCONT)");
            }
            
            strcpy(allJobs[inputJid - 1].status, "Running");
            isFG = 0;
        }
        return 1;
    }

    else if (strcmp(args[0], "fg") == 0)
    {
        int fgPid = 0;
        int inputJid = 0;
        
        if (args[1][0] == '%') 
        {
            inputJid = args[1][1] - '0';
            fgPid = allJobs[inputJid - 1].pid;
        }
        
        else
        {
            fgPid = atoi(args[1]);
            for (i = 0; i < counter; ++i)
            {
                if (fgPid == allJobs[i].pid)
                {
                    inputJid = allJobs[i].jid;
                }
            }
        }

        if ((strcmp(allJobs[inputJid - 1].status, "Stopped") == 0) || (strcmp(allJobs[inputJid - 1].status, "Running") == 0))
        {
            if (kill(fgPid, SIGCONT) == 0)
            {
                for (i = 0; i < counter; ++i)
                {
                    if (fgPid == allJobs[i].pid)
                    {
                        strcpy(allJobs[i].status, "Foreground");
                    }
                }

                while (!isFG)
                {   
                    sleep(1);
                }
                isFG = 0;
            }
        }
        return 1;
    }
    return sh_launch(args);   // Launch
}

void sh_loop(void)
{
    char *line;
    char **args;
    int status; 
    
    signal(SIGINT, sigint_handler);
    signal(SIGTSTP, sigtstp_handler);
    signal(SIGCHLD, sigchld_handler);

    do 
    {
        printf("prompt> ");
        line = sh_read_line();
        args = sh_split_line(line);
        status = sh_execute(args);

        free(line);
        free(args);
    } while (status);
}

int main(int argc, char **argv)
{
    sh_loop();
    return EXIT_SUCCESS;
}