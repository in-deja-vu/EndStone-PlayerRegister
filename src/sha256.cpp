// Copyright (c) 2024, The Endstone Project. (https://endstone.dev) All Rights Reserved.

#include "sha256.h"

#include <cstring>
#include <iomanip>
#include <sstream>

SHA256::SHA256() : m_blocklen(0), m_bitlen(0) {
    m_state[0] = 0x6a09e667;
    m_state[1] = 0xbb67ae85;
    m_state[2] = 0x3c6ef372;
    m_state[3] = 0xa54ff53a;
    m_state[4] = 0x510e527f;
    m_state[5] = 0x9b05688c;
    m_state[6] = 0x1f83d9ab;
    m_state[7] = 0x5be0cd19;
}

void SHA256::update(const uint8_t * data, size_t length) {
    for (size_t i = 0; i < length; i++) {
        m_data[m_blocklen++] = data[i];
        if (m_blocklen == 64) {
            transform();
            m_bitlen += 512;
            m_blocklen = 0;
        }
    }
}

void SHA256::update(const std::string &data) {
    update(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
}

std::array<uint8_t, 32> SHA256::digest() {
    std::array<uint8_t, 32> hash;

    pad();
    revert(hash);

    return hash;
}

std::string SHA256::toString(const std::array<uint8_t, 32> & digest) {
    std::stringstream ss;
    for (uint8_t byte : digest) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return ss.str();
}

std::string SHA256::digest_str(const std::string &data) {
    SHA256 sha;
    sha.update(data);
    return toString(sha.digest());
}

uint32_t SHA256::rotr(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
}

uint32_t SHA256::choose(uint32_t e, uint32_t f, uint32_t g) {
    return (e & f) ^ (~e & g);
}

uint32_t SHA256::majority(uint32_t a, uint32_t b, uint32_t c) {
    return (a & (b | c)) | (b & c);
}

uint32_t SHA256::sig0(uint32_t x) {
    return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

uint32_t SHA256::sig1(uint32_t x) {
    return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

void SHA256::transform() {
    uint32_t w[64];
    uint32_t t1, t2;

    for (int i = 0, j = 0; i < 16; i++, j += 4) {
        w[i] = (m_data[j] << 24) | (m_data[j + 1] << 16) | (m_data[j + 2] << 8) | (m_data[j + 3]);
    }

    for (int i = 16; i < 64; i++) {
        w[i] = sig1(w[i - 2]) + w[i - 7] + sig0(w[i - 15]) + w[i - 16];
    }

    uint32_t a = m_state[0];
    uint32_t b = m_state[1];
    uint32_t c = m_state[2];
    uint32_t d = m_state[3];
    uint32_t e = m_state[4];
    uint32_t f = m_state[5];
    uint32_t g = m_state[6];
    uint32_t h = m_state[7];

    for (int i = 0; i < 64; i++) {
        t1 = h + sig1(e) + choose(e, f, g) + K[i] + w[i];
        t2 = sig0(a) + majority(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    m_state[0] += a;
    m_state[1] += b;
    m_state[2] += c;
    m_state[3] += d;
    m_state[4] += e;
    m_state[5] += f;
    m_state[6] += g;
    m_state[7] += h;
}

void SHA256::pad() {
    uint64_t i = m_blocklen;
    uint8_t temp = 0x80;

    update(&temp, 1);

    while (m_blocklen != 56) {
        temp = 0x00;
        update(&temp, 1);
    }

    uint64_t bitlen = m_bitlen + m_blocklen * 8;
    for (int i = 7; i >= 0; i--) {
        temp = static_cast<uint8_t>((bitlen >> (i * 8)) & 0xFF);
        update(&temp, 1);
    }
}

void SHA256::revert(std::array<uint8_t, 32> & hash) {
    for (int i = 0; i < 8; i++) {
        hash[i * 4 + 0] = (m_state[i] >> 24) & 0xFF;
        hash[i * 4 + 1] = (m_state[i] >> 16) & 0xFF;
        hash[i * 4 + 2] = (m_state[i] >> 8) & 0xFF;
        hash[i * 4 + 3] = m_state[i] & 0xFF;
    }
}