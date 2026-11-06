/****************************************************************************
**
** Copyright (C) 2026 Gianni Peschiutta
** Contact: https://github.com/Artemia76/EasyMorse
**
** EasyMorse is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** EasyMorse is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** The license is as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this software. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
****************************************************************************/

/****************************************************************************
** spscqueue.h is part of EasyMorse project
**
** Wait-free single producer / single consumer ring buffer, used to pass
** events between the GUI thread and the real time audio thread without
** locks nor allocations.
****************************************************************************/

#ifndef SPSCQUEUE_H
#define SPSCQUEUE_H

#include <array>
#include <atomic>
#include <cstddef>
#include <optional>

template <typename T, std::size_t Capacity>
class SpscQueue
{
    static_assert(Capacity >= 2 && (Capacity & (Capacity - 1)) == 0,
                  "Capacity must be a power of two");

public:
    ///
    /// \brief push an item, from the producer thread only
    /// \return false if the queue is full (item dropped)
    ///
    bool push(const T& pItem)
    {
        const std::size_t tail = m_tail.load(std::memory_order_relaxed);
        if (tail - m_head.load(std::memory_order_acquire) == Capacity)
            return false;
        m_items[tail & (Capacity - 1)] = pItem;
        m_tail.store(tail + 1, std::memory_order_release);
        return true;
    }

    ///
    /// \brief front gives the oldest item without removing it, consumer thread only
    ///
    const T* front() const
    {
        const std::size_t head = m_head.load(std::memory_order_relaxed);
        if (head == m_tail.load(std::memory_order_acquire))
            return nullptr;
        return &m_items[head & (Capacity - 1)];
    }

    ///
    /// \brief pop removes the oldest item, consumer thread only
    ///
    std::optional<T> pop()
    {
        const T* item = front();
        if (item == nullptr)
            return std::nullopt;
        T value = *item;
        m_head.store(m_head.load(std::memory_order_relaxed) + 1, std::memory_order_release);
        return value;
    }

    bool empty() const
    {
        return m_head.load(std::memory_order_acquire) == m_tail.load(std::memory_order_acquire);
    }

    static constexpr std::size_t capacity() { return Capacity; }

private:
    // Separate cache lines so producer and consumer do not fight over them
    alignas(64) std::atomic<std::size_t> m_head{0};
    alignas(64) std::atomic<std::size_t> m_tail{0};
    std::array<T, Capacity> m_items{};
};

#endif // SPSCQUEUE_H
