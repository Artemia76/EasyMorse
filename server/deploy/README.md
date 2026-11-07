# Deploying the EasyMorse server

`easymorse-server` puts EasyMorse users in touch: accounts, rooms, calls,
and a STUN/TURN relay for the connections that cannot go direct. Morse
itself goes peer to peer between the applications, end to end encrypted:
the server never sees it.

## Requirements

- A Linux VPS (1 vCPU / 512 MB is plenty) with a public IPv4 address.
- A domain name pointing to it (e.g. `morse.example.org`).
- Open ports: 80/tcp and 443/tcp (HTTPS, Let's Encrypt), 3478/udp (STUN/TURN),
  49160-49200/udp (relayed sessions).

## With Docker (recommended)

```sh
git clone https://github.com/Artemia76/EasyMorse.git
cd EasyMorse/server/deploy
cp .env.example .env     # set EASYMORSE_DOMAIN and EASYMORSE_PUBLIC_IP
docker compose up -d --build
```

Caddy gets the certificate and forwards HTTPS and WebSockets to the server,
which only listens on `127.0.0.1:8080`. Accounts are kept in the
`easymorse-data` volume (`/data/easymorse.db`): back it up.

Update: `git pull && docker compose up -d --build`.

## Without Docker

Build the server alone (Qt 6.8+ with HttpServer, WebSockets and Sql):

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DEASYMORSE_BUILD_APP=OFF
cmake --build build --target easymorse-server
sudo install build/easymorse-server /usr/local/bin/
```

Then use `easymorse-server.service` (systemd, adjust the domain and address)
behind a reverse proxy doing HTTPS, for instance Caddy with the `Caddyfile`
of this directory.

## Options

| Option | Default | Meaning |
|--------|---------|---------|
| `--listen`, `--port` | `127.0.0.1`, `8080` | HTTP and WebSocket address |
| `--database` | `easymorse.db` | SQLite file |
| `--trust-proxy` | off | Read client addresses from `X-Forwarded-For` (behind the proxy only) |
| `--rooms` | `Beginners,QSO` | Permanent rooms; users can open others |
| `--public-host` | none | Host name given to clients for STUN/TURN; enables the relay |
| `--external-address` | none | Public IP announced for relayed sessions |
| `--turn-port` | `3478` | STUN/TURN UDP port |
| `--relay-ports` | any | UDP port range for relayed sessions |

## Security notes

- Passwords are hashed with Argon2id; only a hash of the access tokens is stored.
- Login and registration attempts are limited per address; WebSocket messages
  are limited in size and rate.
- TURN credentials are random, issued to logged in users and valid 12 hours.
