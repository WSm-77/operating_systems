#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
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
    }

    fclose(file);

    return result;
}

bool flipFile(FILE* srcFile, FILE* targetFile) {
    if (!srcFile || !targetFile) {
        return false;
    }
    ssize_t lineSize;
    size_t buffSize = 0;
    char* line;


    while ((lineSize = getline(&line, &buffSize, srcFile)) != -1) {
        ssize_t iterCnt = lineSize / 2 - 1;

        for (ssize_t i = 0; i < iterCnt; i++) {
            char tmp = line[i];
            ssize_t j = lineSize - 2 - i;
            line[i] = line[j];
            line[j] = tmp;
        }

        fprintf(targetFile, "%s", line);
    }

    return true;
}

int main(int argc, char** argv) {
    if (REQUIRED_ARGS_CNT != argc) {
        printf("Wrong arguments count: %d\n", argc);
        printf("You must specify source directory and target directory!!!\n");

        return 1;
    }

    char* sourceDirPath = argv[SRC_DIR_IDX];
    char* targetDirPath = argv[TARGET_DIR_IDX];

    DIR* srcDir = opendir(sourceDirPath);

    if (!srcDir) {
        printf("%s directory can't be opened!!!\n", sourceDirPath);

        return 2;
    }

    DIR* targetDir = opendir(targetDirPath);

    if (!targetDir) {
        printf("%s directory does not exist!\n", targetDirPath);
        printf("creating %s directory...\n", targetDirPath);

        if (mkdir(targetDirPath, 0777) != 0) {
            printf("could not create %s directory!!!", targetDirPath);

            return 3;
        }
    }

    closedir(targetDir);

    struct dirent* currFilePtr;
    char srcFullPath[BUFF_SIZE];
    char targetFullPath[BUFF_SIZE];

    while (NULL != (currFilePtr = readdir(srcDir))) {
        printf("current file: %s\n", currFilePtr->d_name);

        if ((strcmp(currFilePtr->d_name, ".") == 0) || (strcmp(currFilePtr->d_name, "..") == 0))
            continue;

        snprintf(srcFullPath, sizeof(srcFullPath), "%s/%s", sourceDirPath, currFilePtr->d_name);
        printf("full path: %s\n", srcFullPath);

        if (!isTextFile(srcFullPath))
            continue;

        printf("text file: %s\n", srcFullPath);

        snprintf(targetFullPath, sizeof(targetFullPath), "%s/flipped_%s", targetDirPath, currFilePtr->d_name);
        FILE* flippedFile = fopen(targetFullPath, "w");
        FILE* srcFile = fopen(srcFullPath, "r");

        if(!flipFile(srcFile, flippedFile)) {
            printf("couldn't flip %s file\n", srcFullPath);
        }

        fclose(flippedFile);
    }

    closedir(srcDir);

    return 0;
}
