/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "trie_builder.hpp"

using namespace std;

trie_builder::trie_builder() {}
trie_builder::trie_builder(trie_builder&& other) : m_trie(std::move(other.m_trie)) {}

void trie_builder::insert_formula(const std::vector<uint16_t>& tokens)
{
    if (tokens.empty())
        return;

    auto it = m_trie.find(tokens);

    if (it == m_trie.end())
        m_trie.insert(tokens, 1);
    else
        ++it->second; // increment the counter.
}

void trie_builder::merge(const trie_builder& other)
{
    for (const auto& entry : other.m_trie)
    {
        auto it = m_trie.find(entry.first);
        if (it == m_trie.end())
            m_trie.insert(entry.first, entry.second);
        else
            it->second += entry.second;
    }
}

void trie_builder::write(std::ostream& os)
{
    auto packed = m_trie.pack();
    packed.save_state(os);
}

size_t trie_builder::size() const
{
    return m_trie.size();
}

void trie_builder::swap(trie_builder& other)
{
    m_trie.swap(other.m_trie);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
