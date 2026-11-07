# easymorse-server image. Build from the repository root:
#   docker build -f server/deploy/Dockerfile -t easymorse-server .

FROM debian:trixie AS build
RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates cmake g++ git ninja-build \
        qt6-base-dev qt6-httpserver-dev qt6-websockets-dev \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . .
# libjuice is not packaged by Debian: it is downloaded and linked statically
RUN cmake -S . -B /build -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DEASYMORSE_BUILD_APP=OFF -DEASYMORSE_BUILD_TESTS=OFF \
    && cmake --build /build --target easymorse-server \
    && strip /build/easymorse-server

FROM debian:trixie-slim
RUN apt-get update && apt-get install -y --no-install-recommends \
        libqt6core6t64 libqt6network6 libqt6httpserver6 libqt6websockets6 libqt6sql6 libqt6sql6-sqlite \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/* \
    && useradd --system --home-dir /data --no-create-home easymorse \
    && mkdir /data && chown easymorse:easymorse /data
ENV LANG=C.UTF-8
COPY --from=build /build/easymorse-server /usr/local/bin/easymorse-server
USER easymorse
VOLUME /data
WORKDIR /data
# HTTP + WebSocket (behind the reverse proxy), STUN/TURN, relayed sessions
EXPOSE 8080/tcp 3478/udp 49160-49200/udp
ENTRYPOINT ["easymorse-server", "--database", "/data/easymorse.db"]
CMD ["--listen", "127.0.0.1", "--trust-proxy"]
