/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <mdds/trie_map.hpp>

class trie_builder
{
    using key_trait = mdds::trie::std_container_trait<std::vector<uint16_t>>;
    using map_type = mdds::trie_map<key_trait, int>;

    map_type m_trie;

public:
    trie_builder();
    trie_builder(trie_builder&& other);

    void insert_formula(const std::vector<uint16_t>& tokens);

    void merge(const trie_builder& other);

    void write(std::ostream& os);

    size_t size() const;

    void swap(trie_builder& other);
};


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
