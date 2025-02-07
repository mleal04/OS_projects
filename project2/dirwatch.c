// #include <stdio.h>
// #include <stdlib.h>
// #include <dirent.h>
// #include <sys/stat.h>
// #include <string.h>

// #include <pwd.h>
// #include <grp.h>
// #include <unistd.h>
// #include <fcntl.h>
// #include <errno.h>
// #include <time.h>
// #include <ctype.h>
// #include <sys/ioctl.h>

// dirwatch.c
#include "dirwatch.h"
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>

#define EPERM 1  /* Operation not permitted */
#define ENOENT 2 /* No such file or directory */
#define ESRCH 3  /* No such process */
#define EINTR 4  /* Interrupted system call */

#define MAX_LEN 128
#define MSG "(truncated)"

// struct items
// {
//     int files;
//     int dirs;
//     int links;
// };

// fucntion to open the files inside the directory
char *open_file(const char *curr_file_path, const char *filename)
{
    static char buffer[MAX_LEN]; // buffer for string info (restricted to 128)
    memset(buffer, 0, MAX_LEN);  // start the buffer

    // open the file
    FILE *fd = fopen(curr_file_path, "r");
    if (fd)
    {
        if (fgets(buffer, sizeof(buffer), fd)) // fill in the buffer-string with fgets
        {
            // remove new line and place null --> to help us know the end
            size_t len = strlen(buffer);
            if (len > 0 && buffer[len - 1] == '\n')
            {
                buffer[len - 1] = '\0';
            }
        }
        fclose(fd);
    }
    else
    {
        // Print error message and exit
        printf("Unable to open %s: %s\n", filename, strerror(errno));
        exit(1);
    }

    return buffer; // Return the buffer containing file content
}

// pass in full path + curr file name + count struct + wid
void file_info(const char *path, const char *filename, struct items *counts)
{
    // construct the full path
    char curr_file_path[1024];
    snprintf(curr_file_path, sizeof(curr_file_path), "%s/%s", path, filename);

    // use stat to get metadata --> file or folder
    struct stat fileStat;

    // use lstats to check for link as well
    if (lstat(curr_file_path, &fileStat) == 0)
    {
        // FILE NAME
        printf("%-20s ", filename);

        // FILE SIZE
        printf("%-10lldB ", fileStat.st_size);

        // FILE TYPE --> AVALAILE BC OF LSTAT
        if (S_ISLNK(fileStat.st_mode)) // simlink
        {
            printf("%-8s ", "link");
            counts->links++;
        }
        else if (S_ISREG(fileStat.st_mode)) // reg file
        {
            printf("%-8s ", "file");
            counts->files++;
        }
        else if (S_ISDIR(fileStat.st_mode)) // directory
        {
            printf("%-8s ", "dir");
            counts->dirs++;
        }

        // MODE PRINT
        printf("%04o ", fileStat.st_mode & 0777);

        // OWNER
        struct passwd *pw = getpwuid(fileStat.st_uid);
        printf("%-10s ", pw ? pw->pw_name : "Unknown");
    }
    else // check for errors
    {
        perror("error checking content");
    }
    // Add an extra newline
    printf("\n");
}

// pass a pointer to file name because we need to give it as input to opendir
void dirwatch_main_call(const char *given_path)
{

    system("clear");  // clear screen
    struct winsize w; // get screen sizes before we start the program
    ioctl(STDIN_FILENO, TIOCGWINSZ, &w);
    // int wid = w.ws_col;

    // open the directory with the path given
    DIR *dir = opendir(given_path);
    if (dir == NULL) // check for error when syscall OPEN
    {
        printf("Unable to open %s -->  %s\n", given_path, strerror(errno));
        return;
    }

    // begin the template
    time_t curr_time = time(NULL);
    struct tm *local_time = localtime(&curr_time);
    printf("Contents of %s on %02d:%02d:%02d on %02d-%02d-%04d\n", given_path, local_time->tm_hour, local_time->tm_min, local_time->tm_sec, local_time->tm_mday, local_time->tm_mon + 1, local_time->tm_year + 1900);
    printf("\n");
    printf("%-20s %-10s %-8s %-6s %-10s\n", "NAME", "SIZE", "TYPE", "MODE", "OWNER");
    printf("-----------------------------------------------------------------------------------\n");
    fflush(stdout);

    // create a dict to keep track of counts
    struct items counts = {0, 0, 0};

    // read the directory file
    struct dirent *entry;
    int line_c = 0;
    while ((entry = readdir(dir)) != NULL)
    {
        if (line_c >= w.ws_row - 5) // check if we have to truncate on this row
        {
            printf("%s\n", MSG);
            break;
        }
        // skip the parent dirs
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        // send the path, curr_file, struct, wid
        file_info(given_path, entry->d_name, &counts);
        line_c++;
    }

    // print the bottom syntax
    printf("-----------------------------------------------------------------------------------\n");
    printf("total: %d files, %d directories, %d simlinks\n", counts.files, counts.dirs, counts.links);
    fflush(stdout);
    // Close the directory
    closedir(dir);
    printf("\n");
    // sleep(3);
}
