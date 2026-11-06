# Monocypher 4.0.3 - https://monocypher.org
# BSD-2-Clause or CC0 (see LICENCE.md). X25519 key exchange and
# XChaCha20-Poly1305 for the peer to peer sessions.

add_library(monocypher STATIC monocypher.c monocypher.h)
add_library(EasyMorse::monocypher ALIAS monocypher)
target_include_directories(monocypher SYSTEM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(monocypher PROPERTIES POSITION_INDEPENDENT_CODE ON)
