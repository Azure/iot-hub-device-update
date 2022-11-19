#ifndef ROOTKEYPKGTESTUTILS_HPP
#define ROOTKEYPKGTESTUTILS_HPP

#include <string>
#include <vector>

namespace aduc
{
namespace rootkeypkgtestutils
{
class TestRSAPrivateKey
{
public:
    TestRSAPrivateKey(){}
    ~TestRSAPrivateKey(){}

    TestRSAPrivateKey(const TestRSAPrivateKey&) = delete;
    TestRSAPrivateKey& operator=(const TestRSAPrivateKey&) = delete;
    TestRSAPrivateKey(TestRSAPrivateKey&&) = delete;
    TestRSAPrivateKey& operator=(TestRSAPrivateKey&&) = delete;

    std::string SignData(const std::string& data) const {
        return "";
    }
};

class TestRSAPublicKey
{
public:
    TestRSAPublicKey(){}
    ~TestRSAPublicKey(){}

    TestRSAPublicKey(const TestRSAPublicKey&) = delete;
    TestRSAPublicKey& operator=(const TestRSAPublicKey&) = delete;
    TestRSAPublicKey(TestRSAPublicKey&&) = delete;
    TestRSAPublicKey& operator=(TestRSAPublicKey&&) = delete;

    std::string GetModulus() const {
        return "foo";
    }

    std::string GetExponent() const {
        return "bar";
    }

    std::string getSha256HashOfPublicKey() const {
        return "baz";
    }

    bool VerifySignature(const std::string& signature) const {
        return false;
    }
};

class TestRSAKeyPair
{
    TestRSAPrivateKey privateKey;
    TestRSAPublicKey publicKey;

public:
    TestRSAKeyPair(){
        // generate private/public key pairs.
    }

    ~TestRSAKeyPair(){
        // cleanup resources
    }

    TestRSAKeyPair(const TestRSAKeyPair&) = delete;
    TestRSAKeyPair& operator=(const TestRSAKeyPair&) = delete;
    TestRSAKeyPair(TestRSAKeyPair&&) = delete;
    TestRSAKeyPair& operator=(TestRSAKeyPair&&) = delete;

    const TestRSAPublicKey& GetPublicKey() const {
        return publicKey;
    }

    const TestRSAPrivateKey& GetPrivateKey() const
    {
        return privateKey;
    }
};

} // namespace rootkeypkgtestutils
} // namespace aduc

#endif // ROOTKEYPKGTESTUTILS_HPP
