#ifndef _TiBANK_SIGN_HELPER_H_
#define _TiBANK_SIGN_HELPER_H_

#include <map>

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <commonutil/BCD.h>
#include <commonutil/Base64.h>

class SignHelper {

public:
	static SignHelper& instance();

	bool init();
	int check_sign(const std::string& strJson);
	int check_sign(const std::map<std::string, std::string>& mapParam);

    int calc_sign(const std::map<std::string, std::string>& mapParam, std::string& sign);

private:
	SignHelper() {
	}

	~SignHelper() {
		EVP_PKEY_free(tibank_platform_private_key_);
        EVP_PKEY_free(tibank_app_public_key_);
	}

private:
	EVP_PKEY* import_public_key(const char* keyfile);
    EVP_PKEY* import_private_key(const char* keyfile);

	int sha1_with_rsa_verify(const EVP_PKEY* key, const std::string& buffer, const std::string& sig);
	int Sha1_with_rsa_sign(const EVP_PKEY* key, const std::string& buffer, std::string& sign);
    std::string sha1_with_rsa_sign(unsigned char* pBuffer, unsigned int nLen);


    EVP_PKEY* tibank_platform_private_key_;
	EVP_PKEY* tibank_app_public_key_;
};

#endif // _TiBANK_SIGN_HELPER_H_
