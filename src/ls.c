#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAXLEN 256

void ls(char *dir) {
    DIR *dirp;
    struct dirent *entry;
    struct stat st;
    char ref[MAXLEN];

    printf("%s : \n", dir);

    dirp = opendir(dir);

    if (dirp == NULL) {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dirp))) {
        /* if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) { */
        /* if(entry->d_name[0] != '.') { */
        /* printf("%ld\t%s\n", entry->d_ino, entry->d_name); */
        snprintf(ref, MAXLEN, "%s/%s", dir, entry->d_name);
        if (lstat(ref, &st) == -1) {
            perror("stat");
            continue;
        }
        printf("%ld\t%ld\t%ld\t%s\n", st.st_ino, st.st_dev, entry->d_ino,
               entry->d_name);
        /* } */
    }

    closedir(dirp);
}

int main(int argc, char **argv) {
    if (argc == 1)
        ls(".");
    else
        for (int i = 1; i < argc; i++) ls(argv[i]);
    return 0;
}