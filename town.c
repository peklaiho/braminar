#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <stdio.h>
#include <time.h>

#include "braminar.h"

extern int male_servant_price;
extern int female_servant_price;

extern int random_number(int limit);
extern int read_choice(const char* choices);
extern bool read_integer(int* value);
extern bool wait_for_key(void);
extern int ask_yes_no(const char* question);

bool visit_servant_market(Player* player)
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

bool visit_tavern(Player* player)
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

void show_gold(const Player* player)
{
    printf("You have: %7d\n", player->gold);
}

bool buy_store_item(Player* player, const char* name, int price, bool first, bool arrows, bool* bought)
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

bool buy_supplies(Player* player, const char* description, int price, int* quantity, bool marketplace)
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

bool visit_store(Player* player)
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

bool visit_inn(Player* player)
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

bool visit_marketplace(Player* player)
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

bool meet_cleric(Player* player)
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

bool event_town(Player* player, bool city)
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
