#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>

#define PID_FILE ".monitor_pid"

static volatile int running = 1;

void handle_sigusr1(int sig) {
    (void)sig;
    write(STDOUT_FILENO, "[monitor] New report added.\n", 28);
}

void handle_sigint(int sig) {
    (void)sig;
    running = 0;
}

int main() {

    int fd = open(PID_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) { perror("open .monitor_pid"); return 1; }
    char buf[32];
    snprintf(buf, sizeof(buf), "%d\n", getpid());
    write(fd, buf, strlen(buf));
    close(fd);

    printf("[monitor] Started. PID = %d\n", getpid());
    fflush(stdout);

    
    struct sigaction sa_usr1, sa_int;

    memset(&sa_usr1, 0, sizeof(sa_usr1));
    sa_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa_usr1, NULL);

    memset(&sa_int, 0, sizeof(sa_int));
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    sigaction(SIGINT, &sa_int, NULL);

    
    while (running) {
        pause(); // sleep until a signal arrives
    }

    // Cleanup
    printf("\n[monitor] SIGINT received. Shutting down.\n");
    unlink(PID_FILE);
    return 0;
}