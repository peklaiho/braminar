#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "braminar.h"

extern int random_number(int limit);
extern int read_key(void);
extern bool read_line(char* buffer, size_t size);
extern int read_choice(const char* choices);
extern bool read_integer(int* value);
extern bool wait_for_key(void);
extern int ask_yes_no(const char* question);

extern bool fight_monster(Player* player, Monster monster);
extern bool event_combat(Player* player);
extern bool fight_mad_king(Player* player);
extern bool fight_dragon(Player* player);
extern bool fight_gate_creature(Player* player);
extern bool fight_anthrax(Player* player);

extern bool event_town(Player* player, bool city);

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
        int choice = ask_yes_no("Use the Staff of Aviatar to attack?");
        if (choice == EOF) {
            return false;
        }
        if (choice == 'Y') {
            char command[6];
            puts("Please type the Prime Command (in caps): ");
            if (!read_line(command, sizeof(command))) {
                return false;
            }
            if (strcmp(command, player->prime_command) == 0) {
                puts("Suddenly, the staff glows with great power!");
                player->final_assault_ready = true;
            } else {
                puts("The staff doesn't like rookie users.  ");
                puts("It disappears from your hands.");
                player->staff_of_aviatar = false;
            }
        }
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

static void drink_laboratory_potion(Player* player)
{
    static const char* const colors[] = {"Black", "Brown", "Blonde", "Red", "Blue", "White", "Orange", "Plaid"};

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
        player->hair_color = colors[random_number(8)];
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

static bool event_empty_tree(Player* player)
{
    /* Slot 13 at 0x8F9B-0x96E6; see doc/mechanics.md#watched-tree-and-hut. */
    int deed = ' ';
    if (random_number(100) + 1 < 51) {
        int choice = ask_yes_no("\nYou come up to a hollow tree with a door.  Enter? ");
        if (choice == EOF) {
            return false;
        }
        if (choice == 'Y') {
            puts("No one seems to be home.");
            puts("There is a kettle of stew over a fire, and a large");
            puts("wooden trunk next to the wall.");
            puts("\nyou may (S)teal food, (T)ake from trunk or (L)eave.");
            deed = read_choice("STL");
            if (deed == EOF) {
                return false;
            }
        }
    } else {
        puts("\nYou come upon a grass hut.  You hear sounds from within.");
        int choice = ask_yes_no("Will you enter?");
        if (choice == EOF) {
            return false;
        }
        if (choice == 'Y') {
            bool old_man = random_number(2) == 0;
            choice = ask_yes_no(old_man
                ? "You discover an old man in the corner.  He looks ill.  Try to cure him?"
                : "You find a hungry little puppy in the room.  Share some of your food?");
            if (choice == EOF) {
                return false;
            }
            deed = choice == 'Y' ? 'L' : 'S';
            if (choice == 'Y') {
                if (old_man) {
                    if (player->medicine > 0) {
                        puts("you give him some medicine.");
                        --player->medicine;
                    } else {
                        puts("You try to comfort him.");
                    }
                    puts("The old man seems better.");
                } else if (player->food > 0) {
                    --player->food;
                    puts("You feed the puppy.  It is greatful.");
                } else {
                    puts("You have no food, so you pet thepuppy.  ");
                }
                struct timespec delay = {1, 0};
                nanosleep(&delay, NULL);
            }
        }
    }
    if (deed == 'S' || deed == 'T') {
        puts("You suddenly hear a big booming voice say,");
        puts("\"Alright!  I saw that!!\"  You realize that");
        puts("A deity was watching your act.");
        puts("\"For that, I drop you down a level!!\"");
        --player->level;
    } else if (deed == 'L') {
        puts("A bright light fills the room & you are confronted by a");
        puts("deity.  He says, \"Good job.  For doing a good deed,I give you: ");
        switch (random_number(5) + 1) {
        case 1:
            puts("A dozen Male Servants.");
            player->male_servants += 12;
            break;
        case 2:
            puts("A dozen Female Servants.");
            player->female_servants += 12;
            break;
        case 3:
            puts("A giant Hamburger!");
            player->food += 50;
            break;
        case 4:
            puts("A bottle of poison.");
            ++player->poison;
            break;
        case 5:
            if (!player->talisman_of_braminar) {
                puts("The Talisman of Braminar!");
                player->talisman_of_braminar = true;
            } else {
                puts("a box of Duncin' Doughnuts!");
                player->food += 12;
            }
            break;
        }
    }
    return wait_for_key();
}

bool run_event(Player* player)
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
    case 13: return event_empty_tree(player);
    case 14: return event_flowing_river(player);
    case 15: return event_statue_of_pan(player);
    case 16: return event_overlords_keep(player);
    }
    /* TODO: Recover these branches and their pauses; see doc/events.md. */
    puts("This encounter is not implemented yet.");
    return wait_for_key();
}
