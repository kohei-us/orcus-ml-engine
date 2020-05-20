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
#include <mdds/sorted_string_map.hpp>
#include <ixion/formula_function_opcode.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <vector>
#include <unordered_set>


namespace po = boost::program_options;
using std::cout;
using std::cerr;
using std::endl;

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
    { ORCUS_ASCII("&"), op_concat },
    { ORCUS_ASCII("("), op_open },
    { ORCUS_ASCII(")"), op_close },
    { ORCUS_ASCII("*"), op_multiply },
    { ORCUS_ASCII("+"), op_plus },
    { ORCUS_ASCII(","), op_sep },
    { ORCUS_ASCII("-"), op_minus },
    { ORCUS_ASCII("/"), op_divide },
    { ORCUS_ASCII("<"), op_less },
    { ORCUS_ASCII("<="), op_less_equal },
    { ORCUS_ASCII("<>"), op_not_equal },
    { ORCUS_ASCII("="), op_equal },
    { ORCUS_ASCII(">"), op_greater },
    { ORCUS_ASCII(">="), op_greater_equal },
    { ORCUS_ASCII("^"), op_exponent },
};

const map_type& get()
{
    static map_type mt(entries.data(), entries.size(), op_unknown);
    return mt;
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
    // 3 bits (0-7) for token type (1-6),
    // 4 bits (0-15) for operator type (1-15),
    // 9 bits (0-511) for function type (1-323).

    using token_set_t = std::unordered_set<orcus::xml_token_t>;
    using xml_name_t = std::pair<orcus::xmlns_id_t, orcus::xml_token_t>;

    std::vector<xml_name_t> m_stack;
    std::vector<uint16_t> m_formula_tokens;

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

    static uint16_t encode_operator(const op_type::v ot)
    {
        if (ot > 15u)
            throw std::runtime_error("operator type value too large!");

        uint16_t encoded = ot << 3;
        encoded += token_type::t_operator;
        return encoded;
    }

    static uint16_t encode_function(const ixion::formula_function_t fft)
    {
        uint16_t fft_v = uint16_t(fft);

        if (fft_v > 511u)
            throw std::runtime_error("function type value too large!");

        uint16_t encoded = fft_v << 3;
        encoded += token_type::t_function;
        return encoded;
    }

    void start_formula(const xml_name_t parent, const orcus::xml_token_element_t& elem)
    {
        check_parent(parent, XML_formulas);

        orcus::pstring formula;

        for (const auto& attr : elem.attrs)
        {
            switch (attr.name)
            {
                case XML_formula:
                    formula = attr.value;
                    break;
                case XML_valid:
                    m_valid_formula = attr.value == "true";
                    break;
            }
        }

        m_formula_tokens.clear();

        if (m_verbose && m_valid_formula)
            cout << "  * formula: " << formula << endl;
    }

    void end_formula()
    {
        if (m_verbose)
            cout << "    * formula tokens: " << m_formula_tokens.size() << endl;
    }

    void start_token(const xml_name_t parent, const orcus::xml_token_element_t& elem)
    {
        check_parent(parent, {XML_formula, XML_named_expression});

        if (!m_valid_formula)
            return;

        orcus::pstring s, type;

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
            cout << "    * token: '" << s << "'; type: " << type << " (" << tt << ")";

        uint16_t encoded = tt;

        switch (tt)
        {
            case token_type::t_operator:
            {
                op_type::v ot = op_type::get().find(s.data(), s.size());
                if (ot == op_type::op_unknown)
                    throw std::runtime_error("unknown operator!");

                encoded = encode_operator(ot);

                if (m_verbose)
                    cout << ", op-type: " << ot;
                break;
            }
            case token_type::t_function:
            {
                ixion::formula_function_t fft = ixion::get_formula_function_opcode(s.data(), s.size());
                if (fft == ixion::formula_function_t::func_unknown)
                    throw std::runtime_error("unknown function!");

                encoded = encode_function(fft);

                if (m_verbose)
                    cout << ", func-id: " << int(fft) << " (" << ixion::get_formula_function_name(fft) << ")";
                break;
            }
        }

        m_formula_tokens.push_back(encoded);

        if (m_verbose)
            cout << ", encoded: " << encoded << endl;
    }

public:

    xml_handler(bool verbose) : m_verbose(verbose) {}

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
                check_parent(parent, orcus::XML_UNKNOWN_TOKEN);
                break;
            }
            case XML_sheets:
            {
                check_parent(parent, XML_doc);
                break;
            }
            case XML_sheet:
            {
                check_parent(parent, XML_sheets);

                for (const auto& attr : elem.attrs)
                {
                    if (attr.name == XML_name)
                    {
                        if (m_verbose)
                            cout << "  * sheet: " << attr.value << endl;
                    }
                }
                break;
            }
            case XML_named_expressions:
            {
                check_parent(parent, XML_doc);
                break;
            }
            case XML_named_expression:
            {
                check_parent(parent, XML_named_expressions);

                orcus::pstring name, scope;
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
                    }
                }

                if (m_verbose)
                    cout << "  * named expression: " << name << "  (scope: " << scope << ")" << endl;
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
            }
        }

        m_stack.pop_back();
    }
};

void parse_file(const std::string& filepath, bool verbose)
{
    cout << "--" << endl;
    orcus::file_content content(filepath.data());
    cout << "filepath: " << filepath << " (size: " << content.size() << ")" << endl;

    orcus::xmlns_repository repo;
    auto cxt = repo.create_context();
    xml_handler hdl(verbose);
    orcus::tokens token_map(token_labels, ORCUS_N_ELEMENTS(token_labels));
    orcus::sax_token_parser<xml_handler> parser(content.data(), content.size(), token_map, cxt, hdl);
    parser.parse();
}

int main(int argc, char** argv)
{
    bool verbose = false;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "Print this help.")
        ("verbose,v", po::bool_switch(&verbose), "Verbose output.");

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
        parse_file(filepath, verbose);

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
