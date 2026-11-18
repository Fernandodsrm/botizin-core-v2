#pragma once
// Fallback for compiling without CI-generated fingerprint map.
// ci/build.py regenerates this file from immutable, SHA-verified historical BINs.
struct Ota3KnownImage { uint8_t elf_sha256[32]; const char *version; bool ota3; };
static const Ota3KnownImage* ota3KnownImage(const esp_app_desc_t &) { return nullptr; }
