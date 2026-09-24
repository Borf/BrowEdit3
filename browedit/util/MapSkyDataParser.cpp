//#include "MapSkyDataParser.h"
//#include <string>
//#include "Util.h"
//#include "FileIO.h"
//#include <iostream>
//#include <fstream>
//
//MapSkyDataParser::MapSkyDataParser(std::string path, std::string mapName)
//{
//	//auto lub = util::FileIO::open(path);
//	//
//	//if (!lub)
//	//	return;
//	//
//	//std::string data = util::loadLubFileToString(lub);
//	//
//	//if (data == "")
//	//	return;
//	//
//	//sol::state mapSkyDataLua;
//	//
//	//try
//	//{
//	//	std::ofstream luaFile("tmp.lua");
//	//	luaFile << data;
//	//	luaFile.close();
//	//
//	//	mapSkyDataLua.open_libraries(sol::lib::base);
//	//	mapSkyDataLua.script_file("tmp.lua");
//	//	std::filesystem::remove("tmp.lua");
//	//
//	//	sol::table mapSkyDataTable = mapSkyDataLua["MapSkyData"];
//	//
//	//	if (!mapSkyDataTable.valid())
//	//		return;
//	//
//	//	sol::table currentMapSkyDataTable = mapSkyDataTable[mapName];
//	//
//	//	if (!currentMapSkyDataTable.valid())
//	//		return;
//	//
//	//
//	//	valid = true;
//	//}
//	//catch (const std::exception& e)
//	//{
//	//	std::cerr << "Error loading SkyMapData.lua data: " << e.what() << std::endl;
//	//}
//}
//
////MapSkyDataParser::getDecompiledData(std::iostream stream)
////{
////
////}