CC = gcc
CFLAGS = -Wall -Wextra -g

all: city_manager monitor_reports scorer city_hub

city_manager: city_manager.c
	$(CC) $(CFLAGS) -o city_manager city_manager.c

monitor_reports: monitor_reports.c
	$(CC) $(CFLAGS) -o monitor_reports monitor_reports.c

scorer: scorer.c
	$(CC) $(CFLAGS) -o scorer scorer.c

city_hub: city_hub.c
	$(CC) $(CFLAGS) -o city_hub city_hub.c

clean:
	rm -f city_manager monitor_reports scorer city_hub
