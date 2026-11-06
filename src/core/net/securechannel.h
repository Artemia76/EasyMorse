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
* securechannel.h is part of EasyMorse project
*
* End to end encryption of a peer to peer session: X25519 key exchange,
* one XChaCha20-Poly1305 key per direction, replayed packets rejected.
* The match-making server only relays public keys and never sees the keys
* used to encrypt.
* **************************************************************************/

#ifndef SECURECHANNEL_H
#define SECURECHANNEL_H

#include <QByteArray>
#include <QString>

#include <array>
#include <cstdint>
#include <optional>

class SecureChannel
{
public:
    static constexpr int KeySize = 32;
    // Counter (8 bytes) and authentication tag (16 bytes)
    static constexpr int Overhead = 8 + 16;

    ///
    /// \brief creates a new random key pair
    ///
    SecureChannel();
    ~SecureChannel();
    SecureChannel(const SecureChannel&) = delete;
    SecureChannel& operator=(const SecureChannel&) = delete;

    QByteArray  publicKey() const;
    ///
    /// \brief derives the session keys
    /// \return false if the key is invalid
    ///
    bool        setPeerPublicKey(const QByteArray& pPeerKey);
    bool        isReady() const { return m_ready; }

    ///
    /// \brief short code computed from both public keys, the same on both
    /// sides: reading it to each other detects a man in the middle
    ///
    QString     fingerprint() const;

    QByteArray  seal(const QByteArray& pPlain);
    ///
    /// \return the plain text, or nothing if the packet is forged, corrupted or replayed
    ///
    std::optional<QByteArray> open(const QByteArray& pPacket);

private:
    std::array<uint8_t, KeySize>    m_secretKey{};
    std::array<uint8_t, KeySize>    m_publicKey{};
    std::array<uint8_t, KeySize>    m_peerKey{};
    std::array<uint8_t, KeySize>    m_sendKey{};
    std::array<uint8_t, KeySize>    m_receiveKey{};
    bool                            m_ready = false;
    uint64_t                        m_sendCounter = 0;
    // Replay protection: highest counter received and the 64 before it
    uint64_t                        m_receiveHighest = 0;
    uint64_t                        m_receiveWindow = 0;
};

#endif // SECURECHANNEL_H
