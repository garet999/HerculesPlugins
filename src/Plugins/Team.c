#include "common/hercules.h" /* Should always be the first Hercules file included! (if you don't make it first, you won't be able to use interfaces) */
#include "common/memmgr.h"
#include "common/mmo.h"
#include "common/socket.h"
#include "common/strlib.h"
#include "map/mob.h"
#include "map/script.h"
#include "map/battle.h"
#include "map/pc.h"
#include "common/nullpo.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h" /* should always be the last Hercules file included! (if you don't make it last, it'll intentionally break compile time) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HPExport struct hplugin_info pinfo = {
	"Team script command",    // Plugin name
	SERVER_TYPE_MAP,// Which server types this plugin works with?
	"1.0",       // Plugin version
	HPM_VERSION, // HPM Version (don't change, macro is automatically updated)
};


struct s_bonus_data {
	uint16 teamId;
};

// If data is not found, set to null
struct s_bonus_data* bonus_search(struct map_session_data* sd) {
	struct s_bonus_data *data = NULL;
	if ((data = getFromMSD(sd, 0)) == NULL) {
		CREATE(data, struct s_bonus_data, 1);
		data->teamId = 0;
		addToMSD(sd, data, 0, true);
	}
	return data;
}
int battle_check_target_pre(struct block_list **src, struct block_list **target, int *flag)
{
	struct block_list *s_bl = *src, *t_bl = *target;
	nullpo_ret(*src);
	nullpo_ret(*target);
	if (s_bl->type == BL_PC && t_bl->type == BL_PC) {
		struct map_session_data *sd = BL_CAST(BL_PC, *src);
		struct map_session_data *tsd = BL_CAST(BL_PC, *target);
		if (sd != tsd) {
			struct s_bonus_data *sddata, *tsddata;
			sddata = bonus_search(sd);
			tsddata = bonus_search(tsd);
			if (sddata->teamId == tsddata->teamId && (sddata->teamId != 0 || tsddata->teamId != 0)) {
				hookStop();
				return 0;
			}
		}
	}
	return 0;
}
/**
* Adds a built-in script function.
*
* @param id = Id of team joining. Team members cannot attack one another
* @return
*/
BUILDIN(jointeam) {
	struct map_session_data* sd = script->rid2sd(st);
	struct s_bonus_data* data;
	int teamId = script_getnum(st, 2);
	data = bonus_search(sd);
	data->teamId = teamId;
	return 1;
}

BUILDIN(leaveteam) {
	struct map_session_data* sd = script->rid2sd(st);
	struct s_bonus_data* data;
	data = bonus_search(sd);
	data->teamId = 0;
	return 1;
}

HPExport void plugin_init(void) {
	addHookPre(battle,check_target,battle_check_target_pre);
	addScriptCommand("jointeam", "i", jointeam);
	addScriptCommand("leaveteam", "", leaveteam);
}

HPExport void server_online(void) {
	ShowInfo("'%s' Plugin by Jeroen. Version '%s'\n", pinfo.name, pinfo.version);
}