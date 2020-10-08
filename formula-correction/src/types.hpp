/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstdint>
#include <orcus/pstring.hpp>
#include <ixion/formula_opcode.hpp>

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

ixion::fopcode_t  to_formula_op(const char* p, size_t n);

orcus::pstring to_string(ixion::fopcode_t v);

formula_token_t to_formula_token(const char* p, size_t n);

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
