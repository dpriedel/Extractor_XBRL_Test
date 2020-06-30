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

	/* This file is part of Extractor_Markup. */

	/* Extractor_Markup is free software: you can redistribute it and/or modify */
	/* it under the terms of the GNU General Public License as published by */
	/* the Free Software Foundation, either version 3 of the License, or */
	/* (at your option) any later version. */

	/* Extractor_Markup is distributed in the hope that it will be useful, */
	/* but WITHOUT ANY WARRANTY; without even the implied warranty of */
	/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
	/* GNU General Public License for more details. */

	/* You should have received a copy of the GNU General Public License */
	/* along with Extractor_Markup.  If not, see <http://www.gnu.org/licenses/>. */


// =====================================================================================
//        Class:
//  Description:
// =====================================================================================

#include "ExtractorApp.h"

#include <filesystem>
#include <pqxx/pqxx>

#include "spdlog/spdlog.h"

#include <gmock/gmock.h>

#include "Extractor_XBRL_FileFilter.h"
#include "Extractor_Utils.h"

namespace fs = std::filesystem;

const fs::path FILE_WITH_XML_10Q{"/vol_DA/SEC/Archives/edgar/data/1460602/0001062993-13-005017.txt"};
const fs::path FILE_WITH_XML_10K{"/vol_DA/SEC/Archives/edgar/data/google-10k.txt"};
const fs::path FILE_WITHOUT_XML{"/vol_DA/SEC/Archives/edgar/data/841360/0001086380-13-000030.txt"};
const fs::path SEC_DIRECTORY{"/vol_DA/SEC/Archives/edgar/data"};
const fs::path FILE_NO_NAMESPACE_10Q{"/vol_DA/SEC/Archives/edgar/data/68270/0000068270-13-000059.txt"};
const fs::path BAD_FILE2{"/vol_DA/SEC/SEC_forms/0001000180/10-K/0001000180-16-000068.txt"};
const fs::path NO_SHARES_OUT{"/vol_DA/SEC/SEC_forms/0001023453/10-K/0001144204-12-017368.txt"};
const fs::path MISSING_VALUES_LIST{"../Extractor_XBRL_Test/missing_values_files.txt"};
const fs::path MISSING_VALUES_LIST_SHORT{"../Extractor_XBRL_Test/missing_values_files_short.txt"};

const fs::path ORIGINAL_10Q{"/vol_DA/SEC/SEC_forms/0001001258/10-Q/0001193125-14-043453.txt"};
const fs::path AMENDED_10Q{"/vol_DA/SEC/SEC_forms/0001001258/10-Q_A/0001193125-15-234644.txt"};
const fs::path AMENDED_WITH_OLDER_DATA_10Q{"./multiple_amended_files.txt"};

using namespace testing;

std::shared_ptr<spdlog::logger> DEFAULT_LOGGER;

class SingleFileEndToEnd : public Test
{
	public:

        void SetUp() override
        {
            spdlog::set_default_logger(DEFAULT_LOGGER);

		    pqxx::connection c{"dbname=sec_extracts user=extractor_pg"};
		    pqxx::work trxn{c};

		    // make sure the DB is empty before we start

		    trxn.exec("DELETE FROM unified_extracts.sec_filing_id WHERE data_source != 'HTML'");
		    trxn.commit();
        }

//		int CountRows()
//		{
//		    pqxx::connection c{"dbname=sec_extracts user=extractor_pg"};
//		    pqxx::work trxn{c};
//
//		    // make sure the DB is empty before we start
//
//		    auto row = trxn.exec1("SELECT count(*) FROM unified_extracts.sec_xbrl_data");
//		    trxn.commit();
//			return row[0].as<int>();
//		}
		int CountRows()
		{
		    pqxx::connection c{"dbname=sec_extracts user=extractor_pg"};
		    pqxx::work trxn{c};

		    // make sure the DB is empty before we start

		    auto row1 = trxn.query_value<int>("select count(*) from unified_extracts.sec_filing_id as t1 inner join unified_extracts.sec_bal_sheet_data as t2 on t1.filing_id =  t2.filing_id where t1.data_source = 'XLS';");
		    auto row2 = trxn.query_value<int>("select count(*) from unified_extracts.sec_filing_id as t1 inner join unified_extracts.sec_stmt_of_ops_data as t2 on t1.filing_id =  t2.filing_id where t1.data_source = 'XLS';");
		    auto row3 = trxn.query_value<int>("select count(*) from unified_extracts.sec_filing_id as t1 inner join unified_extracts.sec_cash_flows_data as t2 on t1.filing_id =  t2.filing_id where t1.data_source = 'XLS';");
		    trxn.commit();
			int total = row1 + row2 + row3;
            if ( total == 0)
            {
                // maybe we have plain XBRL

                pqxx::work trxn{c};
                total = trxn.query_value<int>("select count(*) from unified_extracts.sec_filing_id as t1 inner join unified_extracts.sec_xbrl_data as t2 on t1.filing_id =  t2.filing_id where t1.data_source = 'XBRL';");
                trxn.commit();
            }
            return total;
		}
};


TEST_F(SingleFileEndToEnd, VerifyCanLoadDataToDBForFileWithXML10Q)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--mode", "XBRL",
        "--log-level", "debug",
		"--form", "10-Q",
		"-f", FILE_WITH_XML_10Q.string()
	};

	try
	{
        ExtractorApp myApp(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ",
                test_info->test_case_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
	}

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}
	ASSERT_EQ(CountRows(), 55);
}

TEST_F(SingleFileEndToEnd, VerifyCanLoadDataToDBForFileWithXMLNoNamespace10Q)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--mode", "XBRL",
        "--log-level", "debug",
		"--form", "10-Q",
		"-f", FILE_NO_NAMESPACE_10Q.string()
	};

	try
	{
        ExtractorApp myApp(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ",
                test_info->test_case_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
	}

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}
	ASSERT_EQ(CountRows(), 92);
}

TEST_F(SingleFileEndToEnd, VerifyCanLoadDataToDBForFileWithXML_NoSharesOUt10K)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--mode", "XBRL",
        "--log-level", "debug",
		"--form", "10-K",
		"-f", NO_SHARES_OUT.string()
	};

	try
	{
        ExtractorApp myApp(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ",
                test_info->test_case_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
	}

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}
	ASSERT_EQ(CountRows(), 79);
}

TEST_F(SingleFileEndToEnd, VerifyCanLoadDataToDBForFileWithXML10K)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--mode", "XBRL",
		"--begin-date", "2013-Oct-14",
		"--end-date", "2015-12-31",
        "--log-level", "debug",
		"--form", "10-K",
		"-f", FILE_WITH_XML_10K.string()
	};

	try
	{
        ExtractorApp myApp(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ",
                test_info->test_case_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
	}

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}
	ASSERT_EQ(CountRows(), 194);
}

TEST_F(SingleFileEndToEnd, WorkWithBadFile210K)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--mode", "XBRL",
        "--log-level", "debug",
		"--form", "10-K",
		"-f", BAD_FILE2.string()
	};

	try
	{
        ExtractorApp myApp(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ",
                test_info->test_case_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
	}

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}
	ASSERT_EQ(CountRows(), 106);
}

class ProcessFolderEndtoEnd : public Test
{
	public:

        void SetUp() override
        {
            spdlog::set_default_logger(DEFAULT_LOGGER);

		    pqxx::connection c{"dbname=sec_extracts user=extractor_pg"};
		    pqxx::work trxn{c};

		    // make sure the DB is empty before we start

		    trxn.exec("DELETE FROM unified_extracts.sec_filing_id WHERE data_source != 'HTML'");
		    trxn.commit();
        }

		int CountRows()
		{
		    pqxx::connection c{"dbname=sec_extracts user=extractor_pg"};
		    pqxx::work trxn{c};

		    // make sure the DB is empty before we start

		    auto row1 = trxn.query_value<int>("select count(*) from unified_extracts.sec_filing_id as t1 inner join unified_extracts.sec_bal_sheet_data as t2 on t1.filing_id =  t2.filing_id where t1.data_source = 'XLS';");
		    auto row2 = trxn.query_value<int>("select count(*) from unified_extracts.sec_filing_id as t1 inner join unified_extracts.sec_stmt_of_ops_data as t2 on t1.filing_id =  t2.filing_id where t1.data_source = 'XLS';");
		    auto row3 = trxn.query_value<int>("select count(*) from unified_extracts.sec_filing_id as t1 inner join unified_extracts.sec_cash_flows_data as t2 on t1.filing_id =  t2.filing_id where t1.data_source = 'XLS';");
		    trxn.commit();
			int total = row1 + row2 + row3;
            if ( total == 0)
            {
                // maybe we have plain XBRL

                pqxx::work trxn{c};
                total = trxn.query_value<int>("select count(*) from unified_extracts.sec_filing_id as t1 inner join unified_extracts.sec_xbrl_data as t2 on t1.filing_id =  t2.filing_id where t1.data_source = 'XBRL';");
                trxn.commit();
            }
            return total;
		}

		int CountMissingValues()
		{
		    pqxx::connection c{"dbname=sec_extracts user=extractor_pg"};
		    pqxx::work trxn{c};

		    auto row = trxn.exec1("SELECT count(*) FROM unified_extracts.sec_xbrl_data WHERE label = 'Missing Value'");
		    trxn.commit();
			return row[0].as<int>();
		}

		int CountFilings()
		{
		    pqxx::connection c{"dbname=sec_extracts user=extractor_pg"};
		    pqxx::work trxn{c};

		    // make sure the DB is empty before we start

		    auto row = trxn.exec1("SELECT count(*) FROM unified_extracts.sec_filing_id WHERE data_source != 'HTML'");
		    trxn.commit();
			return row[0].as<int>();
		}

//        void TearDown() override
//        {
//            spdlog::shutdown();
//        }
};

TEST_F(ProcessFolderEndtoEnd, TestNoInputFiles)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--mode", "XBRL",
        "--log-level", "debug",
		"--form", "10-K"
    };

	try
	{
        ExtractorApp myApp(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ",
                test_info->test_case_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
	}

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}
	ASSERT_EQ(CountFilings(), 0);
}

TEST_F(ProcessFolderEndtoEnd, WorkWithFileList2)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--mode", "XBRL",
        "--log-level", "debug",
		"--form", "10-K",
		"--form-dir", SEC_DIRECTORY.string()
    };

	try
	{
        ExtractorApp myApp(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ",
                test_info->test_case_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
	}

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}
	ASSERT_EQ(CountFilings(), 1);
}

TEST_F(ProcessFolderEndtoEnd, WorkWithFileList310Q)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--mode", "XBRL",
        "--log-level", "debug",
		"--form", "10-Q",
		"--log-path", "/tmp/test4.log",
		"--list", "./test_directory_list.txt"
    };

	try
	{
        ExtractorApp myApp(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ",
                test_info->test_case_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
	}

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}
	ASSERT_EQ(CountFilings(), 152);
}

TEST_F(ProcessFolderEndtoEnd, WorkWithFileListResume10Q)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--mode", "XBRL",
        "--log-level", "debug",
		"--form", "10-Q",
		"--log-path", "/tmp/test4.log",
		"--list", "./test_directory_list.txt",
        "--resume-at", "/vol_DA/SEC/Archives/edgar/data/1326688/0001104659-09-064933.txt"
    };

	try
	{
        ExtractorApp myApp(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ",
                test_info->test_case_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
	}

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}
    // file list contains a very unusual FinancialReport spread sheet. Don't expect to read it
	ASSERT_EQ(CountFilings(), 30);
}

TEST_F(ProcessFolderEndtoEnd, WorkWithFileListContainsBadFile)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--mode", "XBRL",
        "--log-level", "debug",
		"--form", "10-Q,10-K",
		"--log-path", "/tmp/test1.log",
		"--list", "./list_with_bad_file.txt"
    };

	try
	{
        ExtractorApp myApp(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ",
                test_info->test_case_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
	}

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}
    // there are 45 potential filings in the list.  3 are 'bad'.
	ASSERT_EQ(CountFilings(), 41);
}

TEST_F(ProcessFolderEndtoEnd, WorkWithMissingValuesFileList1)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--mode", "XBRL",
        "--log-level", "debug",
		"--log-path", "/tmp/test8.log",
//		"--list", MISSING_VALUES_LIST,
		"--list", MISSING_VALUES_LIST_SHORT,
		"--form", "10-K",
        "-k", "4"
    };

	try
	{
        ExtractorApp myApp(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ",
                test_info->test_case_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
	}

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}
	EXPECT_TRUE(CountFilings() > 0);
	ASSERT_EQ(CountMissingValues(), 157);
}

TEST_F(ProcessFolderEndtoEnd, WorkWithFileListContainsFormName)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--mode", "XBRL",
        "--log-level", "debug",
		"--form", "10-K",
		"--log-path", "/tmp/test1.log",
		"--list", "./list_with_bad_file.txt",
        "filename-has-form"
    };

	try
	{
        ExtractorApp myApp(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ",
                test_info->test_case_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
	}

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}
    // there are 7 potential filings in the list.  2 are 'bad'.
	ASSERT_EQ(CountFilings(), 5);
}

TEST_F(ProcessFolderEndtoEnd, WorkWithFileListContainsBadFileRepeat)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--mode", "XBRL",
        "--log-level", "debug",
		"--form", "10-K",
        "-k", "2",
		"--log-path", "/tmp/test1.log",
		"--list", "./list_with_bad_file.txt",
        
    };

	try
	{
        ExtractorApp myApp(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ",
                test_info->test_case_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }

    }
	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}

    try
    {
        ExtractorApp myApp(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ",
                test_info->test_case_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems restarting program.  No repeat processing done.\n";
        }
	}

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}
    // there are 7 potential filings in the list.  2 are 'bad'.
	ASSERT_EQ(CountFilings(), 5);
}

TEST_F(ProcessFolderEndtoEnd, DISABLED_WorkWithFileListBadFile10K)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--mode", "XBRL",
        "--log-level", "debug",
		"--form", "10-K",
		"--list", "./test_directory_list.txt"
    };

	try
	{
        ExtractorApp myApp(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ",
                test_info->test_case_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
	}

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}
	ASSERT_EQ(CountFilings(), 1);
}

TEST_F(ProcessFolderEndtoEnd, WorkWithFileList3WithLimit10Q)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--mode", "XBRL",
        "--log-level", "debug",
		"--form", "10-Q",
		"--max", "17",
		"--list", "./test_directory_list.txt"
    };

	try
	{
        ExtractorApp myApp(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ",
                test_info->test_case_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
	}

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}
	ASSERT_EQ(CountFilings(), 17);
}

TEST_F(ProcessFolderEndtoEnd, WorkWithFileList3WithLimit10K)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--mode", "XBRL",
        "--log-level", "debug",
		"--form", "10-K",
		"--max", "17",
		"--list", "./test_directory_list.txt"
    };

	try
	{
        ExtractorApp myApp(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ",
                test_info->test_case_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
	}

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}
	ASSERT_EQ(CountFilings(), 1);
}

TEST_F(ProcessFolderEndtoEnd, WorkWithFileList3Async10Q)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--mode", "XBRL",
        "--log-level", "debug",
		"--form", "10-Q",
		"-k", "4",
		"--list", "./test_directory_list.txt"
    };

	try
	{
        ExtractorApp myApp(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ",
                test_info->test_case_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
	}

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}
	ASSERT_EQ(CountFilings(), 152);
}

TEST_F(ProcessFolderEndtoEnd, WorkWithFileList3WithLimitAsync10Q)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--mode", "XBRL",
        "--log-level", "debug",
		"--form", "10-Q",
		"--max", "17",
		"-k", "4",
		"--list", "./test_directory_list.txt"
    };

	try
	{
        ExtractorApp myApp(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ",
                test_info->test_case_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
	}

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}
	ASSERT_EQ(CountFilings(), 17);
}

TEST_F(ProcessFolderEndtoEnd, WorkWithFileList310K)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--mode", "XBRL",
        "--log-level", "debug",
		"--form", "10-K",
		"--list", "./test_directory_list.txt"
    };

	try
	{
        ExtractorApp myApp(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ",
                test_info->test_case_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
	}

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}
	ASSERT_EQ(CountFilings(), 1);
}

TEST_F(ProcessFolderEndtoEnd, VerifyCanApplyFilters)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--mode", "XBRL",
        "--log-level", "debug",
		"--form", "10-K",
		"--form-dir", SEC_DIRECTORY.string()
	};

	try
	{
        ExtractorApp myApp(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ",
                test_info->test_case_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
	}

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}
	ASSERT_EQ(CountFilings(), 1);
}

TEST_F(ProcessFolderEndtoEnd, VerifyCanApplyFilters2)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--mode", "XBRL",
		"--begin-date", "2013-Mar-1",
		"--end-date", "2013-3-31",
        "--log-level", "debug",
		"--form", "10-Q",
		"--form-dir", SEC_DIRECTORY.string()
	};

	try
	{
        ExtractorApp myApp(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ",
                test_info->test_case_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
	}

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}
	ASSERT_EQ(CountFilings(), 5);
}

TEST_F(ProcessFolderEndtoEnd, VerifyCanApplyFilters3)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--mode", "XBRL",
		"--begin-date", "2013-Mar-1",
		"--end-date", "2013-3-31",
        "--log-level", "debug",
		"--form", "10-K,10-Q",
		"--form-dir", SEC_DIRECTORY.string()
	};

	try
	{
        ExtractorApp myApp(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ",
                test_info->test_case_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
	}

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}
	ASSERT_EQ(CountFilings(), 5);
}

TEST_F(ProcessFolderEndtoEnd, VerifyCanApplyFilters4ShortCIKFails)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--mode", "XBRL",
		"--begin-date", "2013-Mar-1",
		"--end-date", "2013-3-31",
        "--log-level", "debug",
		"--form", "10-K,10-Q",
		"--CIK", "1541884",
		"--form-dir", SEC_DIRECTORY.string()
	};

	try
	{
        ExtractorApp myApp(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ",
                test_info->test_case_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
	}

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}
	ASSERT_EQ(CountFilings(), 0);
}

TEST_F(ProcessFolderEndtoEnd, VerifyCanApplyFilters5)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--mode", "XBRL",
		"--begin-date", "2013-Mar-1",
		"--end-date", "2013-3-31",
        "--log-level", "debug",
		"--form", "10-K,10-Q",
		"--CIK", "0000826772,0000826774",
		"--form-dir", SEC_DIRECTORY.string()
	};

	try
	{
        ExtractorApp myApp(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ",
                test_info->test_case_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
	}

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}
	ASSERT_EQ(CountFilings(), 1);
}

TEST_F(ProcessFolderEndtoEnd, LoadLotsOfFiles)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--mode", "XBRL",
        "--log-level", "debug",
		"--form", "10-Q",
		"--form-dir", SEC_DIRECTORY.string()
	};

	try
	{
        ExtractorApp myApp(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ",
                test_info->test_case_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
	}

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}
	// NOTE: there are 157 files which meet the scan criteria BUT 2 of them are duplicated.
	ASSERT_EQ(CountFilings(), 152);
}

TEST_F(ProcessFolderEndtoEnd, LoadLotsOfFilesWithLimit)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--mode", "XBRL",
        "--log-level", "debug",
		"--form", "10-Q",
		"--max", "14",
		"--form-dir", SEC_DIRECTORY.string()
	};

	try
	{
        ExtractorApp myApp(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ",
                test_info->test_case_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
	}

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}
	// NOTE: there are 157 files which meet the scan criteria BUT 2 of them are duplicated.
	ASSERT_EQ(CountFilings(), 14);
}

// TEST(DailyEndToEndTest, VerifyDownloadsOfExistingFormFilesWhenReplaceIsSpecifed)
// {
// 	if (fs::exists("/tmp/index2"))
// 		fs::remove_all("/tmp/index2");
// 	if (fs::exists("/tmp/forms2"))
// 		fs::remove_all("/tmp/forms2");
// 	fs::create_directory("/tmp/forms2");
//
// 	//	NOTE: the program name 'the_program' in the command line below is ignored in the
// 	//	the test program.
//
// 	std::vector<std::string> tokens{"the_program",
//        "--mode", "XBRL",
// 		"--index-dir", "/tmp/index2",
// 		"--form-dir", "/tmp/forms2",
//         "--host", "https://localhost:8443",
// 		"--begin-date", "2013-Oct-14",
// 		"--end-date", "2013-Oct-17",
// 		"--replace-form-files"
// 	};
//
//     myApp.init(tokens);
//
// 	decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
// 	myApp.logger().information(std::string("\n\nTest: ") + test_info->name() + " test case: " + test_info->test_case_name() + "\n\n");
//
//     myApp.run();
// 	// decltype(auto) x1 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/forms2");
//
// 	std::this_thread::sleep_for(std::chrono::seconds{1});
//
// 	myApp.run();
// 	// decltype(auto) x2 = CollectLastModifiedTimesForFilesInDirectoryTree("/tmp/forms2");
//
// 	// ASSERT_THAT(x1 == x2, Eq(false));
// 	ASSERT_TRUE(0);
// }

class ProcessAmendedForms : public Test
{
	public:

        void SetUp() override
        {
            spdlog::set_default_logger(DEFAULT_LOGGER);

		    pqxx::connection c{"dbname=sec_extracts user=extractor_pg"};
		    pqxx::work trxn{c};

		    // make sure the DB is empty before we start

		    trxn.exec("DELETE FROM unified_extracts.sec_filing_id WHERE data_source != 'HTML'");
		    trxn.commit();
        }

        int CountRows()
		{
		    pqxx::connection c{"dbname=sec_extracts user=extractor_pg"};
		    pqxx::work trxn{c};

		    // make sure the DB is empty before we start

		    auto row1 = trxn.query_value<int>("select count(*) from unified_extracts.sec_filing_id as t1 inner join unified_extracts.sec_bal_sheet_data as t2 on t1.filing_id =  t2.filing_id where t1.data_source = 'XLS';");
		    auto row2 = trxn.query_value<int>("select count(*) from unified_extracts.sec_filing_id as t1 inner join unified_extracts.sec_stmt_of_ops_data as t2 on t1.filing_id =  t2.filing_id where t1.data_source = 'XLS';");
		    auto row3 = trxn.query_value<int>("select count(*) from unified_extracts.sec_filing_id as t1 inner join unified_extracts.sec_cash_flows_data as t2 on t1.filing_id =  t2.filing_id where t1.data_source = 'XLS';");
		    trxn.commit();
			int total = row1 + row2 + row3;
            if ( total == 0)
            {
                // maybe we have plain XBRL

                pqxx::work trxn{c};
                total = trxn.query_value<int>("select count(*) from unified_extracts.sec_filing_id as t1 inner join unified_extracts.sec_xbrl_data as t2 on t1.filing_id =  t2.filing_id where t1.data_source = 'XBRL';");
                trxn.commit();
            }
            return total;
		}
};

TEST_F(ProcessAmendedForms, VerifyCanUpdateDataFromAmendedFormToDBForFileWithXML10Q)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens{"the_program",
        "--mode", "XBRL",
        "--log-level", "debug",
		"--form", "10-Q",
		"-f", ORIGINAL_10Q.string()
	};

	try
	{
        ExtractorApp myApp(tokens);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ",
                test_info->test_case_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
	}

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}
	EXPECT_EQ(CountRows(), 69);

	std::vector<std::string> tokens2{"the_program",
        "--mode", "XBRL",
        "--log-level", "debug",
		"--form", "10-Q/A",
		"-f", AMENDED_10Q.string()
	};

	try
	{
        ExtractorApp myApp(tokens2);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ",
                test_info->test_case_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
	}

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}
	EXPECT_EQ(CountRows(), 69);
}

TEST_F(ProcessAmendedForms, VerifyCanAddDataFromAmendedFormToDBWhenNoOriginalDataForFileWithXML10Q)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens2{"the_program",
        "--mode", "XBRL",
        "--log-level", "debug",
		"--form", "10-Q/A",
		"-f", AMENDED_10Q.string()
	};

	try
	{
        ExtractorApp myApp(tokens2);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ",
                test_info->test_case_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            myApp.Run();
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
	}

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}
	EXPECT_EQ(CountRows(), 69);
}

TEST_F(ProcessAmendedForms, VerifyNoThrowWhenTryToAsyncReplaceAmendedDataWithOlderDataForFileWithXML10K)
{
	//	NOTE: the program name 'the_program' in the command line below is ignored in the
	//	the test program.

	std::vector<std::string> tokens2{"the_program",
        "--mode", "XBRL",
        "--log-level", "debug",
		"--form", "10-K,10-K/A",
        "-k", "5",
		"--list", AMENDED_WITH_OLDER_DATA_10Q.string()
	};

	try
	{
        ExtractorApp myApp(tokens2);

		decltype(auto) test_info = UnitTest::GetInstance()->current_test_info();
        spdlog::info(catenate("\n\nTest: ", test_info->name(), " test case: ",
                test_info->test_case_name(), "\n\n"));

        bool startup_OK = myApp.Startup();
        if (startup_OK)
        {
            EXPECT_NO_THROW( myApp.Run());
            myApp.Shutdown();
        }
        else
        {
            std::cout << "Problems starting program.  No processing done.\n";
        }
	}

    // catch any problems trying to setup application

	catch (const std::exception& theProblem)
	{
        spdlog::error(catenate("Something fundamental went wrong: ", theProblem.what()));
	}
	catch (...)
	{		// handle exception: unspecified
        spdlog::error("Something totally unexpected happened.");
	}
    // we should end up with the contents of file /vol_DA/SEC/SEC_forms/0001453883/10-K_A/0001079974-16-001022.txt

	EXPECT_EQ(CountRows(), 33);
}


void InitLogging ()
{
    DEFAULT_LOGGER = spdlog::default_logger();

    //    nothing to do for now.
//    logging::core::get()->set_filter
//    (
//        logging::trivial::severity >= logging::trivial::trace
//    );
}		/* -----  end of function InitLogging  ----- */

int main(int argc, char** argv)
{

    InitLogging();

	InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}
