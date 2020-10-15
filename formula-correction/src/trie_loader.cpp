/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "trie_loader.hpp"
#include "token_decoder.hpp"

#include <iostream>

using std::endl;

trie_loader::trie_loader() {}

void trie_loader::load(std::istream& is)
{
    m_trie.load_state(is);
}

size_t trie_loader::size() const
{
    return m_trie.size();
}

void trie_loader::dump(std::ostream& os, mode_type mode) const
{
    switch (mode)
    {
        case NAME:
        {
            for (const auto& entry : m_trie)
            {
                // number of occurrences as the first value in each line.
                os << entry.second << ' ';

                const std::vector<uint16_t>& tokens = entry.first;
                for (const std::string& s : decode_tokens_to_names(tokens))
                    os << s << ' ';

                os << endl;
            }

            break;
        }
        case SYMBOL:
        {
            for (const auto& entry : m_trie)
            {
                // number of occurrences as the first value in each line.
                os << entry.second << ' ';

                const std::vector<uint16_t>& tokens = entry.first;
                for (const std::string& s : decode_tokens_to_symbols(tokens))
                    os << s << ' ';

                os << endl;
            }

            break;
        }
        case VALUE:
        {
            for (const auto& entry : m_trie)
            {
                // number of occurrences as the first value in each line.
                os << entry.second << ' ';

                // No decoding - print the token values.
                for (const uint16_t v : entry.first)
                    os << v << ' ';

                os << endl;
            }

            break;
        }
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
