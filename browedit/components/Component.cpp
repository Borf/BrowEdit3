#include "Component.h"

#include "Rsw.h"


#define TO_JSON_HELP(type, name, str) \
	const type* name = dynamic_cast<const type*>(&c);\
	if(name)\
	{\
		to_json(j, *name);\
		j["type"] = str;\
	}

void to_json(nlohmann::json& j, const Component& c)
{
	TO_JSON_HELP(RswObject, rswObject, "rswobject");
	TO_JSON_HELP(RswModel, rswModel, "rswmodel");
	TO_JSON_HELP(RswLight, rswLight, "rswlight");
	TO_JSON_HELP(RswEffect, rswEffect, "rsweffect");
	TO_JSON_HELP(RswSound, rswSound, "rswsound");
	//auto rswModel = dynamic_cast<const RswModel*>(&c);
	//if (rswModel)
	//{
	//	to_json(j, *rswModel);
	//	j["type"] = "rswmodel";
	//}
}
