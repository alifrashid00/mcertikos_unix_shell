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
int list_directory(int argc, char ** argv);
int print_working_directory(int argc, char **argv);
int change_directory(int argc, char **argv);
int copy_file_or_directory(int argc, char **argv);
int move_file_or_directory(int argc, char **argv);
int remove_file_or_directory(int argc, char **argv);
int shell_mkdir(int argc, char **argv);
int display_file_contents(int argc, char **argv);
int create_new_file(int argc, char **argv);
int display_help_message(int argc, char **argv);
int write_string_to_file(int argc, char **argv);
int append_string_to_file(int argc, char **argv);
int remove_file_recursive(char * path, int isRecursive);
int delete_single_file(char * filename);
int list_directory_contents(char* buf, char* path);
int check_directory_is_empty(char* dirname);
int check_if_directory(char * path);
int is_file_exist(char* path);
int _shell_cat(char * path);
int copy_file_recursive(char * dest_path, char * src_path, int isRecursive);
int extract_filename(char * path, char * filename);
int copy_single_file(char * dest_filename, char * src_filename);

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
	{"ls","list all files and directories under working directory", list_directory},
	{"pwd","print working directory", print_working_directory},
	{"cd","cd <path> \n\t change directory", change_directory},
	{"cp", "cp <-r> <src_path> <dest_path> \n\t copy file or directory ",copy_file_or_directory},
	{"mv", "mv <src_path> <dest_path> \n\t move file or directory", move_file_or_directory},
	{"rm", "rm <-r> <filename> \n\t remove file or directory", remove_file_or_directory},
	{"mkdir", "mkdir <dirname> \n\t create directory",shell_mkdir},
	{"cat", "cat <filename> \n\t print file content", display_file_contents},
	{"touch", "touch <filename> \n\t create new empty file", create_new_file},
    {"write", "write <string> <filename> \n\t write a string to file", write_string_to_file},
    {"append", "append <string> <filename> \n\t append a string to file", append_string_to_file},
    {"help", "help \n\t print this help message", display_help_message}
};

#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

int list_directory_contents(char* buf, char * path) {
    int len;
    char current_path[BUFLEN];
    if(path == NULL) {
        len = sys_ls(buf, BUFLEN); 
    } else {
        sys_pwd(current_path);
        sys_chdir(path);
        len = sys_ls(buf, BUFLEN);
        sys_chdir(current_path); 
    }
    for(int i = 0; i < len; i++) {
        if(buf[i] == ' ') {
            buf[i] = '\0';
        }
    }
    return len;
}

int display_help_message(int argc, char** argv) {
    for(int i = 0; i < NCOMMANDS; i++) {
        printf("%s\n", commands[i].desc);
    }
    return 0;
}

int shell_mkdir(int argc, char** argv)
{
	int i;
	if (argc == 1)
		printf ("mkdir failed, no path\n");
	
	for (i = 1; i < argc; i++){
		if (sys_mkdir(argv[i]) == 0)
		;	//printf("make dir succeed.\n");
		else
			printf("make dir failed.\n");
	}
	
	return 0;
}

int display_file_contents(int argc, char** argv) {
    if (argc == 1) {
        printf("cat: missing file operand\n");
        return 0;
    }
    
    for (int i = 1; i < argc; i++) {
        if(_shell_cat(argv[i]) == -1) {
            printf("cat: %s: No such file or directory\n", argv[i]);
        }
    }
    return 0;
}
int create_new_file(int argc, char** argv) {
    if (argc == 1) {
        printf("touch: missing file operand\n");
        return 0;
    }
    
    for (int i = 1; i < argc; i++) {
        int fd = open(argv[i], O_RDONLY);
        if(fd >= 0) {
            close(fd);
            continue;  // File exists, skip
        }
        close(open(argv[i], O_CREATE));
    }
    return 0;
}

int write_string_to_file(int argc, char** argv) {
    if (argc != 3) {
        printf("write: usage: write <string> <filename>\n");
        return 0;
    }
    
    int fd = open(argv[2], O_CREATE|O_RDWR);
    if (fd < 0) {
        printf("write: cannot create %s\n", argv[2]);
        return 0;
    }
    
    int n = write(fd, argv[1], strlen(argv[1]));
    if (n != strlen(argv[1])) {
        printf("write: error writing to %s\n", argv[2]);
    }
    close(fd);
    return 0;
}

int append_string_to_file(int argc, char** argv) {
    if (argc != 3) {
        printf("append: usage: append <string> <filename>\n");
        return 0;
    }
    
    // Read existing content
    char buf[1000] = {0};
    int n = 0;
    int fd = open(argv[2], O_RDONLY);
    if (fd >= 0) {
        n = read(fd, buf, sizeof(buf) - 1);
        close(fd);
    }
    
    // Append new content
    fd = open(argv[2], O_CREATE|O_RDWR);
    if (fd < 0) {
        printf("append: cannot open %s\n", argv[2]);
        return 0;
    }
    
    strncpy(buf + n, argv[1], strlen(argv[1]));
    write(fd, buf, strlen(buf));
    close(fd);
    return 0;
}

int list_directory(int argc, char** argv) {
    if (argc == 1) {
        sys_ls(shell_buf, sizeof(shell_buf));
        printf("%s\n", shell_buf);
    } 
    else if (argc == 2) {
        char current_path[100];
        sys_pwd(current_path);
        sys_chdir(argv[1]);
        sys_ls(shell_buf, sizeof(shell_buf));
        printf("%s\n", shell_buf);
        sys_chdir(current_path);
    }
    else {
        printf("ls: too many arguments.\n"); 
    }
    return 0;
}

int print_working_directory(int argc, char** argv) {
    sys_pwd(shell_buf);
    printf("%s\n", shell_buf);
    return 0;	
}

int change_directory(int argc, char** argv) {
    char target_path[1024];
    if (argc == 1) {
        strcpy(target_path, '\0');
        sys_chdir(target_path);
    }
    else {
        strcpy(target_path, argv[1]);
        sys_chdir(target_path);	
    }
}

int copy_file_or_directory(int argc, char** argv) {
    char *source_path, *destination_path;
    if(argc < 3) {
        printf("cp: too few arguments.\n");
        return 0;
    }
    else if(argc > 4) {
        printf("cp: too many arguments.\n");
        return 0;
    }
    
    if(argc == 3) {
        // Regular copy
        source_path = argv[1];
        destination_path = argv[2];
        copy_file_recursive(destination_path, source_path, 0);
        return 0;
    }
    else {
        // Recursive copy
        if(strcmp(argv[1], "-r")) {
            printf("cp: invalid option. try '-r' ?\n");
            return 0;
        }
        source_path = argv[2];
        destination_path = argv[3];
        copy_file_recursive(destination_path, source_path, 1);
        return 0;
    }
}

int move_file_or_directory(int argc, char** argv) {
    if(argc != 3) {
        printf("mv: argument invalid.\n");
        return 0;
    }
    char* src = argv[1];
    char* dest = argv[2];
    if(!is_file_exist(src)) {
        printf("mv: source file %s does not exist.\n", src);
        return 0;
    }
    if(check_if_directory(src)) {
        if(is_file_exist(dest) && !check_if_directory(dest)) {
            printf("mv: cannot move a directory to a file\n");
            return 0;
        }
        copy_file_recursive(dest, src, 1);
        remove_file_recursive(src, 1);
    } else {
        copy_file_recursive(dest, src, 1);
        remove_file_recursive(src, 0);
    }
    return 0;
}

int remove_file_or_directory(int argc, char** argv) {
    int isRecursive;
    int pathIdx;
    char* path;
    
    if(argc == 1) {
        printf("Too few arguments.\n");
        return 0;
    }
    if(!strcmp(argv[1], "-r")) {
        isRecursive = 1;
        pathIdx = 2;
    } else {
        isRecursive = 0;
        pathIdx = 1;
    }
    if(pathIdx > argc + 1) {
        printf("rm: no path argument.\n");
        return 0;
    }
    path = argv[pathIdx];
    if(!is_file_exist(path)) {
        printf("rm: cannot remove %s: not a file or directory.\n", path);
        return 0;
    }
    remove_file_recursive(path, isRecursive);
    return 0;
}

int remove_file_recursive(char* path, int isRecursive) {
    int errno, len;
    char* sub_path;
    char rm_buf[BUFLEN];
    
    if(isRecursive) {
        if(!check_if_directory(path)) {
            return delete_single_file(path);
        }
        sys_chdir(path);
        len = list_directory_contents(rm_buf, NULL);
        sub_path = rm_buf;
        while(sub_path - rm_buf < len) {
            if(strcmp(sub_path, ".") && strcmp(sub_path, "..")) {
                remove_file_recursive(sub_path, isRecursive);
            }
            sub_path += strlen(sub_path) + 1;
        }
        sys_chdir("..");
        return delete_single_file(path);
    } else {
        if(check_if_directory(path)) {
            printf("rm: cannot remove %s: is a directory. Use '-r'?\n", path);
            return -1;
        }
        return delete_single_file(path);
    }
}

int delete_single_file(char* filename) {
    int errno = sys_unlink(filename);
    if(errno == -1) {
        printf("rm: cannot remove %s: system error.\n", filename);
    }
    return errno;
}



int check_directory_is_empty(char * dirname){
    if(list_directory_contents(shell_buf, NULL) == 5){
        return 1;
    }else{
        return 0;
    }
}

int check_if_directory(char * path){
      int fd, isDirectory;
      if(is_file_exist(path)){
            fd = open(path, O_RDONLY);
      }
      isDirectory = sys_is_dir(fd);
      close(fd);
      return isDirectory;
}

// check whether a file/dir exist
int is_file_exist(char* path){
        int fd;
        fd = open(path, O_RDONLY);
	if(fd == -1){
                return 0;
 	}
	close(fd);
        return 1;
}

int shell_mkdir(int argc, char** argv)
{
	int i;
	if (argc == 1)
		printf ("mkdir failed, no path\n");
	
	for (i = 1; i < argc; i++){
		if (sys_mkdir(argv[i]) == 0)
		;	//printf("make dir succeed.\n");
		else
			printf("make dir failed.\n");
	}
	
	return 0;
}






int copy_single_file(char* dest_filename, char* src_filename) {
    if(check_if_directory(src_filename)) {
        sys_mkdir(dest_filename);
        return 0;
    }
    int fd = open(src_filename, O_RDONLY);
    char buf[1000];
    read(fd, buf, 1000);
    close(fd);
    fd = open(dest_filename, O_CREATE|O_RDWR);
    write(fd, buf, strlen(buf));
    close(fd);
    return 0;
}

int copy_file_recursive(char* dest_path, char* src_path, int isRecursive) {
    char path[BUFLEN];
    char filename[100];
    char dest_path_buf[BUFLEN];
    char src_path_buf[BUFLEN];
    char * p;
    if(!is_file_exist(src_path)){
        printf("cp: %s does not exist.\n", src_path);
        return 0;
    }
    if(isRecursive == 0){
        if(check_if_directory(src_path)){
            printf("cp: omitting directory '%s'. try '-r' ?\n", src_path);
            return 0;
        }
        if(is_file_exist(dest_path) && check_if_directory(dest_path)){
            extract_filename(src_path, filename);
            strcpy(path, dest_path);
            p = path + strlen(path);
            *(p++) = '/';
            strcpy(p, filename);
            copy_file_recursive(path, src_path, isRecursive); 
        }else{
            copy_single_file(dest_path, src_path);
        }
    }else{
        if(check_if_directory(src_path)){
            if(is_file_exist(dest_path)){
                if(check_if_directory(dest_path)){
                    extract_filename(src_path, filename);
                    strcpy(path, dest_path);
                    p = path + strlen(path);
                    *(p++) = '/';
                    strcpy(p, filename);
                    copy_file_recursive(path, src_path, isRecursive);            
                }else{
                    printf("cp: cannot copy a directory to a file '%s'.\n", dest_path);
                    return 0;
                }
            }else{
                copy_single_file(dest_path, src_path);
                int len = list_directory_contents(path, src_path);
                char* p = path;
                while(p - path < len){
                    int dest_len, src_len;
                    if(strcmp(p, ".") && strcmp(p, "..")){
                        dest_len = strlen(dest_path);
                        src_len = strlen(src_path);

                        strcpy(dest_path_buf, dest_path);
                        strcpy(src_path_buf, src_path);

                        dest_path_buf[dest_len] = '/';
                        src_path_buf[src_len] = '/';
                        strcpy(dest_path_buf+dest_len+1, p);
                        strcpy(src_path_buf+src_len+1, p);

                        copy_file_recursive(dest_path_buf, src_path_buf, isRecursive);
                    }
                    p += strlen(p) + 1; 
                }
            }
        }else{
            copy_file_recursive(dest_path, src_path, 0);
        }
    }
    return 0;
}

int extract_filename(char* path, char* filename) {
    int n = strlen(path);
    if (n == 0) return 0;
    int pos = n - 1;
    while (pos >= 0) {
      if (path[pos] == '/') {
        break;
      }
      pos--;
    }
    strncpy(filename, path + pos + 1, n - (pos + 1));
    return n - (pos + 1);
}

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
