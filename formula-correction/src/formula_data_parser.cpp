/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <iostream>

#include "formula_xml_processor.hpp"

namespace po = boost::program_options;
namespace fs = boost::filesystem;
using std::cout;
using std::cerr;
using std::endl;

int main(int argc, char** argv)
{
    bool verbose = false;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Print this help.")
        ("verbose,v", po::bool_switch(&verbose), "Verbose output.")
        ("debug,d", po::value<std::string>(), "Debug output directory.")
        ("output,o", po::value<std::string>(), "Output directory.");

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

    if (!vm.count("output"))
    {
        cerr << "output directory path is required." << endl;
        return EXIT_FAILURE;
    }

    if (!vm.count("input-files"))
        return EXIT_SUCCESS;

    std::vector<std::string> input_files = vm["input-files"].as<std::vector<std::string>>();
    fs::path output_dir(vm["output"].as<std::string>());

    try
    {
        fs::create_directory(output_dir);
    }
    catch (const fs::filesystem_error& e)
    {
        cerr << e.what() << endl;
        return EXIT_FAILURE;
    }

    fs::path debug_dir;

    if (vm.count("debug"))
    {
        debug_dir = vm["debug"].as<std::string>();

        try
        {
            fs::create_directory(debug_dir);
        }
        catch (const fs::filesystem_error& e)
        {
            cerr << e.what() << endl;
            return EXIT_FAILURE;
        }
    }

    formula_xml_processor p(output_dir, debug_dir, verbose);
    p.parse_files(input_files);
    p.write_files();

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
