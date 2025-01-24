// Name: Maria Leal
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
#include <ctype.h>
#include <sys/ioctl.h>

#define EPERM 1  /* Operation not permitted */
#define ENOENT 2 /* No such file or directory */
#define ESRCH 3  /* No such process */
#define EINTR 4  /* Interrupted system call */

#define MAX_LEN 128
#define MSG "(truncated)"

struct items
{
    int files;
    int dirs;
    int links;
};

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

// to adjust the buffer -strings based on wid
void truncate_help(char *target, size_t wid)
{
    size_t max_len = (wid > 80) ? wid - 60 : 20;

    if (strlen(target) > max_len)
    {
        target[max_len - 3] = '\0';
        strcat(target, "...");
    }
    printf("%-10s", target);
}

// pass in full path + curr file name + count struct + wid
void file_info(const char *path, const char *filename, struct items *counts, const int wid)
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

        // CONTENT
        if (S_ISLNK(fileStat.st_mode)) // simlink content --> show target path
        {
            char target[MAX_LEN] = ""; // make a buffer for the string (restrict to 128)
            ssize_t len = readlink(curr_file_path, target, sizeof(target) - 1);
            if (len != -1)
            {
                target[len] = '\0';
                truncate_help(target, wid); // adjust based on wid
            }
        }
        else if (S_ISREG(fileStat.st_mode)) // file content --> show the first line
        {
            char *buffer = open_file(curr_file_path, filename);
            for (size_t i = 0; buffer[i] != '\0'; i++)
            {
                if (!isprint(buffer[i]))
                {
                    buffer[i] = '#';
                }
            }

            truncate_help(buffer, wid); // adjust based on size
        }
        else if (S_ISDIR(fileStat.st_mode)) // directory content --> just directory
        {
            printf("(directory)");
        }
    }
    else // check for errors
    {
        perror("error checking content");
    }
    // Add an extra newline
    printf("\n");
}

int main(int argc, char *argv[])
{
    while (1)
    {
        system("clear");  // clear screen
        struct winsize w; // get screen sizes before we start the program
        ioctl(STDIN_FILENO, TIOCGWINSZ, &w);
        int wid = w.ws_col;

        // check we have the correct amount of entries
        if (argc != 2)
        {
            printf("There was an error: not enough entries\n");
            exit(1); // exit with failure
        }

        // open the directory with the path given
        char *path = argv[1];
        DIR *dir = opendir(path);
        if (dir == NULL) // check for error when syscall OPEN
        {
            printf("Unable to open %s -->  %s\n", path, strerror(errno));
            exit(1);
        }

        // begin the template
        time_t curr_time = time(NULL);
        struct tm *local_time = localtime(&curr_time);
        printf("Contents of %s on %02d:%02d:%02d on %02d-%02d-%04d\n", argv[1], local_time->tm_hour, local_time->tm_min, local_time->tm_sec, local_time->tm_mday, local_time->tm_mon + 1, local_time->tm_year + 1900);
        printf("\n");
        printf("%-20s %-10s %-8s %-6s %-10s %-*s\n", "NAME", "SIZE", "TYPE", "MODE", "OWNER", wid, "CONTENTS");
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
            file_info(path, entry->d_name, &counts, wid);
            line_c++;
        }

        // print the bottom syntax
        printf("-----------------------------------------------------------------------------------\n");
        printf("total: %d files, %d directories, %d simlinks\n", counts.files, counts.dirs, counts.links);
        fflush(stdout);
        // Close the directory
        closedir(dir);
        printf("\n");
        sleep(3);
    }
    return 0;
}
