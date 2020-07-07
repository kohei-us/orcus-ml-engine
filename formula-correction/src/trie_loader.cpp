/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "trie_loader.hpp"

trie_loader::trie_loader() {}

void trie_loader::load(std::istream& is)
{
    m_trie.load_state(is);
}

size_t trie_loader::size() const
{
    return m_trie.size();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
