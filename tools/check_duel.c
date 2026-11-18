#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <time.h>

static int duel_nanosleep(const struct timespec* request, struct timespec* remaining);
#define nanosleep duel_nanosleep
#define main braminar_program_main
#include "../braminar.c"
#include "../combat.c"
#include "../events.c"
#include "../town.c"
#include "../final.c"
#undef main
#undef nanosleep

static FinalDuel observed;
static bool local_values;
static double elapsed;

static void checkpoint(const char* kind, double delay, const FinalDuel* duel)
{
    printf("\nCHECK {\"kind\":\"%s\",\"seed\":%u,\"delay\":%.3f,\"elapsed\":%.3f",
           kind, (unsigned)random_state, delay, elapsed);
    if (duel != NULL) {
        printf(",\"player_hp\":%.8f,\"overlord_hp\":%.8f", duel->player_hp, duel->overlord_hp);
    }
    puts("}");
}

static int duel_nanosleep(const struct timespec* request, struct timespec* remaining)
{
    (void)remaining;
    double delay = request->tv_sec + request->tv_nsec / 1000000000.0;
    elapsed += delay;
    checkpoint("delay", delay, local_values ? &observed : NULL);
    return 0;
}

int main(int argc, char** argv)
{
    if (argc != 14) {
        return 2;
    }
    uint32_t seed = (uint32_t)strtoul(argv[1], NULL, 10);
    random_state = seed;
    Player player = {0};
    strcpy(player.name, "AssaultProbe");
    player.level = atoi(argv[2]);
    player.hit_points = strtod(argv[3], NULL);
    player.talisman_of_braminar = atoi(argv[4]);
    player.ring_power = (uint8_t)atoi(argv[5]);
    player.poison = atoi(argv[6]);
    player.swords = atoi(argv[7]);
    player.bows = atoi(argv[8]);
    player.arrows = atoi(argv[9]);
    player.daggers = atoi(argv[10]);
    Player before;
    memcpy(&before, &player, sizeof(player));
    if (argv[11][0] == 'r') {
        local_values = true;
        observed.player_hp = strtod(argv[12], NULL);
        observed.overlord_hp = strtod(argv[13], NULL);
        final_duel_round(&observed);
        checkpoint("finished", 0, &observed);
    } else {
        FinalDuel initial = prepare_final_duel(&player);
        checkpoint("initial", 0, &initial);
        random_state = seed;
        bool victory = fight_overlord(&player);
        checkpoint("finished", 0, NULL);
        printf("VICTORY %d\n", victory);
    }
    printf("UNCHANGED %d\n", memcmp(&before, &player, sizeof(player)) == 0);
    return 0;
}
