#pragma once

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <mutex>

namespace util
{
	template<class T>
	class ResourceManager
	{
		static inline std::vector<std::pair<T*, int> > resources;
		static inline std::map<std::string, std::pair<T*, int>> resmap;
		static inline std::mutex loadMutex;
	public:
		template<class R = T>
		static R* load()
		{
			const std::lock_guard<std::mutex> lock(loadMutex);
			for (auto r : resources)
			{
				R* rr = dynamic_cast<R*>(r.first);
				if (rr)
					return rr;
			}
			R* rr = new R();
			resources.push_back(std::pair<T*, int>(rr,1));
			return rr;
		}
		template<class R = T>
		static R* load(const std::string &str)
		{
			const std::lock_guard<std::mutex> lock(loadMutex);
			auto it = resmap.find(str);
			if (it != resmap.end())
			{
				it->second.second++;
				return dynamic_cast<R*>(it->second.first);
			}
			R* r = new R(str);
			resmap[str] = std::pair<T*, int>(r, 1);
			return r;
		}

		static void clear()
		{
			for (auto it : resmap)
				delete it.second.first;
			for (auto res : resources)
				delete res.first;
			resmap.clear();
			resources.clear();
		}


		static std::vector<T*> getAll()
		{
			std::vector<T*> ret;
			for (auto it : resmap)
				ret.push_back(it.second.first);
			for (auto res : resources)
				ret.push_back(res.first); //TODO: do this with iterator insert
			return ret;
		}

		static std::size_t count()
		{
			return resources.size() + resmap.size();
		}
		static void unload(T* res)
		{
			for (auto kv = resmap.begin(); kv != resmap.end(); kv++)
			{
				if (kv->second.first == res)
				{
					kv->second.second--;
					if (kv->second.second == 0)
					{
						std::cout << "Resource not used anymore, unloading" << std::endl;
						delete kv->second.first;
						resmap.erase(kv);
					}
					return;
				}
			}
			for (auto v = resources.begin(); v != resources.end(); v++)
			{
				if (v->first == res)
				{
					v->second--;
					if (v->second == 0)
					{
						std::cout << "Resource not used anymore, unloading" << std::endl;
						delete v->first;
						resources.erase(v);
					}
					return;
				}
			}
			std::cout << "Unloading resource, could not find it in the resource map!" << std::endl;
		}
	};

//	template<class T>
//	std::vector<T*> ResourceManager<T>::resources;
}

