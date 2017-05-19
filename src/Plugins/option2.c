//===== Hercules Plugin ======================================
//= option2
//===== By: ==================================================
//= Jeroen
//===== Current Version: =====================================
//= 1.0
//===== Description: =========================================
//= Allows you to use option status effects
//===== Additional Comments: =================================
//= Format:
//= setopion2 x,y; x = status id, y = 1 or 0 (on or off)
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

#include "map/pc.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h"

HPExport struct hplugin_info pinfo = {
	"setoption2 script command",    // Plugin name
	SERVER_TYPE_MAP,// Which server types this plugin works with?
	"1.0",       // Plugin version
	HPM_VERSION, // HPM Version (don't change, macro is automatically updated)
};

/**
* Adds a built-in script function.
*
* @param data	Array name where the variables will be stored
* @param monster_id The ID of the monster that we want to find.
* @return
*/

BUILDIN(setoption2) {
	int option;
	int flag = 1;
	struct map_session_data *sd = script->rid2sd(st);
	if (sd == NULL)
		return true;
	option = script_getnum(st, 2);
	if (script_hasdata(st, 3))
		flag = script_getnum(st, 3);
	if (flag)  
		sd->sc.opt1 = option; // op1 is not a bitmask, so we override the previous status.
	else // Remove option
		sd->sc.opt1 = 0; // op1 is not a bitmask, so we just remove the status effect
	return true;
	
}

HPExport void plugin_init(void) {
	addScriptCommand("setoption2", "i?", setoption2);
}

HPExport void server_online(void) {
	ShowInfo("'%s' Plugin by Jeroen. Version '%s'\n", pinfo.name, pinfo.version);
}