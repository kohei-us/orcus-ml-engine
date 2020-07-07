/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstdint>
#include <orcus/pstring.hpp>

enum formula_token_t : uint8_t
{
    t_unknown = 0, // 0
    t_error,       // 1
    t_function,    // 2
    t_name,        // 3
    t_operator,    // 4
    t_reference,   // 5
    t_value,       // 6
};

enum formula_op_t : uint8_t
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

formula_op_t to_formula_op(const char* p, size_t n);

orcus::pstring to_string(formula_op_t v);

formula_token_t to_formula_token(const char* p, size_t n);

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
