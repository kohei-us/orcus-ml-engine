/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <future>
#include <queue>
#include <mutex>

template<typename _Res>
class async_queue
{
    using future_type = std::future<_Res>;

    std::queue<future_type> m_futures;
    std::mutex m_mtx;
    std::condition_variable m_cond;

    size_t m_max_queue;

public:
    async_queue(size_t max_queue) : m_max_queue(max_queue) {}

    template<typename _Fn, typename... _Args>
    void push(_Fn&& fn, _Args&&... args)
    {
        std::unique_lock<std::mutex> lock(m_mtx);

        while (m_futures.size() >= m_max_queue)
            m_cond.wait(lock);

        future_type f = std::async(
            std::launch::async, std::forward<_Fn>(fn), std::forward<_Args>(args)...);
        m_futures.push(std::move(f));
        lock.unlock();

        m_cond.notify_one();
    }

    _Res get_one()
    {
        std::unique_lock<std::mutex> lock(m_mtx);

        while (m_futures.empty())
            m_cond.wait(lock);

        future_type ret = std::move(m_futures.front());
        m_futures.pop();
        lock.unlock();

        _Res res = ret.get();  // This may throw if an exception was thrown on the thread.

        m_cond.notify_one();

        return res;
    }
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
