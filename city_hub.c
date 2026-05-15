#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_DISTRICTS 32

// hub_mon process: forks monitor_reports, reads its output via pipe, prints to user
void run_hub_mon() {
    int pipefd[2];
    if (pipe(pipefd) < 0) { perror("pipe"); exit(1); }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); exit(1); }

    if (pid == 0) {
        // Child (monitor_reports): redirect stdout to write end of pipe
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        execl("./monitor_reports", "monitor_reports", NULL);
        perror("execl monitor_reports");
        exit(1);
    }

    // Parent (hub_mon): read from pipe and display messages
    close(pipefd[1]);

    char buf[256];
    char line[256];
    int pos = 0;
    int n;

    while ((n = read(pipefd[0], buf, sizeof(buf))) > 0) {
        for (int i = 0; i < n; i++) {
            if (buf[i] == '\n') {
                line[pos] = '\0';
                pos = 0;

                // Parse message type
                if (strncmp(line, "MSG:", 4) == 0)
                    printf("[hub_mon] %s\n", line + 4);
                else if (strncmp(line, "ERR:", 4) == 0) {
                    printf("[hub_mon] ERROR: %s\n", line + 4);
                    // Monitor ended due to error
                    printf("[hub_mon] Monitor process ended.\n");
                    close(pipefd[0]);
                    waitpid(pid, NULL, 0);
                    exit(0);
                } else {
                    printf("[hub_mon] %s\n", line);
                }
                fflush(stdout);

                // If monitor is shutting down, stop reading
                if (strcmp(line + 4, "Monitor shutting down.") == 0) {
                    printf("[hub_mon] Monitor process ended.\n");
                    close(pipefd[0]);
                    waitpid(pid, NULL, 0);
                    exit(0);
                }
            } else {
                if (pos < 255) line[pos++] = buf[i];
            }
        }
    }

    close(pipefd[0]);
    waitpid(pid, NULL, 0);
    printf("[hub_mon] Monitor process ended.\n");
    exit(0);
}

void cmd_start_monitor() {
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return; }

    if (pid == 0) {
        // This is hub_mon — runs in background
        run_hub_mon();
        exit(0);
    }

    // Parent (city_hub): don't wait, hub_mon runs in background
    printf("[hub] Monitor started (hub_mon PID = %d).\n", pid);
}

void cmd_calculate_scores(char **districts, int count) {
    int pipes[MAX_DISTRICTS][2];
    pid_t pids[MAX_DISTRICTS];

    // Spawn one scorer per district
    for (int i = 0; i < count; i++) {
        if (pipe(pipes[i]) < 0) { perror("pipe"); continue; }

        pids[i] = fork();
        if (pids[i] < 0) { perror("fork"); continue; }

        if (pids[i] == 0) {
            // Child (scorer): redirect stdout to write end of pipe
            close(pipes[i][0]);
            dup2(pipes[i][1], STDOUT_FILENO);
            close(pipes[i][1]);
            execl("./scorer", "scorer", districts[i], NULL);
            perror("execl scorer");
            exit(1);
        }

        // Parent: close write end
        close(pipes[i][1]);
    }

    // Collect output from all scorers
    printf("\n=== Workload Report ===\n");
    for (int i = 0; i < count; i++) {
        char buf[1024];
        int n;
        while ((n = read(pipes[i][0], buf, sizeof(buf) - 1)) > 0) {
            buf[n] = '\0';
            printf("%s", buf);
        }
        close(pipes[i][0]);
        waitpid(pids[i], NULL, 0);
    }
    printf("======================\n\n");
}

int main() {
    char input[256];
    printf("city_hub started. Commands: start_monitor, calculate_scores <districts...>, exit\n");

    while (1) {
        printf("hub> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "exit") == 0) {
            printf("Exiting city_hub.\n");
            break;
        } else if (strcmp(input, "start_monitor") == 0) {
            cmd_start_monitor();
        } else if (strncmp(input, "calculate_scores", 16) == 0) {
            // Parse district names from the rest of the line
            char *districts[MAX_DISTRICTS];
            int count = 0;
            char *token = strtok(input + 17, " ");
            while (token && count < MAX_DISTRICTS) {
                districts[count++] = token;
                token = strtok(NULL, " ");
            }
            if (count == 0)
                printf("Usage: calculate_scores <district1> <district2> ...\n");
            else
                cmd_calculate_scores(districts, count);
        } else {
            printf("Unknown command: %s\n", input);
        }
    }
    return 0;
}