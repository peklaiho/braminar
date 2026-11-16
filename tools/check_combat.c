#define _POSIX_C_SOURCE 200809L

/* Exercise the game code with scripted keyboard reads and state checkpoints. */
static int combat_getchar(void);
#define getchar combat_getchar
#define main braminar_program_main
#include "../braminar.c"
#undef main
#undef getchar

static Player* observed_player;
static const char* answers;

static void checkpoint(void)
{
    Player* p = observed_player;
    printf("CHECK {\"level\":%d,\"male\":%d,\"female\":%d,\"gold\":%d,\"food\":%d,\"hp\":%.8f,\"experience\":%.8f,\"ring\":%u,\"poison\":%d,\"swords\":%d,\"bows\":%d,\"arrows\":%d,\"daggers\":%d,\"seed\":%u}\n",
           p->level, p->male_servants, p->female_servants, p->gold, p->food,
           p->hit_points, p->experience, (unsigned)p->ring_power, p->poison,
           p->swords, p->bows, p->arrows, p->daggers, (unsigned)random_state);
}

static int combat_getchar(void)
{
    checkpoint();
    return *answers ? *answers++ : ' ';
}
int main(int argc, char** argv)
{
    if (argc != 18) {
        return 2;
    }
    Player p = {0};
    random_state = (uint32_t)strtoul(argv[1], NULL, 10);
    p.hit_points = strtod(argv[2], NULL);
    p.level = atoi(argv[3]);
    p.male_servants = atoi(argv[4]);
    p.female_servants = atoi(argv[5]);
    p.gold = atoi(argv[6]);
    p.food = atoi(argv[7]);
    p.swords = atoi(argv[8]);
    p.bows = atoi(argv[9]);
    p.arrows = atoi(argv[10]);
    p.daggers = atoi(argv[11]);
    p.poison = atoi(argv[12]);
    p.ring_power = atoi(argv[13]);
    p.horse = atoi(argv[14]);
    p.cart = atoi(argv[15]);
    char input[3] = {0};
    int count = 0;
    if (p.ring_power <= 45) {
        input[count++] = argv[16][0];
    }
    if (p.poison > 0) {
        input[count++] = atoi(argv[17]) ? 'y' : 'n';
    }
    answers = input;
    observed_player = &p;
    initialize_input();
    if (!run_event(&p)) {
        return 3;
    }
    finish_turn(&p);
    checkpoint();
    return 0;
}
