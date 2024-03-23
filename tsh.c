#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <limits.h>
#include <sys/ioctl.h>

struct termios terminal_settings;

int tsh_executeCmd(char** args) {
    int pid;
    int wpid;
    int status;

    // check if token array is empty
    // NOT SURE IF THIS IS CATCHING
    if (args[0] == NULL) {return -1;}

    // check if command is a builtin
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {return 1;}
        if (chdir(args[1]) != 0) {return -1;}
    } else if (strcmp(args[0], "exit") == 0) {
        return 0;
    }

    // create child process
    pid = fork();
    if (pid == 0) {
        // child process execute command
        execvp(args[0], args);
        exit(EXIT_FAILURE);
    } else if (pid < 0 ) {
        // fork error
        return -1;
    } else {
        // parent process wait for child
        do {wpid = waitpid(pid, &status, WUNTRACED);}
        while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

char** tsh_parseLine(char* line) {
    int line_p = 0; // line buffer position
    int args_s = 10; // args buffer size
    int args_p = 0; // args buffer position
    int arg_s = 20; // arg buffer size
    int arg_p = 0; // arg buffer position
    char** args = calloc(args_s, sizeof(char*));

    while (1) {
        // allocate space for next token
        args[args_p] =  calloc(arg_s, sizeof(char));

        // parse next token
        while (1) {
            // if end of line is reached
            if (line[line_p] == '\0') {
                // if we are building a token null-terminate it
                if (arg_p != 0) {
                    args[args_p][arg_p] = '\0';
                    args[args_p+1] = NULL;
                } else {args[args_p] = NULL;}
                // return token array
                return args;
            // if escape character
            } else if (line[line_p] == '\\') {
                // get next character and determine escape sequence
                switch(line[line_p+1]) {
                // ignore escape character not part of defined escape sequence
                default:
                    line_p++;
                }
            // if space character
            } else if (line[line_p] == ' ') {
                // if we are building a token null-terminate it, set next token to NULL
                if (arg_p != 0) {
                    args[args_p][arg_p] = '\0';
                    }
                line_p++;
                break;
            // if regular character
            } else {
                // add character to token string
                args[args_p][arg_p] = line[line_p];
                line_p++;
                arg_p++;
            }
            // reallocate more token characters if required
            if (arg_p >= arg_s) {
            arg_s += arg_s;
            args[args_p] = realloc(args[args_p], arg_s * sizeof(char));
            if (!args[args_p]) {return NULL;}
            }   
        }
        // set variables for next token string
        args_p++;
        arg_p = 0;

        // reallocate more token pointers if required
        if (args_p >= args_s) {
        args_s += args_s;
        args = realloc(args, args_s * sizeof(char*));
        if (!args[args_p]) {return NULL;}
        } 
    }
}

char* tsh_getLine(char* prompt, int prompt_l) {
    int buffer_s = 100; // line buffer total size
    int buffer_l = 0; // line buffer character length
    int buffer_p = 0; // line buffer cursor position
    int buffer_o = 0; // line buffer display offset
    int buffer_dl = 0; // line buffer display length
    char cursor_p[10]; // cursor position escape sequence
    char c; // input character
    char eseq[3]; // input escape sequence
    int window_c; // terminal window column width

    // initialize line buffer
    char* buffer = calloc(buffer_s, sizeof(char)); // line buffer
    buffer[0] = '\0';

    // get line display length
    struct winsize ws;
    if (ioctl(1, TIOCGWINSZ, &ws) == -1) {window_c = 80;} else {window_c = ws.ws_col - 1;}
    buffer_dl = window_c - prompt_l;

    do {
        // refresh line
        snprintf(cursor_p, sizeof(cursor_p), "\x1b[%iG", prompt_l + 1 + buffer_p - buffer_o);
        write(STDOUT_FILENO, "\x1b[0G", strlen("\x1b[0G"));
        write(STDOUT_FILENO, prompt, prompt_l);
        write(STDOUT_FILENO, (buffer + buffer_o), (buffer_s < buffer_dl) ? buffer_s : buffer_dl);
        write(STDOUT_FILENO, "\x1b[0K", strlen("\x1b[0K"));
        write(STDOUT_FILENO, cursor_p, strlen(cursor_p));
        // read-in next character
        read(STDIN_FILENO, &c, 1);

        // handle character
        switch(c) {
            case 13: // enter
                goto returnLine;
            case 8: // ctrl+h
            case 127: // backspace
                if (buffer_p > 0) {
                    memmove(buffer+(buffer_p-1), buffer+buffer_p , buffer_l - buffer_p);
                    buffer_p--;
                    buffer_l--;
                    buffer[buffer_l] = '\0';
                }
                break;
            case 3: // ctrl+c
                return NULL;
            case 4: // ctrl+d
                break;
            case 20: // ctrl+t
                break; 
            case 16: // ctrl+p
                break; 
            case 14: // ctrl+n
                break;
            case 11: // ctrl+k
                break;
            case 1: // ctrl+a
                break;
            case 5: // ctrl+e
                break;
            case 12: // ctrl+l
                break;
            case 23: // ctrl+w
                break;
            case 21: // ctrl+u
                free(buffer);
                buffer = calloc(buffer_s, sizeof(char));
                buffer_p = 0;
                buffer_o = 0;
                buffer_l = 0;
                break;
            case 27: // escape character
                // read-in the next two characters
                if (read(STDOUT_FILENO, eseq, 1) == -1) {break;}
                if (read(STDOUT_FILENO, eseq+1, 1) == -1) {break;}
                if (eseq[0] == '[') {
                    switch(eseq[1]) {
                        // right arrow key
                        case 'C':
                            if (buffer_p < buffer_l) {buffer_p++;}
                            if ((buffer_p - buffer_o) > buffer_dl) {buffer_o++;}
                            break;
                        // left arrow key
                        case 'D':
                            if (buffer_p > 0) {buffer_p--;}
                            if ((buffer_p - buffer_o) < 0) {buffer_o--;}
                            break;
                        // up arrow key
                        case 'A':
                            // get previous record in history
                            break;
                        // down arrow key
                        case 'B':
                            // get next record in history
                            break;
                    }
                }
                break;
            default: // store character in buffer
                memmove(buffer+buffer_p+1, buffer+buffer_p, buffer_l - buffer_p);
                buffer[buffer_p] = c;
                buffer_p++;
                buffer_l++;
                buffer[buffer_l] = '\0';
                if ((buffer_p - buffer_o) > buffer_dl) {buffer_o++;}
                break;
        }
        // allocate more space for buffer if required
        if (buffer_l >= buffer_s) {
            buffer_s += buffer_s;
            buffer = realloc(buffer, buffer_s);
            if (!buffer) {return NULL;}
        }
    } while (1);

    returnLine:
        write(STDOUT_FILENO, "\n\r", sizeof("\n\r"));
        return buffer;
}

int enableRawTerminal() {
    struct termios modified_settings;

    // check TTY device
    if (!isatty(STDIN_FILENO)) {return -1;} 

    // change terminal settings
    modified_settings = terminal_settings;
    modified_settings.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    modified_settings.c_oflag &= ~(OPOST);
    modified_settings.c_cflag |= (CS8);
    modified_settings.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    modified_settings.c_cc[VMIN] = 1; 
    modified_settings.c_cc[VTIME] = 0;

    // set new terminal settings
    if (tcsetattr(STDIN_FILENO,TCSAFLUSH,&modified_settings) == -1) {return -1;};

    return 0;
}

void disableRawTerminal() {
    // restore initial settings
    tcsetattr(STDIN_FILENO,TCSAFLUSH,&terminal_settings);
}

int main(int argc, char **argv) {
    char host[_POSIX_HOST_NAME_MAX];
    char cwd[PATH_MAX];
    char prompt[50];
    int prompt_l;
    int status;
    char* line;
    char** args;

    // Enable raw terminal mode
    if (tcgetattr(STDIN_FILENO, &terminal_settings) == -1) {return 1;} 
    if (enableRawTerminal() == -1) {return 1;}
    // if (atexit(disableRawTerminal) != 0) {return 1;}

    // main program loop
    do {
        // assemble prompt string and get its length
        if (gethostname(host, sizeof(host)) == -1 || getcwd(cwd, sizeof(cwd)) == NULL) {return -1;} {
            prompt_l = snprintf(prompt, 50, "%s@%s %s: ", getlogin(), host, strrchr(cwd, '/'));
        }
        // get command line, parse, and execute
        if ((line = tsh_getLine(prompt, prompt_l)) == NULL) {return -1;}
        if ((args = tsh_parseLine(line)) == NULL) {return -1;}
        if ((status = tsh_executeCmd(args)) == -1) {return -1;}
    } while (status);

    // disableRawTerminal();
}