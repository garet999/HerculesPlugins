//===== Hercules ;'zx	 Plugin ======================================
//= bonus bBonusSkillLevel
//===== By: ==================================================
//= Jeroen
//===== Current Version: =====================================
//= 1.0
//===== Description: =========================================
//= bBonusSkillLevel increases the skilllevel of a spell after it's been cast. (Note: going above the skills max level, might have some odd quirks)
//===== Additional Comments: =================================
//= Format:
//= bonus2 bBonusSkillLevel x, y;	// x = skill_id y = skills level to add 
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

#include "map/battle.h"
#include "map/map.h"
#include "map/pc.h"
#include "map/status.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"


HPExport struct hplugin_info pinfo = {
	"Paladin Bonuses",
	SERVER_TYPE_MAP,
	"1.0",
	HPM_VERSION,
};
#define MAX_BONUS_SKILL_LEVELS 10
#define MAX_LEVEL_INCREASE 10

struct Damage(*battle_calc_attack_old) (int attack_type, struct block_list *bl, struct block_list *target, uint16 skill_id, uint16 skill_lv, int count);
/**
* Bonus ID.
* don't mess with this.
*/
int bBonusSkillLevel = -1;

struct bonus_skill_level {
	uint16 skill_id;
	uint16 bonus_level;
};
struct s_bonus_data {
	struct bonus_skill_level bSkillLevel[MAX_BONUS_SKILL_LEVELS];
};

struct s_bonus_data* bonus_search(struct map_session_data* sd) {
	struct s_bonus_data *data = NULL;
	if ((data = getFromMSD(sd, 0)) == NULL) {
		CREATE(data, struct s_bonus_data, 1);
		memset(data->bSkillLevel, 0, sizeof(data->bSkillLevel));
		addToMSD(sd, data, 0, true);
	}
	return data;
}

// Reset on Recalc
int status_calc_pc_pre(struct map_session_data **sd, enum e_status_calc_opt *opt)
{
	struct s_bonus_data *data = NULL;
	if ((data = getFromMSD(*sd, 0)) != NULL) {
		memset(data->bSkillLevel, 0, sizeof(data->bSkillLevel));	// Reset Structure
	}
	return 1;
}

int pc_bonus2_pre(struct map_session_data **sd, int *type, int *type2, int *val)
{
	if (*type == bBonusSkillLevel) {
		struct s_bonus_data *data = bonus_search(*sd);
		int i;

		if (*type2 <= 0) {
			ShowError("pc_bonus2_pre: skill_id for bBonusSkillLevel is not valid\n");
			hookStop();
			return 0;
		}
		if (*val <= 0 || *val > MAX_LEVEL_INCREASE) {
			ShowError("pc_bonus2_pre: Level Increase for bBonusSkillLevel is not valid.\n");
			hookStop();
			return 0;
		}

		// Find free Slot
		for (i = 0; i < MAX_BONUS_SKILL_LEVELS; i++) {
			if (data->bSkillLevel[i].skill_id == 0 || data->bSkillLevel[i].bonus_level == 0) {
				data->bSkillLevel[i].skill_id = *type2;
				data->bSkillLevel[i].bonus_level = *val;
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
struct Damage battle_calc_attack_custom(int attack_type, struct block_list *bl, struct block_list *target, uint16 skill_id, uint16 skill_lv, int count) {
	struct map_session_data *sd = BL_CAST(BL_PC, bl);
	int i;
	if (bl->type == BL_PC) {
		struct s_bonus_data *data = bonus_search(sd);
		for (i = 0; i < MAX_BONUS_SKILL_LEVELS; i++) 
		{
			if (data->bSkillLevel[i].skill_id != 0 || data->bSkillLevel[i].bonus_level != 0) 
			{
				if (skill_id == data->bSkillLevel[i].skill_id)
				{
					skill_lv += data->bSkillLevel[i].bonus_level;
				}
			}
		}
	}
	return battle_calc_attack_old(attack_type, bl, target, skill_id, skill_lv, count);
}

HPExport void plugin_init(void) {
	bBonusSkillLevel = map->get_new_bonus_id();
	script->set_constant("bBonusSkillLevel", bBonusSkillLevel, false, false);
	battle_calc_attack_old = battle->calc_attack;
	battle->calc_attack = battle_calc_attack_custom;
	addHookPre(pc, bonus2, pc_bonus2_pre);
	addHookPre(status, calc_pc_, status_calc_pc_pre);

}

HPExport void server_online(void) {
	ShowInfo("'%s' Plugin by Jeroen Version '%s'\n", pinfo.name, pinfo.version);
}