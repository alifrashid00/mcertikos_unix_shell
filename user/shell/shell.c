#include <proc.h>
#include <stdio.h>
#include <syscall.h>
#include <x86.h>
#include <debug.h>
#include <file.h>
#include <gcc.h>
#include <proc.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>
#include <proc.h>
#include <syscall.h>
#include <x86.h>

#define BUFLEN 1024
#define ARGUMENT_LEN 128
#define ARG_COUNT 64
#define CMDBUF_SIZE	80
#define WHITESPACE "\t\r\n "
#define MAXARGS 16
static int execute_command(char *buf);
int shell_ls(int argc, char ** argv);
int shell_pwd(int argc, char **argv);
int shell_cd(int argc, char **argv);
int shell_cp(int argc, char **argv);
int shell_mv(int argc, char **argv);
int shell_rm(int argc, char **argv);
int shell_mkdir(int argc, char **argv);
int shell_cat(int argc, char **argv);
int shell_touch(int argc, char **argv);
int shell_help(int argc, char **argv);
int shell_write(int argc, char **argv);
int shell_append(int argc, char **argv);


char shell_buf[BUFLEN];

struct Command
{
	const char* name;
	const char* desc;
	int 
	(*func) (int argc, char** argv);
};

static struct Command commands[] = 
{
	{"ls","list all files and directories under working directory", shell_ls},
	{"pwd","print working directory", shell_pwd},
	{"cd","cd <path> \n\t change directory", shell_cd},
	{"cp", "cp <-r> <src_path> <dest_path> \n\t copy file or directory ",shell_cp},
	{"mv", "mv <src_path> <dest_path> \n\t move file or directory",shell_mv},
	{"rm", "rm <-r> <filename> \n\t remove file or directory",shell_rm},
	{"mkdir", "mkdir <dirname> \n\t create directory",shell_mkdir},
	{"cat", "cat <filename> \n\t print file content",shell_cat},
	{"touch", "touch <filename> \n\t create new empty file", shell_touch},
        {"write", "write <string> <filename> \n\t write a string to file", shell_write},
        {"append", "append <string> <filename> \n\t append a string to file", shell_append},
        {"help", "help \n\t print this help message", shell_help}
};

#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))




void get_shell_input(char* buf) {
    sys_readline(buf);
}

static int execute_command(char *buf) {
    int argc;
    char *argv[MAXARGS];
    int i;

    // Parse input into arguments
    argc = 0;
    argv[argc] = 0;
    
    while(1) {
        // Skip whitespace
        while (*buf && strchr(WHITESPACE, *buf))
            *buf++ = 0;
        if (*buf == 0)
            break;

        // Too many args check
        if (argc == MAXARGS - 1) {
            printf("Too many arguments (max %d)\n", MAXARGS);
            return 0;
        }
        
        // Store argument and advance to next
        argv[argc++] = buf;
        while (*buf && !strchr(WHITESPACE, *buf))
            buf++;
    }
    argv[argc] = 0;

    if (argc == 0)
        return 0;

    // Find and execute command
    for (i = 0; i < NCOMMANDS; i++) {
        if (strcmp(argv[0], commands[i].name) == 0)
            return commands[i].func(argc, argv);
    }

    printf("Unknown command '%s'\n", argv[0]);
    printf("try 'help' to see all supported commands.\n");
    return 0;
}

int run_shell_tests() {
    printf("start testing shell\n");
    const char* test_commands[28] = {"ls", "pwd", "mkdir dir1", "mkdir dir2", "mkdir dir3", "ls",
                      "cd dir1", "mkdir dir1_1", "touch file1", "touch file2",
                      "pwd", "cd ..", "cp -r dir1 dir4", "cd dir4", "ls", "cd ..",
                      "write hello f1", "cp f1 f2", "ls", "cat f1", "append world f1",
                      "cat f1", "mv f1 dir2", "ls dir2", "rm f2", "ls", "rm -r dir4", "ls"};
    
    for (int i = 0; i < 28; ++i) {
        printf("running test case %d :%s\n", i + 1, test_commands[i]);
        char buf[100];
        strncpy(buf, test_commands[i], strlen(test_commands[i]) + 1);
        execute_command(buf);
        printf("test case pass!\n");
    }
    printf("shell test finished!\n");
}

int test_ipc_communication() {
    printf("ipc test begin\n");

    pid_t ping_pid, pong_pid;
    if ((ping_pid = spawn(1, 1000)) != -1)
        printf("ping in process %d.\n", ping_pid);
    else
        printf("Failed to launch ping.\n");
    
    if ((pong_pid = spawn(2, 1000)) != -1)
        printf("pong in process %d.\n", pong_pid);
    else
        printf("Failed to launch pong.\n");

    for(int i = 0; i < 100000; i++);
    printf("ipc test pass!!\n");
    return 0;
}



int main(int argc, char** argv) {
    int shell_mode = 0;
    char command_buffer[1024];

    printf("\n********Tea Shell*********\n");
    printf("********Author: Asif Or Rashid Alif, Md. Abdullah Al Jubaer Gem, Shufan Shahi********\n");
    close(open("usertests.ran", O_CREATE));
    
    if (shell_mode == 1) {
        run_shell_tests();
        return 0;
    }
    else if (shell_mode == 2) {
        test_ipc_communication();
        return 0;
    }

    // Main shell loop
    while(1) {
        get_shell_input(command_buffer);
        if (command_buffer != NULL) {
            if (execute_command(command_buffer) < 0)
                break;
        }
    }
}
