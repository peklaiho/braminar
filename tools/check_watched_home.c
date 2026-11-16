#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <time.h>

static int watched_home_getchar(void);
static int watched_home_nanosleep(const struct timespec* request, struct timespec* remaining);
#define getchar watched_home_getchar
#define nanosleep watched_home_nanosleep
#define main braminar_program_main
#include "../braminar.c"
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
           "\"staff\":%d,\"talisman\":%d,\"charm\":%d,\"keys\":%d,\"delay\":%.3f}\n",
           kind, p->hit_points, p->gold, p->food, p->male_servants, p->female_servants,
           p->level, p->experience, p->medicine, p->poison, p->swords, p->bows, p->arrows,
           p->daggers, p->horse, p->cart, (unsigned)p->ring_power, (unsigned)p->plague,
           p->good_looks, p->hair_color, p->illness_protection, (unsigned)random_state,
           p->staff_of_aviatar, p->talisman_of_braminar, p->dragon_charm, key_count, delay_seconds);
}

static int watched_home_getchar(void)
{
    checkpoint("key");
    ++key_count;
    return *keys ? *keys++ : EOF;
}

static int watched_home_nanosleep(const struct timespec* request, struct timespec* remaining)
{
    (void)remaining;
    delay_seconds += request->tv_sec + request->tv_nsec / 1000000000.0;
    return 0;
}

int main(int argc, char** argv)
{
    if (argc != 25) {
        return 2;
    }
    Player p = {0};
    p.level = atoi(argv[17]);
    p.hit_points = strtod(argv[2], NULL);
    p.gold = atoi(argv[3]);
    p.food = atoi(argv[4]);
    p.male_servants = atoi(argv[5]);
    p.female_servants = atoi(argv[6]);
    p.medicine = atoi(argv[7]);
    p.poison = atoi(argv[8]);
    p.staff_of_aviatar = atoi(argv[9]);
    p.talisman_of_braminar = atoi(argv[10]);
    p.dragon_charm = atoi(argv[11]);
    p.swords = atoi(argv[12]);
    p.bows = atoi(argv[13]);
    p.arrows = atoi(argv[14]);
    p.daggers = atoi(argv[15]);
    p.horse = atoi(argv[16]);
    p.cart = p.horse;
    p.good_looks = atoi(argv[21]);
    p.ring_power = atoi(argv[19]);
    p.plague = atoi(argv[20]);
    p.hair_color = "Black";
    p.illness_protection = atoi(argv[18]);
    random_state = (uint32_t)strtoul(argv[1], NULL, 10);
    keys = argv[22];
    observed = &p;
    initialize_input();
    for (int turn = 0; turn < atoi(argv[24]); ++turn) {
        if (!run_event(&p)) {
            return 3;
        }
        if (atoi(argv[23])) {
            finish_turn(&p);
        }
        checkpoint(turn + 1 < atoi(argv[24]) ? "turn" : "final");
    }
    return 0;
}
