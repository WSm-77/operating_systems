#include "zad3.h"

void readwrite(int pd, size_t block_size);

void createpipe(size_t block_size)
{
    /* Utwórz potok nienazwany */
    int fd[2];
    pipe(fd);

    /* Odkomentuj poniższe funkcje zamieniając ... na deskryptory potoku */
    check_pipe(fd);
    check_write(fd, block_size, readwrite);
}

void readwrite(int write_pd, size_t block_size)
{
    /* Otworz plik `unix.txt`, czytaj go po `block_size` bajtów
    i w takich `block_size` bajtowych kawałkach pisz do potoku `write_pd`.*/

    char* buffor = malloc(block_size * sizeof(char));

    FILE* file = fopen("unix.txt", "r");
    size_t bytes_read = 0;

    while ((bytes_read = fread(buffor, sizeof(char), block_size, file)) > 0) {
        write(write_pd, buffor, bytes_read);
    }

    free(buffor);
    /* Zamknij plik */
    fclose(file);
}

int main()
{
    srand(42);
    size_t block_size = rand() % 128;
    createpipe(block_size);

    return 0;
}
