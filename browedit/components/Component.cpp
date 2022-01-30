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
//	TO_JSON_HELP(RswObject, rswObject, "rswobject");
	auto rswObject = dynamic_cast<const RswObject*>(&c);
	if (rswObject)
	{
		to_json(j, *rswObject);
		j["type"] = "rswobject";
	}
	auto rswModel = dynamic_cast<const RswModel*>(&c);
	if (rswModel)
	{
		to_json(j, *rswModel);
		j["type"] = "rswmodel";
	}
}
