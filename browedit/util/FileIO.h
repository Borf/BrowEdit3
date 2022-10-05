#pragma once

#include <json.hpp>
#include <string>
#include <vector>
#include <map>
#include <set>
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
			virtual void listAllFiles(std::vector<std::string>&) = 0;
			virtual std::string toString() = 0;
		};
		class GrfSource : public Source
		{
		private:
			std::string grfFile;
			std::string grfFileName;
			Grf* grf;
			std::map<std::string, int> lookup;
		public:
			GrfSource(const std::string& grfFile);
			bool exists(const std::string& file) override;
			std::istream* open(const std::string& file) override;
			void close() override;
			void listFiles(const std::string& directory, std::vector<std::string>&) override;
			void listAllFiles(std::vector<std::string>&) override;
			virtual std::string toString() override;
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
			void listAllFiles(std::vector<std::string>&) override;
			virtual std::string toString() override;
		};

		static std::vector<Source*> sources;
	public:
		// initialisation
		static void begin();
		static void addGrf(const std::string&);
		static void addDirectory(const std::string&);
		static void end();

		static void reload(const std::string&); // to reload a path

		// FileIO for opening from GRF
		static std::istream* open(const std::string& fileName);
		static bool exists(const std::string& fileName);
		static std::vector<std::string> listFiles(const std::string& directory);
		static std::vector<std::string> listAllFiles();
		static std::string getSrc(const std::string& fileName);

		//helper methods
		static std::string readString(std::istream* is, int maxLength, int length = -1);
		static std::string readStringDyn(std::istream* is);
		static void writeString(std::ostream& os, const std::string &data, int length);
		static nlohmann::json getJson(const std::string& fileName);
		static std::string getString(const std::string& fileName);

		class Node
		{
		public:
			Node* parent = nullptr;
			std::string name;							//name is in utf8
			std::map<std::string, Node*> directories;	//map is in kr
			std::set<std::string> files;				//files are in utf8
			Node(const std::string& name, Node* parent) : name(name), parent(parent) {}
			Node* addFile(const std::string& fileName);
			Node* getDirectory(const std::string& directory);
		};
		static inline Node* rootNode = nullptr;
		static Node* directoryNode(const std::string& directory);
	};








}