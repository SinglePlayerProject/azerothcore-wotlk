#ifndef ROBOT_STRATEGIES_SCRIPT_BASE_H
#define ROBOT_STRATEGIES_SCRIPT_BASE_H

#ifndef MELEE_MAX_DISTANCE
# define MELEE_MAX_DISTANCE 4
#endif

#ifndef MELEE_MIN_DISTANCE
# define MELEE_MIN_DISTANCE 2
#endif

#include "Player.h"
#include "SpellMgr.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"

class Script_Base
{
public:
	Script_Base(RobotAI* pmSourceAI);
	virtual bool DPS(Unit* pmTarget) = 0;
	virtual bool Tank(Unit* pmTarget) = 0;
	virtual bool Healer(Unit* pmTarget) = 0;
	virtual bool Attack(Unit* pmTarget) = 0;
	virtual bool Buff(Unit* pmTarget) = 0;
	virtual bool HealMe() = 0;

	RobotAI * sourceAI;
};
#endif
