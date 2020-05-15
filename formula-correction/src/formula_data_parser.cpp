/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <orcus/sax_token_parser.hpp>
#include <orcus/stream.hpp>
#include <orcus/types.hpp>
#include <orcus/tokens.hpp>
#include <orcus/xml_namespace.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <vector>

namespace po = boost::program_options;
using std::cout;
using std::cerr;
using std::endl;

const char* token_labels[] = {
    "???",                // 0
    "column",             // 1
    "count",              // 2
    "doc",                // 3
    "error",              // 4
    "filepath",           // 5
    "formula",            // 6
    "formulas",           // 7
    "name",               // 8
    "named-expression",   // 9
    "named-expressions",  // 10
    "origin",             // 11
    "row",                // 12
    "s",                  // 13
    "scope",              // 14
    "sheet",              // 15
    "sheets",             // 16
    "token",              // 17
    "type",               // 18
    "valid",              // 19
};

constexpr orcus::xml_token_t XML_unknown            = 0;
constexpr orcus::xml_token_t XML_column             = 1;
constexpr orcus::xml_token_t XML_count              = 2;
constexpr orcus::xml_token_t XML_doc                = 3;
constexpr orcus::xml_token_t XML_error              = 4;
constexpr orcus::xml_token_t XML_filepath           = 5;
constexpr orcus::xml_token_t XML_formula            = 6;
constexpr orcus::xml_token_t XML_formulas           = 7;
constexpr orcus::xml_token_t XML_name               = 8;
constexpr orcus::xml_token_t XML_named_expression   = 9;
constexpr orcus::xml_token_t XML_named_expressions  = 10;
constexpr orcus::xml_token_t XML_origin             = 11;
constexpr orcus::xml_token_t XML_row                = 12;
constexpr orcus::xml_token_t XML_s                  = 13;
constexpr orcus::xml_token_t XML_scope              = 14;
constexpr orcus::xml_token_t XML_sheet              = 15;
constexpr orcus::xml_token_t XML_sheets             = 16;
constexpr orcus::xml_token_t XML_token              = 17;
constexpr orcus::xml_token_t XML_type               = 18;
constexpr orcus::xml_token_t XML_valid              = 19;

class xml_handler : public orcus::sax_token_handler
{
    using xml_name_t = std::pair<orcus::xmlns_id_t, orcus::xml_token_t>;
    std::vector<xml_name_t> m_stack;

public:

    void start_element(const orcus::xml_token_element_t& elem)
    {
        m_stack.emplace_back(elem.ns, elem.name);

        if (elem.ns)
            return;

        // TODO ...
    }

    void end_element(const orcus::xml_token_element_t& elem)
    {
        xml_name_t name(elem.ns, elem.name);
        if (m_stack.empty() || m_stack.back() != name)
            throw std::runtime_error("mis-matching element!");

        m_stack.pop_back();
    }
};

void parse_file(const std::string& filepath)
{
    cout << "--" << endl;
    orcus::file_content content(filepath.data());
    cout << "filepath: " << filepath << " (size: " << content.size() << ")" << endl;

    orcus::xmlns_repository repo;
    auto cxt = repo.create_context();
    xml_handler hdl;
    orcus::tokens token_map(token_labels, ORCUS_N_ELEMENTS(token_labels));
    orcus::sax_token_parser<xml_handler> parser(content.data(), content.size(), token_map, cxt, hdl);
    parser.parse();
}

int main(int argc, char** argv)
{
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Print this help.");

    po::options_description hidden("Hidden options");
    hidden.add_options()
        ("input-files", po::value<std::vector<std::string>>(), "input file");

    po::options_description cmd_opt;
    cmd_opt.add(desc).add(hidden);

    po::positional_options_description po_desc;
    po_desc.add("input-files", -1);

    po::variables_map vm;
    try
    {
        po::store(
            po::command_line_parser(argc, argv).options(cmd_opt).positional(po_desc).run(), vm);
        po::notify(vm);
    }
    catch (const std::exception& e)
    {
        // Unknown options.
        cout << e.what() << endl;
        cout << desc;
        return EXIT_FAILURE;
    }

    if (vm.count("help"))
    {
        cout << desc;
        return EXIT_SUCCESS;
    }

    if (!vm.count("input-files"))
        return EXIT_SUCCESS;

    std::vector<std::string> input_files = vm["input-files"].as<std::vector<std::string>>();

    for (const std::string& filepath : input_files)
        parse_file(filepath);

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
