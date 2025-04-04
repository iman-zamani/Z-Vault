
#include <cryptopp/secblock.h>
using CryptoPP::SecByteBlock;

std::string EncryptString(const std::string& plaintext, const  std::string& password);

std::string DecryptString(const std::string& encodedCiphertext, const std::string& password);

std::string generateRandomPassword(size_t length);


SecByteBlock generateSalt();
std::string savePassword(const SecByteBlock& password, const SecByteBlock& salt);
bool checkPassword(const SecByteBlock& password, const std::string& correct_hash, const SecByteBlock& salt);