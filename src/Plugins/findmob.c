#include "common/hercules.h" /* Should always be the first Hercules file included! (if you don't make it first, you won't be able to use interfaces) */
#include "common/memmgr.h"
#include "common/mmo.h"
#include "common/socket.h"
#include "common/strlib.h"
#include "map/mob.h"
#include "map/script.h"

#include "plugins/HPMHooking.h"
#include "common/HPMDataCheck.h" /* should always be the last Hercules file included! (if you don't make it last, it'll intentionally break compile time) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HPExport struct hplugin_info pinfo = {
	"Find mob script command",    // Plugin name
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

BUILDIN(findmob) {
	struct mob_db* db;
	struct script_data* data = script_getdata(st, 2);
	struct map_session_data* sd = NULL;
	int i, start, id, j, count;
	const char *name;
	char str[15];
	uint16 monster_id = 0;
	id = reference_getid(data);
	start = reference_getindex(data);
	name = reference_getname(data);

	if (!data_isreference(data)) {
		ShowError("script:findmob: not a variable\n");
		script->reportdata(data);
		st->state = END;
		return false; // not a variable
	}

	if (not_server_variable(*name)) {
		sd = script->rid2sd(st);
		if (sd == NULL)
			return true; 
	}


	monster_id = script_getnum(st, 3);
	db = mob->db(monster_id);
	count = 0;
	for (i = 0; i < ARRAYLENGTH(db->spawn); i++) {
		j = map->mapindex2mapid(db->spawn[i].mapindex);
		if (j < 0) {
			continue;
		}
		script->set_reg(st, sd, reference_uid(id, count), name, map->list[j].name, reference_getref(data));
		itoa(db->spawn[i].qty, str, 10);
		script->set_reg(st, sd, reference_uid(id, count + 1), name, str, reference_getref(data));
		count += 2;
	}
	script_pushint(st, count + 1);
	return true;
}

HPExport void plugin_init(void) {
	addScriptCommand("findmob", "ri", findmob);
}

HPExport void server_online(void) {
	ShowInfo("'%s' Plugin by Jeroen. Version '%s'\n", pinfo.name, pinfo.version);
}