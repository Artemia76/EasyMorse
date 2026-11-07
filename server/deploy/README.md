# Deploying the EasyMorse server

`easymorse-server` puts EasyMorse users in touch: accounts, rooms (every
member is connected to the others by itself), an administration page, and a
STUN/TURN relay for the connections that cannot go direct. Morse
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

## Behind an existing proxy (YunoHost or another nginx)

When ports 80 and 443 already belong to a web server, use `proxy/`: the
server alone, listening on `127.0.0.1`, behind that server's HTTPS.

1. Build the image on any machine and copy it (the server needs no sources):

   ```sh
   docker build -f server/deploy/Dockerfile -t easymorse-server:latest .
   docker save easymorse-server:latest | ssh my-server docker load
   ```

2. On the server, with YunoHost (the domain points to the server):

   ```sh
   sudo yunohost domain add easymorse.example.org
   sudo yunohost domain cert install easymorse.example.org   # Let's Encrypt
   sudo cp nginx-easymorse.conf /etc/nginx/conf.d/easymorse.example.org.d/
   sudo nginx -t && sudo systemctl reload nginx
   sudo yunohost firewall allow UDP 3478
   sudo yunohost firewall allow UDP 49160:49200
   ```

   With another nginx, put the two locations of `nginx-easymorse.conf` in the
   server block of the domain and open the same UDP ports.

3. Start it (`.env` from `.env.example`; the data directory belongs to the
   `easymorse` user of the image):

   ```sh
   sudo mkdir -p /var/lib/easymorse
   sudo chown "$(docker run --rm --entrypoint id easymorse-server -u)" /var/lib/easymorse
   docker compose up -d
   curl https://easymorse.example.org/api/v1/status
   ```

Update: load the new image, then `docker compose up -d`. Undo: `docker compose
down`, remove the nginx file and reload nginx, close the UDP ports
(`yunohost firewall disallow ...`), remove the domain.

## Administration

The page `https://<your domain>/admin.html` lists the accounts (online or not,
room, last login) and moderates them: disconnect, suspend with a reason shown
to the user, reactivate, give a new password, make or remove administrators,
delete. Actions are written to the server log.

Nobody is an administrator at first: create your account (web page or
application), then name it from the command line, server running or not:

```sh
# Docker (both compose files)
docker compose exec easymorse easymorse-server --database /data/easymorse.db --grant-admin F4IKZ
# systemd
sudo easymorse-server --database /var/lib/easymorse/easymorse.db --grant-admin F4IKZ
```

`--revoke-admin CALLSIGN` removes the rights. Other administrators can then be
named from the page.

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
| `--grant-admin`, `--revoke-admin` | | Change the administrator rights of an account, then exit |

## Security notes

- Passwords are hashed with Argon2id; only a hash of the access tokens is stored.
- Login and registration attempts are limited per address; WebSocket messages
  are limited in size and rate.
- TURN credentials are random, issued to logged in users and valid 12 hours.
- Rooms hold 8 members at most: each one keeps a link with every other.
- Suspending, disconnecting or changing the password of an account revokes its
  tokens: the application has to log in again.
