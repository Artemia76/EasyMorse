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
* securechannel.cpp is part of EasyMorse project
* **************************************************************************/

#include "securechannel.h"

#include <QRandomGenerator>
#include <QtEndian>

#include <algorithm>
#include <cstring>

#include <monocypher.h>

namespace
{
    constexpr int MacSize = 16;
    constexpr int CounterSize = 8;

    void nonceFor(uint64_t pCounter, uint8_t pNonce[24])
    {
        // Keys differ per direction, so a counter never repeats under one key
        std::memset(pNonce, 0, 24);
        qToBigEndian(pCounter, pNonce);
    }
}

SecureChannel::SecureChannel()
{
    // System random generator: cryptographically secure
    QRandomGenerator::system()->fillRange(reinterpret_cast<quint32*>(m_secretKey.data()), KeySize / 4);
    crypto_x25519_public_key(m_publicKey.data(), m_secretKey.data());
}

SecureChannel::~SecureChannel()
{
    crypto_wipe(m_secretKey.data(), KeySize);
    crypto_wipe(m_sendKey.data(), KeySize);
    crypto_wipe(m_receiveKey.data(), KeySize);
}

QByteArray SecureChannel::publicKey() const
{
    return QByteArray(reinterpret_cast<const char*>(m_publicKey.data()), KeySize);
}

bool SecureChannel::setPeerPublicKey(const QByteArray& pPeerKey)
{
    if (pPeerKey.size() != KeySize || pPeerKey == publicKey())
        return false;
    std::memcpy(m_peerKey.data(), pPeerKey.constData(), KeySize);

    uint8_t shared[KeySize];
    crypto_x25519(shared, m_secretKey.data(), m_peerKey.data());
    // An all zero secret means a low order peer key: refuse it
    static const uint8_t zero[KeySize] = {};
    if (crypto_verify32(shared, zero) == 0)
    {
        crypto_wipe(shared, KeySize);
        return false;
    }

    // Each direction gets its own key: hash(shared, sender key, receiver key)
    const auto derive = [&shared](const uint8_t* pFrom, const uint8_t* pTo, uint8_t* pKey) {
        crypto_blake2b_ctx context;
        crypto_blake2b_init(&context, KeySize);
        static const char label[] = "EasyMorse session v1";
        crypto_blake2b_update(&context, reinterpret_cast<const uint8_t*>(label), sizeof(label) - 1);
        crypto_blake2b_update(&context, shared, KeySize);
        crypto_blake2b_update(&context, pFrom, KeySize);
        crypto_blake2b_update(&context, pTo, KeySize);
        crypto_blake2b_final(&context, pKey);
    };
    derive(m_publicKey.data(), m_peerKey.data(), m_sendKey.data());
    derive(m_peerKey.data(), m_publicKey.data(), m_receiveKey.data());
    crypto_wipe(shared, KeySize);
    m_sendCounter = 0;
    m_receiveHighest = 0;
    m_receiveWindow = 0;
    m_ready = true;
    return true;
}

QString SecureChannel::fingerprint() const
{
    if (!m_ready)
        return QString();
    // Same on both sides: the keys are hashed in a fixed order
    const auto& first = std::min(m_publicKey, m_peerKey);
    const auto& second = std::max(m_publicKey, m_peerKey);
    uint8_t hash[8];
    crypto_blake2b_ctx context;
    crypto_blake2b_init(&context, sizeof(hash));
    crypto_blake2b_update(&context, first.data(), KeySize);
    crypto_blake2b_update(&context, second.data(), KeySize);
    crypto_blake2b_final(&context, hash);
    // Four groups of four digits, easy to read aloud
    QString text;
    for (int i = 0; i < 4; ++i)
    {
        const int value = ((hash[2 * i] << 8) | hash[2 * i + 1]) % 10000;
        text += QString("%1").arg(value, 4, 10, QLatin1Char('0'));
        if (i < 3)
            text += u' ';
    }
    return text;
}

QByteArray SecureChannel::seal(const QByteArray& pPlain)
{
    if (!m_ready)
        return QByteArray();
    const uint64_t counter = ++m_sendCounter;
    QByteArray packet(CounterSize + pPlain.size() + MacSize, Qt::Uninitialized);
    auto* bytes = reinterpret_cast<uint8_t*>(packet.data());
    qToBigEndian(counter, bytes);
    uint8_t nonce[24];
    nonceFor(counter, nonce);
    // The counter is authenticated as additional data
    crypto_aead_lock(bytes + CounterSize, bytes + CounterSize + pPlain.size(),
                     m_sendKey.data(), nonce, bytes, CounterSize,
                     reinterpret_cast<const uint8_t*>(pPlain.constData()), pPlain.size());
    return packet;
}

std::optional<QByteArray> SecureChannel::open(const QByteArray& pPacket)
{
    if (!m_ready || pPacket.size() < Overhead)
        return std::nullopt;
    const auto* bytes = reinterpret_cast<const uint8_t*>(pPacket.constData());
    const uint64_t counter = qFromBigEndian<uint64_t>(bytes);
    if (counter == 0)
        return std::nullopt;
    // Too old, or already received
    if (counter + 64 <= m_receiveHighest)
        return std::nullopt;
    if (counter <= m_receiveHighest && (m_receiveWindow >> (m_receiveHighest - counter)) & 1)
        return std::nullopt;

    const qsizetype plainSize = pPacket.size() - Overhead;
    QByteArray plain(plainSize, Qt::Uninitialized);
    uint8_t nonce[24];
    nonceFor(counter, nonce);
    if (crypto_aead_unlock(reinterpret_cast<uint8_t*>(plain.data()), bytes + CounterSize + plainSize,
                           m_receiveKey.data(), nonce, bytes, CounterSize,
                           bytes + CounterSize, plainSize) != 0)
        return std::nullopt;

    // Only authentic packets move the replay window
    if (counter > m_receiveHighest)
    {
        const uint64_t shift = counter - m_receiveHighest;
        m_receiveWindow = (shift >= 64) ? 1 : ((m_receiveWindow << shift) | 1);
        m_receiveHighest = counter;
    }
    else
    {
        m_receiveWindow |= uint64_t(1) << (m_receiveHighest - counter);
    }
    return plain;
}
