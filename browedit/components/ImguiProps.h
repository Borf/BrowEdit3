#pragma once

#include "Component.h"

class BrowEdit;

class ImguiProps
{
public:
	virtual void buildImGui(BrowEdit* browEdit) = 0;
};