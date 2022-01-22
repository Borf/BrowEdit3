#include "FileIO.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

namespace util
{
	std::vector<FileIO::Source*> FileIO::sources;

	void FileIO::begin()
	{
		for (auto& source : sources)
		{
			source->close();
			delete source;
		}
		sources.clear();
	}

	std::istream* FileIO::open(const std::string& fileName)
	{
		for (auto& source : sources)
		{
			if (source->exists(fileName))
				return source->open(fileName);
		}
		return nullptr;
	}

	bool FileIO::exists(const std::string& fileName)
	{
		for (auto& source : sources)
			if (source->exists(fileName))
				return true;
		return false;
	}

	std::vector<std::string> FileIO::listFiles(const std::string& directory)
	{
		std::vector<std::string> files;
		for (auto& source : sources)
		{
			source->listFiles(directory, files);
		}

		return files;
	}


	void FileIO::addDirectory(const std::string& directory)
	{
		sources.push_back(new DirSource(directory));
	}

	void FileIO::addGrf(const std::string& grf)
	{
		sources.push_back(new GrfSource(grf));
	}


	void FileIO::end()
	{

	}


	////////GRF
	std::string FileIO::GrfSource::sanitizeFileName(std::string fileName)
	{
		std::transform(fileName.begin(), fileName.end(), fileName.begin(), ::tolower);
		std::replace(fileName.begin(), fileName.end(), '/', '\\');

		std::size_t index;
		while ((index = fileName.find("\\\\")) != std::string::npos)
			fileName = fileName.substr(0, index) + fileName.substr(index + 1);
		return fileName;
	}


	FileIO::GrfSource::GrfSource(const std::string& fileName) : grfFile(fileName)
	{
		std::cout << "GRF: Loading " << fileName << std::endl;
		GrfError error;
		grf = grf_open(fileName.c_str(), "rb", &error);
		if (grf == NULL)
		{
			std::cerr << "Error opening GRF file: " << grfFile << std::endl;
			return;
		}
		for (unsigned int i = 0; i < grf->nfiles; i++)
			lookup[sanitizeFileName(grf->files[i].name)] = i;
		std::cout << "GRF: " << lookup.size() << " files loaded" << std::endl;
	}

	void FileIO::GrfSource::close()
	{
		grf_close(grf);
		grf = nullptr;
	}

	std::istream* FileIO::GrfSource::open(const std::string& fileName)
	{
		if (!grf)
			throw "error";
		auto it = lookup.find(sanitizeFileName(fileName));
		if (it == lookup.end())
			throw "error";

		GrfError error;
		unsigned int size = 0;
		char* data = (char*)grf_index_get(grf, it->second, &size, &error);
		auto ss = new std::istringstream(std::string(data, size));
		//todo: check if data should really not be freed
		return ss;
	}

	bool FileIO::GrfSource::exists(const std::string& fileName)
	{
		return lookup.find(sanitizeFileName(fileName)) != lookup.end();
	}

	void FileIO::GrfSource::listFiles(const std::string& directory, std::vector<std::string>& files)
	{
		for (auto kv : lookup)
		{
			auto filename = kv.first;
			if (filename.substr(0, directory.size()) == directory)
			{
				filename = filename.substr(directory.size());
				if (filename[0] == '\\')
					filename = filename.substr(1);
				if (filename.find("\\") == std::string::npos)
					if (std::find(files.begin(), files.end(), kv.first) == files.end())
						files.push_back(kv.first);
			}
		}
	}



	///////////////////////File
	FileIO::DirSource::DirSource(const std::string& dir) : directory(dir)
	{
		std::cout << "Using directory " << dir << std::endl;
	}
	void FileIO::DirSource::close()
	{
	}

	std::istream* FileIO::DirSource::open(const std::string& fileName)
	{
		return new std::ifstream(directory + fileName, std::ios_base::in | std::ios_base::binary);
	}

	bool FileIO::DirSource::exists(const std::string& fileName)
	{
		return std::filesystem::exists(directory + fileName); //TODO
	}

	void FileIO::DirSource::listFiles(const std::string& directory, std::vector<std::string>& files)
	{
	}



	std::string FileIO::readString(std::istream* is, int maxLength, int length)
	{
		char* buf = new char[maxLength];
		is->read(buf, maxLength);
		std::string ret(buf);
		if (length > -1)
			ret = std::string(buf, length);
		delete[] buf;
		return ret;
	}



}