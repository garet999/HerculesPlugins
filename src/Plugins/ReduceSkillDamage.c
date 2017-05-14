//===== Hercules Plugin ======================================
//= bonus bReduceSkillDamage
//===== By: ==================================================
//= Jeroen
//===== Current Version: =====================================
//= 1.0
//===== Description: =========================================
//= bReduceSkillDamage Reduces Skill x's incoming damage by y%
//===== Additional Comments: =================================
//= Format:
//= bonus bReduceSkillDamage x,y;	// x = skill id, y = %
//===== Repo Link: ===========================================
//= https://github.com/garet999/HerculesPlugins
//============================================================
#include "common/hercules.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "common/memmgr.h"
#include "common/mmo.h"
#include "common/socket.h"
#include "common/strlib.h"
#include "common/nullpo.h"

#include "map/battle.h"
#include "map/map.h"
#include "map/pc.h"
#include "map/status.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"


HPExport struct hplugin_info pinfo = {
	"Reduce Skill damage",
	SERVER_TYPE_MAP,
	"1.0",
	HPM_VERSION,
};
#define MAX_BONUS_SKILL_LEVELS 10
/**
* Bonus ID.
* don't mess with this.
*/
int bReduceSkillDamage = -1;

struct reduceSkillDamage {
	uint16 skill_id;
	uint16 reduction;
};
struct s_bonus_data {
	struct reduceSkillDamage bReduceSkillDamage[MAX_BONUS_SKILL_LEVELS];
};

// If data is not found, set to null
struct s_bonus_data* bonus_search(struct map_session_data* sd) {
	struct s_bonus_data *data = NULL;
	if ((data = getFromMSD(sd, 0)) == NULL) {
		CREATE(data, struct s_bonus_data, 1);
		memset(data->bReduceSkillDamage, 0, sizeof(data->bReduceSkillDamage));
		addToMSD(sd, data, 0, true);
	}
	return data;
}

// Reset on Recalc
int status_calc_pc_pre(struct map_session_data **sd, enum e_status_calc_opt *opt)
{
	struct s_bonus_data *data = NULL;
	if ((data = getFromMSD(*sd, 0)) != NULL) {
		memset(data->bReduceSkillDamage, 0, sizeof(data->bReduceSkillDamage));
	}
	return 1;
}

int pc_bonus2_pre(struct map_session_data **sd, int *type, int *type2, int *val)
{
	struct s_bonus_data *data = bonus_search(*sd);
	int i;
	if (*type == bReduceSkillDamage) {
		// Find free Slot
		for (i = 0; i < MAX_BONUS_SKILL_LEVELS; i++) {
			if (data->bReduceSkillDamage[i].skill_id == 0 || data->bReduceSkillDamage[i].reduction == 0) {
				data->bReduceSkillDamage[i].skill_id = *type2;
				data->bReduceSkillDamage[i].reduction = *val;
				break;
			}
		}
		if (i == MAX_BONUS_SKILL_LEVELS) {
			ShowError("pc_bonus2_pre: bBonusSkillLevel Reached %d Slots. No more Free slots for Player.", MAX_BONUS_SKILL_LEVELS);
		}
		hookStop();
	}
	return 0;
}

int64 battle_calc_damage_post(int64 final_damage, struct block_list *src, struct block_list *bl, struct Damage *d, int64 damage, uint16 skill_id, uint16 skill_lv) {
	struct map_session_data *s_sd, *t_sd;
	struct s_bonus_data *data;
	int i;
	s_sd = BL_CAST(BL_PC, src);
	t_sd = BL_CAST(BL_PC, bl);
	data = bonus_search(t_sd);
	for (i = 0; i < MAX_BONUS_SKILL_LEVELS; i++) {
		if (data->bReduceSkillDamage[i].skill_id == skill_id) {
			final_damage = (int64)(final_damage * ((100 - data->bReduceSkillDamage[i].reduction) / 100.0f));
			break;
		}
	}
	return final_damage;
}

HPExport void plugin_init(void) {
	bReduceSkillDamage = map->get_new_bonus_id();
	script->set_constant("bReduceSkillDamage", bReduceSkillDamage, false, false);
	addHookPost(battle, calc_damage, battle_calc_damage_post);
	addHookPre(pc, bonus2, pc_bonus2_pre);
	addHookPre(status, calc_pc_, status_calc_pc_pre);
}

HPExport void server_online(void) {
	ShowInfo("'%s' Plugin by Jeroen Version '%s'\n", pinfo.name, pinfo.version);
}