/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <orcus/sax_ns_parser.hpp>
#include <orcus/stream.hpp>
#include <orcus/types.hpp>
#include <orcus/string_pool.hpp>
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
    orcus::string_pool& m_shared_pool;
    std::vector<orcus::xml_name_t> m_stack;

public:
    xml_handler(orcus::string_pool& pool) : m_shared_pool(pool) {}

    void start_element(const orcus::sax_ns_parser_element& elem)
    {
        m_stack.emplace_back(elem.ns, elem.name);
        m_shared_pool.intern(elem.name);
    }

    void end_element(const orcus::sax_ns_parser_element& elem)
    {
        orcus::xml_name_t name(elem.ns, elem.name);
        if (m_stack.empty() || m_stack.back() != name)
            throw std::runtime_error("mis-matching element!");

        m_stack.pop_back();
    }

    void attribute(const orcus::pstring& /*name*/, const orcus::pstring& /*val*/) {}

    void attribute(const orcus::sax_ns_parser_attribute& attr)
    {
        m_shared_pool.intern(attr.name);
    }
};

class token_collector
{
    orcus::string_pool m_pool;
    const size_t m_file_count;
    size_t m_counter = 0;

public:
    token_collector(size_t file_count) : m_file_count(file_count) {}

    void parse_file(const std::string& filepath)
    {
        orcus::file_content content(filepath.data());
        cout << "[" << ++m_counter << "/" << m_file_count << "] filepath: " << filepath << " (size: " << content.size() << ")" << endl;
        orcus::xmlns_repository repo;
        auto cxt = repo.create_context();
        xml_handler hdl(m_pool);
        orcus::sax_ns_parser<xml_handler> parser(content.data(), content.size(), cxt, hdl);
        parser.parse();
    }

    void dump_tokens()
    {
        m_pool.dump();
    }
};

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

    token_collector collector(input_files.size());
    for (const std::string& filepath : input_files)
        collector.parse_file(filepath);

    collector.dump_tokens();

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
