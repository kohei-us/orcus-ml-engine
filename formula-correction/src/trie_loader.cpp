/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "trie_loader.hpp"

#include <ixion/formula_function_opcode.hpp>
#include <iostream>

using std::endl;

namespace {

std::vector<std::string> decode_tokens(const std::vector<uint16_t>& tokens)
{
    std::vector<std::string> decoded;
    decoded.reserve(tokens.size());

    for (const uint16_t v : tokens)
    {
        // extract the token type value.
        uint16_t ttv = (v & 0x0007) + 1;
        formula_token_t tt = static_cast<formula_token_t>(ttv);

        switch (tt)
        {
            case t_error:
                decoded.push_back("<error>");
                break;
            case t_function:
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
            case t_name:
                decoded.push_back("<name>");
                break;
            case t_operator:
            {
                // Extract the operator token value.
                uint16_t otv = v & 0xFFF8;
                otv = (otv >> 3) + 1;
                auto ot = static_cast<formula_op_t>(otv);
                decoded.push_back(to_string(ot).str());
                break;
            }
            case t_reference:
                decoded.push_back("<ref>");
                break;
            case t_value:
                decoded.push_back("<value>");
                break;
            default:
                throw std::logic_error("wrong token type!");
        }
    }

    return decoded;
}

} // anonymous namespace

trie_loader::trie_loader() {}

void trie_loader::load(std::istream& is)
{
    m_trie.load_state(is);
}

size_t trie_loader::size() const
{
    return m_trie.size();
}

void trie_loader::dump(std::ostream& os) const
{
    for (const auto& entry : m_trie)
    {
        const std::vector<uint16_t>& tokens = entry.first;
        for (const std::string& s : decode_tokens(tokens))
            os << s << ' ';

        os << '(' << entry.second << ')' << endl;
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
