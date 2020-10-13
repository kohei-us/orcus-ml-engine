/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "trie_loader.hpp"

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <orcus/global.hpp>

#include <iostream>
#include <fstream>

namespace po = boost::program_options;
namespace fs = boost::filesystem;
using std::cout;
using std::cerr;
using std::endl;

trie_loader::mode_type to_mode_enum(const std::string& s)
{
    const char* names[] = { "name", "symbol", "value" };
    size_t n = ORCUS_N_ELEMENTS(names);

    for (size_t i = 0; i < n; ++i)
        if (s == names[i])
            return static_cast<trie_loader::mode_type>(i);

    return trie_loader::mode_type::UNKNOWN;
}

int main(int argc, char** argv)
{
    bool verbose = false;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Print this help.")
        ("verbose,v", po::bool_switch(&verbose), "Verbose output.")
        ("mode,m", po::value<std::string>(), "Interpretation mode. Either choose 'name', 'symbol' or 'value'.")
        ("output,o", po::value<std::string>(), "Output file.");

    po::options_description hidden("Hidden options");
    hidden.add_options()
        ("input-file", po::value<std::string>(), "input file");

    po::options_description cmd_opt;
    cmd_opt.add(desc).add(hidden);

    po::positional_options_description po_desc;
    po_desc.add("input-file", 1);

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

    if (!vm.count("input-file"))
        return EXIT_SUCCESS;

    trie_loader::mode_type mode = trie_loader::NAME;
    if (vm.count("mode"))
    {
        std::string mode_s = vm["mode"].as<std::string>();
        mode = to_mode_enum(mode_s);

        if (mode == trie_loader::UNKNOWN)
        {
            cout << "invalid mode: " << mode_s << endl;
            return EXIT_FAILURE;
        }
    }


    trie_loader trie;

    {
        std::ifstream ifs(vm["input-file"].as<std::string>());
        trie.load(ifs);
    }

    cout << "number of entries: " << trie.size() << endl;

    std::ostream* is = &cout;
    std::unique_ptr<std::ofstream> output;

    if (vm.count("output"))
    {
        std::string s = vm["output"].as<std::string>();
        output = std::make_unique<std::ofstream>(s);
        is = output.get();
    }

    trie.dump(*is, mode);

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
