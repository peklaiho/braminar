#define _POSIX_C_SOURCE 200809L

static int tree_getchar(void);
#define getchar tree_getchar
#define main braminar_program_main
#include "braminar.c"
#undef main
#undef getchar

static const char* answers;
static int input_count;

static int tree_getchar(void)
{
    ++input_count;
    return *answers ? *answers++ : EOF;
}

int main(int argc, char** argv)
{
    if (argc != 4) {
        return 2;
    }
    Player player = {0};
    player.level = 8;
    player.hit_points = 50;
    player.male_servants = 5;
    player.female_servants = 5;
    player.gold = 8;
    player.food = 100;
    player.medicine = 3;
    player.arrows = 2;
    player.staff_of_aviatar = atoi(argv[3]) != 0;
    random_state = (uint32_t)strtoul(argv[1], NULL, 10);
    char input[] = {'y', argv[2][0], ' ', '\0'};
    answers = input;
    if (input[1] == 'n') {
        answers = "n ";
    }
    initialize_input();
    if (!run_event(&player) || !finish_turn(&player)) {
        return 3;
    }
    printf("CHECK {\"hp\":%.8f,\"food\":%d,\"medicine\":%d,\"arrows\":%d,\"staff\":%d,\"seed\":%u,\"inputs\":%d,"
           "\"level\":%d,\"male\":%d,\"female\":%d,\"gold\":%d,\"experience\":%.8f,"
           "\"ring\":%u,\"poison\":%d,\"swords\":%d,\"bows\":%d,\"daggers\":%d}\n",
           player.hit_points, player.food, player.medicine, player.arrows,
           player.staff_of_aviatar, (unsigned)random_state, input_count,
           player.level, player.male_servants, player.female_servants, player.gold,
           player.experience, (unsigned)player.ring_power, player.poison,
           player.swords, player.bows, player.daggers);
    return 0;
}
