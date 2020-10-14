/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "types.hpp"

#include <mdds/sorted_string_map.hpp>
#include <orcus/global.hpp>

namespace op_type {

typedef mdds::sorted_string_map<ixion::fopcode_t> map_type;

// Keys must be sorted.
const std::vector<map_type::entry> entries =
{
    { ORCUS_ASCII("close"),            ixion::fop_close            },
    { ORCUS_ASCII("concat"),           ixion::fop_concat           },
    { ORCUS_ASCII("divide"),           ixion::fop_divide           },
    { ORCUS_ASCII("equal"),            ixion::fop_equal            },
    { ORCUS_ASCII("error"),            ixion::fop_error            },
    { ORCUS_ASCII("exponent"),         ixion::fop_exponent         },
    { ORCUS_ASCII("function"),         ixion::fop_function         },
    { ORCUS_ASCII("greater"),          ixion::fop_greater          },
    { ORCUS_ASCII("greater-equal"),    ixion::fop_greater_equal    },
    { ORCUS_ASCII("less"),             ixion::fop_less             },
    { ORCUS_ASCII("less-equal"),       ixion::fop_less_equal       },
    { ORCUS_ASCII("minus"),            ixion::fop_minus            },
    { ORCUS_ASCII("multiply"),         ixion::fop_multiply         },
    { ORCUS_ASCII("named-expression"), ixion::fop_named_expression },
    { ORCUS_ASCII("not-equal"),        ixion::fop_not_equal        },
    { ORCUS_ASCII("open"),             ixion::fop_open             },
    { ORCUS_ASCII("plus"),             ixion::fop_plus             },
    { ORCUS_ASCII("range-ref"),        ixion::fop_range_ref        },
    { ORCUS_ASCII("sep"),              ixion::fop_sep              },
    { ORCUS_ASCII("single-ref"),       ixion::fop_single_ref       },
    { ORCUS_ASCII("string"),           ixion::fop_string           },
    { ORCUS_ASCII("table-ref"),        ixion::fop_table_ref        },
    { ORCUS_ASCII("unknown"),          ixion::fop_unknown          },
    { ORCUS_ASCII("value"),            ixion::fop_value            },
};

// value-to-string map
const std::vector<size_t> v2s_map = {
    22, // unknown
    19, // single-ref
    17, // range-ref
    21, // table-ref
    13, // named-expression
    20, // string
    23, // value
     6, // function
    16, // plus
    11, // minus
     2, // divide
    12, // multiply
     5, // exponent
     1, // concat
     3, // equal
    14, // not-equal
     9, // less
     7, // greater
    10, // less-equal
     8, // greater-equal
    15, // open
     0, // close
    18, // sep
     4, // error
};

const map_type& get()
{
    static map_type mt(entries.data(), entries.size(), ixion::fop_unknown);
    return mt;
}

orcus::pstring str(ixion::fopcode_t v)
{
    size_t v2 = static_cast<size_t>(v);
    if (v2 >= v2s_map.size())
        throw std::logic_error("op type value too large!");

    size_t pos = v2s_map.at(v2);
    const map_type::entry& e = entries.at(pos);
    return orcus::pstring(e.key, e.keylen);
}

} // namespace op_type

namespace token_type {

typedef mdds::sorted_string_map<formula_token_t> map_type;

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

ixion::fopcode_t to_formula_op(const char* p, size_t n)
{
    return op_type::get().find(p, n);
}

orcus::pstring to_string(ixion::fopcode_t v)
{
    return op_type::str(v);
}

orcus::pstring to_symbol(ixion::fopcode_t v)
{
    const char* symbols[] = {
        "???", // fop_unknown = 0,
        "ref", // fop_single_ref,
        "ref:ref", // fop_range_ref,
        "ref[]", // fop_table_ref,
        "name", // fop_named_expression,
        "\"str\"", // fop_string,
        "1", // fop_value,
        "func", // fop_function,
        "+", // fop_plus,
        "-", // fop_minus,
        "/", // fop_divide,
        "*", // fop_multiply,
        "^", // fop_exponent,
        "&", // fop_concat,
        "=", // fop_equal,
        "<>", // fop_not_equal,
        "<", // fop_less,
        ">", // fop_greater,
        "<=", // fop_less_equal,
        ">=", // fop_greater_equal,
        "(", // fop_open,
        ")", // fop_close,
        ",", // fop_sep,
        "error", // fop_error,
    };

    const char* p = symbols[v];
    size_t n = strlen(p);
    return orcus::pstring(p, n);
}

formula_token_t to_formula_token(const char* p, size_t n)
{
    return token_type::get().find(p, n);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
