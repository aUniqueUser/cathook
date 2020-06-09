#pragma once

#include <entitycache.hpp>

// Tf2 flag types
enum ETFFlagType
{
    TF_FLAGTYPE_CTF = 0,
    TF_FLAGTYPE_ATTACK_DEFEND,
    TF_FLAGTYPE_TERRITORY_CONTROL,
    TF_FLAGTYPE_INVADE,
    TF_FLAGTYPE_RESOURCE_CONTROL,
    TF_FLAGTYPE_ROBOT_DESTRUCTION,
    TF_FLAGTYPE_PLAYER_DESTRUCTION

    //
    // ADD NEW ITEMS HERE TO AVOID BREAKING DEMOS
    //
};

// Flag Drop status
enum ETFFlagStatus
{
    TF_FLAGINFO_HOME = 0,
    TF_FLAGINFO_STOLEN,
    TF_FLAGINFO_DROPPED
};

struct flag_info
{
    CachedEntity *ent{ nullptr };
    std::optional<Vector> spawn_pos;
    int team{ TEAM_UNK };
    flag_info(){};
    flag_info(CachedEntity *ent, Vector spawn_pos, int team)
    {
        this->ent       = ent;
        this->spawn_pos = spawn_pos;
        this->team      = team;
    }
};

namespace flagcontroller
{
// Use incase you don't get the needed information from the functions below
flag_info getFlag(int team);

Vector getPosition(CachedEntity *flag);
std::optional<Vector> getPosition(int team);
CachedEntity *getCarrier(CachedEntity *flag);
CachedEntity *getCarrier(int team);
ETFFlagStatus getStatus(CachedEntity *flag);
ETFFlagStatus getStatus(int team);
} // namespace flagcontroller
