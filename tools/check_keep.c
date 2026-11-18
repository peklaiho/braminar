#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <time.h>

static int keep_getchar(void);
static char* keep_fgets(char* buffer, int size, FILE* stream);
static int keep_nanosleep(const struct timespec* request, struct timespec* remaining);
#define getchar keep_getchar
#define fgets keep_fgets
#define nanosleep keep_nanosleep
#define main braminar_program_main
#include "../braminar.c"
#include "../combat.c"
#include "../events.c"
#include "../town.c"
#include "../final.c"
#undef main
#undef nanosleep
#undef getchar
#undef fgets

static Player* observed;
static const char* keys;
static int key_count;
static double delay_seconds;
static bool discarding_line;

static void checkpoint(const char* kind)
{
    Player* p = observed;
    printf("\nCHECK {\"kind\":\"%s\",\"hp\":%.8f,\"gold\":%d,\"food\":%d,\"male\":%d,\"female\":%d,"
           "\"level\":%d,\"experience\":%.8f,\"medicine\":%d,\"poison\":%d,\"swords\":%d,"
           "\"bows\":%d,\"arrows\":%d,\"daggers\":%d,\"horse\":%d,\"cart\":%d,\"ring\":%u,"
           "\"plague\":%u,\"looks\":%d,\"hair\":\"%s\",\"protection\":%d,\"seed\":%u,"
           "\"staff\":%d,\"talisman\":%d,\"charm\":%d,\"keys\":%d,\"delay\":%.3f,"
           "\"ready\":%d,\"command\":\"%s\"}\n",
           kind, p->hit_points, p->gold, p->food, p->male_servants, p->female_servants,
           p->level, p->experience, p->medicine, p->poison, p->swords, p->bows, p->arrows,
           p->daggers, p->horse, p->cart, (unsigned)p->ring_power, (unsigned)p->plague,
           p->good_looks, p->hair_color, p->illness_protection, (unsigned)random_state,
           p->staff_of_aviatar, p->talisman_of_braminar, p->dragon_charm, key_count, delay_seconds, p->final_assault_ready, p->prime_command);
}

static void final_checkpoint(void)
{
    checkpoint("final");
}

static int keep_getchar(void)
{
    /* The ending's status pause is compared separately from Keep input. */
    if (observed->final_assault_ready && delay_seconds > 0) {
        return EOF;
    }
    if (!discarding_line) {
        checkpoint("key");
        ++key_count;
    }
    int ch = *keys ? (unsigned char)*keys++ : EOF;
    if (ch == '\n' || ch == EOF) {
        discarding_line = false;
    }
    return ch;
}

static char* keep_fgets(char* buffer, int size, FILE* stream)
{
    (void)stream;
    checkpoint("line");
    ++key_count;
    if (*keys == '\0') {
        return NULL;
    }
    size_t length = strcspn(keys, "\n");
    printf("%.*s\n", (int)length, keys);
    int count = 0;
    while (count < size - 1 && *keys) {
        int ch = (unsigned char)*keys++;
        buffer[count++] = (char)ch;
        if (ch == '\n') {
            break;
        }
    }
    buffer[count] = '\0';
    discarding_line = strchr(buffer, '\n') == NULL;
    return buffer;
}

static int keep_nanosleep(const struct timespec* request, struct timespec* remaining)
{
    (void)remaining;
    delay_seconds += request->tv_sec + request->tv_nsec / 1000000000.0;
    return 0;
}

int main(int argc, char** argv)
{
    if (argc != 26) {
        return 2;
    }
    static Player p;
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
    strcpy(p.prime_command, argv[22]);
    p.experience = strtod(argv[23], NULL);
    keys = argv[24];
    observed = &p;
    initialize_input();
    if (atexit(final_checkpoint) != 0) {
        return 2;
    }
    for (int turn = 0; turn < atoi(argv[25]); ++turn) {
        if (!run_turn(&p)) {
            return p.hit_points > 0 ? 3 : 0;
        }
        if (turn + 1 < atoi(argv[25])) {
            checkpoint("turn");
        }
    }
    return 0;
}
