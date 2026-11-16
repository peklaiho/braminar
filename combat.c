#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "braminar.h"

extern int random_number(int limit);
extern int read_choice(const char* choices);
extern bool wait_for_key(void);
extern int ask_yes_no(const char* question);

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

bool fight_monster(Player* player, Monster monster)
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

bool event_combat(Player* player)
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

bool fight_mad_king(Player* player)
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

bool fight_dragon(Player* player)
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

bool fight_gate_creature(Player* player)
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

bool fight_anthrax(Player* player)
{
    static const char* const colors[] = {"Black", "Brown", "Blonde", "Red", "Blue", "White", "Orange", "Plaid"};
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
            player->hair_color = colors[random_number(8)];
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
