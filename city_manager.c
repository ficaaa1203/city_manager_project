#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>

#define NAME_LEN     64
#define CATEGORY_LEN 32
#define DESC_LEN     128

typedef struct {
    int    id;
    char   inspector[NAME_LEN];
    double latitude;
    double longitude;
    char   category[CATEGORY_LEN];
    int    severity;
    time_t timestamp;
    char   description[DESC_LEN];
} Report;

/* ── helpers ── */

void permissions_to_string(mode_t mode, char *buf) {
    buf[0] = (mode & S_IRUSR) ? 'r' : '-';
    buf[1] = (mode & S_IWUSR) ? 'w' : '-';
    buf[2] = (mode & S_IXUSR) ? 'x' : '-';
    buf[3] = (mode & S_IRGRP) ? 'r' : '-';
    buf[4] = (mode & S_IWGRP) ? 'w' : '-';
    buf[5] = (mode & S_IXGRP) ? 'x' : '-';
    buf[6] = (mode & S_IROTH) ? 'r' : '-';
    buf[7] = (mode & S_IWOTH) ? 'w' : '-';
    buf[8] = (mode & S_IXOTH) ? 'x' : '-';
    buf[9] = '\0';
}

void log_action(const char *district, const char *user, const char *role, const char *action) {
    char path[256];
    snprintf(path, sizeof(path), "%s/logged_district", district);
    int fd = open(path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd < 0) return;
    char buf[512];
    time_t now = time(NULL);
    snprintf(buf, sizeof(buf), "%ld\t%s\t%s\t%s\n", (long)now, user, role, action);
    write(fd, buf, strlen(buf));
    close(fd);
    chmod(path, 0644);
}

/* ── commands ── */

void cmd_add(const char *district, const char *user, const char *role) {
    struct stat st;
    if (stat(district, &st) != 0) {
        if (mkdir(district, 0750) != 0) { perror("mkdir"); exit(1); }
        chmod(district, 0750);

        char cfg_path[256];
        snprintf(cfg_path, sizeof(cfg_path), "%s/district.cfg", district);
        int fd = open(cfg_path, O_WRONLY | O_CREAT | O_TRUNC, 0640);
        if (fd < 0) { perror("open district.cfg"); exit(1); }
        write(fd, "threshold=2\n", 12);
        close(fd);
        chmod(cfg_path, 0640);
    }

    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);

    int record_count = 0;
    if (stat(path, &st) == 0)
        record_count = (int)(st.st_size / sizeof(Report));

    Report r;
    memset(&r, 0, sizeof(r));
    r.id = record_count + 1;
    strncpy(r.inspector, user, NAME_LEN - 1);
    r.timestamp = time(NULL);

    printf("X: "); scanf("%lf", &r.latitude);
    printf("Y: "); scanf("%lf", &r.longitude);
    printf("Category (road/lighting/flooding/other): ");
    scanf("%31s", r.category);
    printf("Severity level (1/2/3): "); scanf("%d", &r.severity);
    printf("Description: ");
    getchar();
    fgets(r.description, DESC_LEN, stdin);
    r.description[strcspn(r.description, "\n")] = '\0';

    int fd = open(path, O_WRONLY | O_APPEND | O_CREAT, 0664);
    if (fd < 0) { perror("open reports.dat"); exit(1); }
    write(fd, &r, sizeof(r));
    close(fd);
    chmod(path, 0664);

    char link_name[256], link_target[256];
    snprintf(link_name, sizeof(link_name), "active_reports-%s", district);
    snprintf(link_target, sizeof(link_target), "%s/reports.dat", district);
    unlink(link_name);
    symlink(link_target, link_name);

    printf("Report #%d added to %s.\n", r.id, district);
    log_action(district, user, role, "add");
}

void cmd_list(const char *district) {
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);

    struct stat st;
    if (stat(path, &st) != 0) {
        fprintf(stderr, "No reports found for district '%s'.\n", district);
        return;
    }

    char perm_str[10];
    permissions_to_string(st.st_mode, perm_str);
    char time_buf[64];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", localtime(&st.st_mtime));
    printf("File: %s | Permissions: %s | Size: %lld bytes | Last modified: %s\n\n",
           path, perm_str, (long long)st.st_size, time_buf);

    int fd = open(path, O_RDONLY);
    if (fd < 0) { perror("open reports.dat"); return; }

    Report r;
    int count = 0;
    while (read(fd, &r, sizeof(r)) == sizeof(r)) {
        count++;
        char ts[64];
        strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&r.timestamp));
        printf("--- Report #%d ---\n", r.id);
        printf("  Inspector  : %s\n", r.inspector);
        printf("  GPS        : %.4f, %.4f\n", r.latitude, r.longitude);
        printf("  Category   : %s\n", r.category);
        printf("  Severity   : %d\n", r.severity);
        printf("  Timestamp  : %s\n", ts);
        printf("  Description: %s\n\n", r.description);
    }
    close(fd);

    if (count == 0)
        printf("No reports in this district.\n");
}

void cmd_view(const char *district, int report_id) {
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);

    int fd = open(path, O_RDONLY);
    if (fd < 0) { perror("open reports.dat"); return; }

    Report r;
    int found = 0;
    while (read(fd, &r, sizeof(r)) == sizeof(r)) {
        if (r.id == report_id) {
            found = 1;
            char ts[64];
            strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&r.timestamp));
            printf("--- Report #%d ---\n", r.id);
            printf("  Inspector  : %s\n", r.inspector);
            printf("  GPS        : %.4f, %.4f\n", r.latitude, r.longitude);
            printf("  Category   : %s\n", r.category);
            printf("  Severity   : %d\n", r.severity);
            printf("  Timestamp  : %s\n", ts);
            printf("  Description: %s\n", r.description);
            break;
        }
    }
    close(fd);
    if (!found)
        fprintf(stderr, "Report #%d not found in district '%s'.\n", report_id, district);
}

void cmd_remove_report(const char *district, int report_id, const char *role) {
    if (strcmp(role, "manager") != 0) {
        fprintf(stderr, "Permission denied: only managers can remove reports.\n");
        return;
    }

    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);

    struct stat st;
    if (stat(path, &st) != 0) { perror("stat reports.dat"); return; }

    int fd = open(path, O_RDWR);
    if (fd < 0) { perror("open reports.dat"); return; }

    int total = (int)(st.st_size / sizeof(Report));
    Report *records = malloc(total * sizeof(Report));
    if (!records) { fprintf(stderr, "malloc failed\n"); close(fd); return; }

    for (int i = 0; i < total; i++)
        read(fd, &records[i], sizeof(Report));

    int found = -1;
    for (int i = 0; i < total; i++) {
        if (records[i].id == report_id) { found = i; break; }
    }

    if (found == -1) {
        fprintf(stderr, "Report #%d not found.\n", report_id);
        free(records); close(fd); return;
    }

    for (int i = found; i < total - 1; i++) {
        lseek(fd, (off_t)(i * sizeof(Report)), SEEK_SET);
        write(fd, &records[i + 1], sizeof(Report));
    }

    ftruncate(fd, (off_t)((total - 1) * sizeof(Report)));
    free(records);
    close(fd);
    printf("Report #%d removed from district '%s'.\n", report_id, district);
    log_action(district, "system", role, "remove_report");
}

void cmd_update_threshold(const char *district, int value, const char *role) {
    if (strcmp(role, "manager") != 0) {
        fprintf(stderr, "Permission denied: only managers can update the threshold.\n");
        return;
    }

    char path[256];
    snprintf(path, sizeof(path), "%s/district.cfg", district);

    struct stat st;
    if (stat(path, &st) != 0) { perror("stat district.cfg"); return; }
    if ((st.st_mode & 0777) != 0640) {
        fprintf(stderr, "Permission mismatch on district.cfg (expected 640). Refusing to write.\n");
        return;
    }

    int fd = open(path, O_WRONLY | O_TRUNC);
    if (fd < 0) { perror("open district.cfg"); return; }

    char buf[64];
    snprintf(buf, sizeof(buf), "threshold=%d\n", value);
    write(fd, buf, strlen(buf));
    close(fd);
    printf("Threshold for district '%s' updated to %d.\n", district, value);
    log_action(district, "system", role, "update_threshold");
}

/* ── main ── */

int main(int argc, char *argv[]) {
    char *role = NULL, *user = NULL, *command = NULL;
    char *arg1 = NULL, *arg2 = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--role") == 0 && i+1 < argc)                  role = argv[++i];
        else if (strcmp(argv[i], "--user") == 0 && i+1 < argc)             user = argv[++i];
        else if (strcmp(argv[i], "--add") == 0 && i+1 < argc)              { command = "add";              arg1 = argv[++i]; }
        else if (strcmp(argv[i], "--list") == 0 && i+1 < argc)             { command = "list";             arg1 = argv[++i]; }
        else if (strcmp(argv[i], "--view") == 0 && i+2 < argc)             { command = "view";             arg1 = argv[++i]; arg2 = argv[++i]; }
        else if (strcmp(argv[i], "--remove_report") == 0 && i+2 < argc)    { command = "remove_report";    arg1 = argv[++i]; arg2 = argv[++i]; }
        else if (strcmp(argv[i], "--update_threshold") == 0 && i+2 < argc) { command = "update_threshold"; arg1 = argv[++i]; arg2 = argv[++i]; }
        else if (strcmp(argv[i], "--filter") == 0 && i+1 < argc)           { command = "filter";           arg1 = argv[++i]; }
    }

    if (!role || !user || !command) {
        fprintf(stderr, "Usage: ./city_manager --role <role> --user <user> --<command> <args>\n");
        return 1;
    }

    if (strcmp(command, "add") == 0)
        cmd_add(arg1, user, role);
    else if (strcmp(command, "list") == 0)
        cmd_list(arg1);
    else if (strcmp(command, "view") == 0)
        cmd_view(arg1, atoi(arg2));
    else if (strcmp(command, "remove_report") == 0)
        cmd_remove_report(arg1, atoi(arg2), role);
    else if (strcmp(command, "update_threshold") == 0)
        cmd_update_threshold(arg1, atoi(arg2), role);
    else
        printf("Command '%s' not yet implemented.\n", command);

    return 0;
}