#include "common/hercules.h" /* Should always be the first Hercules file included! (if you don't make it first, you won't be able to use interfaces) */
#include "common/memmgr.h"
#include "common/mmo.h"
#include "common/socket.h"
#include "common/strlib.h"
#include "map/mob.h"
#include "map/script.h"
#include "map/pc.h"
#include "map/unit.h"
#include "map/status.h"
#include "map/npc.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h" /* should always be the last Hercules file included! (if you don't make it last, it'll intentionally break compile time) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HPExport struct hplugin_info pinfo = {
	"npcskillarea script command",    // Plugin name
	SERVER_TYPE_MAP,// Which server types this plugin works with?
	"1.0",       // Plugin version
	HPM_VERSION, // HPM Version (don't change, macro is automatically updated)
};


/* Cast a skill on the attached player.
* npcskillarea <skill id>, <skill lvl>, <stat point>, <NPC level>,<x>,<y>;
* npcskillarea "<skill name>", <skill lvl>, <stat point>, <NPC level>,<x>,<y>; */

BUILDIN(npcskillarea)
{
	struct npc_data *nd;
	uint16 skill_id = script_isstringtype(st, 2) ? skill->name2id(script_getstr(st, 2)) : script_getnum(st, 2);
	unsigned short skill_level = script_getnum(st, 3);
	unsigned int stat_point = script_getnum(st, 4);
	unsigned int npc_level = script_getnum(st, 5);
	unsigned int x = script_getnum(st, 6);
	unsigned int y = script_getnum(st, 7);

	nd = map->id2nd(st->oid);

	if (npc_level > MAX_LEVEL) {
		ShowError("npcskill: level exceeded maximum of %d.\n", MAX_LEVEL);
		return false;
	}
	if (nd == NULL) {
		return false;
	}

	nd->level = npc_level;
	nd->stat_point = stat_point;

	if (!nd->status.hp) {
		status_calc_npc(nd, SCO_FIRST);
	}
	else {
		status_calc_npc(nd, SCO_NONE);
	}

	if (skill->get_inf(skill_id)&INF_GROUND_SKILL) {
		unit->skilluse_pos(&nd->bl, x, y, skill_id, skill_level);
	}
	else {
		return false;
	}

	return true;
}

HPExport void plugin_init(void) {
	addScriptCommand("npcskillarea", "riiiii", npcskillarea);
	addScriptCommand("npcskillarea", "iiiiii", npcskillarea);
}

HPExport void server_online(void) {
	ShowInfo("'%s' Plugin by Jeroen. Version '%s'\n", pinfo.name, pinfo.version);
}