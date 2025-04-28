#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define IN_FILE_PATH "odyssey.txt"
#define OUT_FILE_PATH "out.txt"

void check_buffer(char *buffer)
{
    if (strcmp(buffer, "I'm sorry, Dave. I'm afraid I can't do that.\n") == 0)
    {
        printf("PASS: file reading\n");
    }
    else
    {
        printf("FAIL: file reading\n");
    }
}

void check_file()
{
    char command[] = "grep -qxF \"What's the problem?\" out.txt";
    FILE *fp = popen(command, "r");

    if (fp == NULL)
    {
        perror("Error opening pipe");
        exit(1);
    }

    // Get return status of grep
    int status = pclose(fp);

    if (status == 0)
    {
        printf("PASS: file writing\n");
    }
    else
    {
        printf("FAIL: file reading\n");
    }
}
