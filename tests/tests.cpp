#undef NDEBUG
/* horrible ahh code */

#include <iostream>
#include <cassert>
#include <functional>
#include <thread>
#include <map>
#include <cstring>

#include <crypto/box.hpp>
#include <crypto/secret_box.hpp>
#include <util/rng.hpp>
#include <util/debugging.hpp>
#include <util/crypto.hpp>
#include <util/time.hpp>
#include <data/bytebuffer.hpp>

using namespace std::string_literals;
using namespace util::rng;
using namespace util::data;
using namespace util::debugging;
using namespace util;

#if 1
#define CRYPTO_ITERS 4096
#define RNG_ITERS 65536
#else // valgrind is sloooooooooooooow
#define CRYPTO_ITERS 128
#define RNG_ITERS 4096
#endif

#define ASSERT(cond, message) if (!(cond)) { throw std::runtime_error(std::string(message)); }

void tCrypto() {
    CryptoBox alice, bob;
    alice.setPeerKey(bob.getPublicKey());
    bob.setPeerKey(alice.getPublicKey());

    // test with random data
    for (int i = 0; i < CRYPTO_ITERS; i++) {
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

        // now try in place
        const int testl = 64;
        byte* buf2 = new byte[testl + CryptoBox::PREFIX_LEN];
        randombytes_buf(buf2, testl);

        byte* original = new byte[testl];
        std::memcpy(original, buf2, testl);

        size_t encrypted = alice.encryptInPlace(buf2, testl);
        size_t decrypted_ = bob.decryptInPlace(buf2, encrypted);

        for (size_t i = 0; i < testl; i++) {
            if (buf2[i] != original[i]) {
                delete[] buf2;
                delete[] original;
                ASSERT(false, "decrypted data is invalid");
            }
        }

        delete[] buf2;
        delete[] original;
    }
}

void tSecretBox() {
    std::string pw = "Extremely based password";

    SecretBox alice = SecretBox::withPassword(pw);
    SecretBox bob = SecretBox::withPassword(pw);

    // test with random data
    for (int i = 0; i < CRYPTO_ITERS; i++) {
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

        // now try in place
        const int testl = 64;
        byte* buf2 = new byte[testl + CryptoBox::PREFIX_LEN];
        randombytes_buf(buf2, testl);

        byte* original = new byte[testl];
        std::memcpy(original, buf2, testl);

        size_t encrypted = alice.encryptInPlace(buf2, testl);
        size_t decrypted_ = bob.decryptInPlace(buf2, encrypted);

        for (size_t i = 0; i < testl; i++) {
            if (buf2[i] != original[i]) {
                delete[] buf2;
                delete[] original;
                ASSERT(false, "decrypted data is invalid");
            }
        }

        delete[] buf2;
        delete[] original;
    }
}

void tRandom() {
    std::vector<bool> bools;
    Random& rng = Random::get();
    for (int i = 0; i < RNG_ITERS; i++) {
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

    ASSERT(diff < RNG_ITERS / 16, "poor random entropy")
}

void tTotp() {
    auto key = util::crypto::base64Decode("hb35940QWUxD7Ss5kw+goP5QLxRn1Sc7/yIIiQ4bdCg=");
    auto hashed = util::crypto::simpleHash(key);
    // std::cout << "raw key: '" << key << "', encoded: " << util::debugging::hexDumpAddress(hashed.data(), hashed.size()) << std::endl;
    auto totp = util::crypto::simpleTOTP(hashed);

    std::cout << "totp: " << totp << std::endl;
    ASSERT(util::crypto::simpleTOTPVerify(totp, hashed), "simpleTOTPVerify failed");
}

void tEncodings() {
    auto src = std::string("holy balls!");
    auto b64 = util::crypto::base64Encode(src);
    auto hex = util::crypto::hexEncode(src);

    std::cout << "base64: " << b64 << ", hex: " << hex << std::endl;

    auto decoded = util::crypto::base64Decode(b64);
    auto decodedHex = util::crypto::hexDecode(hex);

    std::string decodedS(decoded.begin(), decoded.end());
    std::string decodedH(decodedHex.begin(), decodedHex.end());

    std::cout << "b64 decoded: '" << decodedS << "', size: " << decodedS.size() << std::endl;
    std::cout << "hex decoded: '" << decodedH << "', size: " << decodedH.size() << std::endl;

    ASSERT(decodedS == src, "base64 encode/decode failed");
    ASSERT(decodedH == src, "hex encode/decode failed");
}

using ms = std::chrono::microseconds;

std::map<std::string, std::function<void()>> tests = {
    {"Random"s, tRandom},
    {"CryptoBox"s, tCrypto},
    {"SecretBox"s, tSecretBox},
    {"TOTP", tTotp},
    {"encodings", tEncodings},
};

ms bnCrypto(size_t dataLength, size_t iterations) {
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
    return ms(totalmicros);
}

ms bnCryptoInplace(size_t dataLength, size_t iterations) {
    CryptoBox alice, bob;
    alice.setPeerKey(bob.getPublicKey());
    bob.setPeerKey(alice.getPublicKey());

    byte* buf = new byte[dataLength + CryptoBox::PREFIX_LEN];
    randombytes_buf(buf, dataLength);

    byte* original = new byte[dataLength];
    std::memcpy(original, buf, dataLength);

    Benchmarker bench;
    bench.start("all");

    CryptoBox *box1, *box2;
    for (size_t i = 0; i < iterations; i++) {
        if (Random::get().generate<bool>()) {
            box1 = &bob;
            box2 = &alice;
        } else {
            box1 = &alice;
            box2 = &bob;
        }

        size_t encSize = box1->encryptInPlace(buf, dataLength);
        dataLength = box2->decryptInPlace(buf, encSize);
    }

    auto tooktime = bench.end("all");

    for (size_t i = 0; i < dataLength; i++) {
        if (buf[i] != original[i]) {
            delete[] buf;
            delete[] original;
            ASSERT(false, "decrypted data is invalid");
        }
    }

    delete[] original;
    delete[] buf;

    auto totalmicros = (long long)((float)(tooktime.count()) / (float)(iterations));
    return ms(totalmicros);
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
        // note - the tests aren't made to compare against each other
        // but only among themselves. the in-place crypto is faster
        // because the 1st variation does X runs of encryption and then X runs of decryption
        // achieving bigger data lengths, when 2nd one runs both every iteration
        Benchmarker bench;
        int runs = 1024;
        size_t lengthRuns[] = {64, 1024, 8192, 65536};
        std::cout << "Running crypto benchmarks, 1024 iterations per test" << std::endl;

        for (size_t len : lengthRuns) {
            std::cout << len << " bytes, per iter: " << time::toString(bnCrypto(len, runs)) << std::endl;
        }

        std::cout << "Running in-place crypto benchmarks, 1024 iterations per test" << std::endl;

        for (size_t len : lengthRuns) {
            std::cout << len << " bytes, per iter: " << time::toString(bnCryptoInplace(len, runs)) << std::endl;
        }

    } else {
        std::cerr << "modes: test, benchmark" << std::endl;
    }
}