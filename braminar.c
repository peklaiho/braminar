#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

typedef struct {
    char name[80];
    char gender;
    int level;
    double hit_points;
    const char* hair_color;
    bool staff_of_aviatar;
    bool talisman_of_braminar;
    bool good_looks;
    int male_servants;
    int female_servants;
    int gold;
    int food;
    int medicine;
    int poison;
    int swords;
    int bows;
    int arrows;
    int daggers;
    bool horse;
    bool cart;
    double experience;
    uint8_t ring_power;
    uint8_t plague;
    bool illness_protection;
} Player;

static uint32_t random_state;
static int male_servant_price;
static int female_servant_price;

static int random_number(int limit)
{
    /* Original runtime at file offsets 0x0FDA-0x101A: 32-bit state, 15-bit output. */
    random_state = random_state * UINT32_C(129) + UINT32_C(0x361962E9);
    return (int)((random_state >> 17) % (uint32_t)limit);
}

/* Keep the caller's terminal settings for line input and cleanup. */
static struct termios original_terminal;

static void restore_terminal(void)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &original_terminal);
}

static void cleanup_terminal(void)
{
    /* Normal exit can use buffered output to reset the text color. */
    restore_terminal();
    printf("\033[0m");
    fflush(stdout);
}

static void handle_signal(int signal_number)
{
    /* Signal handlers use write() because stdio functions are not signal-safe. */
    restore_terminal();
    write(STDOUT_FILENO, "\033[0m", 4);
    /* Re-send the signal so termination keeps its normal cause and exit status. */
    kill(getpid(), signal_number);
}

static void initialize_input(void)
{
    /* Save settings before changing input mode; atexit also covers exit(). */
    if (tcgetattr(STDIN_FILENO, &original_terminal) == -1) {
        perror("Cannot read terminal settings");
        exit(EXIT_FAILURE);
    }
    if (atexit(cleanup_terminal) != 0) {
        fputs("Cannot register terminal cleanup\n", stderr);
        exit(EXIT_FAILURE);
    }

    struct sigaction action = {0};
    action.sa_handler = handle_signal;
    /* Restore the default handler before handle_signal re-sends the signal. */
    action.sa_flags = SA_RESETHAND;
    sigemptyset(&action.sa_mask);
    const int signals[] = {SIGINT, SIGTERM, SIGHUP, SIGQUIT};
    for (size_t i = 0; i < sizeof(signals) / sizeof(signals[0]); ++i) {
        if (sigaction(signals[i], &action, NULL) == -1) {
            perror("Cannot install terminal cleanup handler");
            exit(EXIT_FAILURE);
        }
    }

    /* Prevent stdio from reading ahead when switching between input modes. */
    setvbuf(stdin, NULL, _IONBF, 0);
}

static int read_key(void)
{
    struct termios terminal = original_terminal;
    /* Deliver characters without Enter and hide typed characters, including newlines.
       Leave signal processing enabled so Ctrl+C still interrupts the game. */
    terminal.c_lflag &= ~(ICANON | ECHO | ECHONL);
    /* Wait for one byte with no timeout. */
    terminal.c_cc[VMIN] = 1;
    terminal.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &terminal) == -1) {
        perror("Cannot enable character input");
        exit(EXIT_FAILURE);
    }

    /* Show the prompt before blocking for input. */
    fflush(stdout);
    int ch = getchar();
    /* Restore line editing and echo immediately after each character read. */
    restore_terminal();
    return ch;
}

static bool read_line(char* buffer, size_t size)
{
    /* The original terminal mode provides name editing and Enter to submit. */
    fflush(stdout);
    if (fgets(buffer, size, stdin) == NULL) {
        return false;
    }

    size_t length = strcspn(buffer, "\r\n");
    if (buffer[length] == '\0') {
        /* Discard an overlong line so its remainder cannot answer later prompts. */
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {
        }
    }
    buffer[length] = '\0';
    return true;
}

static bool wait_for_key(void)
{
    printf("Press any key...");
    return read_key() != EOF;
}

static void clear_screen(void)
{
    /* ANSI sequences clear the screen and move the cursor to the top left. */
    printf("\033[2J\033[H");
}

static bool display_title(void)
{
    clear_screen();
    printf("\n"
           "                                Welcome to:\n"
           "\n"
           "                           B  R  A  M  I  N  A  R\n"
           "\n"
           "\n"
           "   You will start out a 7-10 level player, and you must gain\n"
           "in experience by battling monsters, to build up to 20th level.\n"
           "Then, if you have the Staff of Aviatar, & it's prime command,\n"
           "you can challenge the Overlord, and battle his armies.  If\n"
           "you succeed, you will be the new king of BRAMINAR!\n"
           "\n"
           "Press any key...");

    return read_key() != EOF;
}

static void generate_player_stats(Player* player)
{
    static const char* const hair_colors[] = {"Blonde", "Brown", "Black", "Red"};

    player->level = random_number(3) + 7;
    player->hit_points = 0;
    /* The original rolls one ten-sided die per level, in this order. */
    for (int i = 0; i < player->level; ++i) {
        player->hit_points += random_number(10) + 1;
    }
    player->hair_color = hair_colors[random_number(4)];
    player->good_looks = true;
    player->staff_of_aviatar = false;
    player->talisman_of_braminar = false;
    player->male_servants = random_number(15);
    player->female_servants = random_number(15);
    player->gold = random_number(25);
    player->food = 100;
    player->medicine = 3;
    player->poison = 0;
    player->swords = 0;
    player->bows = 0;
    player->arrows = 0;
    player->daggers = 0;
    player->horse = false;
    player->cart = false;
    player->experience = 0;
    player->ring_power = 0;
    player->plague = 0;
    player->illness_protection = false;

    male_servant_price = random_number(10) + 5;
    female_servant_price = random_number(10) + 5;
}

static bool create_player(Player* player)
{
    do {
        printf("\nEnter Name: ");
        if (!read_line(player->name, sizeof(player->name))) {
            return false;
        }
    } while (player->name[0] == '\0');

    do {
        printf("Choose Gender (M/F) ");
        int ch = read_key();
        if (ch == EOF) {
            return false;
        }
        player->gender = (char)toupper((unsigned char)ch);
        putchar('\n');
    } while (player->gender != 'M' && player->gender != 'F');

    generate_player_stats(player);
    printf("\nMale servants cost %d gold pieces\n", male_servant_price);
    printf("Female servants cost %d gold pieces\n", female_servant_price);
    return wait_for_key();
}

static bool display_stats(const Player* player)
{
    printf("*** %s ***\n\n", player->name);
    printf("     %-27s%s\n", "Gender:", player->gender == 'M' ? "Male" : "Female");
    printf("     %-27s%d\n", "Level:", player->level);
    printf("     %-27s%.2f\n", "Hit Points:", player->hit_points);
    printf("     %-27s%s\n", "Hair Color:", player->hair_color);
    printf("     %-27s%s\n", "Staff of Aviatar:", player->staff_of_aviatar ? "TRUE" : "FALSE");
    printf("     %-27s%s\n", "Talisman of Braminar:", player->talisman_of_braminar ? "TRUE" : "FALSE");
    printf("     %-27s%s\n", "Good Looks:", player->good_looks ? "TRUE" : "FALSE");
    printf("     %-27s%d\n", "Male Servants:", player->male_servants);
    printf("     %-27s%d\n", "Female Servants:", player->female_servants);
    printf("     %-27s%7d\n", "Gold Pieces:", player->gold);
    printf("     %-27s%7d\n", "Food:", player->food);
    printf("     %-27s%d\n", "Medicine:", player->medicine);
    printf("     %-27s%d\n", "Poison:", player->poison);
    printf("     %-27s%d\n", "Swords:", player->swords);
    printf("     %-27s%d\n", "Bows:", player->bows);
    printf("     %-27s%d\n", "Arrows:", player->arrows);
    printf("     %-27s%d\n", "Daggers:", player->daggers);
    printf("     %-27s%s\n", "Horse:", player->horse ? "TRUE" : "FALSE");
    printf("     %-27s%s\n", "Cart:", player->cart ? "TRUE" : "FALSE");
    printf("\nAny key.\n");

    return read_key() != EOF;
}

static void start_final_battle(void)
{
    puts("\nThe final battle is not implemented yet.");
    exit(EXIT_SUCCESS);
}

static void event_city(void)
{
    puts("\nYou arrive at a city.");
}

static void event_village(void)
{
    puts("\nYou arrive at a village.");
}

static void event_combat(void)
{
    puts("\nYou encounter a monster.");
}

static void event_dark_castle(void)
{
    puts("\nYou find the Dark Castle of the Mad King.");
}

static bool event_enchanted_forest(Player* player)
{
    /* Recovered at 0x8BA7-0x8F9A; see doc/mechanics.md#enchanted-forest. */
    puts("\nYou come upon an enchanted forest.  Enter?");
    int ch;
    do {
        printf("(Y/N) ");
        ch = read_key();
        if (ch == EOF) {
            return false;
        }
        ch = toupper((unsigned char)ch);
        putchar('\n');
    } while (ch != 'Y' && ch != 'N');
    if (ch == 'N') {
        puts("You move on.");
        return true;
    }

    if (random_number(100) + 1 < player->level + 5) {
        puts(player->gender == 'M'
             ? "You are confronted by a beautiful Female Nymph."
             : "You are confronted by an attractive Male Nymph.");
        puts("The nymph is luring you deep into the forest.");
        if (random_number(100) + 1 < 50 + 2 * player->level) {
            puts("but you resist & the nymph disappears.");
        } else {
            puts("& you follow.  You are lost in La La Land.");
            player->hit_points = 0;
        }
    }

    if (player->hit_points > 0 && player->ring_power == 0) {
        puts("The elven folk come out and greet you.");
        puts("They give you food for everyone in your party.");
        player->food += player->male_servants + player->female_servants + 1;
        if (!player->good_looks) {
            puts("The elves also cure your ugliness.");
            player->good_looks = true;
        }
        if (random_number(100) + 1 < 21) {
            puts("A good faerie appears to wish you well on your quest.");
            if (player->plague > 0) {
                puts("She cures you of the Plague.");
                player->plague = 0;
            } else {
                puts("She restores all your hit points.");
                if (player->hit_points < player->level * 10) {
                    player->hit_points = player->level * 10;
                }
            }
        }
    }
    return wait_for_key();
}

static void event_overlords_keep(void)
{
    puts("\nYou find the Evil Overlord's Keep.");
}

static void event_flowing_river(void)
{
    puts("\nYou come upon a flowing river.");
}

static bool event_hollow_tree(Player* player)
{
    puts("\nYou come up to a hollow tree with a door.  Enter?");
    int ch;
    do {
        printf("(Y/N) ");
        ch = read_key();
        if (ch == EOF) {
            return false;
        }
        ch = toupper((unsigned char)ch);
        putchar('\n');
    } while (ch != 'Y' && ch != 'N');
    if (ch == 'N') {
        puts("You move on.");
        return wait_for_key();
    }

    /* Original occupied branch: 0x67C0-0x696D; food award fixed per CHANGES.md. */
    if (random_number(100) + 1 < 56) {
        puts("Inside is a little Gnome smoking a pipe.");
        puts("There is a kettle of stew over a fire, and a large");
        puts("wooden trunk next to the wall.");
        int food = random_number(6) + 3;
        printf("The gnome greets you and gives you %d food.\n\n", food);
        player->food += food;
        if (player->hit_points < player->level * 10) {
            int healing = random_number(5) + 1;
            printf("The gnome gives you an ointment that cures %d hit points\n", healing);
            player->hit_points += healing;
        }
    } else {
        puts("No one seems to be home.");
        puts("There is a kettle of stew over a fire, and a large");
        puts("wooden trunk next to the wall.");
        /* TODO: Recover the empty home's choices and effects; see doc/events.md. */
        puts("This encounter is not implemented yet.");
    }
    return wait_for_key();
}

static void event_grass_hut(void)
{
    puts("\nYou find a grass hut.");
}

static void event_sick_servants(void)
{
    puts("\nSome of your servants are sick.");
}

static void event_statue_of_pan(void)
{
    puts("\nYou find a statue of Pan.");
}

static void event_storm(void)
{
    puts("\nA storm approaches.");
}

static void event_thicket(void)
{
    puts("\nYou find a thicket in the woods.");
}

static void event_gypsy_camp(void)
{
    puts("\nYou come upon a wandering Gypsy camp.");
}

static void event_mountain_trail(void)
{
    puts("\nYou come upon a mountain trail.");
}

static void event_empty_tree(void)
{
    puts("\nYou find a hollow tree with a door.  No one seems to be home.");
}

static bool run_event(Player* player)
{
    /* Original dispatch at 0xEF97-0xF10F; see doc/mechanics.md#main-turn-loop. */
    switch (random_number(16) + 1) {
    case 1: event_village(); break;
    case 2: event_city(); break;
    case 3: event_gypsy_camp(); break;
    case 4: event_combat(); break;
    case 5: event_thicket(); break;
    case 6: return event_hollow_tree(player);
    case 7: event_grass_hut(); break;
    case 8:
        if (player->ring_power > 0) {
            event_mountain_trail();
        } else {
            event_combat();
        }
        break;
    case 9:
        if (player->illness_protection) {
            event_combat();
        } else if (player->male_servants + player->female_servants == 0) {
            puts("There seems to be a bug going around.");
            return true;
        } else {
            event_sick_servants();
        }
        break;
    case 10: event_storm(); break;
    case 11: event_dark_castle(); break;
    case 12: return event_enchanted_forest(player);
    case 13: event_empty_tree(); break;
    case 14: event_flowing_river(); break;
    case 15: event_statue_of_pan(); break;
    case 16: event_overlords_keep(); break;
    }
    /* TODO: Recover these branches and their pauses; see doc/events.md. */
    puts("This encounter is not implemented yet.");
    return wait_for_key();
}

static bool finish_turn(Player* player)
{
    /* Preserve this order: 0xF112-0xF36D; see doc/mechanics.md#main-turn-loop. */
    if (player->hit_points > 0 && player->hit_points < player->level * 10) {
        player->hit_points += 0.10;
    }
    if (player->hit_points <= 0) {
        player->experience = 0;
    }
    if (player->experience > 5) {
        puts("You have raised a level.");
        ++player->level;
        player->experience -= 5;
        player->hit_points += 10;
    }
    if (player->ring_power > 0 && player->ring_power < 100) {
        player->ring_power += 5;
    }
    if (player->plague > 0) {
        player->plague += 5;
    }
    if (player->plague >= 100) {
        puts("You have died from the plague");
        player->hit_points = 0;
    }
    player->food -= (player->male_servants + player->female_servants + 1) / 2;
    if (player->food < 0) {
        player->food = 0;
        if (player->male_servants + player->female_servants > 1) {
            puts("You lose some servants due to starvation.");
            if (player->male_servants > 0) {
                --player->male_servants;
            }
            if (player->female_servants > 0) {
                --player->female_servants;
            }
        }
    }
    if (player->hit_points <= 0) {
        puts("Game Over.");
        return false;
    }
    return true;
}

int main(void)
{
    initialize_input();

    /* The entire game uses yellow text; cleanup resets the terminal color. */
    printf("\033[93m");

    random_state = (uint32_t)time(NULL);

    if (!display_title()) {
        return EXIT_SUCCESS;
    }

    clear_screen();
    Player player = {0};
    if (!create_player(&player)) {
        return EXIT_SUCCESS;
    }

    for (;;) {
        clear_screen();
        if (!display_stats(&player)) {
            break;
        }

        if (!run_event(&player) || !finish_turn(&player)) {
            break;
        }
    }

    return EXIT_SUCCESS;
}
