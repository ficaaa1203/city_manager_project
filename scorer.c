#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

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
    long   timestamp;
    char   description[DESC_LEN];
} Report;

typedef struct {
    char name[NAME_LEN];
    int  score;
} InspectorScore;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: scorer <district>\n");
        return 1;
    }

    const char *district = argv[1];
    char path[256];
    snprintf(path, sizeof(path), "%s/reports.dat", district);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        printf("District '%s': no reports found.\n", district);
        return 0;
    }

    InspectorScore scores[64];
    int count = 0;

    Report r;
    while (read(fd, &r, sizeof(r)) == sizeof(r)) {
        // Find or create inspector entry
        int found = -1;
        for (int i = 0; i < count; i++) {
            if (strcmp(scores[i].name, r.inspector) == 0) {
                found = i;
                break;
            }
        }
        if (found == -1) {
            strncpy(scores[count].name, r.inspector, NAME_LEN - 1);
            scores[count].score = 0;
            found = count++;
        }
        scores[found].score += r.severity;
    }
    close(fd);

    printf("District: %s\n", district);
    for (int i = 0; i < count; i++)
        printf("  Inspector: %-30s Score: %d\n", scores[i].name, scores[i].score);

    return 0;
}