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
* accounts.cpp is part of EasyMorse server
* **************************************************************************/

#include "accounts.h"

#include <QDateTime>
#include <QRandomGenerator>
#include <QStringList>
#include <QUuid>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

#include <vector>

#include <monocypher.h>

#include "protocol/signaling.h"

namespace
{
    // OWASP recommendation for Argon2id: 19 MiB, 2 passes, 1 lane
    constexpr uint32_t ArgonBlocks = 19 * 1024;
    constexpr uint32_t ArgonPasses = 2;
    constexpr int SaltSize = 16;
    constexpr int HashSize = 32;

    QByteArray randomBytes(int pSize)
    {
        // System generator: cryptographically secure
        QByteArray bytes(pSize, Qt::Uninitialized);
        QRandomGenerator::system()->fillRange(reinterpret_cast<quint32*>(bytes.data()), pSize / 4);
        return bytes;
    }

    QByteArray argon2(const QByteArray& pPassword, const QByteArray& pSalt, uint32_t pBlocks, uint32_t pPasses)
    {
        QByteArray hash(HashSize, Qt::Uninitialized);
        std::vector<uint8_t> workArea(static_cast<size_t>(pBlocks) * 1024);
        crypto_argon2_config config{CRYPTO_ARGON2_ID, pBlocks, pPasses, 1};
        crypto_argon2_inputs inputs{reinterpret_cast<const uint8_t*>(pPassword.constData()),
                                    reinterpret_cast<const uint8_t*>(pSalt.constData()),
                                    static_cast<uint32_t>(pPassword.size()),
                                    static_cast<uint32_t>(pSalt.size())};
        crypto_argon2(reinterpret_cast<uint8_t*>(hash.data()), HashSize, workArea.data(),
                      config, inputs, crypto_argon2_no_extras);
        crypto_wipe(workArea.data(), workArea.size());
        return hash;
    }

    ///
    /// \brief only a hash of the tokens is stored: a stolen database gives no access
    ///
    QByteArray tokenHash(const QString& pToken)
    {
        const QByteArray token = pToken.toLatin1();
        QByteArray hash(32, Qt::Uninitialized);
        crypto_blake2b(reinterpret_cast<uint8_t*>(hash.data()), 32,
                       reinterpret_cast<const uint8_t*>(token.constData()), token.size());
        return hash;
    }
}

Accounts::~Accounts()
{
    if (m_db.isValid())
    {
        m_db.close();
        m_db = QSqlDatabase();
        QSqlDatabase::removeDatabase(m_connection);
    }
}

bool Accounts::open(const QString& pPath)
{
    m_connection = QUuid::createUuid().toString();
    m_db = QSqlDatabase::addDatabase("QSQLITE", m_connection);
    m_db.setDatabaseName(pPath);
    if (!m_db.open())
        return false;
    QSqlQuery query(m_db);
    if (!query.exec("PRAGMA foreign_keys = ON")
        || !query.exec("CREATE TABLE IF NOT EXISTS users ("
                       " id INTEGER PRIMARY KEY,"
                       " callsign TEXT NOT NULL UNIQUE,"
                       " password TEXT NOT NULL,"
                       " created INTEGER NOT NULL)")
        || !query.exec("CREATE TABLE IF NOT EXISTS tokens ("
                       " hash BLOB PRIMARY KEY,"
                       " user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,"
                       " expires INTEGER NOT NULL)"))
        return false;

    // Columns added after the first version
    QStringList columns;
    if (!query.exec("PRAGMA table_info(users)"))
        return false;
    while (query.next())
        columns.append(query.value(1).toString());
    const QList<std::pair<QString, QString>> added = {
        {"admin", "INTEGER NOT NULL DEFAULT 0"},
        {"banned", "INTEGER NOT NULL DEFAULT 0"},
        {"ban_reason", "TEXT"},
        {"last_login", "INTEGER"},
    };
    for (const auto& [name, type] : added)
    {
        if (!columns.contains(name) && !query.exec(QString("ALTER TABLE users ADD COLUMN %1 %2").arg(name, type)))
            return false;
    }
    return true;
}

QString Accounts::hashPassword(const QString& pPassword)
{
    const QByteArray salt = randomBytes(SaltSize);
    const QByteArray hash = argon2(pPassword.toUtf8(), salt, ArgonBlocks, ArgonPasses);
    // Parameters are stored so they can be raised later
    return QString("argon2id$m=%1,t=%2$%3$%4").arg(ArgonBlocks).arg(ArgonPasses)
        .arg(QString::fromLatin1(salt.toBase64()), QString::fromLatin1(hash.toBase64()));
}

bool Accounts::verifyPassword(const QString& pPassword, const QString& pHash)
{
    const QStringList parts = pHash.split(u'$');
    if (parts.size() != 4 || parts[0] != "argon2id")
        return false;
    uint32_t blocks = 0, passes = 0;
    for (const QString& parameter : parts[1].split(u','))
    {
        if (parameter.startsWith("m="))
            blocks = parameter.mid(2).toUInt();
        else if (parameter.startsWith("t="))
            passes = parameter.mid(2).toUInt();
    }
    const QByteArray salt = QByteArray::fromBase64(parts[2].toLatin1());
    const QByteArray expected = QByteArray::fromBase64(parts[3].toLatin1());
    if (blocks < 8 || blocks > 1024 * 1024 || passes < 1 || passes > 10 || expected.size() != HashSize)
        return false;
    const QByteArray hash = argon2(pPassword.toUtf8(), salt, blocks, passes);
    return crypto_verify32(reinterpret_cast<const uint8_t*>(hash.constData()),
                           reinterpret_cast<const uint8_t*>(expected.constData())) == 0;
}

Accounts::Result Accounts::registerUser(const QString& pCallsign, const QString& pPassword, QString& pToken)
{
    const QString callsign = Signaling::normalizeCallsign(pCallsign);
    if (callsign.isEmpty())
        return Result::InvalidCallsign;
    if (pPassword.size() < Signaling::MinPasswordChars)
        return Result::WeakPassword;
    QSqlQuery query(m_db);
    query.prepare("SELECT 1 FROM users WHERE callsign = ?");
    query.addBindValue(callsign);
    if (!query.exec())
        return Result::DatabaseError;
    if (query.next())
        return Result::AlreadyExists;
    query.prepare("INSERT INTO users (callsign, password, created) VALUES (?, ?, ?)");
    query.addBindValue(callsign);
    query.addBindValue(hashPassword(pPassword));
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    if (!query.exec())
        return Result::DatabaseError;
    pToken = createToken(query.lastInsertId().toLongLong());
    return pToken.isEmpty() ? Result::DatabaseError : Result::Ok;
}

Accounts::Result Accounts::login(const QString& pCallsign, const QString& pPassword, QString& pToken)
{
    const QString callsign = Signaling::normalizeCallsign(pCallsign);
    QSqlQuery query(m_db);
    query.prepare("SELECT id, password, banned FROM users WHERE callsign = ?");
    query.addBindValue(callsign);
    if (!query.exec())
        return Result::DatabaseError;
    if (!query.next())
    {
        // Same cost as a real check: response time does not tell which callsigns exist
        static const QString dummy = hashPassword(QStringLiteral("not a password"));
        verifyPassword(pPassword, dummy);
        return Result::WrongCredentials;
    }
    if (!verifyPassword(pPassword, query.value(1).toString()))
        return Result::WrongCredentials;
    // Told only to who knows the password
    if (query.value(2).toBool())
        return Result::Banned;
    const qint64 id = query.value(0).toLongLong();
    query.prepare("UPDATE users SET last_login = ? WHERE id = ?");
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    query.addBindValue(id);
    query.exec();
    pToken = createToken(id);
    return pToken.isEmpty() ? Result::DatabaseError : Result::Ok;
}

QString Accounts::createToken(qint64 pUserId)
{
    const QString token = QString::fromLatin1(randomBytes(32).toBase64(QByteArray::Base64UrlEncoding
                                                                       | QByteArray::OmitTrailingEquals));
    QSqlQuery query(m_db);
    // Expired tokens are cleaned up as new ones are created
    query.prepare("DELETE FROM tokens WHERE expires < ?");
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    query.exec();
    query.prepare("INSERT INTO tokens (hash, user_id, expires) VALUES (?, ?, ?)");
    query.addBindValue(tokenHash(token));
    query.addBindValue(pUserId);
    query.addBindValue(QDateTime::currentDateTimeUtc().addDays(TokenLifetimeDays).toSecsSinceEpoch());
    return query.exec() ? token : QString();
}

QString Accounts::callsignForToken(const QString& pToken)
{
    if (pToken.isEmpty())
        return QString();
    QSqlQuery query(m_db);
    query.prepare("SELECT users.callsign FROM tokens JOIN users ON users.id = tokens.user_id"
                  " WHERE tokens.hash = ? AND tokens.expires >= ? AND users.banned = 0");
    query.addBindValue(tokenHash(pToken));
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    if (!query.exec() || !query.next())
        return QString();
    return query.value(0).toString();
}

void Accounts::revokeToken(const QString& pToken)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM tokens WHERE hash = ?");
    query.addBindValue(tokenHash(pToken));
    query.exec();
}

void Accounts::revokeTokens(const QString& pCallsign)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM tokens WHERE user_id = (SELECT id FROM users WHERE callsign = ?)");
    query.addBindValue(Signaling::normalizeCallsign(pCallsign));
    query.exec();
}

QList<Accounts::User> Accounts::users()
{
    QList<User> list;
    QSqlQuery query(m_db);
    if (!query.exec("SELECT callsign, admin, banned, ban_reason, created, last_login FROM users ORDER BY callsign"))
        return list;
    while (query.next())
    {
        list.append({query.value(0).toString(), query.value(1).toBool(), query.value(2).toBool(),
                     query.value(3).toString(), query.value(4).toLongLong(), query.value(5).toLongLong()});
    }
    return list;
}

Accounts::User Accounts::user(const QString& pCallsign)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT callsign, admin, banned, ban_reason, created, last_login FROM users WHERE callsign = ?");
    query.addBindValue(Signaling::normalizeCallsign(pCallsign));
    if (!query.exec() || !query.next())
        return {};
    return {query.value(0).toString(), query.value(1).toBool(), query.value(2).toBool(),
            query.value(3).toString(), query.value(4).toLongLong(), query.value(5).toLongLong()};
}

bool Accounts::isAdmin(const QString& pCallsign)
{
    const User account = user(pCallsign);
    return account.admin && !account.banned;
}

namespace
{
    Accounts::Result update(QSqlDatabase& pDb, const QString& pSql, const QVariantList& pValues)
    {
        QSqlQuery query(pDb);
        query.prepare(pSql);
        for (const QVariant& value : pValues)
            query.addBindValue(value);
        if (!query.exec())
            return Accounts::Result::DatabaseError;
        return query.numRowsAffected() > 0 ? Accounts::Result::Ok : Accounts::Result::NotFound;
    }
}

Accounts::Result Accounts::setAdmin(const QString& pCallsign, bool pAdmin)
{
    return update(m_db, "UPDATE users SET admin = ? WHERE callsign = ?",
                  {pAdmin ? 1 : 0, Signaling::normalizeCallsign(pCallsign)});
}

Accounts::Result Accounts::setBanned(const QString& pCallsign, bool pBanned, const QString& pReason)
{
    const Result result = update(m_db, "UPDATE users SET banned = ?, ban_reason = ? WHERE callsign = ?",
                                 {pBanned ? 1 : 0, pBanned ? pReason.left(200) : QVariant(QMetaType(QMetaType::QString)),
                                  Signaling::normalizeCallsign(pCallsign)});
    if (result == Result::Ok && pBanned)
        revokeTokens(pCallsign);
    return result;
}

Accounts::Result Accounts::setPassword(const QString& pCallsign, const QString& pPassword)
{
    if (pPassword.size() < Signaling::MinPasswordChars)
        return Result::WeakPassword;
    const Result result = update(m_db, "UPDATE users SET password = ? WHERE callsign = ?",
                                 {hashPassword(pPassword), Signaling::normalizeCallsign(pCallsign)});
    if (result == Result::Ok)
        revokeTokens(pCallsign);
    return result;
}

Accounts::Result Accounts::deleteUser(const QString& pCallsign)
{
    // Its tokens go with it (ON DELETE CASCADE)
    return update(m_db, "DELETE FROM users WHERE callsign = ?", {Signaling::normalizeCallsign(pCallsign)});
}
