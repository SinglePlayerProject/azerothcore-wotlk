#include "Script_Warrior.h"

Script_Warrior::Script_Warrior(RobotAI* pmSourceAI) :Script_Base(pmSourceAI)
{

}

bool Script_Warrior::HealMe()
{
    return false;
}

bool Script_Warrior::Tank(Unit* pmTarget)
{
    Player* me = sourceAI->sourcePlayer;
    if (!pmTarget)
    {
        return false;
    }
    else if (!me->IsValidAttackTarget(pmTarget))
    {
        return false;
    }
    else if (!pmTarget->IsAlive())
    {
        return false;
    }
    float targetDistance = me->GetDistance(pmTarget);
    if (targetDistance > 200)
    {
        return false;
    }

    sourceAI->BaseMove(pmTarget);
    uint32 rage = me->GetPower(Powers::POWER_RAGE);
    if (rage > 100)
    {
        if (sourceAI->CastSpell(me, "Battle Shout", MELEE_MIN_DISTANCE, true))
        {
            return true;
        }
        if (sourceAI->CastSpell(pmTarget, "Demoralizing Shout", MELEE_MIN_DISTANCE, true))
        {
            return true;
        }
        if (sourceAI->CastSpell(pmTarget, "Rend", MELEE_MIN_DISTANCE, true, true))
        {
            return true;
        }
    }
    if (rage > 200)
    {
        sourceAI->CastSpell(pmTarget, "Heroic Strike", MELEE_MIN_DISTANCE);
    }

    return true;
}

bool Script_Warrior::Healer(Unit* pmTarget)
{
    return false;
}

bool Script_Warrior::DPS(Unit* pmTarget)
{
    return DPS_Common(pmTarget);
}

bool Script_Warrior::DPS_Common(Unit* pmTarget)
{
    Player* me = sourceAI->sourcePlayer;
    if (!pmTarget)
    {
        return false;
    }
    else if (!me->IsValidAttackTarget(pmTarget))
    {
        return false;
    }
    else if (!pmTarget->IsAlive())
    {
        return false;
    }
    float targetDistance = me->GetDistance(pmTarget);
    if (targetDistance > 200)
    {
        return false;
    }

    sourceAI->BaseMove(pmTarget);
    uint32 rage = me->GetPower(Powers::POWER_RAGE);
    if (rage > 100)
    {
        if (sourceAI->CastSpell(me, "Battle Shout", MELEE_MIN_DISTANCE, true))
        {
            return true;
        }
        if (sourceAI->CastSpell(pmTarget, "Demoralizing Shout", MELEE_MIN_DISTANCE, true))
        {
            return true;
        }
        if (sourceAI->CastSpell(pmTarget, "Rend", MELEE_MIN_DISTANCE, true, true))
        {
            return true;
        }
        if (sourceAI->CastSpell(pmTarget, "Hamstring", MELEE_MIN_DISTANCE, true))
        {
            return true;
        }
    }
    if (rage > 200)
    {
        sourceAI->CastSpell(pmTarget, "Heroic Strike", MELEE_MIN_DISTANCE);
    }

    return true;
}

bool Script_Warrior::Attack(Unit* pmTarget)
{
    return Attack_Common(pmTarget);
}

bool Script_Warrior::Attack_Common(Unit* pmTarget)
{
    Player* me = sourceAI->sourcePlayer;
    if (!pmTarget)
    {
        return false;
    }
    else if (!me->IsValidAttackTarget(pmTarget))
    {
        return false;
    }
    else if (!pmTarget->IsAlive())
    {
        return false;
    }
    float targetDistance = me->GetDistance(pmTarget);
    if (targetDistance > 200)
    {
        return false;
    }

    sourceAI->BaseMove(pmTarget);
    uint32 rage = me->GetPower(Powers::POWER_RAGE);
    if (rage > 100)
    {
        if (sourceAI->CastSpell(me, "Battle Shout", MELEE_MIN_DISTANCE, true))
        {
            return true;
        }
        if (sourceAI->CastSpell(pmTarget, "Demoralizing Shout", MELEE_MIN_DISTANCE, true))
        {
            return true;
        }
        if (sourceAI->CastSpell(pmTarget, "Rend", MELEE_MIN_DISTANCE, true, true))
        {
            return true;
        }
        if (sourceAI->CastSpell(pmTarget, "Hamstring", MELEE_MIN_DISTANCE, true))
        {
            return true;
        }
    }
    if (rage > 200)
    {
        sourceAI->CastSpell(pmTarget, "Heroic Strike", MELEE_MIN_DISTANCE);
    }

    return true;
}

bool Script_Warrior::Buff(Unit* pmTarget)
{
    Player* me = sourceAI->sourcePlayer;
    if (sourceAI->CastSpell(me, "Battle Stance", MELEE_MIN_DISTANCE, true))
    {
        return true;
    }

    return false;
}
