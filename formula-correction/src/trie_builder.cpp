/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "trie_builder.hpp"
#include <iostream>

void trie_builder::insert_formula(const std::vector<uint16_t>& tokens)
{
    if (tokens.empty())
        return;

    // TODO : insert this token set into the trie...
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
