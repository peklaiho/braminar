#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <time.h>

static int gypsy_getchar(void);
static int gypsy_nanosleep(const struct timespec* request, struct timespec* remaining);
#define getchar gypsy_getchar
#define nanosleep gypsy_nanosleep
#define main braminar_program_main
#include "braminar.c"
#undef main
#undef nanosleep
#undef getchar

static Player* observed;
static const char* keys;
static int key_count;
static double delay_seconds;

static void checkpoint(const char* kind)
{
    Player* p = observed;
    printf("\nCHECK {\"kind\":\"%s\",\"hp\":%.8f,\"gold\":%d,\"food\":%d,\"male\":%d,\"female\":%d,"
           "\"level\":%d,\"experience\":%.8f,\"medicine\":%d,\"poison\":%d,\"swords\":%d,"
           "\"bows\":%d,\"arrows\":%d,\"daggers\":%d,\"horse\":%d,\"cart\":%d,\"ring\":%u,"
           "\"plague\":%u,\"looks\":%d,\"hair\":\"%s\",\"protection\":%d,\"seed\":%u,"
           "\"keys\":%d,\"delay\":%.1f}\n",
           kind, p->hit_points, p->gold, p->food, p->male_servants, p->female_servants,
           p->level, p->experience, p->medicine, p->poison, p->swords, p->bows, p->arrows,
           p->daggers, p->horse, p->cart, (unsigned)p->ring_power, (unsigned)p->plague,
           p->good_looks, p->hair_color, p->illness_protection, (unsigned)random_state,
           key_count, delay_seconds);
}

static int gypsy_getchar(void)
{
    checkpoint("key");
    ++key_count;
    return *keys ? *keys++ : EOF;
}

static int gypsy_nanosleep(const struct timespec* request, struct timespec* remaining)
{
    (void)remaining;
    delay_seconds += request->tv_sec + request->tv_nsec / 1000000000.0;
    return 0;
}

int main(int argc, char** argv)
{
    if (argc != 6) {
        return 2;
    }
    Player p = {0};
    p.level = 8;
    p.hit_points = strtod(argv[2], NULL);
    p.gold = atoi(argv[3]);
    p.food = 100;
    p.male_servants = 5;
    p.female_servants = 5;
    p.medicine = 3;
    p.good_looks = true;
    p.hair_color = "Black";
    p.illness_protection = true;
    random_state = (uint32_t)strtoul(argv[1], NULL, 10);
    keys = argv[4];
    observed = &p;
    initialize_input();
    bool completed = run_event(&p);
    if (completed) {
        if (atoi(argv[5])) {
            finish_turn(&p);
        }
        checkpoint("final");
    }
    return completed ? 0 : 3;
}
