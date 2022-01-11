#pragma once

#include <list>
#include <vector>

namespace util
{
	template<int childCount, class child>
	class Tree
	{
	public:
		child* children[childCount];


		Tree()
		{
			for (int i = 0; i < childCount; i++)
				children[i] = NULL;
		}

		template<class T>
		void foreach(T op)
		{
			op((child*)this);
			for (int i = 0; i < childCount; i++)
				if (children[i] != NULL)
					children[i]->foreach(op);
		}
		template<class T>
		void foreachLevel(T op, int level = 0)
		{
			op((child*)this, level);
			for (int i = 0; i < childCount; i++)
				if (children[i] != NULL)
					children[i]->foreachLevel(op, level+1);
		}

		std::vector<child*> flatten()
		{
			std::vector<child*> ret;
			ret.push_back(this);
			for (int i = 0; i < childCount; i++)
				if (children[i] != NULL)
                    ret.insert(ret.end(), children[i]->flatten());
			return ret;
		}
	};


	template<class child>
	class DynTree
	{
	public:
		std::list<child*> children;

		template<class T>
		void foreach(T op)
		{
			op((child*)this);
			for (typename std::list<child*>::iterator it = children.begin(); it != children.end(); it++)
				(*it)->foreach(op);
		}

		template<class T>
		void foreachLevel(T op, int level = 0)
		{
			op((child*)this, level);
			for (typename std::list<child*>::iterator it = children.begin(); it != children.end(); it++)
				(*it)->foreachLevel(op, level + 1);
		}

		std::vector<child*> flatten()
		{
			std::vector<child*> ret;
			ret.push_back(this);
			for (typename std::list<child*>::iterator it = children.begin(); it != children.end(); it++)
				ret.insert(ret.end(), (*it)->flatten());
			return ret;
		}

		template<class F>
		child* get(F func)
		{
			if (func((child*)this))
				return (child*)this;
			for (typename std::list<child*>::iterator it = children.begin(); it != children.end(); it++)
			{
				child* ret = (*it)->get(func);
				if (ret)
					return ret;
			}
			return NULL;
		}

	};
}
