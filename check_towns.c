#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <time.h>

static int town_getchar(void);
static char* town_fgets(char* buffer, int size, FILE* stream);
static int town_nanosleep(const struct timespec* request, struct timespec* remaining);
#define getchar town_getchar
#define fgets town_fgets
#define nanosleep town_nanosleep
#define main braminar_program_main
#include "braminar.c"
#undef main
#undef nanosleep
#undef fgets
#undef getchar

static Player* observed;
static const char* keys;
static const char* lines;
static int key_count;
static int line_count;
static double delay_seconds;

static void checkpoint(const char* kind)
{
    Player* p = observed;
    printf("\nCHECK {\"kind\":\"%s\",\"hp\":%.8f,\"gold\":%d,\"food\":%d,\"male\":%d,\"female\":%d,"
           "\"level\":%d,\"experience\":%.8f,\"medicine\":%d,\"poison\":%d,\"swords\":%d,"
           "\"bows\":%d,\"arrows\":%d,\"daggers\":%d,\"horse\":%d,\"cart\":%d,\"ring\":%u,"
           "\"plague\":%u,\"looks\":%d,\"hair\":\"%s\",\"protection\":%d,\"seed\":%u,"
           "\"keys\":%d,\"lines\":%d,\"delay\":%.1f}\n",
           kind, p->hit_points, p->gold, p->food, p->male_servants, p->female_servants,
           p->level, p->experience, p->medicine, p->poison, p->swords, p->bows, p->arrows,
           p->daggers, p->horse, p->cart, (unsigned)p->ring_power, (unsigned)p->plague,
           p->good_looks, p->hair_color, p->illness_protection, (unsigned)random_state,
           key_count, line_count, delay_seconds);
}

static int town_getchar(void)
{
    checkpoint("key");
    ++key_count;
    return *keys ? *keys++ : EOF;
}

static char* town_fgets(char* buffer, int size, FILE* stream)
{
    (void)stream;
    checkpoint("line");
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

static int town_nanosleep(const struct timespec* request, struct timespec* remaining)
{
    (void)remaining;
    delay_seconds += request->tv_sec + request->tv_nsec / 1000000000.0;
    return 0;
}

int main(int argc, char** argv)
{
    if (argc != 16) {
        return 2;
    }
    Player p = {0};
    p.level = 8;
    p.hit_points = strtod(argv[2], NULL);
    p.gold = atoi(argv[3]);
    p.food = atoi(argv[4]);
    p.male_servants = atoi(argv[5]);
    p.female_servants = atoi(argv[6]);
    p.medicine = atoi(argv[7]);
    p.ring_power = atoi(argv[8]);
    p.plague = atoi(argv[9]);
    p.good_looks = atoi(argv[10]);
    p.hair_color = "Black";
    p.horse = atoi(argv[11]);
    p.cart = atoi(argv[12]);
    p.illness_protection = true;
    male_servant_price = 8;
    female_servant_price = 13;
    random_state = (uint32_t)strtoul(argv[1], NULL, 10);
    keys = argv[13];
    lines = argv[14];
    observed = &p;
    initialize_input();
    bool completed = run_event(&p);
    if (completed) {
        if (atoi(argv[15])) {
            finish_turn(&p);
        }
        checkpoint("final");
    }
    return completed ? 0 : 3;
}
