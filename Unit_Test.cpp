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


#include <algorithm>
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

const fs::path FILE_WITH_XML_10Q{"/vol_DA/EDGAR/Archives/edgar/data/1460602/0001062993-13-005017.txt"};
const fs::path FILE_WITH_XML_10K{"/vol_DA/EDGAR/Archives/edgar/data/google-10k.txt"};
const fs::path FILE_WITHOUT_XML{"/vol_DA/EDGAR/Archives/edgar/data/841360/0001086380-13-000030.txt"};
const fs::path EDGAR_DIRECTORY{"/vol_DA/EDGAR/Archives/edgar/data"};
const fs::path FILE_NO_NAMESPACE_10Q{"/vol_DA/EDGAR/Archives/edgar/data/68270/0000068270-13-000059.txt"};
const fs::path FILE_SOME_NAMESPACE_10Q{"/vol_DA/EDGAR/Archives/edgar/data/1552979/0001214782-13-000386.txt"};
const fs::path FILE_MULTIPLE_LABEL_LINKS{"/vol_DA/EDGAR/Archives/edgar/data/1540334/0001078782-13-002015.txt"};
const fs::path BAD_FILE1{"/vol_DA/EDGAR/Edgar_forms/1000228/10-K/0001000228-11-000014.txt"};

// some utility functions for testing.

bool FindAllLabels(const std::vector<EE::GAAP_Data>& gaap_data, const EE::EDGAR_Labels& labels)
{
	if (gaap_data.empty())
	{
		std::cout << "Empty GAAP data." << "\n";
		return false;
	}
	if (labels.empty())
	{
		std::cout << "Empty EDGAR Labels data." << "\n";
		return false;
	}

	bool all_good = true;
	for (const auto& e : gaap_data)
	{
		// auto pos = std::find_if(std::begin(labels), std::end(labels), [e](const auto& l){return l.system_label == e.label;});
		auto pos = labels.find(e.label);
		if (pos == labels.end())
		{
			std::cout << "Can't find: " << e.label << '\n';
			all_good = false;
		}
	}
	return all_good;
}

bool FindAllContexts(const std::vector<EE::GAAP_Data>& gaap_data, const EE::ContextPeriod& contexts)
{
	if (gaap_data.empty())
	{
		std::cout << "Empty GAAP data." << "\n";
		return false;
	}
	if (contexts.empty())
	{
		std::cout << "Empty Context data." << "\n";
		return false;
	}

	bool all_good = true;
	for (const auto& e : gaap_data)
	{
		// auto pos = std::find_if(std::begin(contexts), std::end(contexts), [e](const auto& c){return c.context_ID == e.context_ID;});
        auto pos = contexts.find(e.context_ID);
		if (pos == contexts.end())
		{
			std::cout << "Can't find: " << e.context_ID << '\n';
			all_good = false;
		}
	}
	return all_good;
}

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

class IdentifyXMLFilesToUse : public Test
{

};

TEST_F(IdentifyXMLFilesToUse, ConfirmFileHasXML)
{
	std::string file_content_10Q(fs::file_size(FILE_WITH_XML_10Q), '\0');
	std::ifstream input_file{FILE_WITH_XML_10Q, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10Q[0], file_content_10Q.size());
	input_file.close();

	FileHasXBRL filter1;
	auto use_file = filter1(EE::SEC_Header_fields{}, file_content_10Q);
	ASSERT_THAT(use_file, Eq(true));
}

TEST_F(IdentifyXMLFilesToUse, ConfirmFileHasNOXML)
{
	std::string file_content_10Q(fs::file_size(FILE_WITHOUT_XML), '\0');
	std::ifstream input_file{FILE_WITHOUT_XML, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10Q[0], file_content_10Q.size());
	input_file.close();

	FileHasXBRL filter1;
	auto use_file = filter1(EE::SEC_Header_fields{}, file_content_10Q);
	ASSERT_THAT(use_file, Eq(false));
}

class ValidateCanNavigateDocumentStructure : public Test
{
};

TEST_F(ValidateCanNavigateDocumentStructure, FindSECHeader_10Q)
{
	std::string file_content_10Q(fs::file_size(FILE_WITH_XML_10Q), '\0');
	std::ifstream input_file{FILE_WITH_XML_10Q, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10Q[0], file_content_10Q.size());
	input_file.close();

	SEC_Header SEC_data;

	ASSERT_NO_THROW(SEC_data.UseData(file_content_10Q));
}

TEST_F(ValidateCanNavigateDocumentStructure, FindSECHeader_10K)
{
	std::string file_content_10K(fs::file_size(FILE_WITH_XML_10K), '\0');
	std::ifstream input_file{FILE_WITH_XML_10K, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10K[0], file_content_10K.size());
	input_file.close();

	SEC_Header SEC_data;

	ASSERT_NO_THROW(SEC_data.UseData(file_content_10K));
}

TEST_F(ValidateCanNavigateDocumentStructure, SECHeaderFindAllFields_10Q)
{
	std::string file_content_10Q(fs::file_size(FILE_WITH_XML_10Q), '\0');
	std::ifstream input_file{FILE_WITH_XML_10Q, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10Q[0], file_content_10Q.size());
	input_file.close();

	SEC_Header SEC_data;
	SEC_data.UseData(file_content_10Q);
	ASSERT_NO_THROW(SEC_data.ExtractHeaderFields());
}

TEST_F(ValidateCanNavigateDocumentStructure, FindsAllDocumentSections_10Q)
{
	std::string file_content_10Q(fs::file_size(FILE_WITH_XML_10Q), '\0');
	std::ifstream input_file{FILE_WITH_XML_10Q, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10Q[0], file_content_10Q.size());
	input_file.close();

	auto result = LocateDocumentSections(file_content_10Q);

	ASSERT_EQ(result.size(), 52);
}

TEST_F(ValidateCanNavigateDocumentStructure, FindsAllDocumentSections_10K)
{
	std::string file_content_10K(fs::file_size(FILE_WITH_XML_10K), '\0');
	std::ifstream input_file{FILE_WITH_XML_10K, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10K[0], file_content_10K.size());
	input_file.close();

	auto result = LocateDocumentSections(file_content_10K);

	ASSERT_EQ(result.size(), 119);
}

class LocateFileContentToUse : public Test
{

};

TEST_F(LocateFileContentToUse, FindInstanceDocument_10Q)
{
	std::string file_content_10Q(fs::file_size(FILE_WITH_XML_10Q), '\0');
	std::ifstream input_file{FILE_WITH_XML_10Q, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10Q[0], file_content_10Q.size());
	input_file.close();

	std::vector<std::string_view> document_sections_10Q{LocateDocumentSections(file_content_10Q)};

	auto instance_document = LocateInstanceDocument(document_sections_10Q);
	ASSERT_TRUE(boost::algorithm::starts_with(instance_document, "<?xml version") && boost::algorithm::ends_with(instance_document, "</xbrl>\n"));
}

TEST_F(LocateFileContentToUse, FindInstanceDocument_10K)
{
	std::string file_content_10K(fs::file_size(FILE_WITH_XML_10K), '\0');
	std::ifstream input_file{FILE_WITH_XML_10K, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10K[0], file_content_10K.size());
	input_file.close();

	std::vector<std::string_view> document_sections_10K{LocateDocumentSections(file_content_10K)};

	auto instance_document = LocateInstanceDocument(document_sections_10K);
	ASSERT_TRUE(boost::algorithm::starts_with(instance_document, "<?xml version") && boost::algorithm::ends_with(instance_document, "</xbrli:xbrl>\n"));
}

TEST_F(LocateFileContentToUse, FindLabelDocument_10Q)
{
	std::string file_content_10Q(fs::file_size(FILE_WITH_XML_10Q), '\0');
	std::ifstream input_file{FILE_WITH_XML_10Q, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10Q[0], file_content_10Q.size());
	input_file.close();

	std::vector<std::string_view> document_sections_10Q{LocateDocumentSections(file_content_10Q)};

	auto labels_document = LocateLabelDocument(document_sections_10Q);
	ASSERT_TRUE(boost::algorithm::starts_with(labels_document, "<?xml version") && boost::algorithm::ends_with(labels_document, "</link:linkbase>\n"));
}

TEST_F(LocateFileContentToUse, FindLabelDocument_10K)
{
	std::string file_content_10K(fs::file_size(FILE_WITH_XML_10K), '\0');
	std::ifstream input_file{FILE_WITH_XML_10K, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10K[0], file_content_10K.size());
	input_file.close();

	std::vector<std::string_view> document_sections_10K{LocateDocumentSections(file_content_10K)};

	auto labels_document = LocateLabelDocument(document_sections_10K);
	ASSERT_TRUE(boost::algorithm::starts_with(labels_document, "<?xml version") && boost::algorithm::ends_with(labels_document, "</link:linkbase>\n"));
}

class ParseDocumentContent : public Test
{

};

TEST_F(ParseDocumentContent, VerifyCanParseInstanceDocument_10Q)
{
	std::string file_content_10Q(fs::file_size(FILE_WITH_XML_10Q), '\0');
	std::ifstream input_file{FILE_WITH_XML_10Q, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10Q[0], file_content_10Q.size());
	input_file.close();

	std::vector<std::string_view> document_sections_10Q{LocateDocumentSections(file_content_10Q)};

	auto instance_document = LocateInstanceDocument(document_sections_10Q);
	ASSERT_NO_THROW(ParseXMLContent(instance_document));
}

TEST_F(ParseDocumentContent, VerifyCanParseInstanceDocument_10K)
{
	std::string file_content_10K(fs::file_size(FILE_WITH_XML_10K), '\0');
	std::ifstream input_file{FILE_WITH_XML_10K, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10K[0], file_content_10K.size());
	input_file.close();

	std::vector<std::string_view> document_sections_10K{LocateDocumentSections(file_content_10K)};

	auto instance_document = LocateInstanceDocument(document_sections_10K);
	ASSERT_NO_THROW(ParseXMLContent(instance_document));
}

TEST_F(ParseDocumentContent, VerifyParseBadInstanceDocumentThrows_10K)
{
	std::string file_content_10K(fs::file_size(BAD_FILE1), '\0');
	std::ifstream input_file{BAD_FILE1, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10K[0], file_content_10K.size());
	input_file.close();

	std::vector<std::string_view> document_sections_10K{LocateDocumentSections(file_content_10K)};

	auto instance_document = LocateInstanceDocument(document_sections_10K);
	ASSERT_THROW(ParseXMLContent(instance_document), ExtractException);
}

TEST_F(ParseDocumentContent, VerifyCanParseLabelsDocument_10Q)
{
	std::string file_content_10Q(fs::file_size(FILE_WITH_XML_10Q), '\0');
	std::ifstream input_file{FILE_WITH_XML_10Q, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10Q[0], file_content_10Q.size());
	input_file.close();

	std::vector<std::string_view> document_sections_10Q{LocateDocumentSections(file_content_10Q)};

	auto labels_document = LocateLabelDocument(document_sections_10Q);
	ASSERT_NO_THROW(ParseXMLContent(labels_document));
}

TEST_F(ParseDocumentContent, VerifyCanParseLabelsDocument_10K)
{
	std::string file_content_10K(fs::file_size(FILE_WITH_XML_10K), '\0');
	std::ifstream input_file{FILE_WITH_XML_10K, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10K[0], file_content_10K.size());
	input_file.close();

	std::vector<std::string_view> document_sections_10K{LocateDocumentSections(file_content_10K)};

	auto labels_document = LocateLabelDocument(document_sections_10K);
	ASSERT_NO_THROW(ParseXMLContent(labels_document));
}

class ExtractDocumentContent : public Test
{

};

template<typename ...Ts>
auto AllNotEmpty(Ts ...ts)
{
	return ((! ts.empty()) && ...);
}

TEST_F(ExtractDocumentContent, VerifyCanExtractFilingData_10Q)
{
	std::string file_content_10Q(fs::file_size(FILE_WITH_XML_10Q), '\0');
	std::ifstream input_file{FILE_WITH_XML_10Q, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10Q[0], file_content_10Q.size());
	input_file.close();

	std::vector<std::string_view> document_sections_10Q{LocateDocumentSections(file_content_10Q)};

	auto instance_document = LocateInstanceDocument(document_sections_10Q);
	auto instance_xml = ParseXMLContent(instance_document);

	const auto& [a, b, c, d] = ExtractFilingData(instance_xml);

	ASSERT_TRUE(AllNotEmpty(a, b, c, d));
}

TEST_F(ExtractDocumentContent, VerifyCanExtractFilingData_10K)
{
	std::string file_content_10K(fs::file_size(FILE_WITH_XML_10K), '\0');
	std::ifstream input_file{FILE_WITH_XML_10K, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10K[0], file_content_10K.size());
	input_file.close();

	std::vector<std::string_view> document_sections_10K{LocateDocumentSections(file_content_10K)};

	auto instance_document = LocateInstanceDocument(document_sections_10K);
	auto instance_xml = ParseXMLContent(instance_document);

	const auto& [a, b, c, d] = ExtractFilingData(instance_xml);

	ASSERT_TRUE(AllNotEmpty(a, b, c, d));
}

TEST_F(ExtractDocumentContent, VerifyCanExtractGAAP_10Q)
{
	std::string file_content_10Q(fs::file_size(FILE_WITH_XML_10Q), '\0');
	std::ifstream input_file{FILE_WITH_XML_10Q, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10Q[0], file_content_10Q.size());
	input_file.close();

	std::vector<std::string_view> document_sections_10Q{LocateDocumentSections(file_content_10Q)};

	auto instance_document = LocateInstanceDocument(document_sections_10Q);
	auto instance_xml = ParseXMLContent(instance_document);

    auto gaap_data = ExtractGAAPFields(instance_xml);

	ASSERT_EQ(gaap_data.size(), 194);
}

TEST_F(ExtractDocumentContent, VerifyCanExtractGAAPNoNamespace_10Q)
{
	std::string file_content_10Q(fs::file_size(FILE_NO_NAMESPACE_10Q), '\0');
	std::ifstream input_file{FILE_NO_NAMESPACE_10Q, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10Q[0], file_content_10Q.size());
	input_file.close();

	std::vector<std::string_view> document_sections_10Q{LocateDocumentSections(file_content_10Q)};

	auto instance_document = LocateInstanceDocument(document_sections_10Q);
	auto instance_xml = ParseXMLContent(instance_document);

    auto gaap_data = ExtractGAAPFields(instance_xml);

	ASSERT_EQ(gaap_data.size(), 723);
}

TEST_F(ExtractDocumentContent, VerifyCanExtractGAAP_10K)
{
	std::string file_content_10K(fs::file_size(FILE_WITH_XML_10K), '\0');
	std::ifstream input_file{FILE_WITH_XML_10K, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10K[0], file_content_10K.size());
	input_file.close();

	std::vector<std::string_view> document_sections_10K{LocateDocumentSections(file_content_10K)};

	auto instance_document = LocateInstanceDocument(document_sections_10K);
	auto instance_xml = ParseXMLContent(instance_document);

    auto gaap_data = ExtractGAAPFields(instance_xml);

	ASSERT_EQ(gaap_data.size(), 1984);
}

TEST_F(ExtractDocumentContent, VerifyCanExtractLabels_10Q)
{
	std::string file_content_10Q(fs::file_size(FILE_WITH_XML_10Q), '\0');
	std::ifstream input_file{FILE_WITH_XML_10Q, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10Q[0], file_content_10Q.size());
	input_file.close();

	std::vector<std::string_view> document_sections_10Q{LocateDocumentSections(file_content_10Q)};

	auto labels_document = LocateLabelDocument(document_sections_10Q);
	auto labels_xml = ParseXMLContent(labels_document);

    auto label_data = ExtractFieldLabels(labels_xml);

	ASSERT_EQ(label_data.size(), 125);
}

TEST_F(ExtractDocumentContent, VerifyCanExtractLabelsNoNamespace_10Q)
{
	std::string file_content_10Q(fs::file_size(FILE_NO_NAMESPACE_10Q), '\0');
	std::ifstream input_file{FILE_NO_NAMESPACE_10Q, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10Q[0], file_content_10Q.size());
	input_file.close();

	std::vector<std::string_view> document_sections_10Q{LocateDocumentSections(file_content_10Q)};

	auto labels_document = LocateLabelDocument(document_sections_10Q);
	auto labels_xml = ParseXMLContent(labels_document);

    auto label_data = ExtractFieldLabels(labels_xml);

	ASSERT_EQ(label_data.size(), 352);
}

TEST_F(ExtractDocumentContent, VerifyCanExtractLabelsMultipleLabelLinks_10Q)
{
	std::string file_content_10Q(fs::file_size(FILE_MULTIPLE_LABEL_LINKS), '\0');
	std::ifstream input_file{FILE_MULTIPLE_LABEL_LINKS, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10Q[0], file_content_10Q.size());
	input_file.close();

	std::vector<std::string_view> document_sections_10Q{LocateDocumentSections(file_content_10Q)};

	auto labels_document = LocateLabelDocument(document_sections_10Q);
	auto labels_xml = ParseXMLContent(labels_document);

    auto label_data = ExtractFieldLabels(labels_xml);

	ASSERT_EQ(label_data.size(), 158);
}

TEST_F(ExtractDocumentContent, VerifyCanExtractLabels_10K)
{
	std::string file_content_10K(fs::file_size(FILE_WITH_XML_10K), '\0');
	std::ifstream input_file{FILE_WITH_XML_10K, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10K[0], file_content_10K.size());
	input_file.close();

	std::vector<std::string_view> document_sections_10K{LocateDocumentSections(file_content_10K)};

	auto labels_document = LocateLabelDocument(document_sections_10K);
	auto labels_xml = ParseXMLContent(labels_document);

    auto label_data = ExtractFieldLabels(labels_xml);

	ASSERT_EQ(label_data.size(), 746);
}

TEST_F(ExtractDocumentContent, VerifyCanExtractContexts_10Q)
{
	std::string file_content_10Q(fs::file_size(FILE_WITH_XML_10Q), '\0');
	std::ifstream input_file{FILE_WITH_XML_10Q, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10Q[0], file_content_10Q.size());
	input_file.close();

	std::vector<std::string_view> document_sections_10Q{LocateDocumentSections(file_content_10Q)};

	auto instance_document = LocateInstanceDocument(document_sections_10Q);
	auto instance_xml = ParseXMLContent(instance_document);

    auto context_data = ExtractContextDefinitions(instance_xml);

	ASSERT_EQ(context_data.size(), 37);
}

TEST_F(ExtractDocumentContent, VerifyCanExtractContextsSomeNamespace_10Q)
{
	std::string file_content_10Q(fs::file_size(FILE_SOME_NAMESPACE_10Q), '\0');
	std::ifstream input_file{FILE_SOME_NAMESPACE_10Q, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10Q[0], file_content_10Q.size());
	input_file.close();

	std::vector<std::string_view> document_sections_10Q{LocateDocumentSections(file_content_10Q)};

	auto instance_document = LocateInstanceDocument(document_sections_10Q);
	auto instance_xml = ParseXMLContent(instance_document);

    auto context_data = ExtractContextDefinitions(instance_xml);

	ASSERT_EQ(context_data.size(), 12);
}

TEST_F(ExtractDocumentContent, VerifyCanExtractContexts_10K)
{
	std::string file_content_10K(fs::file_size(FILE_WITH_XML_10K), '\0');
	std::ifstream input_file{FILE_WITH_XML_10K, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10K[0], file_content_10K.size());
	input_file.close();

	std::vector<std::string_view> document_sections_10K{LocateDocumentSections(file_content_10K)};

	auto instance_document = LocateInstanceDocument(document_sections_10K);
	auto instance_xml = ParseXMLContent(instance_document);

    auto context_data = ExtractContextDefinitions(instance_xml);

	ASSERT_EQ(context_data.size(), 492);
}

TEST_F(ExtractDocumentContent, VerifyCanMatchGAAPDataWithUserLabel_10Q)
{
	std::string file_content_10Q(fs::file_size(FILE_WITH_XML_10Q), '\0');
	std::ifstream input_file{FILE_WITH_XML_10Q, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10Q[0], file_content_10Q.size());
	input_file.close();

	std::vector<std::string_view> document_sections_10Q{LocateDocumentSections(file_content_10Q)};

	auto labels_document = LocateLabelDocument(document_sections_10Q);
	auto labels_xml = ParseXMLContent(labels_document);

    auto label_data = ExtractFieldLabels(labels_xml);

	auto instance_document = LocateInstanceDocument(document_sections_10Q);
	auto instance_xml = ParseXMLContent(instance_document);

    auto gaap_data = ExtractGAAPFields(instance_xml);

	bool result = FindAllLabels(gaap_data, label_data);

	ASSERT_TRUE(result);
}

TEST_F(ExtractDocumentContent, VerifyCanMatchGAAPDataWithUserLabelNoNamespace_10Q)
{
	std::string file_content_10Q(fs::file_size(FILE_NO_NAMESPACE_10Q), '\0');
	std::ifstream input_file{FILE_NO_NAMESPACE_10Q, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10Q[0], file_content_10Q.size());
	input_file.close();

	std::vector<std::string_view> document_sections_10Q{LocateDocumentSections(file_content_10Q)};

	auto labels_document = LocateLabelDocument(document_sections_10Q);
	auto labels_xml = ParseXMLContent(labels_document);

    auto label_data = ExtractFieldLabels(labels_xml);

	auto instance_document = LocateInstanceDocument(document_sections_10Q);
	auto instance_xml = ParseXMLContent(instance_document);

    auto gaap_data = ExtractGAAPFields(instance_xml);

	bool result = FindAllLabels(gaap_data, label_data);

	ASSERT_TRUE(result);
}

TEST_F(ExtractDocumentContent, VerifyCanMatchGAAPDataWithUserLabelSomeNamespace_10Q)
{
	std::string file_content_10Q(fs::file_size(FILE_SOME_NAMESPACE_10Q), '\0');
	std::ifstream input_file{FILE_SOME_NAMESPACE_10Q, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10Q[0], file_content_10Q.size());
	input_file.close();

	std::vector<std::string_view> document_sections_10Q{LocateDocumentSections(file_content_10Q)};

	auto labels_document = LocateLabelDocument(document_sections_10Q);
	auto labels_xml = ParseXMLContent(labels_document);

    auto label_data = ExtractFieldLabels(labels_xml);

	auto instance_document = LocateInstanceDocument(document_sections_10Q);
	auto instance_xml = ParseXMLContent(instance_document);

    auto gaap_data = ExtractGAAPFields(instance_xml);

	bool result = FindAllLabels(gaap_data, label_data);

	ASSERT_TRUE(result);
}

TEST_F(ExtractDocumentContent, VerifyCanMatchGAAPDataWithUserLabel_10K)
{
	std::string file_content_10K(fs::file_size(FILE_WITH_XML_10K), '\0');
	std::ifstream input_file{FILE_WITH_XML_10K, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10K[0], file_content_10K.size());
	input_file.close();

	std::vector<std::string_view> document_sections_10K{LocateDocumentSections(file_content_10K)};

	auto labels_document = LocateLabelDocument(document_sections_10K);
	auto labels_xml = ParseXMLContent(labels_document);

    auto label_data = ExtractFieldLabels(labels_xml);

	auto instance_document = LocateInstanceDocument(document_sections_10K);
	auto instance_xml = ParseXMLContent(instance_document);

    auto context_data = ExtractContextDefinitions(instance_xml);
    auto gaap_data = ExtractGAAPFields(instance_xml);

	bool result = FindAllLabels(gaap_data, label_data);

	ASSERT_TRUE(result);
}

TEST_F(ExtractDocumentContent, VerifyCanMatchGAAPDataWithContext_10Q)
{
	std::string file_content_10Q(fs::file_size(FILE_WITH_XML_10Q), '\0');
	std::ifstream input_file{FILE_WITH_XML_10Q, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10Q[0], file_content_10Q.size());
	input_file.close();

	std::vector<std::string_view> document_sections_10Q{LocateDocumentSections(file_content_10Q)};

	auto labels_document = LocateLabelDocument(document_sections_10Q);

	auto instance_document = LocateInstanceDocument(document_sections_10Q);
	auto instance_xml = ParseXMLContent(instance_document);

    auto gaap_data = ExtractGAAPFields(instance_xml);

    auto context_data = ExtractContextDefinitions(instance_xml);

	bool result = FindAllContexts(gaap_data, context_data);

	ASSERT_TRUE(result);
}

TEST_F(ExtractDocumentContent, VerifyCanMatchGAAPDataWithContextSomeNamespace_10Q)
{
	std::string file_content_10Q(fs::file_size(FILE_SOME_NAMESPACE_10Q), '\0');
	std::ifstream input_file{FILE_SOME_NAMESPACE_10Q, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10Q[0], file_content_10Q.size());
	input_file.close();

	std::vector<std::string_view> document_sections_10Q{LocateDocumentSections(file_content_10Q)};

	auto labels_document = LocateLabelDocument(document_sections_10Q);

	auto instance_document = LocateInstanceDocument(document_sections_10Q);
	auto instance_xml = ParseXMLContent(instance_document);

    auto gaap_data = ExtractGAAPFields(instance_xml);

    auto context_data = ExtractContextDefinitions(instance_xml);

	bool result = FindAllContexts(gaap_data, context_data);

	ASSERT_TRUE(result);
}

TEST_F(ExtractDocumentContent, VerifyCanMatchGAAPDataWithContext_10K)
{
	std::string file_content_10K(fs::file_size(FILE_WITH_XML_10K), '\0');
	std::ifstream input_file{FILE_WITH_XML_10K, std::ios_base::in | std::ios_base::binary};
	input_file.read(&file_content_10K[0], file_content_10K.size());
	input_file.close();
	std::vector<std::string_view> document_sections_10K{LocateDocumentSections(file_content_10K)};

	auto labels_document = LocateLabelDocument(document_sections_10K);

	auto instance_document = LocateInstanceDocument(document_sections_10K);
	auto instance_xml = ParseXMLContent(instance_document);

    auto gaap_data = ExtractGAAPFields(instance_xml);

    auto context_data = ExtractContextDefinitions(instance_xml);

	bool result = FindAllContexts(gaap_data, context_data);

	ASSERT_TRUE(result);
}


class ValidateFolderFilters : public Test
{
public:

};

TEST_F(ValidateFolderFilters, VerifyFindAllXBRL)
{
	int files_with_XML{0};

    auto test_file([&files_with_XML](const auto& dir_ent)
    {
        if (dir_ent.status().type() == fs::file_type::regular)
        {
			std::string file_content(fs::file_size(dir_ent.path()), '\0');
			std::ifstream input_file{dir_ent.path(), std::ios_base::in | std::ios_base::binary};
			input_file.read(&file_content[0], file_content.size());
			input_file.close();

			FileHasXBRL filter;
			bool has_XML = filter(EE::SEC_Header_fields{}, file_content);
			if (has_XML)
				++files_with_XML;
		}
    });

    std::for_each(fs::recursive_directory_iterator(EDGAR_DIRECTORY), fs::recursive_directory_iterator(), test_file);

	ASSERT_EQ(files_with_XML, 159);
}

TEST_F(ValidateFolderFilters, VerifyFindAll10Q)
{
	int files_with_form{0};

    auto test_file([&files_with_form](const auto& dir_ent)
    {
        if (dir_ent.status().type() == fs::file_type::regular)
        {
			std::string file_content(fs::file_size(dir_ent.path()), '\0');
			std::ifstream input_file{dir_ent.path(), std::ios_base::in | std::ios_base::binary};
			input_file.read(&file_content[0], file_content.size());
			input_file.close();

			SEC_Header SEC_data;
			SEC_data.UseData(file_content);
			SEC_data.ExtractHeaderFields();

			FileHasXBRL filter1;
			std::vector<std::string_view> forms{"10-Q"};
			FileHasFormType filter2{forms};

			bool has_form = ApplyFilters(SEC_data.GetFields(), file_content, filter1, filter2);
			if (has_form)
				++files_with_form;
		}
    });

    std::for_each(fs::recursive_directory_iterator(EDGAR_DIRECTORY), fs::recursive_directory_iterator(), test_file);

	ASSERT_EQ(files_with_form, 157);
}

TEST_F(ValidateFolderFilters, VerifyFindAll10K)
{
	int files_with_form{0};

    auto test_file([&files_with_form](const auto& dir_ent)
    {
        if (dir_ent.status().type() == fs::file_type::regular)
        {
			std::string file_content(fs::file_size(dir_ent.path()), '\0');
			std::ifstream input_file{dir_ent.path(), std::ios_base::in | std::ios_base::binary};
			input_file.read(&file_content[0], file_content.size());
			input_file.close();

			SEC_Header SEC_data;
			SEC_data.UseData(file_content);
			SEC_data.ExtractHeaderFields();

			FileHasXBRL filter1;
			std::vector<std::string_view> forms{"10-K"};
			FileHasFormType filter2{forms};

			bool has_form = ApplyFilters(SEC_data.GetFields(), file_content, filter1, filter2);
			if (has_form)
				++files_with_form;
		}
    });

    std::for_each(fs::recursive_directory_iterator(EDGAR_DIRECTORY), fs::recursive_directory_iterator(), test_file);

	ASSERT_EQ(files_with_form, 1);
}

TEST_F(ValidateFolderFilters, VerifyFindAllInDateRange)
{
	int files_with_form{0};

    auto test_file([&files_with_form](const auto& dir_ent)
    {
        if (dir_ent.status().type() == fs::file_type::regular)
        {
			std::string file_content(fs::file_size(dir_ent.path()), '\0');
			std::ifstream input_file{dir_ent.path(), std::ios_base::in | std::ios_base::binary};
			input_file.read(&file_content[0], file_content.size());
			input_file.close();

			SEC_Header SEC_data;
			SEC_data.UseData(file_content);
			SEC_data.ExtractHeaderFields();

			FileHasXBRL filter1;
			FileIsWithinDateRange filter2{bg::from_simple_string("2013-Jan-1"), bg::from_simple_string("2013-09-30")};

			bool has_form = ApplyFilters(SEC_data.GetFields(), file_content, filter1, filter2);
			if (has_form)
				++files_with_form;
		}
    });

    std::for_each(fs::recursive_directory_iterator(EDGAR_DIRECTORY), fs::recursive_directory_iterator(), test_file);

	ASSERT_EQ(files_with_form, 151);
}

TEST_F(ValidateFolderFilters, VerifyFindAllInDateRangeNoMatches)
{
	int files_with_form{0};

    auto test_file([&files_with_form](const auto& dir_ent)
    {
        if (dir_ent.status().type() == fs::file_type::regular)
        {
			std::string file_content(fs::file_size(dir_ent.path()), '\0');
			std::ifstream input_file{dir_ent.path(), std::ios_base::in | std::ios_base::binary};
			input_file.read(&file_content[0], file_content.size());
			input_file.close();

			SEC_Header SEC_data;
			SEC_data.UseData(file_content);
			SEC_data.ExtractHeaderFields();

			FileHasXBRL filter1;
			FileIsWithinDateRange filter2{bg::from_simple_string("2015-Jan-1"), bg::from_simple_string("2015-09-30")};

			bool has_form = ApplyFilters(SEC_data.GetFields(), file_content, filter1, filter2);
			if (has_form)
				++files_with_form;
		}
    });

    std::for_each(fs::recursive_directory_iterator(EDGAR_DIRECTORY), fs::recursive_directory_iterator(), test_file);

	ASSERT_EQ(files_with_form, 0);
}

TEST_F(ValidateFolderFilters, VerifyComboFiltersWithMatches)
{
	int files_with_form{0};

    auto test_file([&files_with_form](const auto& dir_ent)
    {
        if (dir_ent.status().type() == fs::file_type::regular)
        {
			std::string file_content(fs::file_size(dir_ent.path()), '\0');
			std::ifstream input_file{dir_ent.path(), std::ios_base::in | std::ios_base::binary};
			input_file.read(&file_content[0], file_content.size());
			input_file.close();

			SEC_Header SEC_data;
			SEC_data.UseData(file_content);
			SEC_data.ExtractHeaderFields();

			FileHasXBRL filter1;
			std::vector<std::string_view> forms{"10-Q"};
			FileHasFormType filter2{forms};
			FileIsWithinDateRange filter3{bg::from_simple_string("2013-03-1"), bg::from_simple_string("2013-03-31")};

			bool has_form = ApplyFilters(SEC_data.GetFields(), file_content, filter1, filter2, filter3);
			if (has_form)
				++files_with_form;
		}
    });

    std::for_each(fs::recursive_directory_iterator(EDGAR_DIRECTORY), fs::recursive_directory_iterator(), test_file);

	ASSERT_EQ(files_with_form, 5);
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
