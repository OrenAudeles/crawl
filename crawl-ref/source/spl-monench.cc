/**
 * @file
 * @brief Monster-affecting enchantment spells.
 *           Other targeted enchantments are handled in spl-zap.cc.
**/

#include "AppHdr.h"

#include "spl-monench.h"

#include "chaos.h"
#include "env.h"
#include "message.h"
#include "spl-util.h"
#include "terrain.h"
#include "stringutil.h"

int englaciate(coord_def where, int pow, actor *agent)
{
    actor *victim = actor_at(where);

    if (!victim || victim == agent)
        return 0;

    if (agent->is_monster() && mons_aligned(agent, victim))
        return 0; // don't let monsters hit friendlies

    monster* mons = victim->as_monster();

    if (victim->res_cold() > 0
        || victim->is_stationary())
    {
        if (!mons)
            canned_msg(MSG_YOU_UNAFFECTED);
        else if (!mons_is_firewood(*mons))
            simple_monster_message(*mons, " is unaffected.");
        return 0;
    }

    int duration = (roll_dice(3, pow) / 6
                    - random2(victim->get_hit_dice()))
                    * BASELINE_DELAY;

    if (duration <= 0)
    {
        if (!mons)
            canned_msg(MSG_YOU_RESIST);
        else
            simple_monster_message(*mons, " resists.");
        return 0;
    }

    if ((!mons && you.get_mutation_level(MUT_COLD_BLOODED))
        || (mons && mons_class_flag(mons->type, M_COLD_BLOOD)))
    {
        duration *= 2;
    }

    if (!mons)
        return slow_player(duration);

    return do_slow_monster(*mons, agent, duration);
}

spret cast_englaciation(int pow, bool fail)
{
    fail_check();
    mpr("You radiate an aura of cold.");
    apply_area_visible([pow] (coord_def where) {
        return englaciate(where, pow, &you);
    }, you.pos());
    return spret::success;
}

/** magic candle a monster.
 *
 *  @param mons the monster to get a backlight.
 *  @returns true if it got backlit (even if it was already).
 */
bool backlight_monster(monster* mons)
{
    const mon_enchant bklt = mons->get_ench(ENCH_MAGIC_CANDLE);
    const mon_enchant zin_bklt = mons->get_ench(ENCH_SILVER_CANDLE);
    const int lvl = bklt.degree + zin_bklt.degree;

    mons->add_ench(mon_enchant(ENCH_MAGIC_CANDLE, 1));
    string local_name = mons->name(DESC_THE);

    if (lvl == 0)
        mpr(make_stringf("The magic candle sticks upon %s", local_name.c_str()));
    else if (lvl == 4)
        mpr(make_stringf("The magic candle, stuck upon %s, cannot burn any brighter.", local_name.c_str()));
    else
        mpr("The additional candle sticks to the first, reinforcing the light.");

    return true;
}

bool do_slow_monster(monster& mon, const actor* agent, int dur)
{
    if (mon.stasis())
        return true;

    if (!mon.is_stationary()
        && mon.add_ench(mon_enchant(ENCH_SLOW, 0, agent, dur)))
    {
        if (!mon.paralysed() && !mon.petrified()
            && simple_monster_message(mon, " seems to slow down."))
        {
            return true;
        }
    }

    return false;
}
