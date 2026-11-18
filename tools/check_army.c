#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <time.h>

static int army_nanosleep(const struct timespec* request, struct timespec* remaining);
#define nanosleep army_nanosleep
#define main braminar_program_main
#include "../braminar.c"
#include "../combat.c"
#include "../events.c"
#include "../town.c"
#include "../final.c"
#undef main
#undef nanosleep

static FinalArmy observed;
static double elapsed;

static void checkpoint(const char* kind, double delay)
{
    printf("\nCHECK {\"kind\":\"%s\",\"army\":%d,\"enemy\":%d,\"seed\":%u,"
           "\"delay\":%.3f,\"elapsed\":%.3f}\n", kind, observed.soldiers,
           observed.enemy_soldiers, (unsigned)random_state, delay, elapsed);
}

static int army_nanosleep(const struct timespec* request, struct timespec* remaining)
{
    (void)remaining;
    double delay = request->tv_sec + request->tv_nsec / 1000000000.0;
    elapsed += delay;
    checkpoint("delay", delay);
    return 0;
}

static void finished(void)
{
    checkpoint("finished", 0);
}

int main(int argc, char** argv)
{
    if (argc != 5) {
        return 2;
    }
    random_state = (uint32_t)strtoul(argv[1], NULL, 10);
    observed.soldiers = atoi(argv[2]);
    observed.enemy_soldiers = atoi(argv[3]);
    atexit(finished);
    switch (argv[4][0]) {
    case 'c':
        final_army_casualties(&observed);
        break;
    case 'e':
        if (final_roll(100) >= 5) {
            return 2;
        }
        final_army_event(&observed);
        break;
    default: {
        Player player = {0};
        strcpy(player.name, "AssaultProbe");
        Player before;
        memcpy(&before, &player, sizeof(player));
        bool victory = fight_final_armies(&player, &observed);
        printf("VICTORY %d UNCHANGED %d\n", victory, memcmp(&before, &player, sizeof(player)) == 0);
        break;
    }
    }
    return 0;
}
