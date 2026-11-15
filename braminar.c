#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <limits.h>
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
    int dragon_charm;
    char prime_command[6];
} Player;

typedef struct {
    const char* name;
    int level;
    double hit_points;
} Monster;

static uint32_t random_state;
static int male_servant_price;
static int female_servant_price;

static int random_number(int limit)
{
    /* Original runtime at file offsets 0x0FDA-0x101A: 32-bit state, 15-bit output. */
    random_state = random_state * UINT32_C(129) + UINT32_C(0x361962E9);
    if (limit == 0) {
        fputs("Division by zero.\n", stderr);
        exit(EXIT_FAILURE);
    }
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

static int read_choice(const char* choices)
{
    for (;;) {
        int ch = read_key();
        if (ch == EOF) {
            return EOF;
        }
        ch = toupper((unsigned char)ch);
        if (ch != '\0' && strchr(choices, ch) != NULL) {
            putchar('\n');
            return ch;
        }
    }
}

static bool read_integer(int* value)
{
    char line[160];
    for (;;) {
        if (!read_line(line, sizeof(line))) {
            return false;
        }
        *value = 0;
        const char* cursor = line;
        while (isspace((unsigned char)*cursor)) {
            ++cursor;
        }
        if (*cursor == '\0') {
            continue;
        }
        if (strlen(line) == sizeof(line) - 1 || !isdigit((unsigned char)*cursor)) {
            return true;
        }
        int number = 0;
        while (isdigit((unsigned char)*cursor)) {
            int digit = *cursor++ - '0';
            if (number > (INT_MAX - digit) / 10) {
                return true;
            }
            number = number * 10 + digit;
        }
        while (isspace((unsigned char)*cursor)) {
            ++cursor;
        }
        if (*cursor == '\0') {
            *value = number;
        }
        return true;
    }
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
    player->dragon_charm = 0;
    strcpy(player->prime_command, "     ");

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

static int ask_yes_no(const char* question)
{
    puts(question);
    printf("(Y/N) ");
    return read_choice("YN");
}

static bool visit_servant_market(Player* player)
{
    int choice = ask_yes_no("Do you want to go to the servant Market?");
    if (choice != 'Y') {
        return choice != EOF;
    }
    choice = ask_yes_no("Sell Servants?");
    if (choice == EOF) {
        return false;
    }
    int males, females;
    if (choice == 'Y') {
        printf("Males: ");
        if (!read_integer(&males)) {
            return false;
        }
        printf("Females: ");
        if (!read_integer(&females)) {
            return false;
        }
        if (males > player->male_servants) {
            males = player->male_servants;
        }
        if (females > player->female_servants) {
            females = player->female_servants;
        }
        player->male_servants -= males;
        player->female_servants -= females;
        player->gold += males * male_servant_price + females * female_servant_price;
        puts("Done");
    }
    choice = ask_yes_no("Buy Servants?");
    if (choice != 'Y') {
        return choice != EOF;
    }
    printf("Males: ");
    if (!read_integer(&males)) {
        return false;
    }
    printf("Females: ");
    if (!read_integer(&females)) {
        return false;
    }
    if (males <= player->gold / male_servant_price) {
        player->gold -= males * male_servant_price;
        player->male_servants += males;
    }
    if (females <= player->gold / female_servant_price) {
        player->gold -= females * female_servant_price;
        player->female_servants += females;
    }
    puts("Done");
    return true;
}

static bool visit_tavern(Player* player)
{
    int choice = ask_yes_no("Do you wish to enter the Tavern?");
    if (choice != 'Y') {
        return choice != EOF;
    }
    choice = ask_yes_no("Do you wish to buy a drink? (2gp)");
    if (choice == EOF) {
        return false;
    }
    if (choice == 'Y' && player->gold >= 2) {
        player->gold -= 2;
        int healing = random_number(5) + 1;
        if (random_number(100) + 1 < 30) {
            printf("The drink was good.  It raises your hp: %d\n", healing);
            player->hit_points += healing;
            if (player->hit_points > player->level * 10) {
                player->hit_points = player->level * 10;
            }
        } else if (random_number(100) + 1 < 15) {
            if (random_number(100) + 1 < player->level + 20) {
                puts("Just before you drink, you notice that");
                puts(" it is poisoned.  You stop and save it.");
                ++player->poison;
            } else {
                puts("You discover that the drink you just");
                puts(" chugged down was poisoned.");
                if (player->medicine > 0) {
                    puts("You cure yourself with medicine.");
                    --player->medicine;
                } else {
                    puts("You discover you have no medicine!");
                    player->hit_points = 0;
                }
            }
        } else {
            puts("It wasn't so hot.");
        }
    }
    choice = ask_yes_no("Do you wish to socialize?");
    if (choice == EOF) {
        return false;
    }
    if (choice == 'Y') {
        int patron = random_number(100) + 1;
        if (patron <= 20) {
            bool girl = patron <= 10;
            puts(girl ? "You meet a girl" : "You meet a guy");
            static const char* const hair[] = {"Blonde", "Gold", "Brown", "Black"};
            int color = random_number(5);
            if (color < 4) {
                printf("%s hair is %s\n", girl ? "Her" : "His", hair[color]);
            } else {
                puts(girl ? "She's Bald." : "He's Bald.");
            }
            static const char* const looks[] = {"Ugly as sin", "homely", "average-looking", "good-looking"};
            int appearance = random_number(5);
            if (appearance < 4) {
                printf("%s %s\n", girl ? "She's" : "He's", looks[appearance]);
            } else {
                puts(girl ? "She's beautiful!" : "He's gorgeous!");
            }
            choice = ask_yes_no("Will you Move in?");
            if (choice == EOF) {
                return false;
            }
            if (choice == 'Y') {
                int charm = random_number(100) + 1;
                if (charm < 20 && player->good_looks) {
                    printf("%s falls for your suave charm and becomes one of your servants.\n",
                           girl ? "She" : "He");
                    if (girl) {
                        ++player->female_servants;
                    } else {
                        ++player->male_servants;
                    }
                } else {
                    puts(girl ? "She doesn't like you." : "He doesn't like you.");
                }
            }
        } else {
            puts("No one to socialize with.");
        }
    }
    puts("A patron invites you to play darts. Play?");
    choice = ask_yes_no("Ante is 5 GP");
    if (choice == EOF) {
        return false;
    }
    if (choice == 'Y' && player->gold >= 5) {
        struct timespec delay = {0, 500000000};
        nanosleep(&delay, NULL);
        int score = random_number(50) + 1;
        int opponent = random_number(50) + 1;
        printf("You shoot a :%d\nPatron shoots a : %d\n", score, opponent);
        if (score >= opponent) {
            puts("You Win!");
            player->gold += 10;
        } else {
            puts("You lost.");
            if (player->ring_power > 60) {
                puts("You are angry & you take the money & run.");
            } else {
                player->gold -= 5;
            }
        }
    }
    puts("You leave the tavern.");
    return true;
}

static void show_gold(const Player* player)
{
    printf("You have: %7d\n", player->gold);
}

static bool buy_store_item(Player* player, const char* name, int price, bool first, bool arrows, bool* bought)
{
    if (!first) {
        show_gold(player);
    }
    printf("%s Costs %dgp\n", name, price);
    if (first) {
        show_gold(player);
    }
    int choice = ask_yes_no(arrows ? "Buy 12?" : "Buy one?");
    *bought = choice == 'Y' && player->gold >= price;
    if (*bought) {
        player->gold -= price;
    }
    putchar('\n');
    return choice != EOF;
}

static bool buy_supplies(Player* player, const char* description, int price, int* quantity, bool marketplace)
{
    show_gold(player);
    puts(description);
    puts(marketplace ? "How many to buy? (0 for none)" : "How much to buy? (0 for none)");
    do {
        if (marketplace) {
            printf("? ");
        }
        if (!read_integer(quantity)) {
            return false;
        }
    } while (*quantity > player->gold / price);
    player->gold -= *quantity * price;
    return true;
}

static bool visit_store(Player* player)
{
    int choice = ask_yes_no("Do you wish to go to the store?");
    if (choice != 'Y') {
        return choice != EOF;
    }
    puts("    The General Store");
    puts("------------------------------");
    const char* names[] = {"Sword", "Bows", "Daggers", "12 Arrows"};
    int* stocks[] = {&player->swords, &player->bows, &player->daggers, &player->arrows};
    for (int i = 0; i < 4; ++i) {
        int price = random_number(i == 3 ? 20 : 100) + 1;
        bool bought;
        if (!buy_store_item(player, names[i], price, i == 0, i == 3, &bought)) {
            return false;
        }
        if (bought) {
            *stocks[i] += i == 3 ? 12 : 1;
        }
    }
    int quantity;
    if (!buy_supplies(player, "Food is one unit/gp", 1, &quantity, false)) {
        return false;
    }
    player->food += quantity;
    putchar('\n');
    if (!buy_supplies(player, "Medicine is 1 unit / 3gps", 3, &quantity, false)) {
        return false;
    }
    player->medicine += quantity;
    if (!player->horse) {
        bool bought;
        int price = random_number(150) + 1;
        if (!buy_store_item(player, "Horses", price, false, false, &bought)) {
            return false;
        }
        player->horse = bought;
    }
    if (!player->cart) {
        bool bought;
        int price = random_number(250) + 1;
        if (!buy_store_item(player, "Carts", price, false, false, &bought)) {
            return false;
        }
        player->cart = bought;
    }
    return true;
}

static bool visit_inn(Player* player)
{
    int choice = ask_yes_no("Do you want to recouperate at the Inn?");
    if (choice != 'Y') {
        return choice != EOF;
    }
    puts("Rooms are as follows:");
    puts(" 1   Po-Dunkville   5 gold      Low/No class rooms");
    puts(" 2   Holiday out   10 gold      Medium class rooms");
    puts(" 3   Royal Roost   20 gold      High class rooms\n");
    show_gold(player);
    puts("Choose bedding: ");
    choice = read_choice("123");
    if (choice == EOF) {
        return false;
    }
    int cost = choice == '1' ? 5 : (choice == '2' ? 10 : 20);
    player->gold -= cost;
    if (player->gold < 0) {
        puts("You have insufficient funds & are kicked out.");
        player->gold = 0;
        return true;
    }
    puts("You lay back and relax for awhile.");
    player->hit_points += random_number(cost) + 1;
    if (player->male_servants < 1 && random_number(100) + 1 < 15 && player->food > 0) {
        puts("Rats sneak in and eat some of your food");
        --player->food;
    }
    if (player->female_servants > 1) {
        puts("Your female servants bandage your wounds.");
        player->hit_points += player->female_servants / 2;
    }
    struct timespec delay = {2, 0};
    nanosleep(&delay, NULL);
    return true;
}

static bool visit_marketplace(Player* player)
{
    int choice = ask_yes_no("It seems that the Market Place is open.  Enter?");
    if (choice == EOF) {
        return false;
    }
    if (choice == 'N') {
        puts("You proceed on.");
        return true;
    }
    int quantity;
    if (!buy_supplies(player, "Food here is 5 units per gold piece.", 1, &quantity, true)) {
        return false;
    }
    player->food += quantity * 5;
    putchar('\n');
    if (!buy_supplies(player, "Poison costs 100 gold for a dose.", 100, &quantity, true)) {
        return false;
    }
    player->poison += quantity;
    return true;
}

static bool meet_cleric(Player* player)
{
    puts("A Cleric comes up to you and says:  \"I will cure you of all ailments if you ");
    int choice = ask_yes_no("donate all your gold to my church.  Donate?");
    if (choice == EOF) {
        return false;
    }
    if (choice == 'Y') {
        player->gold = 0;
        player->good_looks = true;
        player->hair_color = "Brown";
        player->plague = 0;
        player->ring_power = 0;
        if (player->hit_points < player->level * 10) {
            player->hit_points = player->level * 10;
        }
        puts("the cleric waves a mace over you & disappears.");
    } else {
        puts("He leaves.");
    }
    struct timespec delay = {0, 500000000};
    nanosleep(&delay, NULL);
    return true;
}

static bool event_town(Player* player, bool city)
{
    /* Shared services at 0xAB46-0xC2B8; callers at 0xC2B9 and 0xC4C1. */
    int choice = ask_yes_no(city ? "\nYou come upon a City.  Do you want to enter?"
                                 : "\nYou come upon a Village.  Do you want to enter?");
    if (choice == EOF) {
        return false;
    }
    if (choice == 'N') {
        puts("You go on.");
        return true;
    }
    if (player->ring_power > 10) {
        if (city) {
            puts("As you enter, a centurian says to you");
            puts(random_number(100) + 1 < 51 ? "\"Beware the tretcherous liars!\""
                                             : "\"Have you driven a Ford lately?\"");
        } else {
            puts("A peasant passing by says, \"Mercy is for the Weak!\"");
        }
    }
    if (!city && random_number(100) + 1 < 8) {
        if (random_number(100) + 1 < 25 + player->level) {
            puts("You happen to notice that one sly-looking gentlman");
            puts("is about to slip his hand into your pocket to steal");
            puts("some gold from you, but you bop him in the nose.");
        } else {
            player->gold -= random_number(player->gold) + 1;
        }
    }
    if (!visit_servant_market(player) || !visit_tavern(player) || !visit_store(player)) {
        return false;
    }
    if (city && random_number(100) + 1 < 18 && !visit_marketplace(player)) {
        return false;
    }
    if (!visit_inn(player)) {
        return false;
    }
    if (city) {
        if (random_number(100) + 1 > 88 && !meet_cleric(player)) {
            return false;
        }
        int mugger = random_number(100) + 1;
        if (player->male_servants + player->female_servants == 0 && mugger < 11) {
            puts("On your way out of town, Muggers jump you.");
            if (player->ring_power > 30) {
                puts("But suddenly, your ring shoots lightning bolts at them and they flee.");
            } else {
                int gold = random_number(100) + 1;
                if (gold > player->gold) {
                    gold = player->gold;
                }
                int food = random_number(20) + 1;
                if (food > player->food) {
                    food = player->food;
                }
                printf("They stole %d gold and %d Food.\n", gold, food);
                player->gold -= gold;
                player->food -= food;
                struct timespec delay = {1, 0};
                nanosleep(&delay, NULL);
            }
            return wait_for_key();
        }
    }
    return true;
}

static void combat_round(Player* player, Monster* monster)
{
    static const double player_damage[] = {0, 0, 0.01, 0.20, 2.10, 3.20, 3.90, 4.60, 5.10, 6.20};
    static const double monster_damage[] = {0, 0, 0, 0.01, 0.10, 1.02, 2.10, 3.20, 4.10, 5.00};
    static const char* const attacks[3][9] = {
        {
            "You swing but miss him.",
            "You bump him a bit.",
            "You hit him lightly.",
            "You hit him good.",
            "A strong uppercut knocks him back.",
            "You throw a stone & hit him.",
            "You bust a branch over his head.",
            "You push his face into a tree.",
            "You slam the brute on the floor.",
        },
        {
            "You Swing, but miss.",
            "You nick him slightly.",
            "You cut him slightly.",
            "Your blade draws blood.",
            "You gouge him deep, but he still stands.",
            "Your blade reaches bone.",
            "You sever a finger.",
            "You parry, and open his side.",
            "You nearly sever a limb.",
        },
        {
            "You Shoot, but miss.",
            "Your arrow scratches him.",
            "Your arrow slices him.",
            "Your arrow draws blood.",
            "Your arrow sinks deep, but he's ok.",
            "An arrow pierces his leg.",
            "An arrow pierces his arm.",
            "You dodge, and plant one in his leg.",
            "An arrow lodges deep in his chest.",
        },
    };
    static const char* const retaliation[] = {
        "The beast misses you.",
        "He nudged you slightly.",
        "He scratched you.",
        "He knocked the wind out of you.",
        "He slugged you in the gut well.",
        "He punched you in the face.",
        "He threw a rock and hit you.",
        "He hit you on the back with a tree branch.",
        "He flipped you over, and slammed you.",
    };

    /* Shared combat at 0x2F4B-0x3B53; see doc/mechanics.md#shared-combat. */
    int attack = random_number(9) + 1;
    if (player->ring_power > 0) {
        attack += 2;
    }
    int weapon = 0;
    if (player->swords > 0 || player->daggers > 0) {
        weapon = 1;
    } else if (player->bows > 0 && player->arrows > 0) {
        weapon = 2;
    }
    if (weapon != 0 && random_number(100) + 1 < 26) {
        ++attack;
    }
    if (attack > 9) {
        attack = 9;
    }
    puts(attacks[weapon][attack - 1]);
    monster->hit_points -= player_damage[attack];
    if (weapon == 2) {
        --player->arrows;
    }

    /* The original retaliates even if the player's blow killed the monster. */
    int counterattack = monster->level > 10 ? random_number(8) + 2 : random_number(9) + 1;
    puts(retaliation[counterattack - 1]);
    player->hit_points -= monster_damage[counterattack];
    printf("\n---Player: %5.2f          Monster: %5.2f\n",
           player->hit_points, monster->hit_points);
}

static bool fight_monster(Player* player, Monster monster)
{
    if (strcmp(monster.name, "Rust Golem") == 0) {
        puts("The rust golem causes any swords/daggers to rust.");
        player->swords = 0;
        player->daggers = 0;
    }
    if (strcmp(monster.name, "Drow Elf") == 0) {
        puts("Drow elves Cause Bowstrings to snap & arrows to warp.");
        player->bows = 0;
        player->arrows = 0;
    }
    bool poisoned = false;
    if (player->poison > 0) {
        puts("Do you wish to use your poison?");
        printf("(Y/N) ");
        int choice = read_choice("YN");
        if (choice == EOF) {
            return false;
        }
        if (choice == 'Y') {
            --player->poison;
            poisoned = true;
        }
    }
    monster.hit_points = (random_number(7) + 1) * monster.level;
    if (poisoned) {
        monster.hit_points = 0;
    }
    while (player->hit_points > 0 && monster.hit_points > 0) {
        combat_round(player, &monster);
        if (!wait_for_key()) {
            return false;
        }
    }
    puts(player->hit_points > 0 ? "You Won the fight!" : "You did not survive the fight.");
    puts(monster.hit_points > 0 ? "The monster won the fight." : "The monster is dead.");
    if (player->hit_points > 0) {
        player->experience += monster.level / 4.0;
    }
    int ring_roll = random_number(100) + 1;
    if (ring_roll < player->level && player->hit_points > 0 && player->ring_power == 0) {
        puts("You find a shiny ring on your opponent's finger & take it.");
        player->ring_power = 5;
    }
    if (player->hit_points > 0) {
        int males = random_number(20) + 1;
        int females = random_number(20) + 1;
        int gold = random_number(50) + 1;
        puts("The monsters fled leaving behind,");
        printf("     %d Male servants\n     %d Female Servants\n", males, females);
        printf("     %d Gold Pieces\n     %d Food.\n", gold, males + females);
        player->male_servants += males;
        player->female_servants += females;
        player->gold += gold;
        player->food += males + females;
    }
    return wait_for_key();
}

static bool event_combat(Player* player)
{
    static const Monster monsters[] = {
        {"Goblin", 5, 0}, {"Gremlin", 6, 0}, {"Rust Golem", 6, 0},
        {"Orc", 7, 0}, {"Ogre", 9, 0}, {"Pirate", 10, 0},
        {"Troglodyte", 11, 0}, {"Drow Elf", 12, 0}, {"Demon", 14, 0},
    };
    puts("\nThings seem strangely quiet when out of nowhere jumps");
    Monster monster = monsters[random_number(9)];
    printf("A band of %ss.  They look grumpy.\n", monster.name);
    int males = random_number(10) + 1;
    int females = random_number(10) + 1;
    int gold = random_number(10) + 1;
    puts("They say they will let you pass if you give them:");
    printf("%d  Male servants.\n%d  Female servants.\n%d  gold pieces.\n", males, females, gold);
    puts("or you may fight their champ, and win their hoard.\n");
    puts("Do you wish to (F)ight (G)ive, or (R)un?");
    int choice;
    if (player->ring_power > 45) {
        puts("Your ring compels you to fight.");
        choice = 'F';
    } else {
        choice = read_choice("FGR");
    }
    if (choice == EOF) {
        return false;
    }
    if (choice == 'F') {
        return fight_monster(player, monster);
    }
    if (choice == 'G') {
        if (player->male_servants >= males && player->female_servants >= females && player->gold >= gold) {
            puts("They take their goodies and leave.");
            player->male_servants -= males;
            player->female_servants -= females;
            player->gold -= gold;
        } else {
            printf("You have insufficient funds.  The %ss\n", monster.name);
            puts("Take what you have and assassinate you.");
            player->hit_points = 0;
        }
    } else {
        if (player->horse && !player->cart && player->male_servants + player->female_servants > 2) {
            puts("You lose some servants (you have no cart).");
            if (player->male_servants > 1) {
                player->male_servants = 1;
            }
            if (player->female_servants > 1) {
                player->female_servants = 1;
            }
        }
        int escape_roll = random_number(100) + 1;
        if (!player->horse && monster.level > 9 && escape_roll < monster.level * 3) {
            puts("They caught you.  You must battle.");
            if (!fight_monster(player, monster)) {
                return false;
            }
        } else {
            puts("You got away!");
        }
    }
    fflush(stdout);
    struct timespec delay = {1, choice == 'R' ? 500000000 : 0};
    nanosleep(&delay, NULL);
    return true;
}

static bool fight_mad_king(Player* player)
{
    /* Separate combat at 0x5DBE-0x6768; see doc/mechanics.md#mad-king-combat. */
    static const char* const attacks[] = {
        "You miss the King.",
        "You just nicked the King.",
        "You did slight damage to the king.",
        "You hit the king heartily.",
        "You cut the King.",
        "You strike him with a convenient candle stand.",
        "You kick the king hard.",
        "You grab a nearby shield & hit him with it.",
        "You slam the king into a nearby wall.",
    };
    static const double damage[] = {0, 0, 0.7, 1.6, 2.1, 2.4, 3.2, 3.4, 3.9, 4.5};
    static const char* const counters[] = {
        "The King missed you.", "The King yells: ",
        "The King cold-cuts you.", "The King bites your arm.",
        "The King smacks you with his sceptre.", "The King Bites your leg.",
        "The King cuts you.", "The King stabbs you with a dagger.",
        "The King flips you over & slams you.",
    };
    static const double counter_damage[] = {0, 0, 0, 0.4, 1.1, 1.7, 1.8, 2.4, 3.2, 3.9};
    static const char* const shouts[] = {
        "\"You will not be King, Duncan!!\"",
        "\"Hold the Pickle, Hold the Lettuce!\"",
        "\"Eat my Shorts!!\"",
        "\"Beware of brave Sir Robin!!\"",
        "\"Wouldn't you like to be a Pepper too?\"",
        "\"I just want a Condo with a view!\"",
    };
    struct timespec delay = {0, 150000000};
    nanosleep(&delay, NULL);
    double king_hp = (random_number(5) + 5) * 16;
    if (player->poison > 0) {
        int choice = ask_yes_no("Use your poison? ");
        if (choice == EOF) {
            return false;
        }
        if (choice == 'Y') {
            --player->poison;
            king_hp = 0;
        }
    }
    while (king_hp > 0 && player->hit_points > 0) {
        int attack = random_number(9) + 1;
        if (player->ring_power > 0) {
            attack += 2;
        }
        if (player->swords + player->daggers > 0 || (player->bows > 0 && player->arrows > 0)) {
            attack += random_number(1);
        }
        if (attack > 9) {
            attack = 9;
        }
        puts(attacks[attack - 1]);
        king_hp -= damage[attack];
        int counter = random_number(9) + 1;
        if (counter == 2) {
            printf("%s", counters[counter - 1]);
            puts(shouts[random_number(6)]);
        } else {
            puts(counters[counter - 1]);
        }
        player->hit_points -= counter_damage[counter];
        printf("\n---Player: %5.2f       King: %5.2f\n", player->hit_points, king_hp);
        if (!wait_for_key()) {
            return false;
        }
    }
    puts(player->hit_points > 0 ? "You won!" : "You did not survive the battle.");
    puts(king_hp > 0 ? "The King has won." : "The King disappears.");
    if (player->hit_points > 0) {
        player->experience += random_number(3) + 3;
        puts("You flee before the King re-materializes.");
        player->gold += random_number(10) + 1;
    }
    return true;
}

static bool castle_dungeon(Player* player)
{
    puts("You Enter the Dungeon...");
    puts("It is dark and creepy down here.  There are rats & chains everywhere.");
    puts("You find a few gold pieces on the floor.");
    player->gold += random_number(10) + 1;
    if (random_number(100) + 1 < 19) {
        puts("You are overwhelmed by the smell.");
        player->hit_points -= 0.1;
    }
    if (random_number(100) + 1 < 6 && !player->staff_of_aviatar) {
        puts("You find the STAFF of AVIATAR!!");
        player->staff_of_aviatar = true;
    }
    if (random_number(100) + 1 < 11) {
        int males = random_number(5) + 1;
        printf("You find %d male servants.\n", males);
        player->male_servants += males;
    }
    if (random_number(100) + 1 < 4) {
        puts("You find a vial of Poison!");
        ++player->poison;
    }
    if (random_number(100) + 1 < 13) {
        puts("The door closes shut on the dungeon!!");
        if (player->male_servants + player->female_servants > 0 || player->daggers > 0) {
            puts("You manage to escape with the help of a dagger or your servants.");
        } else {
            puts("You cannot escape!!  You Starve.");
            player->hit_points = 0;
        }
    }
    return true;
}

static bool castle_throne(Player* player)
{
    puts("You spot a large chest of gold at the throne. Press: 1 / = \\ H  keys in that ");
    puts("sequence to simulate grabbing gold & putting it in a bag.  A miss-typed key =");
    puts("dropped gold.  Don't make too much noise, or the king will hear you and enter ");
    puts("the room!!  Space bar = leave the room.");
    int progress = 0;
    int noise = 0;
    bool leave = false;
    do {
        int ch = read_key();
        if (ch == EOF) {
            return false;
        }
        ch = toupper((unsigned char)ch);
        if (ch == ' ') {
            puts("You Leave.");
            leave = true;
        } else {
            const char* sequence = "1/=\\H";
            const char* key = ch == '\0' ? NULL : strchr(sequence, ch);
            if (key != NULL) {
                if (progress == key - sequence) {
                    ++progress;
                } else {
                    puts("You drop some gold.");
                    struct timespec delay = {0, 800000000};
                    nanosleep(&delay, NULL);
                    noise += random_number(3) + 1;
                }
            }
        }
        if (progress == 5) {
            puts("Got some gold.");
            player->gold += random_number(8) + 1;
            progress = 0;
        }
        ++noise;
        if (noise > 10 && progress == 4) {
            struct timespec delay = {0, 30000000};
            nanosleep(&delay, NULL);
        }
    } while (!leave && noise <= 15);
    if (noise > 15) {
        puts("The Mad king rushes at you!!!");
        return fight_mad_king(player);
    }
    return true;
}

static bool castle_galley(Player* player)
{
    static const char* const visions[] = {
        "You see the Staff of Aviatar in the Wizard's home.",
        "You see the Staff of Aviatar in the Gnome's trunk.",
        "You see the Staff of Aviatar in the Grass Hut.",
        "You see the Staff of Aviatar in the Mad Kings Dungeon.",
        "You see the Staff of Aviatar in a flowing river.",
        "You see the Talisman of Braminar in a flowing river.",
        "You see the Talisman of Braminar in the Dark Castle Galley.",
    };
    puts("You enter the galley.  The Kitchen is in bad shape.");
    int choice = ask_yes_no("You discover a crystal ball on the table. gaze into it?");
    if (choice == EOF) {
        return false;
    }
    if (choice == 'Y') {
        int vision = random_number(100) + 1;
        puts(vision <= 14 ? visions[(vision - 1) / 2] : "You see nothing.");
    }
    if (random_number(100) + 1 < 9 && !player->talisman_of_braminar) {
        puts("You find the Talisman of BRAMINAR!!");
        player->talisman_of_braminar = true;
    }
    int food = random_number(200);
    printf("You find %d food.\n", food);
    player->food += food;
    if (random_number(100) + 1 < 36) {
        puts("Some of your servants fall down pits in the floor!");
        player->male_servants /= 2;
        player->female_servants /= 2;
    }
    return true;
}

static bool event_dark_castle(Player* player)
{
    /* Castle at 0x7E15-0x8BA6; see doc/mechanics.md#dark-castle. */
    int choice = ask_yes_no("\nYou come upon the Dark Castle of the Mad King.  Enter?");
    if (choice == EOF) {
        return false;
    }
    if (choice == 'N') {
        puts("You continue on.");
        return true;
    }
    if (random_number(100) + 1 < 26) {
        puts("As you enter, you see Ghost Minstrals Playing music!");
        if (random_number(100) + 1 < player->level + 25) {
            puts("You flee in fear of your life!");
            return wait_for_key();
        }
    }
    puts("You may enter (D)ungeons, (T)hrone Room or (G)alley");
    choice = read_choice("DTG");
    if (choice == EOF) {
        return false;
    }
    bool result = choice == 'D' ? castle_dungeon(player)
                              : choice == 'T' ? castle_throne(player) : castle_galley(player);
    return result && wait_for_key();
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

static bool event_overlords_keep(Player* player)
{
    puts("\nYou find the Evil Overlord's Keep.");
    if (player->level < 20 || !player->staff_of_aviatar) {
        puts("You cannot attack due to lack of Staff or 20th level.");
    } else {
        /* TODO: Recover command input and final-assault transition; see doc/mechanics.md. */
        puts("This encounter is not implemented yet.");
    }
    return wait_for_key();
}

static bool event_flowing_river(Player* player)
{
    /* Original routine at 0x96E7-0x9DC7; see doc/mechanics.md#flowing-river. */
    static const char* visions[] = {
        "An attractive tavern patron following you.",
        "A deity watching over a hollow tree home.",
        "A monstrous beast waiting in a pentacle",
        "A needed treasure in the Gnome's trunk",
        "Your servants dying of starvation",
        "Your hair turning Magenta!",
        "You as the King of Braminar!",
        "A deity giving you a box of Duncin' Doughnuts",
        "Your servants coming down with chicken pox",
        "An old hermit admiring your female servants",
        "You defeating anything, while wearing a gold ring.",
        "A market that sells food very cheap",
        "The Mad King slaying you.",
        "Your troops fighting the Overlord's troops",
        "Your servants turning into demons",
        "You changing sex when you pass thru a water fall.",
        "A giant hamburger chasing you around.",
        "A trap door in the Holiday out hotel",
        "A magical sword that kills Goblins",
        "The Overlord wearing your outfit!!"
    };

    puts("\nYou come upon a flowing river.  Gaze into it?");
    printf("(Y/N) ");
    int choice = read_choice("YN");
    if (choice == EOF) {
        return false;
    }
    putchar('\n');
    if (choice == 'N') {
        puts("You Move on.");
        return true;
    }

    puts("You see the vision of:");
    printf("     %s\n", visions[random_number(20)]);
    int staff_roll = random_number(100) + 1;
    if (!player->staff_of_aviatar && staff_roll < 10) {
        puts("You spot the Staff of Aviatar in the River & get it!");
        player->staff_of_aviatar = true;
    }
    int talisman_roll = random_number(100) + 1;
    if (!player->talisman_of_braminar && talisman_roll < 6) {
        puts("You spot the Talisman of Braminar & get it!");
        player->talisman_of_braminar = true;
    }
    if (random_number(100) + 1 < 26) {
        puts("A group of Troglodytes jump out of the river "
             "& their champion attacks you!");
        Monster monster = {"Troglodyte", 11, 0};
        if (!fight_monster(player, monster)) {
            return false;
        }
    }
    return wait_for_key();
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
        /* Slot 6 empty home at 0x6A48-0x6C99; see doc/mechanics.md#empty-gnome-home. */
        puts("\nyou may (S)teal food, (T)ake from trunk or (L)eave.");
        int choice = read_choice("STL");
        if (choice == EOF) {
            return false;
        }
        if (choice == 'S') {
            int food = random_number(100) + 1;
            printf("You steal %d food.\n", food);
            player->food += food;
        } else if (choice == 'T') {
            switch (random_number(4) + 1) {
            case 1:
                puts("you find some arrows!!!");
                player->arrows += random_number(20) + 1;
                break;
            case 2:
                puts("You Find some medicine!!");
                player->medicine += random_number(10) + 1;
                break;
            case 3:
                if (!player->staff_of_aviatar) {
                    puts("You find the STAFF of AVIATAR!!!!!");
                    player->staff_of_aviatar = true;
                } else {
                    puts("You find some medicine!!!");
                    player->medicine += random_number(10) + 1;
                }
                break;
            case 4:
                puts("You release a Rune Guardian which comes out and burns you.");
                player->hit_points -= random_number(5) + 1;
                break;
            }
        }
    }
    return wait_for_key();
}

static bool fight_dragon(Player* player)
{
    static const char* const attacks[] = {
        "You miss the dragon.",
        "You scratched the dragon.",
        "You removed a few scales.",
        "You draw blood.",
        "You smack the dragon hard.",
        "You hit the dragon's Thor thimble!",
        "You cut off part of the dragons tail.",
    };
    static const char* const retaliation[] = {
        "The dragon missed you.",
        "The dragon nicks you.",
        "The dragon bakes you lightly with fire.",
        "The dragon cuts you with it's tail.",
        "The dragon whips you with it's tail.",
        "The dragon breaths fire on you.",
        "The dragon steps on you.",
    };
    static const double player_damage[] = {0, 0.1, 0.7, 1.2, 2, 4.3, 5.1};
    static const double dragon_damage[] = {0, 0.1, 0.6, 1.4, 2, 3.4, 4.2};
    if (player->dragon_charm > 0) {
        puts("You charm the Dragon into submission!");
        player->experience += random_number(3) + 1;
        return true;
    }
    int level = random_number(14) + 1;
    double hit_points = random_number(50) + 31;
    if (player->poison > 0) {
        int choice = ask_yes_no("Do you wish to use your poison?");
        if (choice == EOF) {
            return false;
        }
        if (choice == 'Y') {
            --player->poison;
            hit_points = 0;
            puts("You use your poison.");
        }
    }
    while (player->hit_points > 0 && hit_points > 0) {
        int attack = random_number(7);
        puts(attacks[attack]);
        hit_points -= player_damage[attack];
        int counterattack = random_number(7);
        puts(retaliation[counterattack]);
        player->hit_points -= dragon_damage[counterattack];
        printf("\n---Player: %5.2f       Dragon: %5.2f\n", player->hit_points, hit_points);
        if (!wait_for_key()) {
            return false;
        }
    }
    puts(player->hit_points > 0 ? "You win the battle!" : "You did not survive the battle.");
    puts(hit_points > 0 ? "The dragon won." : "The dragon is slain.");
    if (player->hit_points > 0) {
        player->experience += level / 2;
    }
    return true;
}

static bool fight_gate_creature(Player* player)
{
    static const char* const names[] = {"Cerberus", "Collosus", "Lolth", "Vaprak", "The Lich King"};
    static const int levels[] = {16, 16, 18, 20, 22};
    static const int hp_dice[] = {5, 7, 10, 11, 14};
    static const int hp_multipliers[] = {14, 12, 10, 10, 12};
    static const char* const attacks[] = {
        "You swing, but miss.",
        "You lightly hit %s\n",
        "You impact well against %s's back.\n",
        "You break a chair over %s\n",
        "You cut deep into %s\n",
        "You throw a test-tube at %s\n",
        "You happen to strike a pendant around %s's neck.\n",
    };
    static const char* const retaliation[] = {
        "%s misses you.\n", "%s nicks you slightly.\n", "%s smacks you in the face.\n",
        "%s hits you in the arm.\n", "%s breaths fire on you.\n",
        "%s cuts you with a clawing sweep.\n", "%s throws you across the room.\n",
    };
    static const double player_damage[] = {0, 0.2, 1.4, 2.5, 3.4};
    static const double monster_damage[] = {0, 1, 3, 5, 7, 9, 11.5};
    int type = random_number(5);
    const char* name = names[type];
    double hit_points = (random_number(hp_dice[type]) + 1) * hp_multipliers[type];
    printf("You are confronted by %s\n", name);
    struct timespec delay = {0, 500000000};
    nanosleep(&delay, NULL);
    if (player->poison > 0) {
        int choice = ask_yes_no("Do you wish to use your poison?");
        if (choice == EOF) {
            return false;
        }
        if (choice == 'Y') {
            --player->poison;
            if (random_number(100) + 1 > levels[type]) {
                puts("Your Poison worked!");
                hit_points = 0;
            } else {
                puts("The monster saves against your poison!");
            }
        }
    }
    while (player->hit_points > 0 && hit_points > 0) {
        int attack = random_number(7);
        if (attack == 0) {
            puts(attacks[attack]);
        } else {
            printf(attacks[attack], name);
        }
        if (attack < 5) {
            hit_points -= player_damage[attack];
        } else if (attack == 5) {
            hit_points -= random_number(10);
        } else {
            hit_points -= random_number(5) + 8;
        }
        int counterattack = random_number(7);
        printf(retaliation[counterattack], name);
        player->hit_points -= monster_damage[counterattack];
        printf("\n---Player: %5.2f       %s: %5.2f\n", player->hit_points, name, hit_points);
        if (!wait_for_key()) {
            return false;
        }
    }
    puts(player->hit_points > 0 ? "You win the battle!!!" : "You did not survive the battle.");
    printf("%s %s\n", name, hit_points > 0 ? "has destroyed you." : "is gone.");
    if (player->hit_points <= 0) {
        return true;
    }
    player->experience += levels[type] / 2;
    puts("\nA diety sees this great deed, and gives you: ");
    int reward = random_number(6) + 1;
    if (reward == 1 && !player->staff_of_aviatar) {
        puts("The STAFF OF AVIATAR!!!");
        player->staff_of_aviatar = true;
    } else if (reward == 2 && !player->talisman_of_braminar) {
        puts("The Talisman of Braminar!");
        player->talisman_of_braminar = true;
    } else if (reward <= 3) {
        puts("50 Lbs of food.");
        player->food += 250;
    } else if (reward == 4) {
        puts("5 units of Poison!");
        player->poison += 5;
    } else if (reward == 5) {
        puts("All Hit points restored!");
        player->hit_points = player->level * 10;
    } else {
        puts("a Super pack!!  (20 food, 10 Medicine 2 poison, and a horse and cart!)");
        player->food += 20;
        player->medicine += 10;
        player->poison += 2;
        player->horse = true;
        player->cart = true;
    }
    return true;
}

static bool event_grass_hut(Player* player)
{
    int occupied = random_number(100) + 1;
    printf("\nYou come upon a grass hut.  %s\n",
           occupied > 50 ? "You hear sounds from within." : "all is silent within.");
    int choice = ask_yes_no("Will you enter?");
    if (choice == EOF) {
        return false;
    }
    if (choice == 'N') {
        puts("You move on.");
        return wait_for_key();
    }
    if (occupied > 50) {
        int occupant = random_number(3) + 1;
        if (occupant == 1) {
            choice = ask_yes_no("You discover an old man in the corner.  He looks ill.  Try to cure him?");
            if (choice == EOF) {
                return false;
            }
            if (choice == 'Y') {
                if (player->medicine > 0) {
                    puts("you give him some medicine.");
                    --player->medicine;
                } else {
                    puts("You try to comfort him.");
                }
                puts("The old man seems better.");
                struct timespec delay = {1, 0};
                nanosleep(&delay, NULL);
            }
        } else if (occupant == 2) {
            choice = ask_yes_no("You find a hungry little puppy in the room.  Share some of your food?");
            if (choice == EOF) {
                return false;
            }
            if (choice == 'Y') {
                if (player->food > 0) {
                    --player->food;
                    puts("You feed the puppy.  It is greatful.");
                } else {
                    puts("You have no food, so you pet thepuppy.  ");
                }
                struct timespec delay = {1, 0};
                nanosleep(&delay, NULL);
            }
        } else {
            puts("You run into an old hermit with a");
            puts("Dragon.  He says if you can beat the dragon,");
            choice = ask_yes_no("he will double your posessions.  Do it?");
            if (choice == EOF) {
                return false;
            }
            if (choice == 'Y') {
                if (!fight_dragon(player)) {
                    return false;
                }
                if (player->hit_points > 0) {
                    if (random_number(100) + 1 < 21) {
                        int females = random_number(10) + 1;
                        if (females > player->female_servants) {
                            females = player->female_servants;
                        }
                        printf("The hermit disappears with %d of your female servants!!!!!\n", females);
                    } else {
                        puts("All your posessions are doubled!!");
                        player->male_servants *= 2;
                        player->female_servants *= 2;
                        player->gold *= 2;
                        player->food *= 2;
                        player->medicine *= 2;
                        player->poison *= 2;
                        player->swords *= 2;
                        player->bows *= 2;
                        player->arrows *= 2;
                        player->daggers *= 2;
                    }
                }
            } else {
                choice = ask_yes_no("Kill the hermit?");
                if (choice == EOF) {
                    return false;
                }
                if (choice == 'Y') {
                    puts("You kill the hermit & the dragon attacks!");
                    if (!fight_dragon(player)) {
                        return false;
                    }
                }
            }
        }
    } else {
        int find = random_number(9) + 1;
        switch (find) {
        case 1:
            puts("You find a rusty old sword.");
            ++player->swords;
            break;
        case 2:
            puts("You find a quiver of arrows.");
            player->arrows += 12;
            break;
        case 3: {
            int staff_roll = random_number(100) + 1;
            if (!player->staff_of_aviatar && staff_roll < 5) {
                puts("You find the STAFF of AVIATAR!!");
                player->staff_of_aviatar = true;
            } else {
                puts("You find nothing.");
            }
            break;
        }
        case 4: {
            int gold = random_number(25) + 1;
            printf("You find %d gold pieces.\n", gold);
            player->gold += gold;
            break;
        }
        case 5:
            puts("You find a female servant.");
            ++player->female_servants;
            break;
        case 6:
            puts("You find a male servant.");
            ++player->male_servants;
            break;
        case 7:
            puts("you discover a bow.");
            ++player->bows;
            break;
        case 8:
            puts("You discover an astral gate inside!");
            if (random_number(100) + 1 <= 12) {
                puts("A creature comes thru the gate.");
                if (!fight_gate_creature(player)) {
                    return false;
                }
            } else {
                puts("The gate closes.");
            }
            break;
        case 9:
            puts("You find nothing.");
            break;
        }
    }
    return wait_for_key();
}

static bool event_sick_servants(Player* player)
{
    /* Original routine at 0x76FE-0x7A5A; see doc/mechanics.md#servant-illness. */
    if (player->male_servants + player->female_servants > 0) {
        static const char* const diseases[] = {
            "The Plague", "Small Pox", "Chicken Pox", "Turkey Pox", "Leprocy",
            "Old-Timer's disease", "A fatal cold", "Bubonic plague", "Scarlet fever", "The Flu"
        };
        int sick_males = random_number(10) + 1;
        if (sick_males > player->male_servants) {
            sick_males = player->male_servants;
        }
        int sick_females = random_number(10) + 1;
        if (sick_females > player->female_servants) {
            sick_females = player->female_servants;
        }
        printf("\n%d male and %d female servants have come down with: %s\n",
               sick_males, sick_females, diseases[random_number(10)]);
        printf("How many males to cure? ");
        int cured;
        if (!read_integer(&cured)) {
            return false;
        }
        if (cured > player->medicine) {
            cured = player->medicine;
        }
        player->medicine -= cured;
        player->male_servants -= sick_males - cured;
        printf("How many females to cure? ");
        if (!read_integer(&cured)) {
            return false;
        }
        if (cured > player->medicine) {
            cured = player->medicine;
        }
        player->medicine -= cured;
        player->female_servants -= sick_females - cured;
        puts("Any not cured pass away.");
        if (!wait_for_key()) {
            return false;
        }
    } else {
        puts("There seems to be a bug going around.");
    }
    struct timespec delay = {1, 0};
    nanosleep(&delay, NULL);
    return true;
}

static bool event_statue_of_pan(Player* player)
{
    /* Original routine at 0x9DC8-0xA0CC; see doc/mechanics.md#statue-of-pan. */
    static const char* const questions[20][4] = {
        {"What does the Harkonen frog taste like?", "Spinach", "Orange Sherbert", "Guacamole Dip"},
        {"What size underwear does Zeus wear?", "35 Large", "85 Large", "None"},
        {"What does the \"Du Lake\" stand for in Launcelot Du Lake's Name?", "Shake 'n Bake", "Wake the Rake", "Do Lake: 1 to 5"},
        {"What is the Meaning of Life?", "Hamburgers", "Doughnuts", "None of the Above"},
        {"How much weight can an unladen swallow bear?", "2 oz.", "3 lbs.", "1.2 tons"},
        {"How much weight can an unladen bear swallow?", "2 oz.", "3 lbs.", "1.2 tons"},
        {"Why did the Monkey fall out of the tree?", "He was Dead.", "87 income taxes.", "He saw Hellen Reddy"},
        {"What does CTHULHU mean?", "Octopuss-Head", "Trouble", "The Spotted-Tick Flu"},
        {"Apollo's Sun Chariot really runs on?", "Un-leaded", "rubber-bands", "pedal-power"},
        {"What are Aphrodite's measurements?", "35\" 23\" 34\"", "37\" 20\" 30\"", "48\" 16\" 24\""},
        {"One of the direct descendants of Athena is ?", "Joan Collins", "Ethel Murmin", "Miss Piggy"},
        {"What will cause a Cyclops to run in terror?", "Hellen Reddy", "Nutrasweet", "Bi-focals"},
        {"Why does HADES let no one leave the Underworld?", "It's a One-way Trip", "He needs friends.", "Why not?"},
        {"Why is HERMES so fast?", "Caffeine", "Addidas", "He uses U.P.S."},
        {"Why does Pan play the flute?", "His Sax is broke", "His Tuba is broke", "cause it's there."},
        {"What does Poseidon fear about the land?", "Yuppies", "McDonalds", "Hellen Reddy"},
        {"What one item will send Lolth back to her Plane?", "Underoos", "Raid", "Taxes"},
        {"What was Thors hammer constructed from ?", "Used Auto Parts", "Metal Alloy", "Chicken Parts"},
        {"Who invented the first Hamburger?", "Cleavon Little", "Ernest Borgnine", "Howdy Duty"},
        {"What is the favorite food of Eganiod moose birds?", "Duncin' Doughnuts", "Snickers", "Granola Dipps"}
    };
    puts("\nYou find a Statue of Pan.  Approach the statue?");
    printf("(Y/N) ");
    int choice = read_choice("YN");
    if (choice == EOF) {
        return false;
    }
    if (choice == 'N') {
        puts("You move on.");
        return true;
    }
    const char* const* question = questions[random_number(20)];
    puts("The statue suddenly animates, & asks you the following riddle.");
    printf("\n%s\n\n", question[0]);
    for (int i = 1; i <= 3; ++i) {
        printf("%d - %s\n", i, question[i]);
    }
    puts("Choose (1,2,3): ");
    choice = read_choice("123");
    if (choice == EOF) {
        return false;
    }
    int answer = random_number(3) + 1;
    if (choice - '0' == answer) {
        puts("You guessed right!");
        for (int i = 0; i < 5; ++i) {
            player->prime_command[i] = (char)('A' + random_number(26));
        }
        player->prime_command[5] = '\0';
        printf("The prime command is: %s\n", player->prime_command);
        puts("Remember it!!!");
    } else {
        printf("Nope.  The answer is # %d The statue Disappears.\n", answer);
    }
    return wait_for_key();
}

static bool event_storm(Player* player)
{
    /* Original routine at 0x7A5B-0x7E14; see doc/mechanics.md#storms. */
    static const char* const storms[] = {
        "A rain storm comes.", "A Hail storm comes.", "A Snow storm comes.",
        "A Wind storm comes.", "A Tornado comes.", "A Hurricane comes.",
        "A Monsoon comes.", "A Tidal Wave comes."
    };
    puts("\nWhile you are walking along, the weather suddenly changes.");
    int storm = random_number(8) + 1;
    puts(storms[storm - 1]);
    puts("Will you Take shelter? (alternative is move on.)");
    printf("(Y/N) ");
    int choice = read_choice("YN");
    if (choice == EOF) {
        return false;
    }
    if (choice == 'Y') {
        puts("You and your servants take shelter.");
        printf("Waiting for storm to pass.");
        int seconds = storm * (random_number(5) + 1);
        for (int i = 0; i < seconds; ++i) {
            fflush(stdout);
            struct timespec delay = {1, 0};
            nanosleep(&delay, NULL);
            putchar('.');
        }
        puts("The storm is gone.");
    } else if (storm >= 2 && storm <= 7) {
        puts("You may have lost some servants in the storm.");
        player->male_servants -= random_number(5) + 1;
        player->female_servants -= random_number(5) + 1;
        if (player->male_servants < 0) {
            player->male_servants = 0;
        }
        if (player->female_servants < 0) {
            player->female_servants = 0;
        }
    } else if (storm == 8) {
        puts("All servants & food are lost.");
        player->male_servants = 0;
        player->female_servants = 0;
        player->food = 0;
    }
    return wait_for_key();
}

static const char* laboratory_hair_color(void)
{
    static const char* const colors[] = {"Black", "Brown", "Blonde", "Red", "Blue", "White", "Orange", "Plaid"};
    return colors[random_number(8)];
}

static bool fight_anthrax(Player* player)
{
    static const char* const attacks[] = {
        "You make a funny face at the Wizard.",
        "You throw a convenient frog at him.",
        "you tear lightly into his skin.",
        "You grab a racoon & hit him with it.",
        "You grab a test tube and hit him with it.",
        "You dive at him but he teleports away.",
        "You dunk his head in some bubbling brew",
        "You take a nick out of his staff.",
        "You grab picture of Hellen Reddy and show it to him.",
    };
    static const double damage[] = {0, 0.3, 1.4, 1.5, 0, 1.1, 0, 5.1, 6.4};
    int level = random_number(10) + 3;
    int hp_roll = random_number(level) + 1;
    double hit_points = hp_roll * (random_number(10) + 1);
    if (player->ring_power > 0) {
        puts("Ligntning jumps from the ring on your finger & vaporizes");
        puts("the wizard.  You have beaten the wizard!!!!");
        player->experience += random_number(4) + 2;
        return true;
    }
    if (player->poison > 0) {
        int choice = ask_yes_no("Do you want to use your poison?");
        if (choice == EOF) {
            return false;
        }
        if (choice == 'Y') {
            hit_points = 0;
            --player->poison;
            puts("You use your poison and kill him.");
        }
    }
    while (player->hit_points > 0 && hit_points > 0) {
        int attack = random_number(9);
        puts(attacks[attack]);
        if (attack == 4) {
            hit_points -= random_number(10) + 3;
        } else if (attack == 6) {
            hit_points -= random_number(5) + 4;
        } else {
            hit_points -= damage[attack];
        }
        if (attack == 5) {
            player->hit_points -= 0.3;
        }
        int counterattack = random_number(9) + 1;
        struct timespec delay = {0, 500000000};
        switch (counterattack) {
        case 1:
            puts("ZAP!  He missed you.");
            break;
        case 2: {
            puts("ZAP!  He hit you with an ugly stick!!!");
            nanosleep(&delay, NULL);
            printf("Saving Throw: ");
            fflush(stdout);
            nanosleep(&delay, NULL);
            int roll = random_number(100) + 1;
            printf("%d\n", roll);
            if (roll > 79 - player->level) {
                puts("You made it!!");
            } else {
                puts("Drag.  You're ugly.");
                player->good_looks = false;
            }
            break;
        }
        case 3:
            puts("ZAP!  He fries one of your servants!");
            if (player->male_servants > 0) {
                --player->male_servants;
            } else if (player->female_servants > 0) {
                --player->female_servants;
            }
            break;
        case 4: {
            puts("ZAP!  He tries to drop you a level!!");
            int roll = random_number(100) + 1;
            printf("Saving throw is: %d\n", roll);
            if (roll > 40) {
                puts("Made it!");
            } else {
                puts("You lost the level.");
                --player->level;
            }
            break;
        }
        case 5:
            player->hair_color = laboratory_hair_color();
            printf("ZAP!  He turns your hair %s !!!!!\n", player->hair_color);
            break;
        case 6: {
            puts("ZAP!    He casts the Plague on you!");
            printf("Saving Throw: ");
            fflush(stdout);
            nanosleep(&delay, NULL);
            int roll = random_number(100) + 1;
            printf("%d\n", roll);
            if (roll > 80 - player->level) {
                puts("You made It!!!");
            } else {
                puts("Drag.  You have the plague!");
                player->plague = (uint8_t)(player->plague + 5);
            }
            break;
        }
        case 7:
            puts("ZAP!  He smacks you with his staff!");
            player->hit_points -= random_number(3) + 1;
            break;
        case 8:
            puts("ZAP!  He shoots a shot at you but you duck and it hits the mirror behind you!!");
            hit_points -= random_number(2) + 2;
            break;
        case 9:
            puts("ZAP!  He hits you with a lightning bolt!");
            player->hit_points -= random_number(5) + 1;
            break;
        }
        printf("\n---Player: %5.2f          Wizard: %5.2f\n", player->hit_points, hit_points);
        if (!wait_for_key()) {
            return false;
        }
    }
    puts(player->hit_points > 0 ? "You won!!" : "You did not survive the battle.");
    puts(hit_points > 0 ? "The wizard won!" : "The wizard disappeard from within his cloak");
    player->experience += level / 2;
    return true;
}

static void drink_laboratory_potion(Player* player)
{
    switch (random_number(11) + 1) {
    case 1:
        puts("You Drop a level.");
        --player->level;
        break;
    case 2:
        puts("You are raised 2 levels!");
        player->level += 2;
        break;
    case 3:
        player->hair_color = laboratory_hair_color();
        printf("Your hair turns %s !!!!!\n", player->hair_color);
        break;
    case 4:
        puts("You burp a purple cloud, that turns all your servants into gremlins that flee.");
        player->male_servants = 0;
        player->female_servants = 0;
        break;
    case 5:
        puts("That bottle was marked ZOMBIE URINE SAMPLE.");
        break;
    case 6:
        puts("10 Hit points are restored.");
        player->hit_points += 10;
        break;
    case 7:
        puts("This potion negates all servant ills until you die!");
        player->illness_protection = true;
        break;
    case 8:
        puts("Charm a Dragon Next meeting!");
        ++player->dragon_charm;
        break;
    case 9:
        puts("You are cured of the Plague!");
        player->plague = 0;
        break;
    case 10:
        puts("This was the Charisma Elixer. Cancels ugliness.");
        player->good_looks = true;
        break;
    case 11:
        puts("it was water.");
        break;
    }
}

static bool event_thicket(Player* player)
{
    int choice = ask_yes_no("\nYou come up to a thicket in the woods.  Enter?");
    if (choice == EOF) {
        return false;
    }
    if (choice == 'N') {
        puts("You proceed on.");
        return true;
    }
    puts("You enter, and find that it is a musky, dark home for ");
    puts("someone.  It seems to be a laboratory.");
    int roast = random_number(8) + 1;
    printf("You see a %dlb roast cooking over a fire.\n", roast);
    struct timespec delay = {0, 500000000};
    nanosleep(&delay, NULL);
    if (random_number(100) + 1 < 51) {
        puts("Suddenly, A Wizard appears in the room!!!");
        puts("It is Anthrax, the evil wizard!!  He doesn't like");
        puts("visitors!");
        if (!fight_anthrax(player)) {
            return false;
        }
    }
    if (player->hit_points <= 0) {
        return wait_for_key();
    }
    player->food += roast * 10;
    puts("You snag up on the roast & some wine.");
    int males = random_number(60) + 1;
    int females = random_number(60) + 1;
    int gold = random_number(250) + 1;
    puts("in compartments in the back, you discover:");
    printf("     %d Male Servants\n     %d Female Servants\n     %d Gold Pieces\n\n", males, females, gold);
    player->male_servants += males;
    player->female_servants += females;
    player->gold += gold;
    int staff_roll = random_number(100) + 1;
    if (staff_roll < 5 && !player->staff_of_aviatar) {
        puts("You trip over and Find THE STAFF OF AVIATAR!!!");
        player->staff_of_aviatar = true;
    }
    if (random_number(100) + 1 < 26) {
        int potions = random_number(5) + 1;
        char question[80];
        snprintf(question, sizeof(question), "You find %d potions.  Try them?", potions);
        choice = ask_yes_no(question);
        if (choice == EOF) {
            return false;
        }
        if (choice == 'Y') {
            struct timespec quarter_second = {0, 250000000};
            for (int i = 1; i <= potions; ++i) {
                printf("Drinking Potion #%d", i);
                for (int dot = 0; dot < 4; ++dot) {
                    putchar('.');
                    fflush(stdout);
                    nanosleep(&quarter_second, NULL);
                }
                drink_laboratory_potion(player);
            }
        }
    }
    if (random_number(100) + 1 < 11) {
        choice = ask_yes_no("As you leave, you discover a pentacle on the floor.  Step in it?");
        if (choice == EOF) {
            return false;
        }
        if (choice == 'Y') {
            switch (random_number(5) + 1) {
            case 1:
                puts("You suddenly see a golden ring materialize on your hand!");
                player->ring_power = 5;
                break;
            case 2:
                puts("A freezing wind overtakes you & knocks you out of the pentacle.");
                player->hit_points -= 15;
                break;
            case 3:
                puts("From a cloudy haze, A giant clawed arm reaches & throws a pie in your face.");
                break;
            case 4:
                puts("Suddenly, a figure forms in the pentacle.");
                if (!fight_gate_creature(player)) {
                    return false;
                }
                break;
            case 5:
                puts("A bright light forms in the pentacle and it raises you to 20th level and gives");
                puts("you the Staff of Aviatar.");
                if (player->level < 20) {
                    player->level = 20;
                }
                player->staff_of_aviatar = true;
                break;
            }
        }
    }
    return wait_for_key();
}

static bool event_gypsy_camp(Player* player)
{
    static const char* advice[] = {
        "Beasts from Pentacles guard their pendants!",
        "Beware pick-pockets in the village!",
        "Someone may poison your drink at the tavern!",
        "Better Bedding means Better healing!",
        "Rats plague the man with no male servants!",
        "Many bargains are found at the City Market",
        "Beware the Ring of Doom!!  It will seduce you!",
        "Drow elves will destroy your bows & arrows!",
        "If you run from battle, own a horse and cart!",
        "The Wizard can give you the plague!",
        "The Staff of Aviatar may be with the Wizard.",
        "Beware of pentacles!",
        "Feed a puppy and you may be rewarded.",
        "The hermit may betray you!",
        "The Guru can remove the Ring of Doom.",
        "Bad weather kills servants.",
        "Drop no gold from the Mad King's chest.",
        "A nymph may win your heart in the forest.",
        "Don't use the Staff without the Command.",
        "Don't piss on a spark plug!",
        "Beware, the Divine beings watch you.",
        "The Talisman may be in the river.",
        "Pan will tell you the Prime Command.",
        "Don't stay at a brawl if your low on hit points",
        "Cure ugliness before you become King!",
        "Don't accumulate too many servants.",
        "The Ring of Doom will aid you in battle.",
        "The Wizard hordes servants and gold.",
        "Never moon a Tarkanian Butt-Biter.",
        "Watch out for sheep that carry Profalactics.",
        "Listen for the Mad King sneaking up on you.",
        "Enter no dungeon without a dagger.",
        "Seek the Crystal ball in the Galley.",
        "The Mad King is crazy, but very strong.",
        "You may find poison in the Mad Kings dungeon.",
    };
    int choice = ask_yes_no("\nYou come upon a Wandering Gypsy Camp.  Enter?");
    if (choice == EOF) {
        return false;
    }
    if (choice == 'N') {
        puts("You move on.");
        return true;
    }
    puts("One thin-looking man offers you a game of Dagger's Hand.");
    puts("(this is where you put your hand down on a board, and you");
    puts("Stab a dagger between each finger.  If it hits your hand,");
    puts("you lose.  Each round, you put in a gold piece.)");
    choice = ask_yes_no("Play the game?");
    if (choice == EOF) {
        return false;
    }
    if (choice == 'Y' && player->gold > 0) {
        int pot = 0;
        int player_roll = 100;
        int opponent_roll = 100;
        struct timespec delay = {0, 500000000};
        while (player->gold > 0 && player_roll > 15 && opponent_roll > 15) {
            puts("You Ante");
            --player->gold;
            pot += 2;
            nanosleep(&delay, NULL);
            printf("You go...");
            player_roll = random_number(100) + 1;
            if (player_roll < 16) {
                puts("Ouch!   You hit a finger.");
                player->hit_points -= 0.4;
                puts("You lose.");
            } else {
                puts("You made it.");
            }
            nanosleep(&delay, NULL);
            printf("He goes...");
            opponent_roll = random_number(100) + 1;
            if (opponent_roll < 16) {
                puts("Ouch!  He hit a finger.");
                player->gold += pot;
                puts("You win!");
            } else {
                puts("He made it.");
            }
            nanosleep(&delay, NULL);
        }
        if (player->gold == 0) {
            puts("You ran out of money.");
        }
        if (!wait_for_key()) {
            return false;
        }
    } else if (player->gold <= 0) {
        puts("You have insufficient funds.");
    }
    puts("\nOn your way out, you discover a fortune teller.");
    puts("the old lady says, \"10 gold buys good advice.\"");
    choice = ask_yes_no("will you buy some advice?");
    if (choice == EOF) {
        return false;
    }
    if (choice == 'Y' && player->gold >= 10) {
        puts(advice[random_number(35)]);
        player->gold -= 10;
        return wait_for_key();
    }
    return true;
}

static bool event_mountain_trail(Player* player)
{
    int choice = ask_yes_no("\nYou come upon a mountain trail.  Climb it?");
    if (choice == EOF) {
        return false;
    }
    if (choice == 'N') {
        puts("You move on.");
    } else {
        puts("At the top, there is an old guru.  He looks up from his");
        choice = ask_yes_no("meditation and says, \"Do you seek the powers of good?\"");
        if (choice == EOF) {
            return false;
        }
        if (choice == 'Y') {
            if (player->staff_of_aviatar) {
                puts("Your staff disappears.");
                player->staff_of_aviatar = false;
            }
            puts("The ring disappears from around your finger.");
            player->ring_power = 0;
        }
    }
    return wait_for_key();
}

static void event_empty_tree(void)
{
    puts("\nYou find a hollow tree with a door.  No one seems to be home.");
}

static bool run_event(Player* player)
{
    /* Original dispatch at 0xEF97-0xF10F; see doc/mechanics.md#main-turn-loop. */
    switch (random_number(16) + 1) {
    case 1: return event_town(player, false);
    case 2: return event_town(player, true);
    case 3: return event_gypsy_camp(player);
    case 4: return event_combat(player);
    case 5: return event_thicket(player);
    case 6: return event_hollow_tree(player);
    case 7: return event_grass_hut(player);
    case 8:
        if (player->ring_power > 0) {
            return event_mountain_trail(player);
        } else {
            return event_combat(player);
        }
        break;
    case 9:
        if (player->illness_protection) {
            return event_combat(player);
        }
        return event_sick_servants(player);
    case 10: return event_storm(player);
    case 11: return event_dark_castle(player);
    case 12: return event_enchanted_forest(player);
    case 13: event_empty_tree(); break;
    case 14: return event_flowing_river(player);
    case 15: return event_statue_of_pan(player);
    case 16: return event_overlords_keep(player);
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
