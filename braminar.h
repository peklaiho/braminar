#ifndef BRAMINAR_H
#define BRAMINAR_H

#include <stdbool.h>
#include <stdint.h>

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
    bool final_assault_ready;
} Player;

typedef struct {
    const char* name;
    int level;
    double hit_points;
} Monster;

typedef struct {
    double net_worth;
    int soldiers;
    int enemy_soldiers;
} FinalArmy;

FinalArmy convert_final_assets(const Player* player);
bool display_stats(const Player* player);
void start_final_battle(const Player* player);

#endif
