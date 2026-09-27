#include "LubSkyMap.h"

#include <browedit/util/Util.h>
#include <browedit/util/FileIO.h>
#include <browedit/BrowEdit.h>
#include <iostream>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>

LubSkyMap::LubSkyMap()
{
}

LubSkyMap::~LubSkyMap()
{
	for (auto cloud : clouds) {
		delete cloud;
	}
}

bool LubSkyMap::preload(const std::string& mapName, sol::state& lua)
{
	lua.open_libraries(sol::lib::base);

	auto lub = util::FileIO::open(mapskyFileName);

	if (!lub)
		return false;

	std::string data = util::loadLubFileToString(lub);
	delete lub;

	auto load_result = lua.load(data);

	if (!load_result.valid()) {
		sol::error err = load_result;
		std::cerr << "Syntax error or failed to read mapskydata.lub: " << err.what() << std::endl;
		return false;
	}

	auto run_result = load_result();
	if (!run_result.valid()) {
		sol::error err = run_result;
		std::cerr << "Runtime error executing decompiled data: " << err.what() << std::endl;
		return false;
	}

	return true;
}

bool LubSkyMap::load(const std::string& mapName)
{
	sol::state lua;

	if (!preload(mapName, lua))
		return false;

	sol::table mapskydata_tbl = lua.get_or("MapSkyData", lua.create_table());
	sol::optional<sol::table> skymap = mapskydata_tbl[mapName + ".rsw"];

	// The map isn't defined in the lub file, just exit early
	if (!skymap)
		return true;

	sol::table skymap_tbl = *skymap;

	glm::from_lua(skymap_tbl["BG_Color"], BG_Color);
	BG_Color /= 255.0f;
	Star_Effect = skymap_tbl["Star_Effect"].get_or(false);
	BG_Fog = skymap_tbl["BG_Fog"].get_or(true);
	oldClouds = skymap_tbl.get_or("Old_Cloud_Effect", std::vector<int>{});

	sol::optional<sol::table> cloudEffect = skymap_tbl["Cloud_Effect"];

	if (cloudEffect) {
		sol::table cloud_effect_tbl = *cloudEffect;

		for (auto&& pair : cloud_effect_tbl) {
			sol::object value_obj = pair.second;
			if (!value_obj.is<sol::table>()) continue;

			sol::table entry = value_obj.as<sol::table>();

			auto cloud = new LubSkyMap::CloudEffect();

			cloud->Num = entry.get_or("Num", 1000);
			cloud->CullDist = entry.get_or("CullDist", 400);
			cloud->Size = entry.get_or("Size", 0.0f);
			cloud->Size_Extra = entry.get_or("Size_Extra", 0.0f);
			cloud->Expand_Rate = entry.get_or("Expand_Rate", 0.0f);
			cloud->Alpha_Inc_Time = entry.get_or("Alpha_Inc_Time", 0.0f);
			cloud->Alpha_Inc_Time_Extra = entry.get_or("Alpha_Inc_Time_Extra", 0.0f);
			cloud->Alpha_Inc_Speed = entry.get_or("Alpha_Inc_Speed", 0.0f);
			cloud->Alpha_Dec_Time = entry.get_or("Alpha_Dec_Time", 0.0f);
			cloud->Alpha_Dec_Time_Extra = entry.get_or("Alpha_Dec_Time_Extra", 0.0f);
			cloud->Alpha_Dec_Speed = entry.get_or("Alpha_Dec_Speed", 0.0f);
			cloud->Height = entry.get_or("Height", 0.0f);
			cloud->Height_Extra = entry.get_or("Height_Extra", 0.0f);

			glm::from_lua(entry["Color"], cloud->Color);
			cloud->Color /= 255.0f;

			clouds.push_back(cloud);
		}
	}

	isEnabled = true;
	return true;
}

bool LubSkyMap::save(const std::string& mapName, BrowEdit* browEdit)
{
	auto lub = util::FileIO::open(mapskyFileName);

	if (!lub)
		return false;

	std::string data = util::loadLubFileToString(lub);
	delete lub;

	std::string lubPath = browEdit->config.ropath + mapskyFileName;
	std::string lubDirectory = lubPath.substr(0, lubPath.rfind("\\"));

	std::cout << "LUB (skymap): " + lubPath << std::endl;
	if (!std::filesystem::exists(lubDirectory)) {
		std::filesystem::create_directories(lubDirectory);
	}

	std::string mapIdentifier = "[\"" + mapName + ".rsw\"]";
	std::string newSkyMapData = createSkyMapTable(mapIdentifier);

	size_t keyPos = data.find(mapIdentifier);
	size_t replaceStart = keyPos;

	// Not found, insert at the start of the file
	if (keyPos == std::string::npos && isEnabled) {
		keyPos = data.find("MapSkyData =");

		if (keyPos == std::string::npos) {
			std::cerr << "Unable to find MapSkyData table within mapskydata.lub" << std::endl;
			return false;
		}

		keyPos = data.find("{", keyPos);

		if (keyPos == std::string::npos) {
			std::cerr << "Unable to find MapSkyData table within mapskydata.lub" << std::endl;
			return false;
		}

		keyPos++;

		data.insert(keyPos, "\n\t" + newSkyMapData + ",");
	}
	else {
		size_t closeBrace = findMatchingTableEnd(data, replaceStart);

		if (closeBrace == std::string::npos)
			return false;

		if (!isEnabled) {
			if (closeBrace + 1 < data.size() && data[closeBrace + 1] == ',')
				closeBrace++;

			data.replace(replaceStart, closeBrace - replaceStart + 1, "");
		}
		else {
			data.replace(replaceStart, closeBrace - replaceStart + 1, newSkyMapData);
		}
	}

	std::ofstream lubFile(lubPath.c_str(), std::ios_base::out | std::ios_base::binary);

	lubFile << data;
	lubFile.close();
	return true;
}

std::string LubSkyMap::createSkyMapTable(std::string mapIdentifier)
{
	std::ostringstream ss;

	ss << mapIdentifier << " = {" << std::endl;
	ss << "\t\tBG_Color = { "
		<< (int)(BG_Color.r * 255.0f) << ", "
		<< (int)(BG_Color.g * 255.0f) << ", "
		<< (int)(BG_Color.b * 255.0f) << " }";

	if (Star_Effect) {
		ss << "," << std::endl;
		ss << "\t\tStar_Effect = true";
	}

	if (BG_Fog) {
		ss << "," << std::endl;
		ss << "\t\tBG_Fog = true";
	}

	if (clouds.size() > 0) {
		ss << "," << std::endl;
		ss << "\t\tCloud_Effect = {" << std::endl;

		for (int i = 0; i < clouds.size(); i++) {
			auto cloud = clouds[i];

			ss << "\t\t\t[" << (i + 1) << "] = {" << std::endl;

#define SAVEPROP3(x,y) ss<<"\t\t\t\t"<<x<<" = { "<<(int)(y[0]*255.0f)<<", "<<(int)(y[1]*255.0f)<<", "<<(int)(y[2]*255.0f)<<" }"
#define SAVEPROPI(x,y) ss<<"\t\t\t\t"<<x<<" = "<<y
#define SAVEPROPF(x,y) ss<<"\t\t\t\t"<<x<<" = "<<std::setprecision(5)<<y

			SAVEPROPI("Num", cloud->Num) << "," << std::endl;
			SAVEPROPI("CullDist", cloud->CullDist) << "," << std::endl;
			SAVEPROP3("Color", cloud->Color) << "," << std::endl;
			SAVEPROPF("Size", cloud->Size) << "," << std::endl;
			SAVEPROPF("Size_Extra", cloud->Size_Extra) << "," << std::endl;
			SAVEPROPF("Expand_Rate", cloud->Expand_Rate) << "," << std::endl;
			SAVEPROPF("Alpha_Inc_Time", cloud->Alpha_Inc_Time) << "," << std::endl;
			SAVEPROPF("Alpha_Inc_Time_Extra", cloud->Alpha_Inc_Time_Extra) << "," << std::endl;
			SAVEPROPF("Alpha_Inc_Speed", cloud->Alpha_Inc_Speed) << "," << std::endl;
			SAVEPROPF("Alpha_Dec_Time", cloud->Alpha_Dec_Time) << "," << std::endl;
			SAVEPROPF("Alpha_Dec_Time_Extra", cloud->Alpha_Dec_Time_Extra) << "," << std::endl;
			SAVEPROPF("Alpha_Dec_Speed", cloud->Alpha_Dec_Speed) << "," << std::endl;
			SAVEPROPF("Height", cloud->Height) << "," << std::endl;
			SAVEPROPF("Height_Extra", cloud->Height_Extra) << std::endl;

			ss << "\t\t\t}";

			if (i != clouds.size() - 1)
				ss << ",";

			ss << std::endl;
		}

		ss << "\t\t}";
	}

	if (oldClouds.size() > 0) {
		ss << "," << std::endl;
		ss << "\t\tOld_Cloud_Effect = { ";

		for (int i = 0; i < oldClouds.size(); i++) {
			ss << oldClouds[i];

			if (i != oldClouds.size() - 1)
				ss << ", ";
		}

		ss << " }";
	}

	ss << std::endl << "\t}";
	return ss.str();
}

size_t LubSkyMap::findMatchingTableEnd(const std::string& data, size_t openBrace)
{
	int depth = 0;
	bool inString = false;
	bool escaped = false;

	for (size_t i = openBrace; i < data.size(); ++i) {
		char c = data[i];

		if (inString) {
			if (escaped) {
				escaped = false;
			}
			else if (c == '\\') {
				escaped = true;
			}
			else if (c == '"') {
				inString = false;
			}

			continue;
		}

		if (c == '"') {
			inString = true;
			continue;
		}

		if (c == '{') {
			++depth;
		}
		else if (c == '}') {
			--depth;

			if (depth == 0)
				return i;
		}
	}

	return std::string::npos;
}

const LubSkyMap::CloudEffect* LubSkyMap::getStarEffectTemplate()
{
	static const LubSkyMap::CloudEffect ct_star_effect = [] {
		LubSkyMap::CloudEffect cloud;
		cloud.Num = -1;
		cloud.NumPerSquared = (float)(60.0f * 4 / glm::pow(400, 2));
		cloud.blendSrc = GL_SRC_ALPHA;
		cloud.blendDst = GL_ONE;
		cloud.Size = 20.0f;
		cloud.Size_Extra = 10.0f;
		cloud.Expand_Rate = 0.0f;
		cloud.Alpha_Inc_Time = 80.0f;
		cloud.Alpha_Inc_Time_Extra = 0.0f;
		cloud.Alpha_Inc_Speed = 3.3f;
		cloud.Alpha_Dec_Time = 500.0f;
		cloud.Alpha_Dec_Time_Extra = 300.0f;
		cloud.Alpha_Dec_Speed = 0.5f;
		cloud.Height = 80.0f;
		cloud.Height_Extra = 10.0f;
		cloud.uvCycleSpeed = 0.143f;
		return cloud;
	}();
	return &ct_star_effect;
}

const LubSkyMap::CloudEffect* LubSkyMap::getTemplate(int id) {
	switch (id) {
	case 1: {
		// Decompiler: 400 range, 60 clouds per quadrants
		// 40 cells
		static const LubSkyMap::CloudEffect ct_1 = [id] {
			LubSkyMap::CloudEffect cloud;
			cloud.oldCloudEffect = id;
			cloud.Num = -1;
			cloud.NumPerSquared = (float)(60.0f * 4 / glm::pow(400, 2));
			cloud.scaleY = -1.0f;
			cloud.Height = 40.0f;
			cloud.Height_Extra = 10.0f;
			return cloud;
		}();
		return &ct_1;
	}
	case 2: {
		// Decompiler: 300 range, 40 clouds per quadrants
		// 30 cells
		static const LubSkyMap::CloudEffect ct_2 = [id] {
			LubSkyMap::CloudEffect cloud;
			cloud.oldCloudEffect = id;
			cloud.Num = -1;
			cloud.NumPerSquared = (float)(40.0f * 4 / glm::pow(300, 2));
			cloud.scaleX = -1;
			cloud.scaleY = -1;
			cloud.Height = 0.0f;
			cloud.Height_Extra = 10.0f;
			return cloud;
		}();
		return &ct_2;
	}
	case 3: {
		// Decompiler: 300 range, 80 clouds per quadrants
		// 30 cells
		static const LubSkyMap::CloudEffect ct_3 = [id] {
			LubSkyMap::CloudEffect cloud;
			cloud.oldCloudEffect = id;
			cloud.Num = -1;
			cloud.NumPerSquared = (float)(80.0f * 4 / glm::pow(300, 2));
			cloud.Color = glm::vec3(196, 133, 111) / 255.0f;
			cloud.scaleY = -1;
			cloud.Height = 0.0f;
			cloud.Height_Extra = -10.0f;
			cloud.dirMode = 1;	// No movement
			cloud.snapToGround = true;
			cloud.useFogTextures = true;
			cloud.Height = 99999.0f;
			return cloud;
		}();
		return &ct_3;
	}
	case 4: {
		// Decompiler: 400 range, 80 clouds per quadrants
		// 40 cells
		static const LubSkyMap::CloudEffect ct_4 = [id] {
			LubSkyMap::CloudEffect cloud;
			cloud.oldCloudEffect = id;
			cloud.Num = -1;
			cloud.NumPerSquared = (float)(80.0f * 4 / glm::pow(400, 2));
			cloud.scaleX = -1;
			cloud.scaleY = -1;
			cloud.Height = 40.0f;
			cloud.Height_Extra = 10.0f;
			cloud.dirMode = 2; // Right-sided movement
			cloud.forcedDir = glm::vec4(10, 0, 0, 0);
			return cloud;
		}();
		return &ct_4;
	}
	case 5: {
		// Decompiler: 300 range, 80 clouds per quadrants
		// 30 cells
		static const LubSkyMap::CloudEffect ct_5 = [id] {
			LubSkyMap::CloudEffect cloud;
			cloud.oldCloudEffect = id;
			cloud.Num = -1;
			cloud.NumPerSquared = (float)(80.0f * 4 / glm::pow(300, 2));
			cloud.Color = glm::vec3(94, 0, 0) / 255.0f;
			cloud.scaleY = -1;
			cloud.Height = 20.0f;
			cloud.Height_Extra = 10.0f;
			return cloud;
		}();
		return &ct_5;
	}
	case 7: {
		// Decompiler: 400 range, 80 clouds per quadrants
		// 40 cells
		static const LubSkyMap::CloudEffect ct_7 = [id] {
			LubSkyMap::CloudEffect cloud;
			cloud.oldCloudEffect = id;
			cloud.Num = -1;
			cloud.NumPerSquared = (float)(80.0f * 4 / glm::pow(400, 2));
			cloud.Color = glm::vec3(0, 0, 0);
			cloud.scaleY = -1;
			cloud.Height = 40.0f;
			cloud.Height_Extra = 10.0f;
			return cloud;
		}();
		return &ct_7;
	}
	case 8: {
		// Decompiler: 400 range, 80 clouds per quadrants
		// 40 cells
		static const LubSkyMap::CloudEffect ct_8 = [id] {
			LubSkyMap::CloudEffect cloud;
			cloud.oldCloudEffect = id;
			cloud.Num = -1;
			cloud.NumPerSquared = (float)(80.0f * 4 / glm::pow(400, 2));
			cloud.Color = glm::vec3(255, 180, 180) / 255.0f;
			cloud.scaleY = -1;
			cloud.Height = 40.0f;
			cloud.Height_Extra = 10.0f;
			return cloud;
		}();
		return &ct_8;
	}
	case 9: {
		// Decompiler: 400 range, 65 clouds per quadrants
		// 40 cells
		static const LubSkyMap::CloudEffect ct_9 = [id] {
			LubSkyMap::CloudEffect cloud;
			cloud.oldCloudEffect = id;
			cloud.Num = -1;
			cloud.NumPerSquared = (float)(65.0f * 4 / glm::pow(400, 2));
			cloud.Alpha_Inc_Time = 100.0f;
			cloud.Alpha_Inc_Time_Extra = 0;
			cloud.Alpha_Inc_Speed = 2.45f;
			cloud.blendSrc = GL_SRC_ALPHA;
			cloud.blendDst = GL_ONE;
			cloud.Height = 85.0f;
			cloud.Height_Extra = 10.0f;
			cloud.uvCycleSpeed = 0.143f;
			cloud.useStarTextures = true;
			return cloud;
		}();
		return &ct_9;
	}
	case 10: {
		// ??
		// 30 cells
		static const LubSkyMap::CloudEffect ct_10 = [id] {
			LubSkyMap::CloudEffect cloud;
			cloud.oldCloudEffect = id;
			cloud.Num = -1;
			cloud.NumPerSquared = (float)(60.0f * 4 / glm::pow(400, 2));
			cloud.Alpha_Inc_Time = 100.0f;
			cloud.Alpha_Inc_Time_Extra = 0;
			cloud.Alpha_Inc_Speed = 1.5f;
			cloud.Color = glm::vec3(94, 0, 0) / 255.0f;
			cloud.scaleY = -1;
			cloud.Height = 30.0f;
			cloud.Height_Extra = 10.0f;
			return cloud;
		}();
		return &ct_10;
	}
	case 11: {
		// Cloud type 11 uses 4 sub clouds, by itself it has nothing to render
		static const LubSkyMap::CloudEffect ct_11g = [this, id] {
			LubSkyMap::CloudEffect cloud;
			cloud.oldCloudEffect = id;
			cloud.subClouds.push_back(getTemplate(-11));
			cloud.subClouds.push_back(getTemplate(-12));
			cloud.subClouds.push_back(getTemplate(-13));
			cloud.subClouds.push_back(getTemplate(-14));
			return cloud;
		}();
		return &ct_11g;
	}
	case -11: {
		static LubSkyMap::CloudEffect ct_11 = [id] {
			LubSkyMap::CloudEffect cloud;
			cloud.oldCloudEffect = id;
			cloud.Num = -1;
			cloud.NumPerSquared = (float)(30.0f * 4 / glm::pow(400, 2));
			cloud.Alpha_Inc_Time = 100.0f;
			cloud.Alpha_Inc_Time_Extra = 0;
			cloud.Alpha_Inc_Speed = 0.69f;
			cloud.scaleY = -1;
			cloud.Height = 30.0f;
			cloud.Height_Extra = 20.0f;

			cloud.Color = glm::vec3(0, 0, 0) / 255.0f;
			cloud.blendSrc = GL_SRC_ALPHA;
			cloud.blendDst = GL_ONE_MINUS_SRC_ALPHA;
			return cloud;
		}();
		return &ct_11;
	}
	case -12: {
		static const LubSkyMap::CloudEffect ct_12 = [id] {
			LubSkyMap::CloudEffect cloud;
			cloud.oldCloudEffect = id;
			cloud.Num = -1;
			cloud.NumPerSquared = (float)(30.0f * 4 / glm::pow(400, 2));
			cloud.Alpha_Inc_Time = 100.0f;
			cloud.Alpha_Inc_Time_Extra = 0;
			cloud.Alpha_Inc_Speed = 0.69f;
			cloud.scaleY = -1;
			cloud.Height = 30.0f;
			cloud.Height_Extra = 20.0f;

			cloud.Color = glm::vec3(255, 181, 181) / 255.0f;
			cloud.blendSrc = GL_SRC_ALPHA;
			cloud.blendDst = GL_ONE_MINUS_SRC_ALPHA;
			return cloud;
		}();
		return &ct_12;
	}
	case -13: {
		static const LubSkyMap::CloudEffect ct_13 = [id] {
			LubSkyMap::CloudEffect cloud;
			cloud.oldCloudEffect = id;
			cloud.Num = -1;
			cloud.NumPerSquared = (float)(30.0f * 4 / glm::pow(400, 2));
			cloud.Alpha_Inc_Time = 100.0f;
			cloud.Alpha_Inc_Time_Extra = 0;
			cloud.Alpha_Inc_Speed = 0.69f;
			cloud.scaleY = -1;
			cloud.Height = 30.0f;
			cloud.Height_Extra = 20.0f;

			cloud.Color = glm::vec3(92, 0, 0) / 255.0f;
			cloud.blendSrc = GL_SRC_ALPHA;
			cloud.blendDst = GL_ONE_MINUS_SRC_ALPHA;
			return cloud;
		}();
		return &ct_13;
	}
	case -14: {
		static const LubSkyMap::CloudEffect ct_14 = [id] {
			LubSkyMap::CloudEffect cloud;
			cloud.oldCloudEffect = id;
			cloud.Num = -1;
			cloud.NumPerSquared = (float)(30.0f * 4 / glm::pow(400, 2));
			cloud.Alpha_Inc_Time = 100.0f;
			cloud.Alpha_Inc_Time_Extra = 0;
			cloud.Alpha_Inc_Speed = 0.69f;
			cloud.scaleY = -1;
			cloud.Height = 30.0f;
			cloud.Height_Extra = 20.0f;

			cloud.Color = glm::vec3(64, 70, 203) / 255.0f;
			cloud.blendSrc = GL_SRC_ALPHA;
			cloud.blendDst = GL_ONE;
			return cloud;
		}();
		return &ct_14;
	}
	case 15: {
		// Decompiler: 400 range, 65 clouds per quadrants
		// 40 cells
		static const LubSkyMap::CloudEffect ct_15 = [id] {
			LubSkyMap::CloudEffect cloud;
			cloud.oldCloudEffect = id;
			cloud.Num = -1;
			cloud.NumPerSquared = (float)(65.0f * 4 / glm::pow(400, 2));
			cloud.Alpha_Inc_Time = 100.0f;
			cloud.Alpha_Inc_Time_Extra = 0;
			cloud.Alpha_Inc_Speed = 2.45f;
			cloud.blendSrc = GL_SRC_ALPHA;
			cloud.blendDst = GL_ONE;
			cloud.Height = -40.0f;
			cloud.Height_Extra = 10.0f;
			cloud.uvCycleSpeed = 0.0f;
			cloud.useStarTextures = true;
			return cloud;
		}();
		return &ct_15;
	}
	}

	return nullptr;
}

LubSkyMap::CloudEffect::CloudEffect()
{
}

LubSkyMap::CloudEffect::CloudEffect(LubSkyMap::CloudEffect* cloud)
{
	Num = cloud->Num;
	NumPerSquared = cloud->NumPerSquared;
	CullDist = cloud->CullDist;
	Color = cloud->Color;
	Size = cloud->Size;
	Size_Extra = cloud->Size_Extra;
	Expand_Rate = cloud->Expand_Rate;
	Alpha_Inc_Time = cloud->Alpha_Inc_Time;
	Alpha_Inc_Time_Extra = cloud->Alpha_Inc_Time_Extra;
	Alpha_Inc_Speed = cloud->Alpha_Inc_Speed;
	Alpha_Dec_Time = cloud->Alpha_Dec_Time;
	Alpha_Dec_Time_Extra = cloud->Alpha_Dec_Time_Extra;
	Alpha_Dec_Speed = cloud->Alpha_Dec_Speed;
	Height = cloud->Height;
	Height_Extra = cloud->Height_Extra;

	dirMode = cloud->dirMode;
	forcedDir = cloud->forcedDir;
	scaleX = cloud->scaleX;
	scaleY = cloud->scaleY;
	uvCycleSpeed = cloud->uvCycleSpeed;
	subClouds = std::vector<const LubSkyMap::CloudEffect*>(cloud->subClouds);
	snapToGround = cloud->snapToGround;
	blendSrc = cloud->blendSrc;
	blendDst = cloud->blendDst;
	oldCloudEffect = cloud->oldCloudEffect;
	useStarTextures = cloud->useStarTextures;
	useFogTextures = cloud->useFogTextures;
	isValid = cloud->isValid;
}