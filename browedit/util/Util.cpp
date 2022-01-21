#include "Util.h"
#include <browedit/BrowEdit.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <iostream>
#include <browedit/actions/ObjectChangeAction.h>

namespace util
{
	short swapShort(const short s)
	{
		return ((s&0xff)<<8) | ((s>>8)&0xff);
	}


	bool DragFloat3(BrowEdit* browEdit, Node* node, const char* label, glm::vec3 *ptr, float v_speed, float v_min, float v_max, const std::string &action)
	{
		static glm::vec3 startValue;
		bool ret = ImGui::DragFloat3(label, glm::value_ptr(*ptr), v_speed, v_min, v_max);
			
		if (ImGui::IsItemActivated())
		{
			startValue = *ptr;
		}
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			browEdit->doAction(new ObjectChangeAction(node, ptr, startValue, action == "" ? label : action));
		}
		return ret;
	}
}