// =====================================================================================
//
//       Filename:  EndToEndTest.cpp
//
//    Description:  Driver program for end-to-end tests
//
//        Version:  1.0
//        Created:  01/03/2014 11:13:53 AM
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
#include <experimental/filesystem>

// #include <boost/filesystem.hpp>
// #include <boost/program_options/parsers.hpp>

#include <gmock/gmock.h>

#include "ExtractEDGAR_XBRLApp.h"

//	need these to feed into ExtractEDGAR_XBRLApp.

int G_ARGC = 0;
char** G_ARGV = nullptr;

namespace fs = std::experimental::filesystem;
// namespace po = boost::program_options;

using namespace testing;

/* MATCHER_P(DirectoryContainsNFiles, value, std::string(negation ? "doesn't" : "does") + */
/*                         " contain " + std::to_string(value) + " files") */

// NOTE:  file will be counted even if it is empty which suits my tests since a failed
// attempt will leave an empty file in the target directory.  This lets me see
// that an attempt was made and what file was to be downloaded which is all I need
// for these tests.

int CountFilesInDirectoryTree(const fs::path& directory)
{
	int count = std::count_if(fs::recursive_directory_iterator(directory), fs::recursive_directory_iterator(),
			[](const fs::directory_entry& entry) { return entry.status().type() == fs::file_type::regular; });
	return count;
}

bool DirectoryTreeContainsDirectory(const fs::path& tree, const fs::path& directory)
{
	for (auto x = fs::recursive_directory_iterator(tree); x != fs::recursive_directory_iterator(); ++x)
	{
		if (x->status().type() == fs::file_type::directory)
		{
			if (x->path().filename() == directory)
				return true;
		}
	}
	return false;
}

// NOTE: I'm going to run all these daily index tests against my local FTP server.

class DailyEndToEndTest : public Test
{
	public:
};


TEST(DailyEndToEndTest, VerifyDownloadCorrectNumberOfFormFilesForSingleIndexFile)
{
	if (fs::exists("/tmp/form.20131011.idx"))
		fs::remove("/tmp/form.20131011.idx");

	if (fs::exists("/tmp/forms"))
		fs::remove_all("/tmp/forms");
	fs::create_directory("/tmp/forms");

	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
		"--index-dir", "/tmp",
		"--begin-date", "2013-Oct-14",
		"--form-dir", "/tmp/forms",
        "--host",  "https://localhost:8443",
        "--log-level", "debug"
	};

    ExtractEDGAR_XBRLApp myApp;
	try
	{
        myApp.init(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
		myApp.logger().information(std::string("\n\nTest: ") + test_info->name() + " test case: " + test_info->test_case_name() + "\n\n");

        myApp.run();
	}

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
		// poco_fatal(myApp->logger(), theProblem.what());

		myApp.logger().error(std::string("Something fundamental went wrong: ") + theProblem.what());
		throw;	//	so test framework will get it too.
	}
	catch (...)
	{		// handle exception: unspecified
		myApp.logger().error("Something totally unexpected happened.");
		throw;
	}
	ASSERT_THAT(CountFilesInDirectoryTree("/tmp/forms"), Eq(18));
}

TEST(DailyEndToEndTest, VerifyDownloadsOfExistingFormFilesWhenReplaceIsSpecifed)
{
	if (fs::exists("/tmp/index2"))
		fs::remove_all("/tmp/index2");
	if (fs::exists("/tmp/forms2"))
		fs::remove_all("/tmp/forms2");
	fs::create_directory("/tmp/forms2");

	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
		"--index-dir", "/tmp/index2",
		"--form-dir", "/tmp/forms2",
        "--host", "https://localhost:8443",
		"--begin-date", "2013-Oct-14",
		"--end-date", "2013-Oct-17",
		"--replace-form-files"
	};

	ExtractEDGAR_XBRLApp myApp;
    myApp.init(tokens);

	decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
	myApp.logger().information(std::string("\n\nTest: ") + test_info->name() + " test case: " + test_info->test_case_name() + "\n\n");

    myApp.run();
	// decltype(auto) x1 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/forms2");

	std::this_thread::sleep_for(std::chrono::seconds{1});

	myApp.run();
	// decltype(auto) x2 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/forms2");

	// ASSERT_THAT(x1 == x2, Eq(false));
	ASSERT_TRUE(0);
}


int main(int argc, char** argv) {
	G_ARGC = argc;
	G_ARGV = argv;
	testing::InitGoogleMock(&argc, argv);
   return RUN_ALL_TESTS();
}
