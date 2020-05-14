/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <orcus/sax_ns_parser.hpp>
#include <orcus/stream.hpp>
#include <orcus/xml_namespace.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <vector>

namespace po = boost::program_options;
using std::cout;
using std::cerr;
using std::endl;

class xml_handler : public orcus::sax_ns_handler
{
public:

};

void parse_file(const std::string& filepath)
{
    cout << "--" << endl;
    orcus::file_content content(filepath.data());
    cout << "filepath: " << filepath << " (size: " << content.size() << ")" << endl;

    orcus::xmlns_repository repo;
    auto cxt = repo.create_context();
    xml_handler hdl;
    orcus::sax_ns_parser<xml_handler> parser(content.data(), content.size(), cxt, hdl);
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
