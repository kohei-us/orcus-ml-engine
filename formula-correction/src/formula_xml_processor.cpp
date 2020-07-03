/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "formula_xml_processor.hpp"

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

namespace op_type {

enum v : uint8_t
{
    op_unknown = 0,   // 0
    op_plus,          // 1
    op_minus,         // 2
    op_divide,        // 3
    op_multiply,      // 4
    op_exponent,      // 5
    op_concat,        // 6
    op_equal,         // 7
    op_not_equal,     // 8
    op_less,          // 9
    op_greater,       // 10
    op_less_equal,    // 11
    op_greater_equal, // 12
    op_open,          // 13
    op_close,         // 14
    op_sep,           // 15
};

typedef mdds::sorted_string_map<v> map_type;

// Keys must be sorted.
const std::vector<map_type::entry> entries =
{
    { ORCUS_ASCII("&"),  op_concat        }, // 0
    { ORCUS_ASCII("("),  op_open          }, // 1
    { ORCUS_ASCII(")"),  op_close         }, // 2
    { ORCUS_ASCII("*"),  op_multiply      }, // 3
    { ORCUS_ASCII("+"),  op_plus          }, // 4
    { ORCUS_ASCII(","),  op_sep           }, // 5
    { ORCUS_ASCII("-"),  op_minus         }, // 6
    { ORCUS_ASCII("/"),  op_divide        }, // 7
    { ORCUS_ASCII("<"),  op_less          }, // 8
    { ORCUS_ASCII("<="), op_less_equal    }, // 9
    { ORCUS_ASCII("<>"), op_not_equal     }, // 10
    { ORCUS_ASCII("="),  op_equal         }, // 11
    { ORCUS_ASCII(">"),  op_greater       }, // 12
    { ORCUS_ASCII(">="), op_greater_equal }, // 13
    { ORCUS_ASCII("^"),  op_exponent      }, // 14
};

// value-to-string map
const std::vector<int8_t> v2s_map = {
    -1, // op_unknown
    4,  // op_plus
    6,  // op_minus
    7,  // op_divide
    3,  // op_multiply
    14, // op_exponent
    0,  // op_concat
    11, // op_equal
    10, // op_not_equal
    8,  // op_less
    12, // op_greater
    9,  // op_less_equal
    13, // op_greater_equal
    1,  // op_open
    2,  // op_close
    5,  // op_sep
};

const map_type& get()
{
    static map_type mt(entries.data(), entries.size(), op_unknown);
    return mt;
}

pstring str(op_type::v v)
{
    if (v >= v2s_map.size())
        throw std::logic_error("op type value too large!");

    int8_t pos = v2s_map[v];
    if (pos < 0)
        return pstring();

    const map_type::entry& e = entries[pos];
    return pstring(e.key, e.keylen);
}

} // namespace op_type

namespace token_type {

enum v : uint8_t
{
    t_unknown = 0, // 0
    t_error,       // 1
    t_function,    // 2
    t_name,        // 3
    t_operator,    // 4
    t_reference,   // 5
    t_value,       // 6
};

typedef mdds::sorted_string_map<v> map_type;

// Keys must be sorted.
const std::vector<map_type::entry> entries =
{
    { ORCUS_ASCII("error"),     t_error     },
    { ORCUS_ASCII("function"),  t_function  },
    { ORCUS_ASCII("name"),      t_name      },
    { ORCUS_ASCII("operator"),  t_operator  },
    { ORCUS_ASCII("reference"), t_reference },
    { ORCUS_ASCII("unknown"),   t_unknown   },
    { ORCUS_ASCII("value"),     t_value     },
};

const map_type& get()
{
    static map_type mt(entries.data(), entries.size(), t_unknown);
    return mt;
}

} // namespace token_type

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
constexpr orcus::xml_token_t XML_valid              = 19;

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

    static std::vector<std::string> decode_tokens(const std::vector<uint16_t>& tokens)
    {
        std::vector<std::string> decoded;
        decoded.reserve(tokens.size());

        for (const uint16_t v : tokens)
        {
            // extract the token type value.
            uint16_t ttv = (v & 0x0007) + 1;
            token_type::v tt = static_cast<token_type::v>(ttv);

            switch (tt)
            {
                case token_type::t_error:
                    decoded.push_back("<error>");
                    break;
                case token_type::t_function:
                {
                    // Extract the function token value.
                    uint16_t ftv = v & 0xFFF8;
                    ftv = (ftv >> 3) + 1;
                    auto fft = static_cast<ixion::formula_function_t>(ftv);
                    std::ostringstream os;
                    os << "<func:" << ixion::get_formula_function_name(fft) << ">";
                    decoded.push_back(os.str());
                    break;
                }
                case token_type::t_name:
                    decoded.push_back("<name>");
                    break;
                case token_type::t_operator:
                {
                    // Extract the operator token value.
                    uint16_t otv = v & 0xFFF8;
                    otv = (otv >> 3) + 1;
                    auto ot = static_cast<op_type::v>(otv);
                    decoded.push_back(op_type::str(ot).str());
                    break;
                }
                case token_type::t_reference:
                    decoded.push_back("<ref>");
                    break;
                case token_type::t_value:
                    decoded.push_back("<value>");
                    break;
                default:
                    throw std::logic_error("wrong token type!");
            }
        }

        return decoded;
    }

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

    static uint16_t encode_operator(const op_type::v ot)
    {
        // 3 bits (0-7) for token type (1-6)
        // 4 bits (0-15) for operator type (1-15)

        uint16_t ot_v = ot;

        if (!ot_v || ot_v > 15u)
            throw std::runtime_error("operator type value is out-of-range!");

        // subtract it by one and shift 3 bits.
        uint16_t encoded = --ot_v << 3;
        encoded += token_type::t_operator - 1; // subtract it by one
        return encoded;
    }

    static uint16_t encode_function(const ixion::formula_function_t fft)
    {
        // 3 bits (0-7) for token type (1-6)
        // 9 bits (0-511) for function type (1-323)

        uint16_t fft_v = uint16_t(fft);

        if (!fft_v || fft_v > 511u)
            throw std::runtime_error("function type value is out-of-range!");

        // subtract it by one and shift 3 bits.
        uint16_t encoded = --fft_v << 3;
        encoded += token_type::t_function - 1; // subtract it by one
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

#if 0
        // Write the tokens to the output file.
        std::copy(m_formula_tokens.begin(), m_formula_tokens.end(), std::ostream_iterator<uint16_t>(m_of, ","));

        // Decode encoded tokens and write it to the output file too.
        auto decoded = decode_tokens(m_formula_tokens);
#endif
    }

    void start_token(const xml_name_t parent, const orcus::xml_token_element_t& elem)
    {
        check_parent(parent, {XML_formula, XML_named_expression});

        if (!m_valid_formula)
            return;

        pstring s, type;

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
            }
        }

        token_type::v tt = token_type::get().find(type.data(), type.size());
        if (tt == token_type::t_unknown)
            throw std::runtime_error("unknown token type!");

        if (m_verbose)
            m_co << "    * token: '" << s << "', type: " << type << " (" << tt << ")";

        uint16_t encoded = tt - 1;

        switch (tt)
        {
            case token_type::t_operator:
            {
                op_type::v ot = op_type::get().find(s.data(), s.size());
                if (ot == op_type::op_unknown)
                    throw std::runtime_error("unknown operator!");

                encoded = encode_operator(ot);

                if (m_verbose)
                    m_co << ", op-type: " << ot;
                break;
            }
            case token_type::t_function:
            {
                ixion::formula_function_t fft = ixion::get_formula_function_opcode(s.data(), s.size());
                if (fft == ixion::formula_function_t::func_unknown)
                    throw std::runtime_error("unknown function!");

                encoded = encode_function(fft);

                if (m_verbose)
                    m_co << ", func-id: " << int(fft) << " (" << ixion::get_formula_function_name(fft) << ")";
                break;
            }
            case token_type::t_name:
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
            case token_type::t_error:
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

void formula_xml_processor::launch_queue_dispatcher_thread(
    queue_type* queue, const std::vector<std::string>& filepaths)
{
    for (const std::string& filepath : filepaths)
        queue->push(&formula_xml_processor::parse_file, this, filepath);
}

trie_builder formula_xml_processor::launch_worker_thread(const path_pos_pair_type& filepaths)
{
    trie_builder trie;

    std::for_each(filepaths.first, filepaths.second,
        [&trie, this](const std::string& filepath)
        {
            trie_builder this_trie = parse_file(filepath);
            trie.merge(this_trie);
        }
    );

    return trie;
}

trie_builder formula_xml_processor::parse_file(const std::string& filepath)
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

formula_xml_processor::formula_xml_processor(const fs::path& outdir, bool verbose) :
    m_outdir(outdir), m_verbose(verbose) {}

void formula_xml_processor::parse_files(const std::vector<std::string>& filepaths)
{
    switch (m_thread_policy)
    {
        case thread_policy::disabled:
        {
            for (const std::string& filepath : filepaths)
            {
                trie_builder trie = parse_file(filepath);
                m_trie.merge(trie);
            }

            break;
        }
        case thread_policy::linear_async:
        {
            size_t file_count = filepaths.size();

            queue_type queue(32);

            std::thread t(&formula_xml_processor::launch_queue_dispatcher_thread, this, &queue, filepaths);
            scoped_guard guard(std::move(t));

            for (size_t i = 0; i < file_count; ++i)
            {
                trie_builder trie = queue.get_one();
                m_trie.merge(trie);
            }
            break;
        }
        case thread_policy::split_load:
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

            break;
        }
    }
}

void formula_xml_processor::write_files()
{
    fs::path p = m_outdir / "formula-tokens.bin";
    std::ofstream of(p.string());
    m_trie.write(of);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
