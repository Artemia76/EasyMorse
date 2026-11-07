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
* accounts.h is part of EasyMorse server
*
* User accounts in SQLite: callsign and Argon2id password hash, access
* tokens of which only a hash is stored.
* **************************************************************************/

#ifndef ACCOUNTS_H
#define ACCOUNTS_H

#include <QString>
#include <QtSql/QSqlDatabase>

class Accounts
{
public:
    enum class Result
    {
        Ok,
        InvalidCallsign,
        WeakPassword,
        AlreadyExists,
        WrongCredentials,
        DatabaseError
    };

    static constexpr int TokenLifetimeDays = 90;

    Accounts() = default;
    ~Accounts();
    Accounts(const Accounts&) = delete;
    Accounts& operator=(const Accounts&) = delete;

    ///
    /// \param pPath SQLite file, created if missing (":memory:" for tests)
    ///
    bool    open(const QString& pPath);

    ///
    /// \brief create an account and return a token for it
    ///
    Result  registerUser(const QString& pCallsign, const QString& pPassword, QString& pToken);
    ///
    /// \brief check the password and return a new token
    ///
    Result  login(const QString& pCallsign, const QString& pPassword, QString& pToken);
    ///
    /// \return the callsign the token belongs to, empty if unknown or expired
    ///
    QString callsignForToken(const QString& pToken);
    void    revokeToken(const QString& pToken);

    // Password hashes, exposed for the tests
    static QString hashPassword(const QString& pPassword);
    static bool    verifyPassword(const QString& pPassword, const QString& pHash);

private:
    QString createToken(qint64 pUserId);

    QSqlDatabase    m_db;
    QString         m_connection;
};

#endif // ACCOUNTS_H
