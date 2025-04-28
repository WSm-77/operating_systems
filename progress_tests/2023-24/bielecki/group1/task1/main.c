#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <error.h>

int main() {
    pid_t pid;
    time_t t;
    int status;

    if ((pid = fork()) < 0)
        perror("fork");
    else if (pid == 0) {
        sleep(5);
        exit(5);
    }
    else do {
//  sprawdź status procesu potomka, nie zawieszając przy tym procesu macierzystego
//  1) W przypadku błędu zwróć go.
//  2) W przypadku, gdy proces się  jeszcze nie zakończył:
//     - pobierz aktualną datę.
//     - wypisz "Proces potomny nadal pracuje %data%"
//     - uśpij proces macierzysty na jedną sekundę
        pid = waitpid(pid, &status, WNOHANG);

        if (pid == -1) {
            perror("waitpid\n");
            return WEXITSTATUS(status);
        }
        else if (pid == 0) {
            char dateStr[80];
            struct tm *tmp;
            t = time(NULL);
            tmp = localtime(&t);
            strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", tmp);
            printf("Proces potomny nadal pracuje %s\n", dateStr);
            sleep(1);
        }

//  koniec
//  ciąg dalszy do while , przypadek, gdy proces potomny się zakończył
        if (WIFEXITED(status))
            printf("Proces potomny zakonczył się statusem %d\n", WEXITSTATUS(status));
        else puts("Proces potomny nie zakonczył się prawidłowo");
    } while (pid == 0);
puts("KONIEC");
  return 0;
}
