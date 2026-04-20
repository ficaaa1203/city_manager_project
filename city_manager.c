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

int main() {
    return 0;
}
