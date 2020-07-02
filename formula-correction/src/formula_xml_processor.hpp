/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "trie_builder.hpp"
#include "async_queue.hpp"

#include <boost/filesystem.hpp>
#include <string>
#include <vector>

class formula_xml_processor
{
    using paths_type = std::vector<std::string>;
    using path_pos_pair_type = std::pair<paths_type::const_iterator, paths_type::const_iterator>;

    enum class thread_policy { disabled, linear_async, split_load };
public:
    using queue_type = async_queue<trie_builder>;

private:
    trie_builder m_trie;
    boost::filesystem::path m_outdir;
    const bool m_verbose;
    const thread_policy m_thread_policy = thread_policy::split_load;

    void launch_queue_dispatcher_thread(queue_type* queue, const std::vector<std::string>& filepaths);

    trie_builder launch_worker_thread(const path_pos_pair_type& filepaths);

    trie_builder parse_file(const std::string& filepath);

public:

    formula_xml_processor(const boost::filesystem::path& outdir, bool verbose);

    formula_xml_processor(const formula_xml_processor&) = delete;

    void parse_files(const std::vector<std::string>& filepaths);

    void write_files();
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
