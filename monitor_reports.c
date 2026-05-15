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
    write(STDOUT_FILENO, "MSG:New report added.\n", 22);
}

void handle_sigint(int sig) {
    (void)sig;
    running = 0;
}

int main() {
    // Check if another monitor is already running
    int existing_fd = open(PID_FILE, O_RDONLY);
    if (existing_fd >= 0) {
        char buf[32];
        memset(buf, 0, sizeof(buf));
        read(existing_fd, buf, sizeof(buf) - 1);
        close(existing_fd);
        pid_t existing_pid = (pid_t)atoi(buf);
        // Check if that process is actually alive
        if (existing_pid > 0 && kill(existing_pid, 0) == 0) {
            // Another monitor is running — report error through pipe and exit
            char msg[128];
            snprintf(msg, sizeof(msg), "ERR:Monitor already running with PID %d.\n", existing_pid);
            write(STDOUT_FILENO, msg, strlen(msg));
            return 1;
        }
    }

    // Write our PID to .monitor_pid
    int fd = open(PID_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        write(STDOUT_FILENO, "ERR:Could not create PID file.\n", 31);
        return 1;
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "%d\n", getpid());
    write(fd, buf, strlen(buf));
    close(fd);

    char start_msg[64];
    snprintf(start_msg, sizeof(start_msg), "MSG:Monitor started. PID = %d\n", getpid());
    write(STDOUT_FILENO, start_msg, strlen(start_msg));

    // Set up signal handlers
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

    while (running)
        pause();

    write(STDOUT_FILENO, "MSG:Monitor shutting down.\n", 27);
    unlink(PID_FILE);
    return 0;
}