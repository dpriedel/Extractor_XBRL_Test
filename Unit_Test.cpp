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


#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <system_error>
#include <thread>

#include <gmock/gmock.h>

#include <range/v3/action/sort.hpp>
#include <range/v3/algorithm/equal.hpp>
#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/algorithm/set_algorithm.hpp>
#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/iterator.hpp>
#include <range/v3/view/filter.hpp>

namespace fs = std::filesystem;

using namespace testing;

#include "Extractor.h"
#include "Extractor_Utils.h"
#include "Extractor_XBRL_FileFilter.h"
#include "SEC_Header.h"
#include "XLS_Data.h"


// some specific files for Testing.

const EM::FileName FILE_WITH_XML_10Q{"/vol_DA/SEC/Archives/edgar/data/1460602/0001062993-13-005017.txt"};
const EM::FileName FILE_WITH_XML_10K{"/vol_DA/SEC/Archives/edgar/data/google-10k.txt"};
const EM::FileName FILE_WITHOUT_XML{"/vol_DA/SEC/Archives/edgar/data/841360/0001086380-13-000030.txt"};
const EM::FileName EDGAR_DIRECTORY{"/vol_DA/SEC/Archives/edgar/data"};
const EM::FileName FILE_NO_NAMESPACE_10Q{"/vol_DA/SEC/Archives/edgar/data/68270/0000068270-13-000059.txt"};
const EM::FileName FILE_SOME_NAMESPACE_10Q{"/vol_DA/SEC/Archives/edgar/data/1552979/0001214782-13-000386.txt"};
const EM::FileName FILE_MULTIPLE_LABEL_LINKS{"/vol_DA/SEC/Archives/edgar/data/1540334/0001078782-13-002015.txt"};
const EM::FileName BAD_FILE1{"/vol_DA/SEC/SEC_forms/0001000228/10-K/0001000228-11-000014.txt"};
const EM::FileName BAD_FILE2{"/vol_DA/SEC/SEC_forms/0001000180/10-K/0001000180-16-000068.txt"};
const EM::FileName BAD_FILE3{"/vol_DA/SEC/SEC_forms/0001000697/10-K/0000950123-11-018381.txt"};
const EM::FileName NO_SHARES_OUT{"/vol_DA/SEC/SEC_forms/0001023453/10-K/0001144204-12-017368.txt"};
const EM::FileName TEST_FILE_LIST{"./list_with_bad_file.txt"};
const EM::FileName MISSING_VALUES1_10K{"/vol_DA/SEC/SEC_forms/0001004980/10-K/0001193125-12-065537.txt"};
const EM::FileName MISSING_VALUES2_10K{"/vol_DA/SEC/SEC_forms/0001005210/10-Q_A/0001193125-12-335145.txt"};

const EM::FileName ORIGINAL_10Q{"/vol_DA/SEC/SEC_forms/0001001258/10-Q/0001193125-14-043453.txt"};
const EM::FileName AMENDED_10Q{"/vol_DA/SEC/SEC_forms/0001001258/10-Q_A/0001193125-15-234644.txt"};

const EM::FileName XLS_SHEET_1{"/vol_DA/SEC/SEC_forms/0001453883/10-K_A/0001079974-16-001022.txt"};
const EM::FileName XLS_SHEET_2{"/vol_DA/SEC/Archives/edgar/data/68270/0000068270-13-000059.txt"};

// This ctype facet does NOT classify spaces and tabs as whitespace
// from cppreference example

struct line_only_whitespace : std::ctype<char>
{
    static const mask* make_table()
    {
        // make a copy of the "C" locale table
        static std::vector<mask> v(classic_table(), classic_table() + table_size);
        v['\t'] &= ~space;      // tab will not be classified as whitespace
        v[' '] &= ~space;       // space will not be classified as whitespace
        return &v[0];
    }
    explicit line_only_whitespace(std::size_t refs = 0) : ctype(make_table(), false, refs) {}
};

// some utility functions for testing.

int FindAllLabels(const std::vector<EM::GAAP_Data>& gaap_data, const EM::Extractor_Labels& labels)
{
    if (gaap_data.empty())
    {
        std::cout << "Empty GAAP data." << "\n";
        return -1;
    }
    if (labels.empty())
    {
        std::cout << "Empty EDGAR Labels data." << "\n";
        return -1;
    }

    int missing = 0;
    for (const auto& e : gaap_data)
    {
        auto pos = labels.find(e.label);
        if (pos == labels.end())
        {
            std::cout << "Can't find: " << e.label << '\n';
            missing += 1;
        }
    }
    return missing;
}

bool FindAllContexts(const std::vector<EM::GAAP_Data>& gaap_data, const EM::ContextPeriod& contexts)
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
        auto pos = contexts.find(e.context_ID);
        if (pos == contexts.end())
        {
            std::cout << "Can't find: " << e.context_ID << '\n';
            all_good = false;
        }
    }
    return all_good;
}

// Function to compare 2 ranges and print differences

void PrintRangeDifferences(const std::vector<EM::GAAP_Data>& rng1, const std::vector<EM::GAAP_Data>& rng2)
{
    std::cout << "Items in first range: " << rng1.size() << '\n';
    std::vector<EM::GAAP_Data> in_rng1_only;
    auto x = ranges::set_difference(rng1, rng2,
            ranges::back_inserter(in_rng1_only),
            [](const auto& lhs, const auto& rhs) { return lhs.label < rhs.label; });

    std::cout << "\n\nItems missing from amended form.\n\n";
    ranges::for_each(in_rng1_only, [](const auto& e) {std::cout << e.label << " : " << e.context_ID << " : " << e.value << '\n'; } );

    std::cout << "Items in second range: " << rng2.size() << '\n';
    std::vector<EM::GAAP_Data> in_rng2_only;
    auto y = ranges::set_difference(rng2, rng1,
            ranges::back_inserter(in_rng2_only),
            [](const auto& lhs, const auto& rhs) { return lhs.label < rhs.label; });

    std::cout << "\n\nItems missing from original form.\n\n";
    ranges::for_each(in_rng2_only, [](const auto& e) {std::cout << e.label << " : " << e.context_ID << " : " << e.value << '\n'; } );

}

class IdentifyXMLFilesToUse : public Test
{

};

TEST_F(IdentifyXMLFilesToUse, ConfirmFileHasXML)
{
    auto file_content_10Q = LoadDataFileForUse(FILE_WITH_XML_10Q);
    EM::FileContent file_content{file_content_10Q};
    const auto sections = LocateDocumentSections(file_content);

    SEC_Header SEC_data;
    SEC_data.UseData(file_content);
    SEC_data.ExtractHeaderFields();

    FileHasXBRL filter1;
    auto use_file = filter1(SEC_data.GetFields(), sections);
    ASSERT_THAT(use_file, Eq(true));
}

TEST_F(IdentifyXMLFilesToUse, ConfirmFileHasNOXML)
{
    auto file_content_10Q = LoadDataFileForUse(FILE_WITHOUT_XML);
    EM::FileContent file_content{file_content_10Q};
    const auto sections = LocateDocumentSections(file_content);

    SEC_Header SEC_data;
    SEC_data.UseData(file_content);
    SEC_data.ExtractHeaderFields();

    FileHasXBRL filter1;
    auto use_file = filter1(SEC_data.GetFields(), sections);
    ASSERT_THAT(use_file, Eq(false));
}

class ValidateCanNavigateDocumentStructure : public Test
{
};

TEST_F(ValidateCanNavigateDocumentStructure, FindSECHeader10Q)
{
    auto file_content_10Q = LoadDataFileForUse(FILE_WITH_XML_10Q);
    EM::FileContent file_content{file_content_10Q};

    SEC_Header SEC_data;

    ASSERT_NO_THROW(SEC_data.UseData(file_content));
}

TEST_F(ValidateCanNavigateDocumentStructure, FindSECHeader10K)
{
    auto file_content_10K = LoadDataFileForUse(FILE_WITH_XML_10K);
    EM::FileContent file_content{file_content_10K};

    SEC_Header SEC_data;

    ASSERT_NO_THROW(SEC_data.UseData(file_content));
}

TEST_F(ValidateCanNavigateDocumentStructure, SECHeaderFindAllFields10Q)
{
    auto file_content_10Q = LoadDataFileForUse(FILE_WITH_XML_10Q);
    EM::FileContent file_content{file_content_10Q};

    SEC_Header SEC_data;
    SEC_data.UseData(file_content);
    ASSERT_NO_THROW(SEC_data.ExtractHeaderFields());
}

TEST_F(ValidateCanNavigateDocumentStructure, FindsAllDocumentSections10Q)
{
    auto file_content_10Q = LoadDataFileForUse(FILE_WITH_XML_10Q);
    EM::FileContent file_content{file_content_10Q};

    const auto result = LocateDocumentSections(file_content);

    ASSERT_EQ(result.size(), 52);
}

TEST_F(ValidateCanNavigateDocumentStructure, FindsAllDocumentSections10K)
{
    auto file_content_10K = LoadDataFileForUse(FILE_WITH_XML_10K);
    EM::FileContent file_content{file_content_10K};

    const auto result = LocateDocumentSections(file_content);

    ASSERT_EQ(result.size(), 119);
}

class LocateFileContentToUse : public Test
{

};

TEST_F(LocateFileContentToUse, FindInstanceDocument10Q)
{
    auto file_content_10Q = LoadDataFileForUse(FILE_WITH_XML_10Q);
    EM::FileContent file_content{file_content_10Q};

    const auto document_sections_10Q{LocateDocumentSections(file_content)};

    auto instance_document = LocateInstanceDocument(document_sections_10Q, FILE_WITH_XML_10Q);
    ASSERT_TRUE(instance_document.get().starts_with("<?xml version")
        && instance_document.get().ends_with("</xbrl>\n"));
}

TEST_F(LocateFileContentToUse, FindInstanceDocument10K)
{
    auto file_content_10K = LoadDataFileForUse(FILE_WITH_XML_10K);
    EM::FileContent file_content{file_content_10K};

    const auto document_sections_10K{LocateDocumentSections(file_content)};

    auto instance_document = LocateInstanceDocument(document_sections_10K, FILE_WITH_XML_10K);
    ASSERT_TRUE(instance_document.get().starts_with("<?xml version")
        && instance_document.get().ends_with("</xbrli:xbrl>\n"));
}

TEST_F(LocateFileContentToUse, FindLabelDocument10Q)
{
    auto file_content_10Q = LoadDataFileForUse(FILE_WITH_XML_10Q);
    EM::FileContent file_content{file_content_10Q};

    const auto document_sections_10Q{LocateDocumentSections(file_content)};

    auto labels_document = LocateLabelDocument(document_sections_10Q, FILE_WITH_XML_10Q);
    ASSERT_TRUE(labels_document.get().starts_with("<?xml version")
        && labels_document.get().ends_with("</link:linkbase>\n"));
}

TEST_F(LocateFileContentToUse, FindLabelDocument10K)
{
    auto file_content_10K = LoadDataFileForUse(FILE_WITH_XML_10K);
    EM::FileContent file_content{file_content_10K};

    const auto document_sections_10K{LocateDocumentSections(file_content)};

    auto labels_document = LocateLabelDocument(document_sections_10K, FILE_WITH_XML_10K);
    ASSERT_TRUE(labels_document.get().starts_with("<?xml version")
        && labels_document.get().ends_with("</link:linkbase>\n"));
}

class ParseDocumentContent : public Test
{

};

TEST_F(ParseDocumentContent, VerifyCanParseInstanceDocument10Q)
{
    auto file_content_10Q = LoadDataFileForUse(FILE_WITH_XML_10Q);
    EM::FileContent file_content{file_content_10Q};

    const auto document_sections_10Q{LocateDocumentSections(file_content)};

    auto instance_document = LocateInstanceDocument(document_sections_10Q, FILE_WITH_XML_10Q);
    ASSERT_NO_THROW(ParseXMLContent(instance_document));
}

TEST_F(ParseDocumentContent, VerifyCanParseInstanceDocument10K)
{
    auto file_content_10K = LoadDataFileForUse(FILE_WITH_XML_10K);
    EM::FileContent file_content{file_content_10K};

    const auto document_sections_10K{LocateDocumentSections(file_content)};

    auto instance_document = LocateInstanceDocument(document_sections_10K, FILE_WITH_XML_10K);
    ASSERT_NO_THROW(ParseXMLContent(instance_document));
}

TEST_F(ParseDocumentContent, VerifyParseBadInstanceDocumentThrows10K)
{
    auto file_content_10K = LoadDataFileForUse(BAD_FILE1);
    EM::FileContent file_content{file_content_10K};

    const auto document_sections_10K{LocateDocumentSections(file_content)};

    auto instance_document = LocateInstanceDocument(document_sections_10K, BAD_FILE1);
    ASSERT_THROW(ParseXMLContent(instance_document), ExtractorException);
}

TEST_F(ParseDocumentContent, VerifyParseBadInstanceDocumentThrows210K)
{
    auto file_content_10K = LoadDataFileForUse(BAD_FILE3);
    EM::FileContent file_content{file_content_10K};

    const auto document_sections_10K{LocateDocumentSections(file_content)};

    auto instance_document = LocateInstanceDocument(document_sections_10K, BAD_FILE3);
    ASSERT_THROW(ParseXMLContent(instance_document), ExtractorException);
}

TEST_F(ParseDocumentContent, VerifyCanParseLabelsDocument10Q)
{
    auto file_content_10Q = LoadDataFileForUse(FILE_WITH_XML_10Q);
    EM::FileContent file_content{file_content_10Q};

    const auto document_sections_10Q{LocateDocumentSections(file_content)};

    auto labels_document = LocateLabelDocument(document_sections_10Q, FILE_WITH_XML_10Q);
    ASSERT_NO_THROW(ParseXMLContent(labels_document));
}

TEST_F(ParseDocumentContent, VerifyCanParseLabelsDocument10K)
{
    auto file_content_10K = LoadDataFileForUse(FILE_WITH_XML_10K);
    EM::FileContent file_content{file_content_10K};

    const auto document_sections_10K{LocateDocumentSections(file_content)};

    auto labels_document = LocateLabelDocument(document_sections_10K, FILE_WITH_XML_10K);
    ASSERT_NO_THROW(ParseXMLContent(labels_document));
}

class ExtractDocumentContent : public Test
{

};

TEST_F(ExtractDocumentContent, VerifyCanExtractFilingData10Q)
{
    auto file_content_10Q = LoadDataFileForUse(FILE_WITH_XML_10Q);
    EM::FileContent file_content{file_content_10Q};

    const auto document_sections_10Q{LocateDocumentSections(file_content)};

    auto instance_document = LocateInstanceDocument(document_sections_10Q, FILE_WITH_XML_10Q);
    auto instance_xml = ParseXMLContent(instance_document);

    const auto& [a, b, c, d] = ExtractFilingData(instance_xml);

    ASSERT_TRUE(AllNotEmpty(a, b, c, d));
}

TEST_F(ExtractDocumentContent, VerifyCanExtractFilingData10K)
{
    auto file_content_10K = LoadDataFileForUse(FILE_WITH_XML_10K);
    EM::FileContent file_content{file_content_10K};

    const auto document_sections_10K{LocateDocumentSections(file_content)};

    auto instance_document = LocateInstanceDocument(document_sections_10K, FILE_WITH_XML_10K);
    auto instance_xml = ParseXMLContent(instance_document);

    const auto& [a, b, c, d] = ExtractFilingData(instance_xml);

    ASSERT_TRUE(AllNotEmpty(a, b, c, d));
}

TEST_F(ExtractDocumentContent, VerifyCanExtractFilingDataNoSharesOut10K)
{
    auto file_content_10K = LoadDataFileForUse(NO_SHARES_OUT);
    EM::FileContent file_content{file_content_10K};

    const auto document_sections_10K{LocateDocumentSections(file_content)};

    auto instance_document = LocateInstanceDocument(document_sections_10K, NO_SHARES_OUT);
    auto instance_xml = ParseXMLContent(instance_document);

    const auto& [a, b, c, d] = ExtractFilingData(instance_xml);

    // there is no symbol in this file either.
    ASSERT_TRUE(AllNotEmpty(b, c, d));
}

TEST_F(ExtractDocumentContent, VerifyCanExtractGAAP10Q)
{
    auto file_content_10Q = LoadDataFileForUse(FILE_WITH_XML_10Q);
    EM::FileContent file_content{file_content_10Q};

    const auto document_sections_10Q{LocateDocumentSections(file_content)};

    auto instance_document = LocateInstanceDocument(document_sections_10Q, FILE_WITH_XML_10Q);
    auto instance_xml = ParseXMLContent(instance_document);

    auto gaap_data = ExtractGAAPFields(instance_xml);

    ASSERT_EQ(gaap_data.size(), 192);
}

TEST_F(ExtractDocumentContent, VerifyCanExtractGAAPNoNamespace10Q)
{
    auto file_content_10Q = LoadDataFileForUse(FILE_NO_NAMESPACE_10Q);
    EM::FileContent file_content{file_content_10Q};

    const auto document_sections_10Q{LocateDocumentSections(file_content)};

    auto instance_document = LocateInstanceDocument(document_sections_10Q, FILE_NO_NAMESPACE_10Q);
    auto instance_xml = ParseXMLContent(instance_document);

    auto gaap_data = ExtractGAAPFields(instance_xml);

    ASSERT_EQ(gaap_data.size(), 699);
}

TEST_F(ExtractDocumentContent, VerifyCanExtractGAAP10K)
{
    auto file_content_10K = LoadDataFileForUse(FILE_WITH_XML_10K);
    EM::FileContent file_content{file_content_10K};

    const auto document_sections_10K{LocateDocumentSections(file_content)};

    auto instance_document = LocateInstanceDocument(document_sections_10K, FILE_WITH_XML_10K);
    auto instance_xml = ParseXMLContent(instance_document);

    auto gaap_data = ExtractGAAPFields(instance_xml);

    ASSERT_EQ(gaap_data.size(), 1958);
}

TEST_F(ExtractDocumentContent, VerifyCanExtractLabels10Q)
{
    auto file_content_10Q = LoadDataFileForUse(FILE_WITH_XML_10Q);
    EM::FileContent file_content{file_content_10Q};

    const auto document_sections_10Q{LocateDocumentSections(file_content)};

    auto labels_document = LocateLabelDocument(document_sections_10Q, FILE_WITH_XML_10Q);
    auto labels_xml = ParseXMLContent(labels_document);

    /* auto label_data = ExtractFieldLabels(labels_xml); */
    auto label_data = ExtractFieldLabels(labels_xml);

    ASSERT_EQ(label_data.size(), 125);
}

TEST_F(ExtractDocumentContent, VerifyCanExtractLabelsNoNamespace10Q)
{
    auto file_content_10Q = LoadDataFileForUse(FILE_NO_NAMESPACE_10Q);
    EM::FileContent file_content{file_content_10Q};

    const auto document_sections_10Q{LocateDocumentSections(file_content)};

    auto labels_document = LocateLabelDocument(document_sections_10Q, FILE_NO_NAMESPACE_10Q);
    auto labels_xml = ParseXMLContent(labels_document);

    auto label_data = ExtractFieldLabels(labels_xml);

    ASSERT_EQ(label_data.size(), 352);
}

TEST_F(ExtractDocumentContent, VerifyCanExtractLabelsMultipleLabelLinks10Q)
{
    auto file_content_10Q = LoadDataFileForUse(FILE_MULTIPLE_LABEL_LINKS);
    EM::FileContent file_content{file_content_10Q};

    const auto document_sections_10Q{LocateDocumentSections(file_content)};

    auto labels_document = LocateLabelDocument(document_sections_10Q, FILE_MULTIPLE_LABEL_LINKS);
    auto labels_xml = ParseXMLContent(labels_document);

    auto label_data = ExtractFieldLabels(labels_xml);

    ASSERT_EQ(label_data.size(), 158);
}

TEST_F(ExtractDocumentContent, VerifyCanExtractLabels10K)
{
    auto file_content_10K = LoadDataFileForUse(FILE_WITH_XML_10K);
    EM::FileContent file_content{file_content_10K};

    const auto document_sections_10K{LocateDocumentSections(file_content)};

    auto labels_document = LocateLabelDocument(document_sections_10K, FILE_WITH_XML_10K);
    auto labels_xml = ParseXMLContent(labels_document);

    auto label_data = ExtractFieldLabels(labels_xml);

    ASSERT_EQ(label_data.size(), 746);
}

TEST_F(ExtractDocumentContent, VerifyCanExtractContexts10Q)
{
    auto file_content_10Q = LoadDataFileForUse(FILE_WITH_XML_10Q);
    EM::FileContent file_content{file_content_10Q};

    const auto document_sections_10Q{LocateDocumentSections(file_content)};

    auto instance_document = LocateInstanceDocument(document_sections_10Q, FILE_WITH_XML_10Q);
    auto instance_xml = ParseXMLContent(instance_document);

    auto context_data = ExtractContextDefinitions(instance_xml);

    ASSERT_EQ(context_data.size(), 37);
}

TEST_F(ExtractDocumentContent, VerifyCanExtractContextsSomeNamespace10Q)
{
    auto file_content_10Q = LoadDataFileForUse(FILE_SOME_NAMESPACE_10Q);
    EM::FileContent file_content{file_content_10Q};

    const auto document_sections_10Q{LocateDocumentSections(file_content)};

    auto instance_document = LocateInstanceDocument(document_sections_10Q, FILE_SOME_NAMESPACE_10Q);
    auto instance_xml = ParseXMLContent(instance_document);

    auto context_data = ExtractContextDefinitions(instance_xml);

    ASSERT_EQ(context_data.size(), 12);
}

TEST_F(ExtractDocumentContent, VerifyCanExtractContexts10K)
{
    auto file_content_10K = LoadDataFileForUse(FILE_WITH_XML_10K);
    EM::FileContent file_content{file_content_10K};

    const auto document_sections_10K{LocateDocumentSections(file_content)};

    auto instance_document = LocateInstanceDocument(document_sections_10K, FILE_WITH_XML_10K);
    auto instance_xml = ParseXMLContent(instance_document);

    auto context_data = ExtractContextDefinitions(instance_xml);

    ASSERT_EQ(context_data.size(), 492);
}

TEST_F(ExtractDocumentContent, VerifyCanMatchGAAPDataWithUserLabel10Q)
{
    auto file_content_10Q = LoadDataFileForUse(FILE_WITH_XML_10Q);
    EM::FileContent file_content{file_content_10Q};

    const auto document_sections_10Q{LocateDocumentSections(file_content)};

    auto labels_document = LocateLabelDocument(document_sections_10Q, FILE_WITH_XML_10Q);
    auto labels_xml = ParseXMLContent(labels_document);

    auto label_data = ExtractFieldLabels(labels_xml);

    auto instance_document = LocateInstanceDocument(document_sections_10Q, FILE_WITH_XML_10Q);
    auto instance_xml = ParseXMLContent(instance_document);

    auto gaap_data = ExtractGAAPFields(instance_xml);

    int result = FindAllLabels(gaap_data, label_data);

    ASSERT_TRUE(result == 0);
}

TEST_F(ExtractDocumentContent, VerifyCanMatchGAAPDataWithUserLabelBadFile210K)
{
    auto file_content_10K = LoadDataFileForUse(BAD_FILE2);
    EM::FileContent file_content{file_content_10K};

    const auto document_sections_10K{LocateDocumentSections(file_content)};

    auto labels_document = LocateLabelDocument(document_sections_10K, BAD_FILE2);
    auto labels_xml = ParseXMLContent(labels_document);

    auto label_data = ExtractFieldLabels(labels_xml);

    auto instance_document = LocateInstanceDocument(document_sections_10K, BAD_FILE2);
    auto instance_xml = ParseXMLContent(instance_document);

    auto gaap_data = ExtractGAAPFields(instance_xml);

    int result = FindAllLabels(gaap_data, label_data);

    ASSERT_TRUE(result == 0);
}

TEST_F(ExtractDocumentContent, VerifyCanMatchGAAPDataWithUserLabelNoNamespace10Q)
{
    auto file_content_10Q = LoadDataFileForUse(FILE_NO_NAMESPACE_10Q);
    EM::FileContent file_content{file_content_10Q};

    const auto document_sections_10Q{LocateDocumentSections(file_content)};

    auto labels_document = LocateLabelDocument(document_sections_10Q, FILE_NO_NAMESPACE_10Q);
    auto labels_xml = ParseXMLContent(labels_document);

    auto label_data = ExtractFieldLabels(labels_xml);

    auto instance_document = LocateInstanceDocument(document_sections_10Q, FILE_NO_NAMESPACE_10Q);
    auto instance_xml = ParseXMLContent(instance_document);

    auto gaap_data = ExtractGAAPFields(instance_xml);

    int result = FindAllLabels(gaap_data, label_data);

    ASSERT_TRUE(result == 0);
}

TEST_F(ExtractDocumentContent, VerifyCanMatchGAAPDataWithUserLabelSomeNamespace10Q)
{
    auto file_content_10Q = LoadDataFileForUse(FILE_SOME_NAMESPACE_10Q);
    EM::FileContent file_content{file_content_10Q};

    const auto document_sections_10Q{LocateDocumentSections(file_content)};

    auto labels_document = LocateLabelDocument(document_sections_10Q, FILE_SOME_NAMESPACE_10Q);
    auto labels_xml = ParseXMLContent(labels_document);

    auto label_data = ExtractFieldLabels(labels_xml);

    auto instance_document = LocateInstanceDocument(document_sections_10Q, FILE_SOME_NAMESPACE_10Q);
    auto instance_xml = ParseXMLContent(instance_document);

    auto gaap_data = ExtractGAAPFields(instance_xml);

    int result = FindAllLabels(gaap_data, label_data);

    ASSERT_TRUE(result == 0);
}

TEST_F(ExtractDocumentContent, VerifyCanMatchGAAPDataWithUserLabel10K)
{
    auto file_content_10K = LoadDataFileForUse(FILE_WITH_XML_10K);
    EM::FileContent file_content{file_content_10K};

    const auto document_sections_10K{LocateDocumentSections(file_content)};

    auto labels_document = LocateLabelDocument(document_sections_10K, FILE_WITH_XML_10K);
    auto labels_xml = ParseXMLContent(labels_document);

    auto label_data = ExtractFieldLabels(labels_xml);

    auto instance_document = LocateInstanceDocument(document_sections_10K, FILE_WITH_XML_10K);
    auto instance_xml = ParseXMLContent(instance_document);

    auto context_data = ExtractContextDefinitions(instance_xml);
    auto gaap_data = ExtractGAAPFields(instance_xml);

    int result = FindAllLabels(gaap_data, label_data);

    ASSERT_TRUE(result == 0);
}

TEST_F(ExtractDocumentContent, VerifyCanMatchGAAPDataWithUserLabelMissingValues10K)
{
    auto file_content_10K = LoadDataFileForUse(MISSING_VALUES1_10K);
    EM::FileContent file_content{file_content_10K};

    const auto document_sections_10K{LocateDocumentSections(file_content)};

    auto labels_document = LocateLabelDocument(document_sections_10K, MISSING_VALUES1_10K);
    auto labels_xml = ParseXMLContent(labels_document);

    auto label_data = ExtractFieldLabels(labels_xml);

    auto instance_document = LocateInstanceDocument(document_sections_10K, MISSING_VALUES1_10K);
    auto instance_xml = ParseXMLContent(instance_document);

    auto gaap_data = ExtractGAAPFields(instance_xml);

    int result = FindAllLabels(gaap_data, label_data);

    ASSERT_TRUE(result == 0);
}

TEST_F(ExtractDocumentContent, DISABLED_VerifyCanMatchGAAPDataWithUserLabelMissingValues210K)
{
    // disbled because all labels are now found. also, this is using an ammended file
    // which is not intended.

    auto file_content_10K = LoadDataFileForUse(MISSING_VALUES2_10K);
    EM::FileContent file_content{file_content_10K};

    const auto document_sections_10K{LocateDocumentSections(file_content)};

    auto labels_document = LocateLabelDocument(document_sections_10K, MISSING_VALUES2_10K);
    auto labels_xml = ParseXMLContent(labels_document);

    auto label_data = ExtractFieldLabels(labels_xml);
    std::cout << "labels: " << label_data.size() << '\n';

    auto instance_document = LocateInstanceDocument(document_sections_10K, MISSING_VALUES2_10K);
    auto instance_xml = ParseXMLContent(instance_document);

    auto gaap_data = ExtractGAAPFields(instance_xml);
    std::cout << "gaap data: " << gaap_data.size() << '\n';

    int result = FindAllLabels(gaap_data, label_data);

    // some values really are missing from file.
    ASSERT_EQ(result, 5);
}

TEST_F(ExtractDocumentContent, VerifyCanMatchGAAPDataWithContext10Q)
{
    auto file_content_10Q = LoadDataFileForUse(FILE_WITH_XML_10Q);
    EM::FileContent file_content{file_content_10Q};

    const auto document_sections_10Q{LocateDocumentSections(file_content)};

    auto instance_document = LocateInstanceDocument(document_sections_10Q, FILE_WITH_XML_10Q);
    auto instance_xml = ParseXMLContent(instance_document);

    auto gaap_data = ExtractGAAPFields(instance_xml);

    auto context_data = ExtractContextDefinitions(instance_xml);

    bool result = FindAllContexts(gaap_data, context_data);

    ASSERT_TRUE(result);
}

TEST_F(ExtractDocumentContent, VerifyCanMatchGAAPDataWithContextSomeNamespace10Q)
{
    auto file_content_10Q = LoadDataFileForUse(FILE_SOME_NAMESPACE_10Q);
    EM::FileContent file_content{file_content_10Q};

    const auto document_sections_10Q{LocateDocumentSections(file_content)};

    auto instance_document = LocateInstanceDocument(document_sections_10Q, FILE_SOME_NAMESPACE_10Q);
    auto instance_xml = ParseXMLContent(instance_document);

    auto gaap_data = ExtractGAAPFields(instance_xml);

    auto context_data = ExtractContextDefinitions(instance_xml);

    bool result = FindAllContexts(gaap_data, context_data);

    ASSERT_TRUE(result);
}

TEST_F(ExtractDocumentContent, VerifyCanMatchGAAPDataWithContext10K)
{
    auto file_content_10K = LoadDataFileForUse(FILE_WITH_XML_10K);
    EM::FileContent file_content{file_content_10K};

    const auto document_sections_10K{LocateDocumentSections(file_content)};

    auto instance_document = LocateInstanceDocument(document_sections_10K, FILE_WITH_XML_10K);
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
            auto file_content_dir = LoadDataFileForUse(EM::FileName{dir_ent.path().c_str()});
            EM::FileContent file_content{file_content_dir};
            const auto sections = LocateDocumentSections(file_content);

            SEC_Header SEC_data;
            SEC_data.UseData(file_content);
            SEC_data.ExtractHeaderFields();

            FileHasXBRL filter;
            bool has_XML = filter(SEC_data.GetFields(), sections);
            if (has_XML)
            {
                ++files_with_XML;
            }
        }
    });

    std::for_each(fs::recursive_directory_iterator(EDGAR_DIRECTORY.get()), fs::recursive_directory_iterator(), test_file);

    ASSERT_EQ(files_with_XML, 159);
}

TEST_F(ValidateFolderFilters, VerifyFindAll10Q)
{
    int files_with_form{0};

    auto test_file([&files_with_form](const auto& dir_ent)
    {
        if (dir_ent.status().type() == fs::file_type::regular)
        {
            auto file_content_dir = LoadDataFileForUse(EM::FileName{dir_ent.path().c_str()});
            EM::FileContent file_content{file_content_dir};
            const auto sections = LocateDocumentSections(file_content);

            SEC_Header SEC_data;
            SEC_data.UseData(file_content);
            SEC_data.ExtractHeaderFields();

            FileHasXBRL filter1;
            std::vector<std::string> forms{"10-Q"};
            FileHasFormType filter2{forms};

            bool has_form = ApplyFilters(SEC_data.GetFields(), sections, filter1, filter2);
            if (has_form)
            {
                ++files_with_form;
            }
        }
    });

    std::for_each(fs::recursive_directory_iterator(EDGAR_DIRECTORY.get()), fs::recursive_directory_iterator(), test_file);

    ASSERT_EQ(files_with_form, 157);
}

TEST_F(ValidateFolderFilters, VerifyFindAll10K)
{
    int files_with_form{0};

    auto test_file([&files_with_form](const auto& dir_ent)
    {
        if (dir_ent.status().type() == fs::file_type::regular)
        {
            auto file_content_dir = LoadDataFileForUse(EM::FileName{dir_ent.path().c_str()});
            EM::FileContent file_content{file_content_dir};
            const auto sections = LocateDocumentSections(file_content);

            SEC_Header SEC_data;
            SEC_data.UseData(file_content);
            SEC_data.ExtractHeaderFields();

            FileHasXBRL filter1;
            std::vector<std::string> forms{"10-K"};
            FileHasFormType filter2{forms};

            bool has_form = ApplyFilters(SEC_data.GetFields(), sections, filter1, filter2);
            if (has_form)
            {
                ++files_with_form;
            }
        }
    });

    std::for_each(fs::recursive_directory_iterator(EDGAR_DIRECTORY.get()), fs::recursive_directory_iterator(), test_file);

    ASSERT_EQ(files_with_form, 1);
}

TEST_F(ValidateFolderFilters, VerifyFindAllInDateRange)
{
    int files_with_form{0};

    auto test_file([&files_with_form](const auto& dir_ent)
    {
        if (dir_ent.status().type() == fs::file_type::regular)
        {
            auto file_content_dir = LoadDataFileForUse(EM::FileName{dir_ent.path().c_str()});
            EM::FileContent file_content{file_content_dir};
            const auto sections = LocateDocumentSections(file_content);

            SEC_Header SEC_data;
            SEC_data.UseData(file_content);
            SEC_data.ExtractHeaderFields();

            FileHasXBRL filter1;
            using namespace date;
            FileIsWithinDateRange filter2{year_month_day{2013_y/January/1}, year_month_day{2013_y/9/30}};

            bool has_form = ApplyFilters(SEC_data.GetFields(), sections, filter1, filter2);
            if (has_form)
            {
                ++files_with_form;
            }
        }
    });

    std::for_each(fs::recursive_directory_iterator(EDGAR_DIRECTORY.get()), fs::recursive_directory_iterator(), test_file);

    ASSERT_EQ(files_with_form, 151);
}

TEST_F(ValidateFolderFilters, VerifyFindAllInDateRangeNoMatches)
{
    int files_with_form{0};

    auto test_file([&files_with_form](const auto& dir_ent)
    {
        if (dir_ent.status().type() == fs::file_type::regular)
        {
            auto file_content_dir = LoadDataFileForUse(EM::FileName{dir_ent.path().c_str()});
            EM::FileContent file_content{file_content_dir};
            const auto sections = LocateDocumentSections(file_content);

            SEC_Header SEC_data;
            SEC_data.UseData(file_content);
            SEC_data.ExtractHeaderFields();

            using namespace date;
            FileHasXBRL filter1;
            FileIsWithinDateRange filter2{year_month_day{2015_y/January/1}, year_month_day{2015_y/9/30}};

            bool has_form = ApplyFilters(SEC_data.GetFields(), sections, filter1, filter2);
            if (has_form)
            {
                ++files_with_form;
            }
        }
    });

    std::for_each(fs::recursive_directory_iterator(EDGAR_DIRECTORY.get()), fs::recursive_directory_iterator(), test_file);

    ASSERT_EQ(files_with_form, 0);
}

TEST_F(ValidateFolderFilters, VerifyComboFiltersWithMatches)
{
    int files_with_form{0};

    auto test_file([&files_with_form](const auto& dir_ent)
    {
        if (dir_ent.status().type() == fs::file_type::regular)
        {
            auto file_content_dir = LoadDataFileForUse(EM::FileName{dir_ent.path().c_str()});
            EM::FileContent file_content{file_content_dir};
            const auto sections = LocateDocumentSections(file_content);

            SEC_Header SEC_data;
            SEC_data.UseData(file_content);
            SEC_data.ExtractHeaderFields();

            using namespace date;
            FileHasXBRL filter1;
            std::vector<std::string> forms{"10-Q"};
            FileHasFormType filter2{forms};
            FileIsWithinDateRange filter3{year_month_day{2013_y/3/1}, year_month_day{2013_y/3/31}};

            bool has_form = ApplyFilters(SEC_data.GetFields(), sections, filter1, filter2, filter3);
            if (has_form)
            {
                ++files_with_form;
            }
        }
    });

    std::for_each(fs::recursive_directory_iterator(EDGAR_DIRECTORY.get()), fs::recursive_directory_iterator(), test_file);

    ASSERT_EQ(files_with_form, 5);
}

TEST_F(ValidateFolderFilters, VerifyFindFormsInFileNameList)
{
    std::vector<std::string> list_of_files_to_process;

    std::ifstream input_file{TEST_FILE_LIST.get()};

    // Tell the stream to use our facet, so only '\n' is treated as a space.

    input_file.imbue(std::locale(input_file.getloc(), new line_only_whitespace()));

    std::istream_iterator<std::string> itor{input_file};
    std::istream_iterator<std::string> itor_end;
    std::copy(
        itor,
        itor_end,
        std::back_inserter(list_of_files_to_process)
    );
    input_file.close();
    
    std::vector<std::string> forms1{"10-Q"};
    auto qs = std::count_if(std::begin(list_of_files_to_process), std::end(list_of_files_to_process), [&forms1](const auto &fname) { return FormIsInFileName(forms1, EM::FileName{fname}); });
    EXPECT_EQ(qs, 114);

    std::vector<std::string> forms2{"10-K"};
    auto ks = std::count_if(std::begin(list_of_files_to_process), std::end(list_of_files_to_process), [&forms2](const auto &fname) { return FormIsInFileName(forms2, EM::FileName{fname}); });
    EXPECT_EQ(ks, 25);

    std::vector<std::string> forms3{"10-K", "10-Q"};
    auto kqs = std::count_if(std::begin(list_of_files_to_process), std::end(list_of_files_to_process), [&forms3](const auto &fname) { return FormIsInFileName(forms3, EM::FileName{fname}); });
    ASSERT_EQ(kqs, 139);
}

class ProcessAmendedForms : public Test
{

};

TEST_F(ProcessAmendedForms, VerifyFormTypeEditsWorkWithAmendedFiles)
{
    auto file_content_10Q = LoadDataFileForUse(ORIGINAL_10Q);
    EM::FileContent file_content{file_content_10Q};
    const auto sections = LocateDocumentSections(file_content);

    SEC_Header SEC_data;
    SEC_data.UseData(file_content);
    SEC_data.ExtractHeaderFields();

    std::vector<std::string> forms{"10-Q"};
    FileHasFormType filter1{forms};

    bool has_form = ApplyFilters(SEC_data.GetFields(), sections, filter1);
    EXPECT_EQ(has_form, true);

    // NOTE: code internally uses '_' instead of '/' so we
    // need to code it that way here.

    std::vector<std::string> forms_a{"10-Q_A"};
    FileHasFormType filter2{forms_a};

    has_form = ApplyFilters(SEC_data.GetFields(), sections, filter2);

    EXPECT_EQ(has_form, false);

    auto file_content_10Q_a = LoadDataFileForUse(AMENDED_10Q);
    EM::FileContent file_content_a{file_content_10Q_a};
    const auto sections_a = LocateDocumentSections(file_content_a);

    SEC_Header SEC_data_a;
    SEC_data_a.UseData(file_content_a);
    SEC_data_a.ExtractHeaderFields();

    has_form = ApplyFilters(SEC_data_a.GetFields(), sections_a, filter1);
    EXPECT_EQ(has_form, false);

    has_form = ApplyFilters(SEC_data_a.GetFields(), sections_a, filter2);

    EXPECT_EQ(has_form, true);

    std::vector<std::string> forms_b{"10-Q", "10-Q_A"};
    FileHasFormType filter3{forms_b};

    has_form = ApplyFilters(SEC_data.GetFields(), sections, filter3);
    EXPECT_EQ(has_form, true);

    has_form = ApplyFilters(SEC_data_a.GetFields(), sections_a, filter3);

    ASSERT_EQ(has_form, true);
}

TEST_F(ProcessAmendedForms, CanReadAmended10Q)
{
    auto file_content_10Q = LoadDataFileForUse(AMENDED_10Q);
    EM::FileContent file_content{file_content_10Q};

    const auto document_sections_10Q{LocateDocumentSections(file_content)};

    auto labels_document = LocateLabelDocument(document_sections_10Q, AMENDED_10Q);
    auto labels_xml = ParseXMLContent(labels_document);

    auto label_data = ExtractFieldLabels(labels_xml);

    auto instance_document = LocateInstanceDocument(document_sections_10Q, AMENDED_10Q);
    auto instance_xml = ParseXMLContent(instance_document);

    auto gaap_data = ExtractGAAPFields(instance_xml);

    int result = FindAllLabels(gaap_data, label_data);

    ASSERT_TRUE(result == 0);
}

TEST_F(ProcessAmendedForms, CompareOriginalAndAmended10Qs)
{
    auto orig_file_content_10Q = LoadDataFileForUse(ORIGINAL_10Q);
    EM::FileContent orig_file_content{orig_file_content_10Q};

    const auto orig_document_sections_10Q{LocateDocumentSections(orig_file_content)};

    auto orig_labels_document = LocateLabelDocument(orig_document_sections_10Q, ORIGINAL_10Q);
    auto orig_labels_xml = ParseXMLContent(orig_labels_document);

    auto orig_label_data = ExtractFieldLabels(orig_labels_xml);

    auto orig_instance_document = LocateInstanceDocument(orig_document_sections_10Q, ORIGINAL_10Q);
    auto orig_instance_xml = ParseXMLContent(orig_instance_document);

    auto orig_gaap_data = ExtractGAAPFields(orig_instance_xml);
//    ranges::for_each(orig_gaap_data, [](const auto& d) { std::cout << d.label << '\n'; }); 
    EXPECT_EQ(orig_gaap_data.size(), 397);
    orig_gaap_data = std::move(orig_gaap_data) | ranges::actions::sort([](const auto& lhs, const auto& rhs) { return lhs.label < rhs.label; }) ;

    int orig_result = FindAllLabels(orig_gaap_data, orig_label_data);

    EXPECT_TRUE(orig_result == 0);

    auto file_content_10Q = LoadDataFileForUse(AMENDED_10Q);
    EM::FileContent file_content{file_content_10Q};

    const auto document_sections_10Q{LocateDocumentSections(file_content)};

    auto labels_document = LocateLabelDocument(document_sections_10Q, AMENDED_10Q);
    auto labels_xml = ParseXMLContent(labels_document);

    auto label_data = ExtractFieldLabels(labels_xml);

    auto instance_document = LocateInstanceDocument(document_sections_10Q, AMENDED_10Q);
    auto instance_xml = ParseXMLContent(instance_document);

    auto gaap_data = ExtractGAAPFields(instance_xml);
    gaap_data = std::move(gaap_data) | ranges::actions::sort([](const auto& lhs, const auto& rhs) { return lhs.label < rhs.label; }) ;
    EXPECT_EQ(gaap_data.size(), 680);

    int result = FindAllLabels(gaap_data, label_data);

    EXPECT_TRUE(result == 0);

    ASSERT_FALSE(ranges::equal(orig_gaap_data, gaap_data, [](const auto& lhs, const auto& rhs) { return lhs.label == rhs.label && lhs.value == rhs.value; }));

//    PrintRangeDifferences(orig_gaap_data, gaap_data);
}

class ProcessXLSXContent : public Test
{

};

TEST_F(ProcessXLSXContent, CanProcess10KAmendedFile1)
{
    auto file_content_10K = LoadDataFileForUse(XLS_SHEET_1);
    EM::FileContent file_content{file_content_10K};

    const auto document_sections_10K{LocateDocumentSections(file_content)};

    auto xls_content = LocateXLSDocument(document_sections_10K, XLS_SHEET_1);
    EXPECT_TRUE(! xls_content.get().empty());

    auto xls_data = ExtractXLSData(xls_content);
    EXPECT_TRUE(! xls_data.empty());

   XLS_File xls_file{std::move(xls_data)};

   int number_of_sheets = ranges::distance(xls_file);
   EXPECT_EQ(number_of_sheets, 20);

   auto bal_sheets = ranges::find_if(xls_file, [] (const auto& x) { return x.GetSheetName() == "balance sheets"; } );
   EXPECT_TRUE(bal_sheets != ranges::end(xls_file));

   int rows = ranges::distance(*bal_sheets);
   EXPECT_EQ(rows, 16);

   auto stmt_of_ops = ranges::find_if(xls_file, [] (const auto& x) { return x.GetSheetName() == "statements of operations"; } );
   EXPECT_TRUE(stmt_of_ops != ranges::end(xls_file));

   int rows2 = ranges::distance(*stmt_of_ops);
   EXPECT_EQ(rows2, 18);

   auto cash_flows = ranges::find_if(xls_file, [] (const auto& x) { return x.GetSheetName() == "statements of cash flows"; } );
   EXPECT_TRUE(cash_flows != ranges::end(xls_file));

   int rows3 = ranges::distance(*cash_flows);
   ASSERT_EQ(rows3, 20);

}

TEST_F(ProcessXLSXContent, CanProcess10QFile1)
{
    auto file_content_10Q = LoadDataFileForUse(XLS_SHEET_2);
    EM::FileContent file_content{file_content_10Q};

    const auto document_sections_10Q{LocateDocumentSections(file_content)};

    auto xls_content = LocateXLSDocument(document_sections_10Q, XLS_SHEET_2);
    EXPECT_TRUE(! xls_content.get().empty());

    auto xls_data = ExtractXLSData(xls_content);
    EXPECT_TRUE(! xls_data.empty());

   XLS_File xls_file{std::move(xls_data)};

   int number_of_sheets = ranges::distance(xls_file);
   EXPECT_EQ(number_of_sheets, 69);

   auto bal_sheets = ranges::find_if(xls_file, [] (const auto& x) { return (x.GetSheetNameFromInside().find("balance sheets") != std::string::npos); } );
   EXPECT_TRUE(bal_sheets != ranges::end(xls_file));

   int rows = ranges::distance(*bal_sheets);
   EXPECT_EQ(rows, 40);

   auto stmt_of_ops = ranges::find_if(xls_file, [] (const auto& x) { return x.GetSheetNameFromInside().find("statements of operations") != std::string::npos; } );
   EXPECT_TRUE(stmt_of_ops != ranges::end(xls_file));

   int rows2 = ranges::distance(*stmt_of_ops);
   EXPECT_EQ(rows2, 35);

   auto cash_flows = ranges::find_if(xls_file, [] (const auto& x) { return x.GetSheetNameFromInside().find("statements of cash flows") != std::string::npos; } );
   EXPECT_TRUE(cash_flows != ranges::end(xls_file));

   int rows3 = ranges::distance(*cash_flows);
   ASSERT_EQ(rows3, 44);

}

TEST_F(ProcessXLSXContent, CanProcess10QFile1HighLevel)
{
    auto file_content_10Q = LoadDataFileForUse(XLS_SHEET_2);
    EM::FileContent file_content{file_content_10Q};

    const auto document_sections_10Q{LocateDocumentSections(file_content)};

    auto financial_content = FindAndExtractXLSContent(document_sections_10Q, XLS_SHEET_2);
    EXPECT_TRUE(financial_content.has_data());

}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  InitLogging
 *  Description:  
 * =====================================================================================
 */
void InitLogging ()
{
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
