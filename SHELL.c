#include <stdio.h>
#include <zconf.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

char cwd[4048];//current woring directory
char hn[4048];//hostname
char un[4048];//user name
char *env;//Home directory path
size_t envlength;
int returnStatus;// for waiting
int speacialUsed = 0;//special char used in string
int numberOfAnd = 0;
int numberOfPipe = 0;
int pid, exec_code;
char s[20000] = "";
char *pos, *p2;
int *indexesofand;
int *indexesofpipes;


void clearScreen() {
    printf("\033[2J\033[1;1H");
}

void init_cwd() {
    getcwd(cwd, sizeof(cwd));
    if (strstr(cwd, env) != NULL) {
        memmove(&cwd[0], &cwd[0 + envlength], strlen(cwd) + envlength);
    }
}

void sig_handler(int signo) {
    if (signo == SIGINT) {

    } else if (signo == SIGSTOP) {

    }
}

void changeDir(char **argv) {
    if (argv[1] == NULL) {
        if (chdir(env) == -1) {
            printf(" %s: no such directory\n", argv[1]);
        } else {
            init_cwd();
        }
    } else {
        if (chdir(argv[1]) == -1) {
            printf(" %s: no such directory\n", argv[1]);
        } else {
            init_cwd();
        }
    }
}

void andHandler(int argc, char **argv, const int *inds, int count) {
    int lastAnd = 0;
    int z = 0;
    for (int j = 0; j < count; ++j) {
        int offset = lastAnd == 0 ? 0 : 1;
        int length = inds[j] - lastAnd - offset;
        char *argg[length];
        z = 0;
        for (int i = lastAnd + offset; i < inds[j]; ++i) {
            argg[z] = argv[i];
            z++;
        }
        argg[z]=0;
        int pid = fork();
        if (pid == 0) {
            signal(SIGINT, SIG_DFL);
            signal(SIGSTOP, SIG_DFL);
            execvp(argg[0], argg);
        }
        if (speacialUsed==6 &&j==count){
            waitpid(pid, &returnStatus, WNOHANG);
        }else {
            waitpid(pid, &returnStatus, 0);
        }
        lastAnd = inds[j];
    }
    // process last coming and
    while (strcmp(argv[argc - 1], "&&") == 0 || strcmp(argv[argc - 1], "&") == 0) {
        argc = 0;
        printf(">");
        fgets(s, 4048, stdin);

        if (s[0] == '\n') {
            continue;
        }
        if ((pos = strchr(s, '\n')) != NULL)
            *pos = '\0';

        p2 = strtok(s, " ");
        while (p2 && argc < 64 - 1) {
            argv[argc++] = p2;
            p2 = strtok(0, " ");
        }
        argv[argc] = 0;
        int pid = fork();
        if (pid == 0) {
            signal(SIGINT, SIG_DFL);
            signal(SIGSTOP, SIG_DFL);
            execvp(argv[0], argv);
        }
        if (speacialUsed==6 && strcmp(argv[argc - 1], "&") == 0){
            waitpid(pid, &returnStatus, WNOHANG);
        }else {
            waitpid(pid, &returnStatus, 0);
        }
    }
    numberOfAnd = 0;
}

void init() {
    getlogin_r(un, sizeof(un));
    gethostname(hn, sizeof(hn));
    env = getenv("HOME");
    envlength = strlen(env);
}

int resultOutToFIle(int argc, char **argv, int i) {
    int def = dup(1);
    char *argg[i];
    int file;
    if (speacialUsed == 3) {
        file = open(argv[argc - 1], O_WRONLY | O_CREAT | O_APPEND);

    } else {
        file = open(argv[argc - 1], O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRGRP | S_IROTH);
    }
    if (file < 0) return 1;

    for (int a = 0; a < i; a++) {
        argg[a] = argv[a];
    }
    argv = argg;
    argv[i] = 0;

    //redirect standard output to the
    if (dup2(file, 1) < 0) return 1;

    int pid = fork();
    if (pid == 0) {
        close(file);
        close(def);
        execvp(argv[0], argv);
        return 0;
    }
    dup2(def, 1);
    waitpid(pid, &returnStatus, 0);
    close(def);

    close(file);
    return 0;

}

int resultInFromFIle(int argc, char **argv, int i) {
    int def = dup(0);
    char *argg[i];
    int file;
    file = open(argv[argc - 1], O_RDONLY);
    if (file < 0) return 1;

    for (int a = 0; a < i; a++) {
        argg[a] = argv[a];
    }
    argv = argg;
    argv[i] = 0;
    //redirect standard output to the
    if (dup2(file, 0) < 0) return 1;

    int pid = fork();
    if (pid == 0) {
        close(file);
        close(def);
        execvp(argv[0], argv);
        return 0;
    }
    dup2(def, 0);
    waitpid(pid, &returnStatus, 0);
    close(def);

    close(file);
    return 0;

}

void pipeHandler(int argc, char **argv, const int *inds, int count) {
    int lastPipe = 0;
    int z;
    int length = inds[0] - lastPipe - 0;
    char *argg[length];
    z = 0;
    for (int i = lastPipe + 0; i < inds[0]; ++i) {
        argg[z] = argv[i];
        z++;
    }
    argg[z]=0;
    int lastPipe2 = inds[0];
    int length2 = inds[1] - lastPipe2 - 1;
    char *argg2[length2];
    z = 0;
    for (int i = lastPipe2 + 1; i < inds[1]; ++i) {
        argg2[z] = argv[i];
        z++;
    }
    argg2[z]=0;

    int pipefd[2];

    if (pipe(pipefd)) {
        perror("error occurred on pipe stage");
        exit(127);
    }
    pid=fork();
    switch (pid) {
        case -1:
            perror("fork()");
            break;
        case 0:
            signal(SIGINT, SIG_DFL);
            signal(SIGSTOP, SIG_DFL);
            close(pipefd[0]);
            dup2(pipefd[1], 1);
            close(pipefd[1]);
           execvp(argg[0], argg);

            break;
        default:
            close(pipefd[1]);
            dup2(pipefd[0], 0);
            close(pipefd[0]);
            execvp(argg2[0], argg2);
            break;
    }
    numberOfPipe = 0;
}

void loopPipe(int argc, char **argv, const int *inds, int count){

    int   p[2];
    pid_t pid;
    int   fd_in = 0;

    int lastAnd = 0;
    int z = 0;
    for (int j = 0; j < count; ++j) {
        int offset = lastAnd == 0 ? 0 : 1;
        int length = inds[j] - lastAnd - offset;
        char *argg[length];
        z = 0;
        for (int i = lastAnd + offset; i < inds[j]; ++i) {
            argg[z] = argv[i];
            z++;
        }
        lastAnd = inds[j];
        argg[z]=0;
        pipe(p);
        if ((pid = fork()) == -1)
        {
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            dup2(fd_in, 0);
            if (j+1!=count)
                dup2(p[1], 1);
            close(p[0]);
            execvp(argg[0], argg);
            exit(EXIT_FAILURE);
        }
        else
        {
            wait(NULL);
            close(p[1]);
            fd_in = p[0];
        }

    }
}
void processCommand(int argc, char **argv) {
    indexesofand = malloc(sizeof(int) * 64);
    indexesofpipes = malloc(sizeof(int) * 64);
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], ">") == 0) {
            speacialUsed = 1;
            resultOutToFIle(argc, argv, i);
        } else if (strcmp(argv[i], "<") == 0) {
            speacialUsed = 2;
            resultInFromFIle(argc, argv, i);
        } else if (strcmp(argv[i], ">>") == 0) {
            speacialUsed = 3;
            resultOutToFIle(argc, argv, i);
        } else if (strcmp(argv[i], "|") == 0) {
            speacialUsed = 4;
            indexesofpipes[numberOfPipe] = i;
            numberOfPipe++;

        } else if (strcmp(argv[i], "&&") == 0) {
            speacialUsed = 5;
            indexesofand[numberOfAnd] = i;
            numberOfAnd++;
        } else if (strcmp(argv[i], "&") == 0) {
            speacialUsed = 6;
            indexesofand[numberOfAnd] = i;
            numberOfAnd++;

        }
    }
    int status;
    if (speacialUsed == 5 || speacialUsed == 6) {
        if (strcmp(argv[argc - 1], "&&") == 0 || strcmp(argv[argc - 1], "&") == 0) {

        } else {
            indexesofand[numberOfAnd] = argc;
            numberOfAnd++;
        }
        andHandler(argc, argv, indexesofand, numberOfAnd);
    }
    if (speacialUsed == 4) {
        if (strcmp(argv[argc - 1], "|") == 0) {

        } else {
            indexesofpipes[numberOfPipe] = argc;
            numberOfPipe++;
        }
        switch ((pid = fork())) {
            case -1:
                perror("fork");
                break;
            case 0:
                signal(SIGINT, SIG_DFL);
                signal(SIGSTOP, SIG_DFL);
                printf(" ");
                if (numberOfPipe>2){
                    loopPipe(argc, argv, indexesofpipes, numberOfPipe);
                }
                pipeHandler(argc, argv, indexesofpipes, numberOfPipe);
                break;
            default:
                pid = wait(&status);
                numberOfPipe = 0;
                break;
        }

    }
}

int main(int argc, char **argv) {
    init();
    init_cwd();

    signal(SIGINT, SIG_IGN);
    signal(SIGSTOP, SIG_IGN);
    while (1) {

        argc = 0;
        fprintf(stdout, "%s@%s:~%s$ ", un, hn, cwd);
        fgets(s, 4048, stdin);

        if (s[0] == '\n') {
//            fprintf(stdout, "%s@%s:~%s$ ", un, hn, cwd);
            continue;
        }
        if ((pos = strchr(s, '\n')) != NULL)
            *pos = '\0';

        pos = strchr(s, '\\');
        if ((pos = strchr(s, '\\')) != NULL)
            *pos = '\0';

//      for taking new line
        while (pos) {
            char s2[4048] = "";
            printf(">");
            fgets(s2, 4048, stdin);

            if (s2[0] == '\n') {
                break;
            }
            if ((pos = strchr(s2, '\n')) != NULL)
                *pos = '\0';

            if ((pos = strchr(s2, '\\')) != NULL)
                *pos = '\0';

            strcpy(s, s);
            strcat(s, s2);
        }
//        end taking line
//        processing command
        p2 = strtok(s, " ");
        while (p2 && argc < 64 - 1) {
            argv[argc++] = p2;
            p2 = strtok(0, " ");
        }
        argv[argc] = 0;

        processCommand(argc, argv);


        if (speacialUsed == 0) {
            pid = fork();

            if (pid == 0) {
                signal(SIGINT, SIG_DFL);

                if (strcmp("clear", argv[0]) == 0) {
                    clearScreen();
                    return 1;
                }
                if (strcmp("cd", argv[0]) == 0) {

                    return 1;
                }
                if ((strcmp(argv[0], "exit") == 0) || (strcmp(argv[0], "quit") == 0)) {
                    kill(0, SIGTERM);
                }

                exec_code = execvp(argv[0], argv);//normal execution

                if (exec_code == -1) {
                    printf("%s: command not found\n", argv[0]);
                    exit(1);
                }
            } else {
                waitpid(pid, &returnStatus, 0);
                if (strcmp("cd", argv[0]) == 0) {
                    changeDir(argv);
                }
//                fprintf(stdout, "%s@%s:~%s$ ", un, hn, cwd);
            }
        } else {
            if (speacialUsed > 0) {
                speacialUsed = 0;
            }
            continue;
        }
    }

}