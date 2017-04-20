//===== Hercules ;'zx	 Plugin ======================================
//= bonus bAddWeightDamage
//===== By: ==================================================
//= Jeroen
//===== Current Version: =====================================
//= 1.0
//===== Description: =========================================
//= bAddWeightDamage increases damage to Shield chain and Shield Boomerang as if the shield has more weight.
//= bAddPressureDamage increases damage to Pressure
//===== Additional Comments: =================================
//= Format:
//= bonus bAddWeightDamage, x;	// x = Bonus weight
//= e.g:
//=   bonus bAddWeightDamage, 2000; // Increases weight by 200 (Virtually) 
//===== Repo Link: ===========================================
//= https://github.com/garet999

#include "common/hercules.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "common/HPMi.h"
#include "common/mmo.h"
#include "common/socket.h"
#include "common/memmgr.h"
#include "common/nullpo.h"
#include "common/mapindex.h"
#include "common/strlib.h"
#include "common/utils.h"
#include "common/cbasetypes.h"
#include "common/timer.h"
#include "common/showmsg.h"
#include "common/conf.h"
#include "common/db.h"

#include "map/battle.h"
#include "map/map.h"
#include "map/clif.h"
#include "map/pc.h"
#include "map/script.h"
#include "map/skill.h"
#include "map/status.h"
#include "map/chrif.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"



HPExport struct hplugin_info pinfo = {
	"Paladin Bonuses",
	SERVER_TYPE_MAP,
	"0.7",
	HPM_VERSION,
};

/**
* Bonus ID.
* don't mess with this.
*/
int bAddWeightDamage = -1;
int bAddPressureDamage = -1;
int bGospelSelf = -1;
int bGospelNoDispell = -1;
int bHolyCrossElementChange = -1;

struct paladin_bonus_data {
	int bAddWeightDamage;
	int bAddPressureDamage;
	int bGospelSelf;
	int bGospelNoDispell;
	int bHolyCrossElementChange; 
};

struct paladin_bonus_data* bonus_search(struct map_session_data* sd) {
	struct paladin_bonus_data *data = NULL;
	if ((data = getFromMSD(sd, 0)) == NULL) {
		CREATE(data, struct paladin_bonus_data, 1);
		data->bAddWeightDamage = 0; // default
		data->bAddPressureDamage = 0; // default
		data->bGospelSelf = 0; // default
		data->bGospelNoDispell = 0; // default
		data->bHolyCrossElementChange = 0; // default
		addToMSD(sd, data, 0, true);
	}
	return data;
}

struct Damage battle_calc_weapon_attack_post(struct Damage wd, struct block_list *src, struct block_list *target, uint16 skill_id, uint16 skill_lv, int wflag) {
	struct map_session_data *sd;
	struct paladin_bonus_data *data;
	struct status_change *sc = status->get_sc(src);
	if (skill_id != PA_SHIELDCHAIN && skill_id != CR_SHIELDBOOMERANG) { // If it isn't about these skills or if the character doesn't have the bonus, drop it!, drop it!
		return wd;
	}
	sd = BL_CAST(BL_PC, src);
	data = bonus_search(sd);
	if (sd && data->bAddWeightDamage != 0) { //Lets do this!
		wd.damage += ((data->bAddWeightDamage / 10) * ((sc && sc->data[SC_SOULLINK] && sc->data[SC_SOULLINK]->val2 == SL_CRUSADER && skill_id != PA_SHIELDCHAIN)? 2 : 1));
	}

	return wd;

}struct Damage battle_calc_misc_attack_post(struct Damage md, struct block_list *src, struct block_list *target, uint16 skill_id, uint16 skill_lv, int mflag) {
	struct map_session_data *sd;
	struct paladin_bonus_data *data;
	struct status_change *sc = status->get_sc(src);
	if (skill_id != PA_PRESSURE) {
		return md;
	}

	sd = BL_CAST(BL_PC, src);
	data = bonus_search(sd);

	if (sd && data->bAddPressureDamage != 0) { //Lets do this!
		md.damage += data->bAddPressureDamage;
	}

	return md;
}

int pc_bonus_pre(struct map_session_data **sd, int *type, int *val)
{
	struct paladin_bonus_data *data = bonus_search(*sd);
	if (*type == bAddWeightDamage) {
		data->bAddWeightDamage += *val;
		hookStop();
	}
	else if (*type == bAddPressureDamage){
		data->bAddPressureDamage += *val;
		hookStop();
	}	
	else if (*type == bGospelSelf){
		if (*val != 1) {
			ShowError("pc_bonus2_pre: The flag set for bGospelSelf is not correct, please set it to 1.\n");
			hookStop();
			return 0;
		}
		data->bGospelSelf = *val;
		hookStop();
	}
	else if (*type == bGospelNoDispell){
		if (*val != 1) {
			ShowError("pc_bonus2_pre: The flag set for bGospelNoDispell is not correct, please set it to 1.\n");
			hookStop();
			return 0;
		}
		data->bGospelNoDispell = *val;
		hookStop();
	}
	return 0;
}

int skill_castend_pos2_pre(struct block_list* *src, int *x, int *y, uint16 *skill_id, uint16 *skill_lv, int64 *tick, int *flag) {
	struct map_session_data* sd;
	struct status_change* sc;
	struct status_change_entry *sce;
	struct skill_unit_group* sg;
	enum sc_type type;
	nullpo_ret(src);

	sd = BL_CAST(BL_PC, *src);
	sc = status->get_sc(*src);
	type = status->skill2sc(*skill_id);
	sce = (sc != NULL && type != SC_NONE) ? sc->data[type] : NULL;

	if (*skill_id == PA_GOSPEL) {
		struct paladin_bonus_data *data = bonus_search(sd);
		if (sce && sce->val4 == BCT_SELF) {
			status_change_end(*src, SC_GOSPEL, INVALID_TIMER);
			return 0;
		}
		else {
			sg = skill->unitsetting(*src, *skill_id, *skill_lv, sd->bl.x, sd->bl.y, 0);
			if (sg) {
				if (sce)
					status_change_end(*src, type, INVALID_TIMER); //Was under someone else's Gospel. [Skotlex]
//				if(!data->bGospelNoDispell)
					status->change_clear_buffs(*src, 3);
				sc_start4(*src, *src, type, 100, *skill_lv, 0, sg->group_id, BCT_SELF, skill->get_time(*skill_id, *skill_lv));
				clif->skill_poseffect(*src, *skill_id, *skill_lv, 0, 0, *tick); // PA_GOSPEL music packet
			}
		}
		hookStop();
	}
	return 0;
}
int status_change_start_pre(struct block_list **src, struct block_list **bl, enum sc_type *type, int *rate, int *val1, int *val2, int *val3, int *val4, int *tick, int *flag) {

	if (*type == SC_GOSPEL) 
	{
		struct map_session_data *sd = NULL;
		struct status_change* sc;
		struct status_change_entry* sce;
		struct status_data *st;
		struct view_data *vd;
		int calc_flag, val_flag = 0, tick_time = 0;

		nullpo_ret(*bl);
		sc = status->get_sc(*bl);
		st = status->get_status_data(*bl);
		if (!sc)
			return 0; //Unable to receive status changes

		if (status->isdead(*bl) && *type != SC_NOCHAT) // SC_NOCHAT should work even on dead characters
			return 0;

		sd = BL_CAST(BL_PC, *bl);

		//Must not override a casting gospel char.
		if ((sce = sc->data[*type])) {
			if (sce->val4 == BCT_SELF)
				return 0;
			if (sce->val1 > *val1)
				return 1;
		}
		vd = status->get_viewdata(*bl);
		calc_flag = status->dbs->ChangeFlagTable[*type];
		if (!(*flag&SCFLAG_LOADED)) { // Do not parse val settings when loading SCs
			if (*val4 == BCT_SELF)
			{
				struct paladin_bonus_data *data = bonus_search(sd);
				// self effect
				*val2 = *tick / 10000;
				tick_time = 10000;
				//if (!data->bGospelNoDispell)
				status->change_clear_buffs(*bl, 3); //Remove buffs/debuffs
				hookStop();
				return 1;
			}
		}

		if (!(*flag&SCFLAG_NOICON) && !(*flag&SCFLAG_LOADED && status->dbs->DisplayType[*type]))
			clif->status_change(*bl, status->dbs->IconChangeTable[*type], 1, *tick, (val_flag & 1) ? *val1 : 1, (val_flag & 2) ? *val2 : 0, (val_flag & 4) ? *val3 : 0);

		/**
		* used as temporary storage for scs with interval ticks, so that the actual duration is sent to the client first.
		**/
		if (tick_time)
			*tick = tick_time;

		//Don't trust the previous sce assignment, in case the SC ended somewhere between there and here.
		if ((sce = sc->data[*type])) {// reuse old sc
			if (sce->timer != INVALID_TIMER)
				timer->delete(sce->timer, status->change_timer);
		}
		else {// new sc
			++(sc->count);
			sce = sc->data[*type] = ers_alloc(status->data_ers, struct status_change_entry);
		}

		sce->val1 = *val1;
		sce->val2 = *val2;
		sce->val3 = *val3;
		sce->val4 = *val4;

		if (tick >= 0) {
			sce->timer = timer->add(timer->gettick() + *tick, status->change_timer, sd->bl.id, *type);
			sce->infinite_duration = false;
		}
		else {
			sce->timer = INVALID_TIMER; //Infinite duration
			sce->infinite_duration = true;
			if (sd)
				chrif->save_scdata_single(sd->status.account_id, sd->status.char_id, *type, sce);
		}

		if (calc_flag)
			status_calc_bl(*bl, calc_flag);

	}
	return 0;
}

/*
*/
// Reset on Recalc
int status_calc_pc_pre(struct map_session_data **sd, enum e_status_calc_opt *opt)
{
	struct paladin_bonus_data *data = NULL;
	if ((data = getFromMSD(*sd, 0)) != NULL) {
		data->bAddWeightDamage = 0; //Default
		data->bAddPressureDamage = 0; //Default
		data->bGospelSelf = 0; //Default
		data->bGospelNoDispell = 0; //Default
		data->bHolyCrossElementChange = 0; //Default
	}
	return 1;
}

HPExport void plugin_init(void) {
	bAddWeightDamage = map->get_new_bonus_id();
	bAddPressureDamage = map->get_new_bonus_id();
	bGospelSelf = map->get_new_bonus_id();
	bGospelNoDispell = map->get_new_bonus_id();
	bHolyCrossElementChange = map->get_new_bonus_id();
	script->set_constant("bAddWeightDamage", bAddWeightDamage, false, false);
	script->set_constant("bAddPressureDamage", bAddPressureDamage, false, false);
	script->set_constant("bGospelSelf", bGospelSelf, false, false);
	script->set_constant("bGospelNoDispell", bGospelSelf, false, false);
	script->set_constant("bHolyCrossElementChange", bHolyCrossElementChange, false, false);
	
	addHookPre(pc, bonus, pc_bonus_pre);
	addHookPre(status, calc_pc_, status_calc_pc_pre);
	/* Todo: these don't work for now -> beg*/
	
	//addHookPre(status, change_start, status_change_start_pre);
	//addHookPre(skill, castend_pos2, skill_castend_pos2_pre);
	
	/* TODO: Calc weapon damage post for renewal! */
	//addHookPost(battle, calc_weapon_damage, battle_calc_weapon_damage_post); // Shield weight Renewal TODO
	addHookPost(battle, calc_weapon_attack, battle_calc_weapon_attack_post); // For The shield weight Pre-renewal TODO: add guard
	addHookPost(battle, calc_misc_attack, battle_calc_misc_attack_post); // For pressure
}

HPExport void server_online(void) {
	ShowInfo("'%s' Plugin by Jeroen Version '%s'\n", pinfo.name, pinfo.version);
}