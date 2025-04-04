#include <iostream>
#include <fstream>
#include <string>
#include <cryptopp/modes.h>
#include <cryptopp/aes.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <cryptopp/pwdbased.h>
#include <cryptopp/sha.h>
#include <cryptopp/osrng.h>
#include <cryptopp/gcm.h>
#include <cryptopp/secblock.h> 
#include <cctype>
#include "encrypt.h"
using namespace CryptoPP;

#include <iostream>
#include <string>

// with my tests, the program will produce a not completely complex password every 50 times or less 
// this function will prevent that 
bool isValidPassword(const std::string& password) {
    bool hasNumber = false;
    bool hasLower = false;
    bool hasUpper = false;
    bool hasSymbol = false;

    for (char ch : password) {
        if (std::isdigit(ch)) {
            hasNumber = true;
        } else if (std::islower(ch)) {
            hasLower = true;
        } else if (std::isupper(ch)) {
            hasUpper = true;
        } else if (!std::isalnum(ch)) {
            hasSymbol = true;
        }
    }

    return hasNumber && hasLower && hasUpper && hasSymbol;
}

std::string generateRandomPassword(size_t length) {

    std::string chars = "abcdefghijklmnopqrstuvwxyz"
                        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                        "0123456789"
                        "!@#$%^&*()_+{}:<>,.?"; 
    std::string result;
    result.reserve(length);
    // preventing too much cycle (will not happen but lets be cautious )
    int count = 10;
    do{
        count--;
        if (!result.empty()) {
            volatile char* p = &result[0];
            for (size_t i = 0; i < result.size(); ++i) {
                p[i] = '\0';
            }
            result.clear();
        }
        
        // use secure memory handling
        CryptoPP::SecByteBlock password(reinterpret_cast<const CryptoPP::byte*>(chars.data()), chars.size());
        // create a random number generator
        CryptoPP::AutoSeededRandomPool rng;

        // generate the password with no bias
        CryptoPP::byte index;
        for (size_t i = 0; i < length; ++i) {
            do {
                rng.GenerateBlock(&index, 1);
            } while (index >= (256 - (256 % chars.size())));  // avoid modulo bias
            result += chars[index % chars.size()];
        }

        // clear sensitive data from memory
        memset(password.data(), 0, password.size());
    
    }while(count>0 && !isValidPassword(result));
    if (count <= 0){
        return "";
    }
    return result;
}

// define a suitable IV size
// 12 bytes is common for GCM to avoid the need for counter wrapping considerations
const size_t IV_SIZE = 12;  

std::string EncryptString(const std::string& plaintext, const  std::string& password) {
    std::string salt, ciphertext, encoded;

    // generate a random salt
    AutoSeededRandomPool prng;
    CryptoPP::byte tempSalt[8];
    prng.GenerateBlock(tempSalt, sizeof(tempSalt));

    // derive key from password and salt
    SecByteBlock key(AES::DEFAULT_KEYLENGTH);
    PKCS5_PBKDF2_HMAC<SHA256> pbkdf;
    pbkdf.DeriveKey(key.data(), key.size(), 0x00, reinterpret_cast<const CryptoPP::byte*>(password.data()), password.size(), tempSalt, sizeof(tempSalt), 10000);

    // encrypt with aes-gcm
    CryptoPP::byte iv[IV_SIZE];
    prng.GenerateBlock(iv, sizeof(iv));

    GCM<AES>::Encryption enc;
    enc.SetKeyWithIV(key, key.size(), iv);

    StringSource ss(plaintext, true,
        new AuthenticatedEncryptionFilter(enc,
            new StringSink(ciphertext)));

    // encode salt, IV, and ciphertext for transport
    HexEncoder encoder;
    encoder.Attach(new StringSink(encoded));
    encoder.Put(tempSalt, sizeof(tempSalt));
    encoder.Put(iv, sizeof(iv));
    encoder.Put(reinterpret_cast<const CryptoPP::byte*>(ciphertext.data()), ciphertext.size());
    encoder.MessageEnd();

    return encoded;
}

std::string DecryptString(const std::string& encodedCiphertext, const std::string& password) {
    std::string decoded;

    // decode from Hex
    StringSource ss(encodedCiphertext, true, new HexDecoder(new StringSink(decoded)));

    // extract salt and IV
    CryptoPP::byte tempSalt[8], iv[IV_SIZE];
    memcpy(tempSalt, decoded.data(), sizeof(tempSalt));
    memcpy(iv, decoded.data() + sizeof(tempSalt), sizeof(iv));

    // derive key
    SecByteBlock key(AES::DEFAULT_KEYLENGTH);
    PKCS5_PBKDF2_HMAC<SHA256> pbkdf;
    pbkdf.DeriveKey(key.data(), key.size(), 0x00, reinterpret_cast<const CryptoPP::byte*>(password.data()), password.size(), tempSalt, sizeof(tempSalt), 10000);

    // decrypt with aes-gcm
    GCM<AES>::Decryption dec;
    dec.SetKeyWithIV(key, key.size(), iv);

    const CryptoPP::byte* ciphertext = reinterpret_cast<const CryptoPP::byte*>(decoded.data() + sizeof(tempSalt) + sizeof(iv));
    size_t ciphertextLen = decoded.size() - sizeof(tempSalt) - sizeof(iv);
    std::string decryptedtext;

    try {
        AuthenticatedDecryptionFilter df(dec, new StringSink(decryptedtext));
        df.Put(ciphertext, ciphertextLen);
        df.MessageEnd();
    } catch (const CryptoPP::Exception& e) {
        std::cerr << "Decryption failed: " << e.what() << std::endl;
        exit (-1);
    }

    return decryptedtext;
}



SecByteBlock generateSalt() {
    AutoSeededRandomPool prng;
    SecByteBlock salt(SHA256::DIGESTSIZE);
    prng.GenerateBlock(salt, salt.size());
    return salt;
}

std::string savePassword(const SecByteBlock& password, const SecByteBlock& salt) {
    SHA256 hash;
    std::string digest;

    // Using a more secure password hashing approach
    PKCS5_PBKDF2_HMAC<SHA256> pbkdf;
    byte key[SHA256::DIGESTSIZE];
    pbkdf.DeriveKey(key, sizeof(key), 0, password, password.size(), salt, salt.size(), 10000);

    // Convert key to hex string
    StringSource ss(key, sizeof(key), true, new HexEncoder(new StringSink(digest)));
    return digest;
}

bool checkPassword(const SecByteBlock& password, const std::string& correct_hash, const SecByteBlock& salt) {
    std::string new_hash = savePassword(password, salt);
    return new_hash == correct_hash;
}