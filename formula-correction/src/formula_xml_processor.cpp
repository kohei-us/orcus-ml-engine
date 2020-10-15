/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "formula_xml_processor.hpp"
#include "types.hpp"

#include <mdds/sorted_string_map.hpp>
#include <orcus/sax_token_parser.hpp>
#include <orcus/stream.hpp>
#include <orcus/types.hpp>
#include <orcus/tokens.hpp>
#include <orcus/xml_namespace.hpp>
#include <orcus/string_pool.hpp>
#include <ixion/formula_function_opcode.hpp>

namespace fs = boost::filesystem;
using std::endl;
using orcus::pstring;

namespace {

class scoped_guard
{
    std::thread m_thread;
public:
    scoped_guard(std::thread thread) : m_thread(std::move(thread)) {}
    scoped_guard(scoped_guard&& other) : m_thread(std::move(other.m_thread)) {}

    scoped_guard(const scoped_guard&) = delete;
    scoped_guard& operator= (const scoped_guard&) = delete;

    ~scoped_guard()
    {
        m_thread.join();
    }
};

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
    "op",                 // 19
    "valid",              // 20
};

// skip 0 which is reserved for the unknown token.
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
constexpr orcus::xml_token_t XML_op                 = 19;
constexpr orcus::xml_token_t XML_valid              = 20;

class xml_handler : public orcus::sax_token_handler
{
    struct null_buffer : public std::streambuf
    {
        int overflow(int c) { return c; }
    };

    // 3 bits (0-7) for token type (1-6),
    // 4 bits (0-15) for operator type (1-15),
    // 9 bits (0-511) for function type (1-323).

    using token_set_t = std::unordered_set<orcus::xml_token_t>;
    using xml_name_t = std::pair<orcus::xmlns_id_t, orcus::xml_token_t>;
    using name_set_t = std::unordered_set<pstring, pstring::hash>;
    using named_name_set_t = std::unordered_map<pstring, name_set_t, pstring::hash>;
    using str_counter_t = std::map<pstring, uint32_t>;

    std::ostringstream& m_co; // console output buffer

    std::vector<xml_name_t> m_stack;
    std::vector<uint16_t> m_formula_tokens;
    name_set_t m_global_named_exps;
    named_name_set_t m_sheet_named_exps;
    name_set_t m_sheet_names;

    orcus::string_pool m_str_pool;

    pstring m_filepath;
    pstring m_cur_formula_sheet;

    str_counter_t m_invalid_formula_counts;
    str_counter_t m_invalid_name_counts;

    trie_builder m_trie;
    const bool m_verbose;
    bool m_valid_formula = false;

    static void check_parent(const xml_name_t& parent, const orcus::xml_token_t expected)
    {
        if (parent != xml_name_t(orcus::XMLNS_UNKNOWN_ID, expected))
            throw std::runtime_error("invalid structure");
    }

    static void check_parent(const xml_name_t& parent, const token_set_t expected)
    {
        if (!expected.count(parent.second))
            throw std::runtime_error("invalid structure");
    }

    static uint16_t encode_function(const ixion::formula_function_t fft)
    {
        // 5 bits (0-31) for opcode; 9 bits (0-511) for function type (1-323)

        uint16_t fft_v = uint16_t(fft);

        if (!fft_v || fft_v > 511u)
            throw std::runtime_error("function type value is out-of-range!");

        // subtract it by one (to eliminate the 'unknown' value) and shift 5 bits.
        uint16_t encoded = --fft_v << 5;
        encoded += ixion::fop_function - 1; // eliminate the unknown value again.
        return encoded;
    }

    static void increment_name_count(str_counter_t& counter, const pstring& name)
    {
        auto it = counter.find(name);
        if (it == counter.end())
            counter.insert({name, 1u});
        else
            ++it->second;
    }

    bool name_exists(const pstring& name, const pstring& sheet)
    {
        if (m_global_named_exps.count(name))
            // this is a global name.
            return true;

        auto it = m_sheet_named_exps.find(sheet);
        if (it == m_sheet_named_exps.end())
            return false;

        const name_set_t& sheet_names = it->second;
        return sheet_names.count(name);
    }

    void start_doc(const xml_name_t parent, const orcus::xml_token_element_t& elem)
    {
        check_parent(parent, orcus::XML_UNKNOWN_TOKEN);

        for (const auto& attr : elem.attrs)
        {
            if (attr.name == XML_filepath)
                m_filepath = attr.transient ? m_str_pool.intern(attr.value).first : attr.value;
        }
    }

    void end_doc()
    {
        bool print_report = !m_invalid_name_counts.empty() || !m_invalid_formula_counts.empty();

        if (!print_report)
            return;

        m_co << endl;
        m_co << "  document path: " << m_filepath << endl;

        if (!m_invalid_formula_counts.empty())
        {
            m_co << endl;
            m_co << "  invalid formulas:" << endl;
            for (const auto& entry : m_invalid_formula_counts)
                m_co << "    - " << entry.first << ": " << entry.second << endl;
        }

        if (!m_invalid_name_counts.empty())
        {
            m_co << endl;
            m_co << "  invalid names:" << endl;
            for (const auto& entry : m_invalid_name_counts)
                m_co << "    - " << entry.first << ": " << entry.second << endl;
        }
    }

    void start_sheet(const xml_name_t parent, const orcus::xml_token_element_t& elem)
    {
        check_parent(parent, XML_sheets);

        for (const auto& attr : elem.attrs)
        {
            if (attr.name == XML_name)
            {
                m_sheet_names.insert(m_str_pool.intern(attr.value).first);

                if (m_verbose)
                    m_co << "  * sheet: " << attr.value << endl;
            }
        }
    }

    void start_named_expression(const xml_name_t parent, const orcus::xml_token_element_t& elem)
    {
        check_parent(parent, XML_named_expressions);

        pstring name, scope, sheet;
        m_valid_formula = false;

        for (const auto& attr : elem.attrs)
        {
            switch (attr.name)
            {
                case XML_name:
                    name = attr.value;
                    break;
                case XML_scope:
                    scope = attr.value;
                    break;
                case XML_sheet:
                    sheet = attr.value;
                    break;
            }
        }

        if (m_verbose)
            m_co << "  * named expression: name: '" << name << "', scope: " << scope;

        if (scope == "global")
            m_global_named_exps.insert(m_str_pool.intern(name).first);
        else
        {
            // sheet-local named expression
            if (m_verbose)
                m_co << ", sheet='" << sheet << "'";

            if (!m_sheet_names.count(sheet))
            {
                std::ostringstream os;
                os << "sheet name '" << sheet << "' does not exist in this document.";
                throw std::runtime_error(os.str());
            }

            auto it = m_sheet_named_exps.find(sheet);
            if (it == m_sheet_named_exps.end())
                it = m_sheet_named_exps.insert({sheet, name_set_t()}).first;

            name_set_t& names = it->second;
            names.insert(m_str_pool.intern(name).first);
        }

        if (m_verbose)
            m_co << endl;
    }

    void start_formula(const xml_name_t parent, const orcus::xml_token_element_t& elem)
    {
        check_parent(parent, XML_formulas);

        pstring formula, sheet;

        for (const auto& attr : elem.attrs)
        {
            switch (attr.name)
            {
                case XML_formula:
                    formula = attr.value;
                    break;
                case XML_sheet:
                    sheet = attr.value;
                    break;
                case XML_valid:
                    m_valid_formula = attr.value == "true";
                    break;
            }
        }

        if (!m_valid_formula)
        {
            increment_name_count(m_invalid_formula_counts, formula);
            return;
        }

        m_formula_tokens.clear();
        m_cur_formula_sheet = m_str_pool.intern(sheet).first;

        if (m_verbose)
            m_co << "  * formula: " << formula << endl;
    }

    void end_formula()
    {
        if (!m_valid_formula)
            return;

        if (m_verbose)
            m_co << "    * formula tokens: " << m_formula_tokens.size() << endl;

        m_trie.insert_formula(m_formula_tokens);
    }

    void start_token(const xml_name_t parent, const orcus::xml_token_element_t& elem)
    {
        check_parent(parent, {XML_formula, XML_named_expression});

        if (!m_valid_formula)
            return;

        pstring s, type, op;

        for (const auto& attr : elem.attrs)
        {
            switch (attr.name)
            {
                case XML_s:
                    s = attr.value;
                    break;
                case XML_type:
                    type = attr.value;
                    break;
                case XML_op:
                    op = attr.value;
                    break;
            }
        }

        ixion::fopcode_t opc = to_formula_op(op.data(), op.size());
        if (opc == ixion::fop_unknown)
            throw std::runtime_error("unknown operator!");

        if (m_verbose)
            m_co << "    * token: '" << s << "', op: " << op;

        uint16_t encoded = opc - 1;

        switch (opc)
        {
            case ixion::fop_function:
            {
                ixion::formula_function_t fft = ixion::get_formula_function_opcode(s.data(), s.size());
                if (fft == ixion::formula_function_t::func_unknown)
                    throw std::runtime_error("unknown function!");

                encoded = encode_function(fft);

                if (m_verbose)
                    m_co << ", func-id: " << int(fft) << " (" << ixion::get_formula_function_name(fft) << ")";
                break;
            }
            case ixion::fop_named_expression:
            {
                // Make sure the name actually exists in the document.
                if (!name_exists(s, m_cur_formula_sheet))
                {
                    if (m_verbose)
                        m_co << ", (name not found)";

                    m_valid_formula = false;
                    increment_name_count(m_invalid_name_counts, s);
                }
                break;
            }
            case ixion::fop_error:
            {
                // Don't process formula containing error tokens.
                if (m_verbose)
                    m_co << ", (skip this formula)";

                m_valid_formula = false;
                break;
            }
        }

        if (m_valid_formula)
        {
            m_formula_tokens.push_back(encoded);

            if (m_verbose)
                m_co << ", encoded: " << encoded;
        }

        if (m_verbose)
            m_co << endl;
    }

public:

    xml_handler(bool verbose, std::ostringstream& co) :
        m_co(co),
        m_verbose(verbose) {}

    void start_element(const orcus::xml_token_element_t& elem)
    {
        xml_name_t parent(orcus::XMLNS_UNKNOWN_ID, orcus::XML_UNKNOWN_TOKEN);
        if (!m_stack.empty())
            parent = m_stack.back();

        m_stack.emplace_back(elem.ns, elem.name);

        if (elem.ns)
            // This XML structure doesn't use any namespaces.
            return;

        switch (elem.name)
        {
            case XML_doc:
            {
                start_doc(parent, elem);
                break;
            }
            case XML_sheets:
            {
                check_parent(parent, XML_doc);
                break;
            }
            case XML_sheet:
            {
                start_sheet(parent, elem);
                break;
            }
            case XML_named_expressions:
            {
                check_parent(parent, XML_doc);
                break;
            }
            case XML_named_expression:
            {
                start_named_expression(parent, elem);
                break;
            }
            case XML_formulas:
                check_parent(parent, XML_doc);
                break;
            case XML_formula:
                start_formula(parent, elem);
                break;
            case XML_token:
                start_token(parent, elem);
                break;
            default:
                ;
        }
    }

    void end_element(const orcus::xml_token_element_t& elem)
    {
        xml_name_t name(elem.ns, elem.name);
        if (m_stack.empty() || m_stack.back() != name)
            throw std::runtime_error("mis-matching element!");

        if (elem.ns == orcus::XMLNS_UNKNOWN_ID)
        {
            switch (elem.name)
            {
                case XML_formula:
                {
                    end_formula();
                    break;
                }
                case XML_doc:
                {
                    end_doc();
                    break;
                }
            }
        }

        m_stack.pop_back();
    }

    void pop_trie(trie_builder& trie)
    {
        m_trie.swap(trie);
    }
};

} // anonymous namespace

trie_builder formula_xml_processor::launch_worker_thread(const path_pos_pair_type& filepaths) const
{
    trie_builder trie;
    thread_context tc;

    if (!m_debug_dir.empty())
    {
        // Create a debug log file.
        std::ostringstream filename;
        filename << std::this_thread::get_id() << ".log";

        fs::path debug_file_path = m_debug_dir / filename.str();
        tc.debug_output.open(debug_file_path.string(), std::ios::binary);
    }

    std::for_each(filepaths.first, filepaths.second,
        [&trie, &tc, this](const std::string& filepath)
        {
            trie_builder this_trie = parse_file(filepath, tc);
            trie.merge(this_trie);
        }
    );

    return trie;
}

trie_builder formula_xml_processor::parse_file(const std::string& filepath, thread_context& tc) const
{
    std::ostringstream co; // console output

    orcus::file_content content(filepath.data());
    co << "--" << endl;
    co << "filepath: " << filepath << " (size: " << content.size() << ")" << endl;

    orcus::xmlns_repository repo;
    auto cxt = repo.create_context();
    xml_handler hdl(m_verbose, co);
    orcus::tokens token_map(token_labels, ORCUS_N_ELEMENTS(token_labels));
    orcus::sax_token_parser<xml_handler> parser(content.data(), content.size(), token_map, cxt, hdl);

    try
    {
        parser.parse();
    }
    catch (const std::exception& e)
    {
        co << endl;
        co << "  XML parse error: " << e.what() << endl;
        std::cout << co.str();
        return trie_builder();
    }

    trie_builder trie;
    hdl.pop_trie(trie);
    std::cout << co.str();
    return trie;
}

formula_xml_processor::formula_xml_processor(
    const fs::path& output_dir, const fs::path& debug_dir, bool verbose) :
    m_output_dir(output_dir),
    m_debug_dir(debug_dir),
    m_verbose(verbose) {}

void formula_xml_processor::parse_files(const std::vector<std::string>& filepaths)
{
    size_t worker_count = std::thread::hardware_concurrency();
    size_t data_size = filepaths.size() / worker_count;

    // Split the data load evenly.
    std::vector<path_pos_pair_type> filepaths_set(worker_count);
    auto pos = filepaths.begin();
    auto end = pos;
    std::advance(end, data_size);

    for (size_t i = 0; i < worker_count - 1; ++i)
    {
        auto& set = filepaths_set[i];
        set.first = pos;
        set.second = end;
        pos = end;
        std::advance(end, data_size);
    }

    end = filepaths.end();
    filepaths_set.back().first = pos;
    filepaths_set.back().second = end;

    using future_type = std::future<trie_builder>;

    std::vector<future_type> futures;

    // Dispatch the worker threads.
    for (size_t i = 0; i < worker_count; ++i)
    {
        auto future = std::async(
            std::launch::async, &formula_xml_processor::launch_worker_thread, this, filepaths_set[i]);

        futures.push_back(std::move(future));
    }

    // Wait on all worker threads.

    for (future_type& future : futures)
    {
        trie_builder trie = future.get();
        m_trie.merge(trie);
    }

    std::cout << "total entries: " << m_trie.size() << endl;
}

void formula_xml_processor::write_files()
{
    fs::path p = m_output_dir / "formula-tokens.bin";
    std::ofstream of(p.string());
    m_trie.write(of);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
