#define _POSIX_C_SOURCE 200809L

#include <stdio.h>

static int ending_getchar(void);
#define getchar ending_getchar
#define main braminar_program_main
#include "../braminar.c"
#include "../combat.c"
#include "../events.c"
#include "../town.c"
#include "../final.c"
#undef main
#undef getchar

static int key_count;
static int ending_key;

static int ending_getchar(void)
{
    ++key_count;
    return ending_key;
}

int main(int argc, char** argv)
{
    if (argc != 25) {
        return 2;
    }
    Player player = {0};
    strcpy(player.name, "AssaultProbe");
    player.hair_color = "Black";
    random_state = (uint32_t)strtoul(argv[1], NULL, 10);
    player.hit_points = strtod(argv[2], NULL);
    player.gold = atoi(argv[3]);
    player.food = atoi(argv[4]);
    player.male_servants = atoi(argv[5]);
    player.female_servants = atoi(argv[6]);
    player.medicine = atoi(argv[7]);
    player.poison = atoi(argv[8]);
    player.staff_of_aviatar = atoi(argv[9]);
    player.talisman_of_braminar = atoi(argv[10]);
    player.dragon_charm = atoi(argv[11]);
    player.swords = atoi(argv[12]);
    player.bows = atoi(argv[13]);
    player.arrows = atoi(argv[14]);
    player.daggers = atoi(argv[15]);
    player.horse = atoi(argv[16]);
    player.cart = atoi(argv[17]);
    player.level = atoi(argv[18]);
    player.illness_protection = atoi(argv[19]);
    player.ring_power = (uint8_t)atoi(argv[20]);
    player.good_looks = atoi(argv[21]);
    player.gender = argv[22][0];
    ending_key = atoi(argv[24]);
    Player before;
    memcpy(&before, &player, sizeof(player));
    initialize_input();
    if (argv[23][0] == 'r') {
        final_reward(&player);
    } else {
        finish_final_victory(&player);
    }
    printf("\nRESULT {\"seed\":%u,\"unchanged\":%d,\"keys\":%d}\n",
           (unsigned)random_state, memcmp(&before, &player, sizeof(player)) == 0, key_count);
    return 0;
}
