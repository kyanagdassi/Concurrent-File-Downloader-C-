#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

struct file {
    char *name;
    char *link;
    int seconds;
};

// Function for downloading files with struct input
int downloadFile(struct file input) {
    char command[256];
    snprintf(command, sizeof(command), "curl -sS -m %d -o %s %s > /dev/null 2>&1", input.seconds, input.name, input.link);
    execl("/bin/sh", "sh", "-c", command, (char *)NULL);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Invalid command.\n");
        return 1;
    }
    
    struct stat fileStat;
    if (stat(argv[1], &fileStat) != 0) {
        printf("File not found.\n");
        return 1;
    }
    
    int num = atoi(argv[2]);
    if (num <= 0) {
        printf("Invalid positive integer.\n");
        return 1;
    }

    struct file *arr = NULL;
    size_t arr_size = 0;
    size_t capacity = 200; // initial capacity

    arr = malloc(capacity * sizeof(struct file));
    if (arr == NULL) {
        printf("Memory allocation failure.\n");
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        printf("File not found.\n");
        free(arr);
        return 1;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (arr_size >= capacity) {
            capacity *= 2;
            struct file *tmp = realloc(arr, capacity * sizeof(struct file));
            if (tmp == NULL) {
                fclose(file);
                free(arr);
                return 1;
            }
            arr = tmp;
        }

        char fileName[30];
        char link[200];
        int seconds = 100;
        
        if (sscanf(line, "%29s %199s %d", fileName, link, &seconds) < 2) {
            continue;
        }

        arr[arr_size].name = malloc(strlen(fileName) + 1);
        arr[arr_size].link = malloc(strlen(link) + 1);
        
        if (arr[arr_size].name == NULL || arr[arr_size].link == NULL) {
            fclose(file);
            free(arr);
            return 1;
        }

        strcpy(arr[arr_size].name, fileName);
        strcpy(arr[arr_size].link, link);
        arr[arr_size].seconds = seconds;
        arr_size++;
    }
    fclose(file);

    // Downloading files
    size_t i = 0;
    int active_processes = 0;

    while (i < arr_size) {
        if (active_processes < num) {
            pid_t pid = fork();
            if (pid == 0) {
                // Child
                downloadFile(arr[i]);
                free(arr[i].name);
                free(arr[i].link);
                exit(0);
            } else if (pid > 0) {
                // Parent
                printf("process %d processing line #%d\n", (int)pid, (int)(i+1));
                active_processes++;
                i++;
            } else {
                // Fork failure
                perror("fork");
                break;
            }
        } else {
            // Wait for any child process to finish
            wait(NULL);
            active_processes--;
        }
    }

    // Wait for any remaining child processes to finish
    while (active_processes > 0) {
        wait(NULL);
        active_processes--;
    }


    for (i = 0; i < arr_size; i++) {
        free(arr[i].name);
        free(arr[i].link);
    }
    free(arr);

    return 0;
}
