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

typedef mdds::sorted_string_map<formula_op_t> map_type;

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

orcus::pstring str(formula_op_t v)
{
    if (v >= v2s_map.size())
        throw std::logic_error("op type value too large!");

    int8_t pos = v2s_map[v];
    if (pos < 0)
        return orcus::pstring();

    const map_type::entry& e = entries[pos];
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

formula_op_t to_formula_op(const char* p, size_t n)
{
    return op_type::get().find(p, n);
}

orcus::pstring to_string(formula_op_t v)
{
    return op_type::str(v);
}

formula_token_t to_formula_token(const char* p, size_t n)
{
    return token_type::get().find(p, n);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
