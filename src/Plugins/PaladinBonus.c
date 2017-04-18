//===== Hercules ;'zx	 Plugin ======================================
//= bonus bAddWeightDamage
//===== By: ==================================================
//= Jeroen
//===== Current Version: =====================================
//= 0.5 (Beta Testing, for now Non renewal only)
//===== Description: =========================================
//= bAddWeightDamage increases damage to Shield chain and Shield Boomerang as if the shield has more weight.
//===== Additional Comments: =================================
//= Format:
//= bonus bAddWeightDamage, x;	// x = Bonus weight
//= e.g:
//=   bonus bAddWeightDamage, 2000; // Increases weight by 200 (Virtually) 
//===== Repo Link: ===========================================
//= https://github.com/garet999/HerculesPlugins
//============================================================

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
#include "common/sql.h"

#include "map/battle.h"
#include "map/map.h"
#include "map/clif.h"
#include "map/pc.h"
#include "map/script.h"
#include "map/status.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"



HPExport struct hplugin_info pinfo = {
	"Paladin Bonuses",
	SERVER_TYPE_MAP,
	"0.5",
	HPM_VERSION,
};

/**
* Bonus ID.
* don't mess with this.
*/
int bAddWeightDamage = -1;

struct paladin_bonus_data {
	int bAddWeightDamage;
};

struct paladin_bonus_data* bonus_search(struct map_session_data* sd) {
	struct paladin_bonus_data *data = NULL;
	if ((data = getFromMSD(sd, 0)) == NULL) {
		CREATE(data, struct paladin_bonus_data, 1);
		data->bAddWeightDamage = 0; // default
		addToMSD(sd, data, 0, true);
	}
	return data;
}

struct Damage battle_calc_weapon_attack_post(struct Damage wd, struct block_list *src, struct block_list *target, uint16 skill_id, uint16 skill_lv, int wflag) {
	struct map_session_data *sd;
	struct paladin_bonus_data *data;
	struct status_change *sc = status->get_sc(src);
	sd = BL_CAST(BL_PC, src);
	data = bonus_search(sd);
	if (skill_id != PA_SHIELDCHAIN && skill_id != CR_SHIELDBOOMERANG) { // If it isn't about these skills or if the character doesn't have the bonus, drop it!, drop it!
		return wd;
	}

	if (sd && data->bAddWeightDamage != 0) { //Lets do this!
		wd.damage += ((data->bAddWeightDamage / 10) * ((sc && sc->data[SC_SOULLINK] && sc->data[SC_SOULLINK]->val2 == SL_CRUSADER && skill_id != PA_SHIELDCHAIN)? 2 : 1));
	}

	return wd;
}

int pc_bonus_pre(struct map_session_data **sd, int *type, int *val)
{
	char entry_name[10];
	if (*type == bAddWeightDamage) {
		struct paladin_bonus_data *data = bonus_search(*sd);
		data->bAddWeightDamage += *val;
		hookStop();
	}

	return 0;
}

// Reset on Recalc
int status_calc_pc_pre(struct map_session_data **sd, enum e_status_calc_opt *opt)
{
	struct paladin_bonus_data *data = NULL;
	if ((data = getFromMSD(*sd, 0)) != NULL) {
		data->bAddWeightDamage = 0; //Default
	}
	return 1;
}

HPExport void plugin_init(void) {
	bAddWeightDamage = map->get_new_bonus_id();
	script->set_constant("bAddWeightDamage", bAddWeightDamage, false, false);

	addHookPre(pc, bonus, pc_bonus_pre);
	addHookPre(status, calc_pc_, status_calc_pc_pre);
//  addHookPost(battle, calc_weapon_damage, battle_calc_weapon_damage_post);
	addHookPost(battle, calc_weapon_attack, battle_calc_weapon_attack_post);
}

HPExport void server_online(void) {
	ShowInfo("'%s' Plugin by Jeroen Version '%s'\n", pinfo.name, pinfo.version);
}