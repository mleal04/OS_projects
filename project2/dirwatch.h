// dirwatch.h
#ifndef DIRWATCH_H
#define DIRWATCH_H

struct items
{
    int files;
    int dirs;
    int links;
};

char *open_file(const char *curr_file_path, const char *filename);
void file_info(const char *path, const char *filename, struct items *counts);
void dirwatch_main_call(const char *given_path);

#endif // DIRWATCH_H