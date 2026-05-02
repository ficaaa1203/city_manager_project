CC = gcc
CFLAGS = -Wall -Wextra -g

all: city_manager monitor_reports

city_manager: city_manager.c
	$(CC) $(CFLAGS) -o city_manager city_manager.c

monitor_reports: monitor_reports.c
	$(CC) $(CFLAGS) -o monitor_reports monitor_reports.c

clean:
	rm -f city_manager monitor_reports
