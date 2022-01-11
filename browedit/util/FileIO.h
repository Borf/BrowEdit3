#pragma once

#include <string>
#include <vector>
#include <map>
#include <list>
#include <sstream>
#include <grf.h>

namespace util
{
	class FileIO
	{
		class Source
		{
		public:
			virtual bool exists(const std::string& file) = 0;
			virtual std::istream* open(const std::string& file) = 0;
			virtual void close() = 0;
			virtual void listFiles(const std::string& directory, std::vector<std::string>&) = 0;
		};
		class GrfSource : public Source
		{
		private:
			std::string grfFile;
			Grf* grf;
			std::map<std::string, int> lookup;
		public:
			GrfSource(const std::string& grfFile);
			bool exists(const std::string& file) override;
			std::istream* open(const std::string& file) override;
			void close() override;
			void listFiles(const std::string& directory, std::vector<std::string>&) override;
		private:
			std::string sanitizeFileName(std::string fileName);
		};
		class DirSource : public Source
		{
		private:
			std::string directory;
		public:
			DirSource(const std::string& directory);
			bool exists(const std::string& file) override;
			std::istream* open(const std::string& file) override;
			void close() override;
			void listFiles(const std::string& directory, std::vector<std::string>&) override;
		};

		static std::vector<Source*> sources;
	public:
		// initialisation
		static void begin();
		static void addGrf(const std::string&);
		static void addDirectory(const std::string&);
		static void end();

		// FileIO for opening from GRF
		static std::istream* open(const std::string& fileName);
		static bool exists(const std::string& fileName);
		static std::vector<std::string> listFiles(const std::string& directory);

		//helper methods
		static std::string readString(std::istream* is, int maxLength, int length = -1);

	};








}