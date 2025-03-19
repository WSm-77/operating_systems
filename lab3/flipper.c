#include <stdio.h>
#include <ctype.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

#define REQUIRED_ARGS_CNT 3
#define SRC_DIR_IDX 1
#define TARGET_DIR_IDX 2
#define bool int
#define true 1
#define false 0
#define BUFF_SIZE 1024
#define TXT_EXTENSION_LEN 4

bool has_txt_extension(const char *filePath) {
    const char *extension = strrchr(filePath, '.');

    return NULL != extension && strcmp(extension, ".txt") == 0;
}

bool isTextFile(const char* filePath) {
    // check if file is directory
    struct stat fileStat;

    if (!has_txt_extension(filePath)) {
        return false;
    }

    if (-1 == stat(filePath, &fileStat)) {
        return false;
    }

    if (S_ISDIR(fileStat.st_mode)) {
        return false;
    }

    // check if it is text file
    FILE* file = fopen(filePath, "r");
    if (!file) {
        printf("Couldn't open file %s!!!\n", filePath);
        return false;
    }

    char ch;
    bool result = true;

    while (EOF != (ch = fgetc(file))) {
        if (!isprint(ch) && !isspace(ch)) {
            result = false;
            break;
        }

        // putc(ch, stdout);
    }

    fclose(file);

    return result;
}

int main(int argc, char** argv) {
    if (REQUIRED_ARGS_CNT != argc) {
        printf("Wrong arguments count: %d\n", argc);
        printf("You must specify source directory and target directory!!!\n");

        return 1;
    }

    char* sourceDirPath = argv[SRC_DIR_IDX];
    char* targetDirPath = argv[TARGET_DIR_IDX];

    // printf("%s\n", sourceDirPath);
    // printf("%s\n", targetDirPath);

    DIR* srcDir = opendir(sourceDirPath);

    if (!srcDir) {
        printf("%s directory can't be opened!!!\n", sourceDirPath);

        return 2;
    }

    DIR* targetDir = opendir(targetDirPath);

    if (!targetDir) {
        printf("%s directory can't be opened!!!\n", targetDirPath);

        return 2;
    }

    struct dirent* currFilePtr;
    char fullPath[BUFF_SIZE];

    while (NULL != (currFilePtr = readdir(srcDir))) {
        printf("current file: %s\n", currFilePtr->d_name);

        if ((strcmp(currFilePtr->d_name, ".") == 0) || (strcmp(currFilePtr->d_name, "..") == 0))
            continue;

        snprintf(fullPath, sizeof(fullPath), "%s/%s", sourceDirPath, currFilePtr->d_name);
        printf("full path: %s\n", fullPath);

        if (!isTextFile(fullPath))
            continue;

        printf("text file: %s\n", fullPath);
    }


    closedir(srcDir);
    closedir(targetDir);

    return 0;
}
