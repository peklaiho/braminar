#define _POSIX_C_SOURCE 200809L

static int forest_getchar(void);
#define getchar forest_getchar
#define main braminar_program_main
#include "braminar.c"
#undef main
#undef getchar

static const char* keys;
static int input_count;

static int forest_getchar(void)
{
    ++input_count;
    return *keys ? *keys++ : EOF;
}

static void checkpoint(const char* kind, const Player* p)
{
    printf("\nCHECK {\"kind\":\"%s\",\"hp\":%.8f,\"gold\":%d,\"food\":%d,"
           "\"level\":%d,\"male\":%d,\"female\":%d,\"experience\":%.8f,"
           "\"medicine\":%d,\"poison\":%d,\"swords\":%d,\"bows\":%d,"
           "\"arrows\":%d,\"daggers\":%d,\"ring\":%u,\"plague\":%u,\"looks\":%d,"
           "\"staff\":%d,\"talisman\":%d,\"horse\":%d,\"cart\":%d,\"inputs\":%d}\n",
           kind, p->hit_points, p->gold, p->food, p->level, p->male_servants,
           p->female_servants, p->experience, p->medicine, p->poison, p->swords,
           p->bows, p->arrows, p->daggers, (unsigned)p->ring_power, (unsigned)p->plague,
           p->good_looks, p->staff_of_aviatar, p->talisman_of_braminar,
           p->horse, p->cart, input_count);
}

int main(int argc, char** argv)
{
    if (argc != 8) {
        return 2;
    }
    Player p = {0};
    p.level = 8;
    p.hit_points = strtod(argv[2], NULL);
    p.ring_power = atoi(argv[3]);
    p.plague = atoi(argv[4]);
    p.good_looks = atoi(argv[5]);
    p.gender = argv[6][0];
    p.hair_color = "Black";
    p.male_servants = 5;
    p.female_servants = 5;
    p.gold = 8;
    p.food = 100;
    p.medicine = 3;
    p.arrows = 2;
    random_state = (uint32_t)strtoul(argv[1], NULL, 10);
    keys = argv[7];
    initialize_input();
    if (!run_event(&p)) {
        return 3;
    }
    checkpoint("event", &p);
    bool survived = finish_turn(&p);
    checkpoint("final", &p);
    printf("SURVIVED %d\n", survived);
    return 0;
}
