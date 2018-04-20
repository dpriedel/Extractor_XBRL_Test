// =====================================================================================
//
//       Filename:  Unit_Test.cpp
//
//    Description:  Driver program for Unit tests
//
//        Version:  1.0
//        Created:  04/16/2017 3:45:53 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (dpr), driedel@cox.net
//        License:  GNU General Public License v3
//        Company:
//
// =====================================================================================

	/* This file is part of CollectEDGARData. */

	/* CollectEDGARData is free software: you can redistribute it and/or modify */
	/* it under the terms of the GNU General Public License as published by */
	/* the Free Software Foundation, either version 3 of the License, or */
	/* (at your option) any later version. */

	/* CollectEDGARData is distributed in the hope that it will be useful, */
	/* but WITHOUT ANY WARRANTY; without even the implied warranty of */
	/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
	/* GNU General Public License for more details. */

	/* You should have received a copy of the GNU General Public License */
	/* along with CollectEDGARData.  If not, see <http://www.gnu.org/licenses/>. */


// =====================================================================================
//        Class:
//  Description:
// =====================================================================================


#include <string>
#include <chrono>
#include <thread>
#include <iostream>
#include <numeric>
#include <fstream>
#include <system_error>
#include <experimental/filesystem>

#include <boost/algorithm/string/predicate.hpp>
#include <gmock/gmock.h>

#include "Poco/Util/Application.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"
#include "Poco/Util/AbstractConfiguration.h"
#include "Poco/AutoPtr.h"
#include <Poco/Net/NetException.h>
#include "Poco/Logger.h"
#include "Poco/Channel.h"
#include "Poco/Message.h"

#include "Poco/ConsoleChannel.h"
// #include "Poco/SimpleFileChannel.h"

namespace fs = std::experimental::filesystem;

using namespace testing;


using Poco::Util::Application;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::HelpFormatter;
using Poco::Util::AbstractConfiguration;
using Poco::Util::OptionCallback;
using Poco::AutoPtr;

#include "EDGAR_XML_FileFilter.h"
#include "SEC_Header.h"


// some specific files for Testing.

fs::path FILE_WITH_XML_10Q{"/vol_DA/EDGAR/Archives/edgar/data/1460602/0001062993-13-005017.txt"};
fs::path FILE_WITH_XML_10K{"/vol_DA/EDGAR/Archives/edgar/data/google-10k.txt"};
fs::path FILE_WITHOUT_XML{"/vol_DA/EDGAR/Archives/edgar/data/841360/0001086380-13-000030.txt"};

//	need these to feed into testing framework.

int G_ARGC = 0;
char** G_ARGV = nullptr;

Poco::Logger* THE_LOGGER = nullptr;

// using one of the example Poco programs to get going

class XBRL_Extract_Unit_Test: public Application
	/// This sample demonstrates some of the features of the Util::Application class,
	/// such as configuration file handling and command line arguments processing.
	///
	/// Try XBRL_Extract_Unit_Test --help (on Unix platforms) or XBRL_Extract_Unit_Test /help (elsewhere) for
	/// more information.
{
public:
	XBRL_Extract_Unit_Test(): _helpRequested(false)
	{
	}

protected:
	void initialize(Application& self)
	{
		loadConfiguration(); // load default configuration files, if present
		Application::initialize(self);
		// add your own initialization code here
	}

	void uninitialize()
	{
		// add your own uninitialization code here
		Application::uninitialize();
	}

	void reinitialize(Application& self)
	{
		Application::reinitialize(self);
		// add your own reinitialization code here
	}

	void defineOptions(OptionSet& options)
	{
		Application::defineOptions(options);

		options.addOption(
			Option("help", "h", "display help information on command line arguments")
				.required(false)
				.repeatable(false)
				.callback(OptionCallback<XBRL_Extract_Unit_Test>(this, &XBRL_Extract_Unit_Test::handleHelp)));

		options.addOption(
			Option("gtest_filter", "", "select which tests to run.")
				.required(false)
				.repeatable(true)
				.argument("name=value")
				.callback(OptionCallback<XBRL_Extract_Unit_Test>(this, &XBRL_Extract_Unit_Test::handleDefine)));

		/* options.addOption( */
		/* 	Option("define", "D", "define a configuration property") */
		/* 		.required(false) */
		/* 		.repeatable(true) */
		/* 		.argument("name=value") */
		/* 		.callback(OptionCallback<XBRL_Extract_Unit_Test>(this, &XBRL_Extract_Unit_Test::handleDefine))); */

		/* options.addOption( */
		/* 	Option("config-file", "f", "load configuration data from a file") */
		/* 		.required(false) */
		/* 		.repeatable(true) */
		/* 		.argument("file") */
		/* 		.callback(OptionCallback<XBRL_Extract_Unit_Test>(this, &XBRL_Extract_Unit_Test::handleConfig))); */

		/* options.addOption( */
		/* 	Option("bind", "b", "bind option value to test.property") */
		/* 		.required(false) */
		/* 		.repeatable(false) */
		/* 		.argument("value") */
		/* 		.binding("test.property")); */
	}

	void handleHelp(const std::string& name, const std::string& value)
	{
		_helpRequested = true;
		displayHelp();
		stopOptionsProcessing();
	}

	void handleDefine(const std::string& name, const std::string& value)
	{
		defineProperty(value);
	}

	void handleConfig(const std::string& name, const std::string& value)
	{
		loadConfiguration(value);
	}

	void displayHelp()
	{
		HelpFormatter helpFormatter(options());
		helpFormatter.setCommand(commandName());
		helpFormatter.setUsage("OPTIONS");
		helpFormatter.setHeader("A sample application that demonstrates some of the features of the Poco::Util::Application class.");
		helpFormatter.format(std::cout);
	}

	void defineProperty(const std::string& def)
	{
		std::string name;
		std::string value;
		std::string::size_type pos = def.find('=');
		if (pos != std::string::npos)
		{
			name.assign(def, 0, pos);
			value.assign(def, pos + 1, def.length() - pos);
		}
		else name = def;
		config().setString(name, value);
	}

	int main(const ArgVec& args)
	{
        setLogger(*THE_LOGGER);
		if (!_helpRequested)
		{
			logger().information("Command line:");
			std::ostringstream ostr;
			for (ArgVec::const_iterator it = argv().begin(); it != argv().end(); ++it)
			{
				ostr << *it << ' ';
			}
			logger().information(ostr.str());
			logger().information("Arguments to main():");
			for (ArgVec::const_iterator it = args.begin(); it != args.end(); ++it)
			{
				logger().information(*it);
			}
			logger().information("Application properties:");
			printProperties("");

			//	run our tests

			testing::InitGoogleMock(&G_ARGC, G_ARGV);
			return RUN_ALL_TESTS();
		}
		return Application::EXIT_OK;
	}

	void printProperties(const std::string& base)
	{
		AbstractConfiguration::Keys keys;
		config().keys(base, keys);
		if (keys.empty())
		{
			if (config().hasProperty(base))
			{
				std::string msg;
				msg.append(base);
				msg.append(" = ");
				msg.append(config().getString(base));
				logger().information(msg);
			}
		}
		else
		{
			for (AbstractConfiguration::Keys::const_iterator it = keys.begin(); it != keys.end(); ++it)
			{
				std::string fullKey = base;
				if (!fullKey.empty()) fullKey += '.';
				fullKey.append(*it);
				printProperties(fullKey);
			}
		}
	}

private:
	bool _helpRequested;
};

//  some utility routines used to check test output.

MATCHER_P(StringEndsWith, value, std::string(negation ? "doesn't" : "does") +
                        " end with " + value)
{
	return boost::algorithm::ends_with(arg, value);
}

int CountFilesInDirectoryTree(const fs::path& directory)
{
	int count = std::count_if(fs::recursive_directory_iterator(directory), fs::recursive_directory_iterator(),
			[](const fs::directory_entry& entry) { return entry.status().type() == fs::file_type::regular; });
	return count;
}

std::map<std::string, fs::file_time_type> CollectLastModifiedTimesForFilesInDirectory(const fs::path& directory)
{
	std::map<std::string, fs::file_time_type> results;

    auto save_mod_time([&results] (const auto& dir_ent)
    {
		if (dir_ent.status().type() == fs::file_type::regular)
			results[dir_ent.path().filename().string()] = fs::last_write_time(dir_ent.path());
    });

    std::for_each(fs::directory_iterator(directory), fs::directory_iterator(), save_mod_time);

	return results;
}

std::map<std::string, fs::file_time_type> CollectLastModifiedTimesForFilesInDirectoryTree(const fs::path& directory)
{
	std::map<std::string, fs::file_time_type> results;

    auto save_mod_time([&results] (const auto& dir_ent)
    {
		if (dir_ent.status().type() == fs::file_type::regular)
			results[dir_ent.path().filename().string()] = fs::last_write_time(dir_ent.path());
    });

    std::for_each(fs::recursive_directory_iterator(directory), fs::recursive_directory_iterator(), save_mod_time);

	return results;
}

// int CountTotalFormsFilesFound(const FormFileRetriever::FormsAndFilesList& file_list)
// {
// 	int grand_total{0};
//     // grand_total = std::accumulate(std::begin(file_list), std::end(file_list), 0, [] (auto a, const auto& b) { return a + b.second.size(); });
// 	for (const auto& [form_type, form_list] : file_list)
// 		grand_total += form_list.size();
//
// 	return grand_total;
// }

// NOTE: for some of these tests, I run an HTTPS server on localhost using
// a directory structure that mimics part of the SEC server.
//
class IdentifyXMLFilesToUse : public Test
{

};

TEST_F(IdentifyXMLFilesToUse, ConfirmFileHasXML)
{
    std::ifstream input_file{FILE_WITH_XML_10Q};
    const std::string file_content{std::istreambuf_iterator<char>{input_file}, std::istreambuf_iterator<char>{}};
    input_file.close();

	auto use_file = UseEDGAR_File(file_content);
	ASSERT_THAT(use_file, Eq(true));
}

TEST_F(IdentifyXMLFilesToUse, ConfirmFileHasNOXML)
{
    std::ifstream input_file{FILE_WITHOUT_XML};
    const std::string file_content{std::istreambuf_iterator<char>{input_file}, std::istreambuf_iterator<char>{}};
    input_file.close();

	auto use_file = UseEDGAR_File(file_content);
	ASSERT_THAT(use_file, Eq(false));
}

class ValidateCanNavigateDocumentStructure : public Test
{
};

TEST_F(ValidateCanNavigateDocumentStructure, FindSECHeader_10Q)
{
    std::ifstream input_file{FILE_WITH_XML_10Q};
    const std::string file_content{std::istreambuf_iterator<char>{input_file}, std::istreambuf_iterator<char>{}};
    input_file.close();

	SEC_Header SEC_data;

	ASSERT_NO_THROW(SEC_data.UseData(file_content));
}

TEST_F(ValidateCanNavigateDocumentStructure, FindSECHeader_10K)
{
    std::ifstream input_file{FILE_WITH_XML_10K};
    const std::string file_content{std::istreambuf_iterator<char>{input_file}, std::istreambuf_iterator<char>{}};
    input_file.close();

	SEC_Header SEC_data;

	ASSERT_NO_THROW(SEC_data.UseData(file_content));
}

TEST_F(ValidateCanNavigateDocumentStructure, SECHeaderFindAllFields_10Q)
{
    std::ifstream input_file{FILE_WITH_XML_10Q};
    const std::string file_content{std::istreambuf_iterator<char>{input_file}, std::istreambuf_iterator<char>{}};
    input_file.close();

	SEC_Header SEC_data;
	SEC_data.UseData(file_content);
	ASSERT_NO_THROW(SEC_data.ExtractHeaderFields());
}

TEST_F(ValidateCanNavigateDocumentStructure, FindsAllDocumentSections_10Q)
{
    std::ifstream input_file{FILE_WITH_XML_10Q};
    const std::string file_content{std::istreambuf_iterator<char>{input_file}, std::istreambuf_iterator<char>{}};
    input_file.close();

	auto result = LocateDocumentSections(file_content);

	ASSERT_EQ(result.size(), 52);
}

TEST_F(ValidateCanNavigateDocumentStructure, FindsAllDocumentSections_10K)
{
    std::ifstream input_file{FILE_WITH_XML_10K};
    const std::string file_content{std::istreambuf_iterator<char>{input_file}, std::istreambuf_iterator<char>{}};
    input_file.close();

	auto result = LocateDocumentSections(file_content);

	ASSERT_EQ(result.size(), 119);
}

class LocateFileContentToUse : public Test
{
	
};

TEST_F(LocateFileContentToUse, FindInstanceDocument_10Q)
{
    std::ifstream input_file_10Q{FILE_WITH_XML_10Q};
    const std::string file_content_10Q{std::istreambuf_iterator<char>{input_file_10Q}, std::istreambuf_iterator<char>{}};
	std::vector<std::string_view> document_sections_10Q{LocateDocumentSections(file_content_10Q)};

	auto instance_document = LocateInstanceDocument(document_sections_10Q);
	ASSERT_GT(instance_document.size(), 0);
}

TEST_F(LocateFileContentToUse, FindInstanceDocument_10K)
{
    std::ifstream input_file_10K{FILE_WITH_XML_10K};
    const std::string file_content_10K{std::istreambuf_iterator<char>{input_file_10K}, std::istreambuf_iterator<char>{}};
	std::vector<std::string_view> document_sections_10K{LocateDocumentSections(file_content_10K)};

	auto instance_document = LocateInstanceDocument(document_sections_10K);
	ASSERT_GT(instance_document.size(), 0);
}

TEST_F(LocateFileContentToUse, FindLabelDocument_10Q)
{
    std::ifstream input_file_10Q{FILE_WITH_XML_10Q};
    const std::string file_content_10Q{std::istreambuf_iterator<char>{input_file_10Q}, std::istreambuf_iterator<char>{}};
	std::vector<std::string_view> document_sections_10Q{LocateDocumentSections(file_content_10Q)};

	auto label_document = LocateLabelDocument(document_sections_10Q);
	ASSERT_GT(label_document.size(), 0);
}

TEST_F(LocateFileContentToUse, FindLabelDocument_10K)
{
    std::ifstream input_file_10K{FILE_WITH_XML_10K};
    const std::string file_content_10K{std::istreambuf_iterator<char>{input_file_10K}, std::istreambuf_iterator<char>{}};
	std::vector<std::string_view> document_sections_10K{LocateDocumentSections(file_content_10K)};

	auto label_document = LocateLabelDocument(document_sections_10K);
	ASSERT_GT(label_document.size(), 0);
}

class ParseFileContent : public Test
{

};

TEST(ParseFileContent, VerifyCanParseInstanceDocument_10Q)
{
    std::ifstream input_file_10Q{FILE_WITH_XML_10Q};
    const std::string file_content_10Q{std::istreambuf_iterator<char>{input_file_10Q}, std::istreambuf_iterator<char>{}};
	std::vector<std::string_view> document_sections_10Q{LocateDocumentSections(file_content_10Q)};

	ASSERT_NO_THROW(LocateInstanceDocument(document_sections_10Q));
}

TEST(ParseFileContent, VerifyCanParseInstanceDocument_10K)
{
    std::ifstream input_file_10K{FILE_WITH_XML_10K};
    const std::string file_content_10K{std::istreambuf_iterator<char>{input_file_10K}, std::istreambuf_iterator<char>{}};
	std::vector<std::string_view> document_sections_10K{LocateDocumentSections(file_content_10K)};

	ASSERT_NO_THROW(LocateInstanceDocument(document_sections_10K));
}


int main(int argc, char** argv)
{
	G_ARGC = argc;
	G_ARGV = argv;

    int result = 0;

	XBRL_Extract_Unit_Test the_app;
	try
	{
		the_app.init(argc, argv);

        THE_LOGGER = &Poco::Logger::get("TestLogger");
        AutoPtr<Poco::Channel> pChannel(new Poco::ConsoleChannel);
        // pChannel->setProperty("path", "/tmp/Testing.log");
        THE_LOGGER->setChannel(pChannel);
        THE_LOGGER->setLevel(Poco::Message::PRIO_DEBUG);

    	result = the_app.run();
	}
	catch (Poco::Exception& exc)
	{
		the_app.logger().log(exc);
		result =  Application::EXIT_CONFIG;
	}

    return result;
}
