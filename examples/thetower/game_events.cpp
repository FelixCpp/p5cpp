#include "game_events.hpp"

void game_events_clear(GameEvents& events)
{
    events.damage.clear();
    events.deaths.clear();
    events.wavesCompleted.clear();
    events.towerHits.clear();
    events.enemyShots.clear();
}
