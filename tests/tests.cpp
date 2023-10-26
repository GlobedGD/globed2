#undef NDEBUG

#include <iostream>
#include <cassert>
#include <functional>
#include <thread>
#include <map>

#include <crypto/box.hpp>
#include <util/rng.hpp>
#include <util/debugging.hpp>
#include <util/time.hpp>
#include <data/bytebuffer.hpp>

using namespace std::string_literals;
using namespace util::rng;
using namespace util::data;
using namespace util::debugging;
using namespace util;

#define ASSERT(cond, message) if (!(cond)) { throw std::runtime_error(std::string(message)); }

void tCrypto() {
    CryptoBox alice, bob;
    alice.setPeerKey(bob.getPublicKey());
    bob.setPeerKey(alice.getPublicKey());

    // test with random data
    for (int i = 0; i < 4096; i++) {
        Random& rng = Random::get();
        ByteBuffer buf;

        uint32_t length = rng.generate(0, 16384);
        uint32_t pushed = 0;

        while (pushed < length) {
            uint32_t remaining = length - pushed;
            if (remaining >= 4) {
                buf.write(rng.generate<uint32_t>());
                pushed += 4;
            } else if (remaining >= 2) {
                buf.write(rng.generate<uint16_t>());
                pushed += 2;
            } else if (remaining == 1) {
                uint8_t num = rng.generate<uint16_t>() % 255;
                buf.write(num);
                pushed += 1;
            }
        }

        auto contents = buf.getDataRef();
        bytevector decrypted;

        if (rng.generate<bool>()) {
            auto encrypted = alice.encrypt(contents);
            decrypted = bob.decrypt(encrypted);
        } else {
            auto encrypted = bob.encrypt(contents);
            decrypted = alice.decrypt(encrypted);
        }

        for (size_t i = 0; i < decrypted.size(); i++) {
            ASSERT(contents[i] == decrypted[i], "decrypted data is invalid");
        }
    }
}

void tRandom() {
    std::vector<bool> bools;
    Random& rng = Random::get();
    for (int i = 0; i < 65536; i++) {
        bools.push_back(rng.generate<bool>());
    }

    // test entropy
    int trues = 0;
    int falses = 0;
    for (int i = 0; i < bools.size(); i++) {
        if (bools[i]) trues++;
        else falses++;
    }

    auto diff = std::abs(trues - falses);

    ASSERT(diff < 4096, "poor random entropy")
}

std::map<std::string, std::function<void()>> tests = {
    {"Random"s, tRandom},
    {"Crypto"s, tCrypto},
};

std::chrono::microseconds bnCrypto(size_t dataLength, size_t iterations) {
    CryptoBox alice, bob;
    alice.setPeerKey(bob.getPublicKey());
    bob.setPeerKey(alice.getPublicKey());

    byte* buf = new byte[dataLength];
    randombytes_buf(buf, dataLength);

    bytevector encrypted(buf, buf + dataLength);

    Benchmarker bench;
    bench.start("all");
    for (size_t i = 0; i < iterations; i++) {
        encrypted = alice.encrypt(encrypted);
    }

    bytevector decrypted(encrypted);
    for (size_t i = 0; i < iterations; i++) {
        decrypted = bob.decrypt(decrypted);
    }
    auto tooktime = bench.end("all");

    for (size_t i = 0; i < decrypted.size(); i++) {
        if (buf[i] != decrypted[i]) {
            delete[] buf;
            ASSERT(false, "decrypted data is invalid");
        }
    }

    delete[] buf;

    auto totalmicros = (long long)((float)(tooktime.count()) / (float)(iterations));
    return std::chrono::microseconds(totalmicros);
}

int main(int argc, const char* *argv) {
    int passed = 0;
    int failed = 0;

    std::string mode = "test";
    if (argc >= 2) {
        mode = std::string(argv[1]);
    }

    if (mode == "test") {
        Benchmarker bench;
        for (auto& [name, test] : tests) {
            bench.start(name);
            try {
                test();
                auto time = time::toString(bench.end(name));
                std::cout << "\033[32mTest passed: " << name << " (took " << time << ")\033[0m" << std::endl;
            } catch (const std::runtime_error& e) {
                auto time = time::toString(bench.end(name));
                std::cerr << "\033[31mTest failed: " << name << " - " << e.what() << " (took " << time << ")\033[0m" << std::endl;
            }
        }
    } else if (mode == "benchmark") {
        Benchmarker bench;
        int runs = 1024;
        std::cout << "Running crypto benchmarks, 1024 iterations per test" << std::endl;
        std::cout << "64 bytes, per iter: " << time::toString(bnCrypto(64, runs)) << std::endl;
        std::cout << "1024 bytes, per iter: " << time::toString(bnCrypto(1024, runs)) << std::endl;
        std::cout << "8192 bytes, per iter: " << time::toString(bnCrypto(8192, runs)) << std::endl;
        std::cout << "65536 bytes, per iter: " << time::toString(bnCrypto(65536, runs)) << std::endl;
    } else {
        std::cerr << "modes: test, benchmark" << std::endl;
    }
}