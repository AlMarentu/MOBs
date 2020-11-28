// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2020 Matthias Lautner
//
// This is part of MObs https://github.com/AlMarentu/MObs.git
//
// MObs is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


#include "objtypes.h"
#include "aes.h"
#include "logging.h"

#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <vector>


#define KEYBUFLEN 32
#define INPUT_BUFFER_LEN 1024

namespace {

std::string getError() {
  u_long e;
  std::string res = "OpenSSL: ";
  while ((e = ERR_get_error())) {
    static bool errLoad = false;
    if (not errLoad) {
      errLoad = true;
//      ERR_load_crypto_strings(); // nur crypto-fehler laden
      SSL_load_error_strings(); // NOLINT(hicpp-signed-bitwise)
      atexit([](){ ERR_free_strings(); });
    }
    res += ERR_error_string(e, nullptr);
  }
  return res;
}

class openssl_exception : public std::runtime_error {
public:
  openssl_exception() : std::runtime_error(getError()) {
    LOG(LM_DEBUG, "openssl: " << what());
  }
};

}

class mobs::CryptBufAesData { // NOLINT(cppcoreguidelines-pro-type-member-init)
public:
  ~CryptBufAesData()
  {
    if (ctx)
      EVP_CIPHER_CTX_free(ctx);
  }
  void initAES()
  {
    // This initially zeros out the Key and IV, and then uses the EVP_BytesToKey to populate these two data structures.
    // In this case we are using Sha1 as the key-derivation function and the same password used when we encrypted the
    // plaintext. We use a single iteration (the 6th parameter).
    key.fill(0);
    iv.fill(0);
    EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), &salt[0], (u_char*)passwd.c_str(), passwd.length(), 1, &key[0], &iv[0]);
  }

  void newSalt()
  {
    int r = RAND_bytes(&salt[0], salt.size());
    if (r < 0)
      throw openssl_exception();
  }


  std::array<mobs::CryptBufAes::char_type, INPUT_BUFFER_LEN> buffer;
  std::array<u_char, 8> salt{};
  std::array<u_char, KEYBUFLEN> iv;
  std::array<u_char, KEYBUFLEN> key;
  EVP_CIPHER_CTX *ctx = nullptr;
  std::string passwd;
//  bool finishing = false;
};


mobs::CryptBufAes::CryptBufAes(const std::string &pass) : CryptBufBase() {
  TRACE("");
  data = new mobs::CryptBufAesData;
  data->passwd = pass;
}


mobs::CryptBufAes::~CryptBufAes() {
  TRACE("");
  delete data;
}




mobs::CryptBufAes::int_type mobs::CryptBufAes::underflow() {
  TRACE("");
//  std::cout << "underflow3 " << "\n";
  try {
    if (finished())
      return Traits::eof();
    std::array<u_char, INPUT_BUFFER_LEN + EVP_MAX_BLOCK_LENGTH> buf; // NOLINT(cppcoreguidelines-pro-type-member-init)
    int len;
    {
      u_char *start = &buf[0];
      size_t sz = doRead((char *) &buf[0], buf.size());
      // Input Buffer wenigsten halb voll kriegen
      while (sz < buf.size() / 2 and not CryptBufBase::finished())
        sz += doRead((char *) &buf[sz], buf.size() - sz);
      if (not data->ctx) {
        LOG(LM_INFO, "AES init");
        if (sz >= 16 and std::string((char *) &buf[0], 8) == "Salted__") {
          memcpy(&data->salt[0], start + 8, 8);
          start += 16;
          sz -= 16;
        }

        data->initAES();
        if (not(data->ctx = EVP_CIPHER_CTX_new()))
          throw openssl_exception();
        if (1 != EVP_DecryptInit_ex(data->ctx, EVP_aes_256_cbc(), nullptr, &data->key[0], &data->iv[0]))
          throw openssl_exception();
      }

      if (1 != EVP_DecryptUpdate(data->ctx, (u_char *) &data->buffer[0], &len, start, sz))
        throw openssl_exception();
    }
    if (len == 0 and CryptBufBase::finished()) {
      if (1 != EVP_DecryptFinal_ex(data->ctx, (u_char *) &data->buffer[0], &len))
        throw openssl_exception();
      //    EVP_CIPHER_CTX_reset(data->ctx);
      EVP_CIPHER_CTX_free(data->ctx);
      LOG(LM_INFO, "AES done");
      data->ctx = nullptr;
    }
    Base::setg(&data->buffer[0], &data->buffer[0], &data->buffer[len]);
//    std::cout << "GC2 = " << len << " ";
    if (len == 0) {
      if (data->ctx)
        throw std::runtime_error("Keine Daten obwohl Quelle nicht leer");
      return Traits::eof();
    }
    return Traits::to_int_type(*Base::gptr());
  } catch (std::exception &e) {
    LOG(LM_ERROR, "EEE " << e.what());
    if (data->ctx) {
      EVP_CIPHER_CTX_free(data->ctx);
      data->ctx = nullptr;
    }
    throw e;
  }
}

mobs::CryptBufAes::int_type mobs::CryptBufAes::overflow(mobs::CryptBufAes::int_type ch) {
  TRACE("");
//  std::cout << "overflow3 " << int(ch) << "\n";
  if (Base::pbase() != Base::pptr()) {
    if (not data->ctx) {
      LOG(LM_INFO, "AES init");
      openSalt();
      data->initAES();
      if (not (data->ctx = EVP_CIPHER_CTX_new()))
        throw openssl_exception();
      if (1 != EVP_EncryptInit_ex(data->ctx, EVP_aes_256_cbc(), nullptr, &data->key[0], &data->iv[0]))
        throw openssl_exception();
    }

    int len;
    std::array<u_char, INPUT_BUFFER_LEN + EVP_MAX_BLOCK_LENGTH> buf; // NOLINT(cppcoreguidelines-pro-type-member-init)
    if (1 != EVP_EncryptUpdate(data->ctx, &buf[0], &len, (u_char *)(Base::pbase()),
                               std::distance(Base::pbase(), Base::pptr())))
      throw openssl_exception();
    LOG(LM_INFO, "Writing " << len << "  was " << std::distance(Base::pbase(), Base::pptr()));
    doWrite((char *)(&buf[0]), len);
//      std::cout << "overflow2 schreibe " << std::distance(Base::pbase(), Base::pptr()) << "\n";

    CryptBufBase::setp(data->buffer.begin(), data->buffer.end()); // buffer zurücksetzen
  }

  if (not Traits::eq_int_type(ch, Traits::eof()))
    Base::sputc(ch);
  if (isGood())
    return ch;
  return Traits::eof();
}

void mobs::CryptBufAes::finalize() {
  TRACE("");
//  std::cout << "finalize3\n";
  // bei leerer Eingabe, hier Verschlüsselung beginnen
  if (not data->ctx) {
    LOG(LM_INFO, "AES init");
    openSalt();
    data->initAES();
    if (not (data->ctx = EVP_CIPHER_CTX_new()))
      throw openssl_exception();
    if (1 != EVP_EncryptInit_ex(data->ctx, EVP_aes_256_cbc(), nullptr, &data->key[0], &data->iv[0]))
      throw openssl_exception();
  }
  if (data->ctx) {
    std::array<u_char, EVP_MAX_BLOCK_LENGTH> buf; // NOLINT(cppcoreguidelines-pro-type-member-init)
    int len;
    if (1 != EVP_EncryptFinal_ex(data->ctx, &buf[0], &len))
      throw openssl_exception();
//    EVP_CIPHER_CTX_reset(data->ctx);
    EVP_CIPHER_CTX_free(data->ctx);
    data->ctx = nullptr;
    LOG(LM_INFO, "Writing. " << len);
    doWrite(reinterpret_cast<char *>(&buf[0]), len);
  }
  CryptBufBase::finalize();
}

bool mobs::CryptBufAes::finished() {
  TRACE("");
//  std::cout << "finished3\n";
    if (not CryptBufBase::finished())
      return false;
    return (not data->ctx);
}

void mobs::CryptBufAes::openSalt() {
  TRACE("");
  data->newSalt();
  doWrite("Salted__", 8);
  doWrite(reinterpret_cast<char *>(&data->salt[0]), data->salt.size());
  LOG(LM_INFO, "Writing salt " << 8 + data->salt.size());
}





std::string mobs::to_aes_string(const std::string &s, const std::string &pass) {
  TRACE("");
  std::stringstream ss;

  mobs::CryptOstrBuf streambuf(ss, new mobs::CryptBufAes(pass));
  std::wostream xStrOut(&streambuf);
  xStrOut << mobs::CryptBufAes::base64(true);
  xStrOut << mobs::to_wstring(s);
  streambuf.finalize();

  return ss.str();
}

std::string mobs::from_aes_string(const std::string &s, const std::string &pass) {
  TRACE("");
  std::stringstream ss(s);
  mobs::CryptIstrBuf streambuf(ss, new mobs::CryptBufAes(pass));
  std::wistream xStrIn(&streambuf);
  streambuf.getCbb()->setBase64(true);
  std::string res;
  wchar_t c;
  while (not xStrIn.get(c).eof())
    res += u_char(c);
  return res;
}



