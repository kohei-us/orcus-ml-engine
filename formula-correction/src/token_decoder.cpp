/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "token_decoder.hpp"
#include "types.hpp"

#include <ixion/formula_function_opcode.hpp>
#include <ixion/formula_opcode.hpp>
#include <sstream>

std::vector<std::string> decode_tokens_to_names(const std::vector<uint16_t>& tokens)
{
    std::vector<std::string> decoded;
    decoded.reserve(tokens.size());

    for (const uint16_t v : tokens)
    {
        // extract the opcode value
        uint16_t ttv = (v & 0x001F) + 1;
        ixion::fopcode_t op = static_cast<ixion::fopcode_t>(ttv);

        switch (op)
        {
            case ixion::fop_error:
                decoded.push_back("<error>");
                break;
            case ixion::fop_function:
            {
                // Extract the function token value.
                uint16_t ftv = v & 0xFFE0;
                ftv = (ftv >> 5) + 1;
                auto fft = static_cast<ixion::formula_function_t>(ftv);
                std::ostringstream os;
                os << "func:" << ixion::get_formula_function_name(fft);
                decoded.push_back(os.str());
                break;
            }
            default:
            {
                decoded.push_back(to_string(op).str());
            }
        }
    }

    return decoded;
}

std::vector<std::string> decode_tokens_to_symbols(const std::vector<uint16_t>& tokens)
{
    std::vector<std::string> decoded;
    decoded.reserve(tokens.size());

    for (const uint16_t v : tokens)
    {
        // extract the opcode value
        uint16_t ttv = (v & 0x001F) + 1;
        ixion::fopcode_t op = static_cast<ixion::fopcode_t>(ttv);

        switch (op)
        {
            case ixion::fop_error:
                decoded.push_back("<error>");
                break;
            case ixion::fop_function:
            {
                // Extract the function token value.
                uint16_t ftv = v & 0xFFE0;
                ftv = (ftv >> 5) + 1;
                auto fft = static_cast<ixion::formula_function_t>(ftv);
                std::ostringstream os;
                os << ixion::get_formula_function_name(fft);
                decoded.push_back(os.str());
                break;
            }
            default:
            {
                decoded.push_back(to_symbol(op).str());
            }
        }
    }

    return decoded;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
