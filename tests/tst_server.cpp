// Tests for the match-making server, run in process - part of EasyMorse, GPLv3

#include <QTest>

#include <QDir>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QWebSocket>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

#include <vector>

#include "accounts.h"
#include "protocol/signaling.h"
#include "server.h"

namespace
{
    struct Reply
    {
        int         status = 0;
        QJsonObject body;
    };

    ///
    /// \brief WebSocket client keeping every message received
    ///
    class Client : public QObject
    {
    public:
        QWebSocket          socket;
        QList<QJsonObject>  messages;

        Client()
        {
            connect(&socket, &QWebSocket::textMessageReceived, this, [this](const QString& pText) {
                messages.append(Signaling::decode(pText.toUtf8()));
            });
        }

        bool open(quint16 pPort)
        {
            QSignalSpy connected(&socket, &QWebSocket::connected);
            socket.open(QUrl(QString("ws://127.0.0.1:%1/api/v1/ws").arg(pPort)));
            return connected.wait(3000);
        }

        void send(const QJsonObject& pMessage)
        {
            socket.sendTextMessage(QString::fromUtf8(Signaling::encode(pMessage)));
        }

        ///
        /// \brief wait for a message of this type and return it (the first not yet taken)
        ///
        QJsonObject take(const QString& pType, int pTimeoutMs = 3000)
        {
            QElapsedTimer timer;
            timer.start();
            while (timer.elapsed() < pTimeoutMs)
            {
                for (qsizetype i = 0; i < messages.size(); ++i)
                {
                    if (messages[i].value("type").toString() == pType)
                        return messages.takeAt(i);
                }
                QTest::qWait(10);
            }
            return {};
        }
    };
}

class TestServer : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir               m_temp;
    std::unique_ptr<Server>     m_server;
    QNetworkAccessManager       m_network;

    Reply request(const QString& pMethod, const QString& pPath, const QJsonObject& pBody = {},
                  const QString& pToken = QString())
    {
        QNetworkRequest request(QUrl(QString("http://127.0.0.1:%1%2").arg(m_server->port()).arg(pPath)));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        if (!pToken.isEmpty())
            request.setRawHeader("Authorization", "Bearer " + pToken.toLatin1());
        QNetworkReply* reply = (pMethod == "POST")
            ? m_network.post(request, QJsonDocument(pBody).toJson())
            : m_network.get(request);
        QSignalSpy finished(reply, &QNetworkReply::finished);
        if (!finished.wait(10000))
            return {};
        Reply result{reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(),
                     QJsonDocument::fromJson(reply->readAll()).object()};
        reply->deleteLater();
        return result;
    }

    ///
    /// \brief replace the server; the HTTP connections kept alive are closed
    /// first, QHttpServer does not free them when it is destroyed
    ///
    bool restart(const Server::Config& pConfig)
    {
        stop();
        m_server = std::make_unique<Server>();
        return m_server->start(pConfig);
    }

    void stop()
    {
        m_network.clearConnectionCache();
        QTest::qWait(50);
        m_server.reset();
    }

    QString tokenFor(const QString& pCallsign)
    {
        const Reply reply = request("POST", "/api/v1/register", {{"callsign", pCallsign}, {"password", "morse code!"}});
        return reply.body.value("token").toString();
    }

    std::unique_ptr<Client> connectedClient(const QString& pCallsign)
    {
        auto client = std::make_unique<Client>();
        if (!client->open(m_server->port()))
            return nullptr;
        client->send(Signaling::message(Signaling::Type::Auth, {{"token", tokenFor(pCallsign)}}));
        if (client->take(Signaling::Type::Welcome).isEmpty())
            return nullptr;
        return client;
    }

private slots:
    void init()
    {
        m_server = std::make_unique<Server>();
        Server::Config config;
        config.port = 0;
        config.database = ":memory:";
        config.authAttempts = 50;
        QVERIFY(m_server->start(config));
        QVERIFY(m_server->port() != 0);
    }

    void cleanup()
    {
        stop();
    }

    void passwordHashes()
    {
        const QString hash = Accounts::hashPassword("secret password");
        QVERIFY(hash.startsWith("argon2id$"));
        QVERIFY(!hash.contains("secret"));
        QVERIFY(Accounts::verifyPassword("secret password", hash));
        QVERIFY(!Accounts::verifyPassword("secret passwore", hash));
        // Salted: the same password gives another hash
        QVERIFY(Accounts::hashPassword("secret password") != hash);
        QVERIFY(!Accounts::verifyPassword("x", "garbage"));
    }

    void registerAndLogin()
    {
        Reply reply = request("POST", "/api/v1/register", {{"callsign", "f4ikz"}, {"password", "morse code!"}});
        QCOMPARE(reply.status, 201);
        QCOMPARE(reply.body.value("callsign").toString(), QString("F4IKZ"));
        QVERIFY(!reply.body.value("token").toString().isEmpty());

        QCOMPARE(request("POST", "/api/v1/register", {{"callsign", "F4IKZ"}, {"password", "another one"}}).status, 409);
        QCOMPARE(request("POST", "/api/v1/register", {{"callsign", "x"}, {"password", "morse code!"}}).status, 400);
        QCOMPARE(request("POST", "/api/v1/register", {{"callsign", "F5ABC"}, {"password", "short"}}).status, 400);

        reply = request("POST", "/api/v1/login", {{"callsign", "F4ikz"}, {"password", "morse code!"}});
        QCOMPARE(reply.status, 200);
        QVERIFY(!reply.body.value("token").toString().isEmpty());
        QCOMPARE(request("POST", "/api/v1/login", {{"callsign", "F4IKZ"}, {"password", "wrong!!!!"}}).status, 401);
        QCOMPARE(request("POST", "/api/v1/login", {{"callsign", "NOBODY"}, {"password", "wrong!!!!"}}).status, 401);
    }

    void limitsAuthAttempts()
    {
        Server::Config config;
        config.port = 0;
        config.database = ":memory:";
        config.authAttempts = 3;
        QVERIFY(restart(config));
        for (int i = 0; i < 3; ++i)
            QCOMPARE(request("POST", "/api/v1/login", {{"callsign", "F4IKZ"}, {"password", "guess0000"}}).status, 401);
        QCOMPARE(request("POST", "/api/v1/login", {{"callsign", "F4IKZ"}, {"password", "guess0000"}}).status, 429);
    }

    void roomsAndPages()
    {
        const Reply rooms = request("GET", "/api/v1/rooms");
        QCOMPARE(rooms.status, 200);
        QCOMPARE(rooms.body.value("rooms").toArray().size(), 2);
        QCOMPARE(request("GET", "/").status, 200);
        QCOMPARE(request("GET", "/missing.html").status, 404);
    }

    void refusesBadTokens()
    {
        Client client;
        QVERIFY(client.open(m_server->port()));
        QSignalSpy closed(&client.socket, &QWebSocket::disconnected);
        client.send(Signaling::message(Signaling::Type::Join, {{"room", "QSO"}}));
        QCOMPARE(client.take(Signaling::Type::Error).value("code").toString(), QString("not_authenticated"));
        client.send(Signaling::message(Signaling::Type::Auth, {{"token", "forged"}}));
        QCOMPARE(client.take(Signaling::Type::Error).value("code").toString(), QString("bad_token"));
        QTRY_VERIFY_WITH_TIMEOUT(closed.count() > 0, 3000);
    }

    void roomPresenceAndChat()
    {
        auto alice = connectedClient("F4IKZ");
        auto bob = connectedClient("F5XYZ");
        QVERIFY(alice && bob);

        alice->send(Signaling::message(Signaling::Type::Join, {{"room", "QSO"}}));
        QCOMPARE(alice->take(Signaling::Type::Room).value("members").toArray(), QJsonArray({"F4IKZ"}));
        bob->send(Signaling::message(Signaling::Type::Join, {{"room", "qso"}}));
        QCOMPARE(bob->take(Signaling::Type::Room).value("members").toArray(), QJsonArray({"F4IKZ", "F5XYZ"}));
        QCOMPARE(alice->take(Signaling::Type::Joined).value("callsign").toString(), QString("F5XYZ"));
        QCOMPARE(request("GET", "/api/v1/rooms").body.value("rooms").toArray().at(1).toObject().value("members").toInt(), 2);

        bob->send(Signaling::message(Signaling::Type::Chat, {{"text", "  GM OM  "}}));
        const QJsonObject chat = alice->take(Signaling::Type::Chat);
        QCOMPARE(chat.value("from").toString(), QString("F5XYZ"));
        QCOMPARE(chat.value("text").toString(), QString("GM OM"));

        bob->socket.close();
        QCOMPARE(alice->take(Signaling::Type::Left).value("callsign").toString(), QString("F5XYZ"));
    }

    void signalsStayInTheRoom()
    {
        auto alice = connectedClient("F4IKZ");
        auto bob = connectedClient("F5XYZ");
        auto eve = connectedClient("F6EVE");
        QVERIFY(alice && bob && eve);
        alice->send(Signaling::message(Signaling::Type::Join, {{"room", "QSO"}}));
        bob->send(Signaling::message(Signaling::Type::Join, {{"room", "QSO"}}));
        eve->send(Signaling::message(Signaling::Type::Join, {{"room", "Beginners"}}));
        QVERIFY(!bob->take(Signaling::Type::Room).isEmpty());
        QVERIFY(!eve->take(Signaling::Type::Room).isEmpty());

        alice->send(Signaling::message(Signaling::Type::Signal,
            {{"to", "f5xyz"}, {"kind", Signaling::Kind::Invitation}, {"data", "EM1:abc"}}));
        const QJsonObject signal = bob->take(Signaling::Type::Signal);
        QCOMPARE(signal.value("from").toString(), QString("F4IKZ"));
        QCOMPARE(signal.value("kind").toString(), Signaling::Kind::Invitation);
        QCOMPARE(signal.value("data").toString(), QString("EM1:abc"));

        // Not to someone in another room, and no unknown kinds
        eve->send(Signaling::message(Signaling::Type::Signal, {{"to", "F4IKZ"}, {"kind", Signaling::Kind::Call}}));
        QCOMPARE(eve->take(Signaling::Type::Error).value("code").toString(), QString("unknown_member"));
        alice->send(Signaling::message(Signaling::Type::Signal, {{"to", "F5XYZ"}, {"kind", "hack"}}));
        QCOMPARE(alice->take(Signaling::Type::Error).value("code").toString(), QString("bad_message"));
        QVERIFY(alice->take(Signaling::Type::Signal, 300).isEmpty());
    }

    void newConnectionReplacesTheOld()
    {
        auto first = connectedClient("F4IKZ");
        QVERIFY(first);
        QSignalSpy closed(&first->socket, &QWebSocket::disconnected);
        Client second;
        QVERIFY(second.open(m_server->port()));
        const Reply login = request("POST", "/api/v1/login", {{"callsign", "F4IKZ"}, {"password", "morse code!"}});
        second.send(Signaling::message(Signaling::Type::Auth, {{"token", login.body.value("token").toString()}}));
        QVERIFY(!second.take(Signaling::Type::Welcome).isEmpty());
        QTRY_VERIFY_WITH_TIMEOUT(closed.count() > 0, 3000);
    }

    void roomsAreLimited()
    {
        std::vector<std::unique_ptr<Client>> members;
        for (int i = 0; i < Signaling::MaxRoomMembers; ++i)
        {
            members.push_back(connectedClient(QString("F%1AAA").arg(i)));
            QVERIFY(members.back());
            members.back()->send(Signaling::message(Signaling::Type::Join, {{"room", "QSO"}}));
            QVERIFY(!members.back()->take(Signaling::Type::Room).isEmpty());
        }
        auto late = connectedClient("F9LATE");
        QVERIFY(late);
        late->send(Signaling::message(Signaling::Type::Join, {{"room", "QSO"}}));
        QCOMPARE(late->take(Signaling::Type::Error).value("code").toString(), QString("room_full"));
        // Another room is fine
        late->send(Signaling::message(Signaling::Type::Join, {{"room", "Beginners"}}));
        QVERIFY(!late->take(Signaling::Type::Room).isEmpty());
    }

    void administration()
    {
        const QString admin = tokenFor("F1ADM");
        const QString user = tokenFor("F5XYZ");
        // Not an administrator yet
        QCOMPARE(request("GET", "/api/v1/admin/users", {}, admin).status, 403);
        QCOMPARE(request("GET", "/api/v1/admin/users").status, 403);
        QVERIFY(m_server->accounts().setAdmin("f1adm", true) == Accounts::Result::Ok);
        QCOMPARE(request("GET", "/api/v1/me", {}, admin).body.value("admin").toBool(), true);
        QCOMPARE(request("GET", "/api/v1/me", {}, user).body.value("admin").toBool(), false);
        QCOMPARE(request("POST", "/api/v1/login", {{"callsign", "F1ADM"}, {"password", "morse code!"}})
                     .body.value("admin").toBool(), true);
        QCOMPARE(request("POST", "/api/v1/admin/users", {{"action", "ban"}, {"callsign", "F1ADM"}}, user).status, 403);

        // The user is online in a room
        Client client;
        QVERIFY(client.open(m_server->port()));
        QSignalSpy closed(&client.socket, &QWebSocket::disconnected);
        client.send(Signaling::message(Signaling::Type::Auth, {{"token", user}}));
        QVERIFY(!client.take(Signaling::Type::Welcome).isEmpty());
        client.send(Signaling::message(Signaling::Type::Join, {{"room", "QSO"}}));
        QVERIFY(!client.take(Signaling::Type::Room).isEmpty());
        Reply list = request("GET", "/api/v1/admin/users", {}, admin);
        QCOMPARE(list.status, 200);
        QCOMPARE(list.body.value("online").toInt(), 1);
        const QJsonArray users = list.body.value("users").toArray();
        QCOMPARE(users.size(), 2);
        QCOMPARE(users.at(0).toObject().value("callsign").toString(), QString("F1ADM"));
        QCOMPARE(users.at(0).toObject().value("admin").toBool(), true);
        QCOMPARE(users.at(1).toObject().value("room").toString(), QString("QSO"));
        QVERIFY(users.at(1).toObject().value("lastLogin").isUndefined());

        // Suspended: disconnected with the reason, cannot come back
        QCOMPARE(request("POST", "/api/v1/admin/users",
                         {{"action", "ban"}, {"callsign", "f5xyz"}, {"reason", "spam"}}, admin).status, 204);
        const QJsonObject banned = client.take(Signaling::Type::Error);
        QCOMPARE(banned.value("code").toString(), QString("banned"));
        QCOMPARE(banned.value("message").toString(), QString("spam"));
        QTRY_VERIFY_WITH_TIMEOUT(closed.count() > 0, 3000);
        QCOMPARE(request("GET", "/api/v1/me", {}, user).status, 401);
        Reply login = request("POST", "/api/v1/login", {{"callsign", "F5XYZ"}, {"password", "morse code!"}});
        QCOMPARE(login.status, 403);
        QCOMPARE(login.body.value("error").toString(), QString("banned"));
        // A wrong password does not tell the account is suspended
        QCOMPARE(request("POST", "/api/v1/login", {{"callsign", "F5XYZ"}, {"password", "wrong!!!!"}}).status, 401);
        list = request("GET", "/api/v1/admin/users", {}, admin);
        QCOMPARE(list.body.value("users").toArray().at(1).toObject().value("reason").toString(), QString("spam"));

        QCOMPARE(request("POST", "/api/v1/admin/users", {{"action", "unban"}, {"callsign", "F5XYZ"}}, admin).status, 204);
        QCOMPARE(request("POST", "/api/v1/login", {{"callsign", "F5XYZ"}, {"password", "morse code!"}}).status, 200);

        // New password, given by the administrator
        QCOMPARE(request("POST", "/api/v1/admin/users",
                         {{"action", "password"}, {"callsign", "F5XYZ"}, {"password", "short"}}, admin).status, 400);
        QCOMPARE(request("POST", "/api/v1/admin/users",
                         {{"action", "password"}, {"callsign", "F5XYZ"}, {"password", "new password"}}, admin).status, 204);
        QCOMPARE(request("POST", "/api/v1/login", {{"callsign", "F5XYZ"}, {"password", "morse code!"}}).status, 401);
        const QString second = request("POST", "/api/v1/login", {{"callsign", "F5XYZ"}, {"password", "new password"}})
                                   .body.value("token").toString();
        QVERIFY(!second.isEmpty());

        // Kick: logged out everywhere
        QCOMPARE(request("POST", "/api/v1/admin/users", {{"action", "kick"}, {"callsign", "F5XYZ"}}, admin).status, 204);
        QCOMPARE(request("GET", "/api/v1/me", {}, second).status, 401);

        // Rights, and no locking oneself out
        QCOMPARE(request("POST", "/api/v1/admin/users", {{"action", "grant"}, {"callsign", "F5XYZ"}}, admin).status, 204);
        QVERIFY(m_server->accounts().isAdmin("F5XYZ"));
        QCOMPARE(request("POST", "/api/v1/admin/users", {{"action", "revoke"}, {"callsign", "F5XYZ"}}, admin).status, 204);
        QVERIFY(!m_server->accounts().isAdmin("F5XYZ"));
        QCOMPARE(request("POST", "/api/v1/admin/users", {{"action", "ban"}, {"callsign", "F1ADM"}}, admin).status, 400);
        QCOMPARE(request("POST", "/api/v1/admin/users", {{"action", "revoke"}, {"callsign", "F1ADM"}}, admin).status, 400);
        QCOMPARE(request("POST", "/api/v1/admin/users", {{"action", "dance"}, {"callsign", "F5XYZ"}}, admin).status, 400);
        QCOMPARE(request("POST", "/api/v1/admin/users", {{"action", "ban"}, {"callsign", "NOBODY"}}, admin).status, 404);

        // Deleted
        QCOMPARE(request("POST", "/api/v1/admin/users", {{"action", "delete"}, {"callsign", "F5XYZ"}}, admin).status, 204);
        QCOMPARE(request("POST", "/api/v1/login", {{"callsign", "F5XYZ"}, {"password", "new password"}}).status, 401);
        QCOMPARE(request("GET", "/api/v1/admin/users", {}, admin).body.value("users").toArray().size(), 1);
        QCOMPARE(request("GET", "/admin.html").status, 200);
    }

    void olderDatabasesAreUpgraded()
    {
        // A database of the first version, without the moderation columns
        const QString path = QDir(m_temp.path()).filePath("old.db");
        {
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "old");
            db.setDatabaseName(path);
            QVERIFY(db.open());
            QSqlQuery query(db);
            QVERIFY(query.exec("CREATE TABLE users (id INTEGER PRIMARY KEY, callsign TEXT NOT NULL UNIQUE,"
                               " password TEXT NOT NULL, created INTEGER NOT NULL)"));
            query.prepare("INSERT INTO users (callsign, password, created) VALUES ('F4IKZ', ?, 0)");
            query.addBindValue(Accounts::hashPassword("morse code!"));
            QVERIFY(query.exec());
            db.close();
        }
        QSqlDatabase::removeDatabase("old");
        Accounts accounts;
        QVERIFY(accounts.open(path));
        QString token;
        QVERIFY(accounts.login("F4IKZ", "morse code!", token) == Accounts::Result::Ok);
        QVERIFY(accounts.setAdmin("F4IKZ", true) == Accounts::Result::Ok);
        QVERIFY(accounts.user("F4IKZ").lastLogin > 0);
    }

    void relayCredentials()
    {
        Server::Config config;
        config.port = 0;
        config.database = ":memory:";
        config.publicHost = "morse.example.org";
        config.turnEnabled = true;
        config.turn.port = 0;
        config.turn.bindAddress = "127.0.0.1";
        QVERIFY(restart(config));
        Client client;
        QVERIFY(client.open(m_server->port()));
        client.send(Signaling::message(Signaling::Type::Auth, {{"token", tokenFor("F4IKZ")}}));
        const QJsonObject ice = client.take(Signaling::Type::Welcome).value("ice").toObject();
        QVERIFY(ice.value("stun").toString().startsWith("morse.example.org:"));
        const QJsonObject turn = ice.value("turn").toObject();
        QCOMPARE(turn.value("host").toString(), QString("morse.example.org"));
        QVERIFY(!turn.value("username").toString().isEmpty());
        QVERIFY(turn.value("password").toString().size() >= 20);
    }
};

QTEST_GUILESS_MAIN(TestServer)
#include "tst_server.moc"
