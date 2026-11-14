#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <time.h>

static int event_getchar(void);
static char* event_fgets(char* buffer, int size, FILE* stream);
static int event_nanosleep(const struct timespec* request, struct timespec* remaining);
#define getchar event_getchar
#define fgets event_fgets
#define nanosleep event_nanosleep
#define main braminar_program_main
#include "braminar.c"
#undef main
#undef nanosleep
#undef fgets
#undef getchar

static const char* keys;
static const char* lines;
static int key_count;
static int line_count;
static double delay_seconds;

static int event_getchar(void)
{
    ++key_count;
    return *keys ? *keys++ : EOF;
}

static char* event_fgets(char* buffer, int size, FILE* stream)
{
    (void)stream;
    ++line_count;
    if (!*lines) {
        return NULL;
    }
    int length = 0;
    while (*lines && length < size - 1) {
        buffer[length++] = *lines++;
        if (buffer[length - 1] == '\n') {
            break;
        }
    }
    buffer[length] = '\0';
    fputs(buffer, stdout);
    return buffer;
}

static int event_nanosleep(const struct timespec* request, struct timespec* remaining)
{
    (void)remaining;
    delay_seconds += request->tv_sec + request->tv_nsec / 1000000000.0;
    return 0;
}

int main(int argc, char** argv)
{
    if (argc != 9) {
        return 2;
    }
    Player p = {0};
    p.level = 8;
    p.hit_points = 50;
    p.gold = 100;
    random_state = (uint32_t)strtoul(argv[1], NULL, 10);
    p.male_servants = atoi(argv[2]);
    p.female_servants = atoi(argv[3]);
    p.medicine = atoi(argv[4]);
    p.food = atoi(argv[5]);
    p.illness_protection = atoi(argv[6]) != 0;
    keys = argv[7];
    lines = argv[8];
    initialize_input();
    bool survived = run_event(&p);
    if (survived) {
        survived = finish_turn(&p);
    }
    printf("\nCHECK {\"level\":%d,\"hp\":%.8f,\"male\":%d,\"female\":%d,\"medicine\":%d,\"food\":%d,"
           "\"gold\":%d,\"experience\":%.8f,\"ring\":%u,\"plague\":%u,\"protection\":%d,"
           "\"poison\":%d,\"swords\":%d,\"bows\":%d,\"arrows\":%d,\"daggers\":%d,\"seed\":%u,"
           "\"aborted\":%d,\"keys\":%d,\"lines\":%d,\"delay\":%.1f}\n",
           p.level, p.hit_points, p.male_servants, p.female_servants, p.medicine, p.food,
           p.gold, p.experience, (unsigned)p.ring_power, (unsigned)p.plague, p.illness_protection,
           p.poison, p.swords, p.bows, p.arrows, p.daggers, (unsigned)random_state,
           !survived, key_count, line_count, delay_seconds);
    return 0;
}
