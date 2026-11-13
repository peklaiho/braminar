#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <signal.h>
#include <stdbool.h>
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
} Player;

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

    player->level = 8;
    player->hit_points = 30.20;
    player->hair_color = "Brown";
    player->good_looks = true;
    player->male_servants = 12;
    player->female_servants = 9;
    player->gold = 10;
    player->food = 91;
    player->medicine = 3;
    return true;
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

static void event_enchanted_forest(void)
{
    puts("\nYou enter an enchanted forest.");
}

static void event_overlords_keep(void)
{
    puts("\nYou find the Evil Overlord's Keep.");
}

static void event_flowing_river(void)
{
    puts("\nYou come upon a flowing river.");
}

static void event_hollow_tree(void)
{
    puts("\nYou find a hollow tree with a door.");
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

static void run_event(void)
{
    static void (*const events[])(void) = {
        event_city,
        event_village,
        event_combat,
        event_dark_castle,
        event_enchanted_forest,
        event_overlords_keep,
        event_flowing_river,
        event_hollow_tree,
        event_grass_hut,
        event_sick_servants,
        event_statue_of_pan,
        event_storm,
        event_thicket,
        event_gypsy_camp,
    };

    /* Equal chances are placeholders until the original probabilities are recovered. */
    size_t count = sizeof(events) / sizeof(events[0]);
    events[rand() % count]();
}

int main(void)
{
    initialize_input();

    /* The entire game uses yellow text; cleanup resets the terminal color. */
    printf("\033[93m");

    srand((unsigned int)time(NULL));

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

        run_event();
        if (!wait_for_key()) {
            break;
        }
    }

    return EXIT_SUCCESS;
}
