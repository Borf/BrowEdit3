#include <browedit/Map.h>
#include <browedit/Node.h>
#include <browedit/actions/Action.h>
#include <browedit/components/Rsw.h>
#include <iostream>


Map::Map(const std::string& name) : name(name)
{
	rootNode = new Node(name);
	auto rsw = new Rsw();
	rootNode->addComponent(rsw);	
	rsw->load(name);
}



void Map::doAction(Action* action, BrowEdit* browEdit)
{
	if (std::find(undoStack.begin(), undoStack.end(), action) != undoStack.end())
	{
		std::cerr << "Double undo!" << std::endl;
		throw "Oops";
		return;
	}
	action->perform(this, browEdit);
	undoStack.push_back(action);

	for (auto a : redoStack)
		delete a;
	redoStack.clear();
}

void Map::redo(BrowEdit* browEdit)
{
	if (redoStack.size() > 0)
	{
		redoStack.front()->perform(this, browEdit);
		undoStack.push_back(redoStack.front());
		redoStack.erase(redoStack.begin());
	}
}
void Map::undo(BrowEdit* browEdit)
{
	if (undoStack.size() > 0)
	{
		undoStack.back()->undo(this, browEdit);
		redoStack.insert(redoStack.begin(), undoStack.back());
		undoStack.pop_back();
	}
}
