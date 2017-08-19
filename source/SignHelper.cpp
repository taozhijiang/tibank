#include <map>
#include <sstream>

#include <openssl/ssl.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/err.h>

#include <commonutil/Sha1.h>
#include <commonutil/BCD.h>
#include <json/json.h>

#include "Log.h"
#include "SignHelper.h"

SignHelper& SignHelper::instance() {
	static SignHelper helper;
	return helper;
}

bool SignHelper::init() {

	tibank_app_public_key_ = import_public_key("../signfile/app_public_key.pem");
	if (!tibank_app_public_key_) {
		log_error("Import app public key failed, fatal!");
		return false;
	}

    tibank_platform_private_key_ = import_private_key("../signfile/platform_private_key.pem");
	if (!tibank_platform_private_key_) {
		log_error("Import platform private key failed, fatal!");
		return false;
	}

	return true;
}

EVP_PKEY* SignHelper::import_public_key(const char* keyfile){

	EVP_PKEY* pkey = NULL;
	X509* x509 = NULL;

	FILE *fp = fopen(keyfile, "r");
	if(fp == NULL){
		log_error("read file error");
		return NULL;
	}

	::rewind(fp);
    x509 = PEM_read_X509(fp, NULL, NULL, NULL);
	if (x509) {
		pkey = X509_get_pubkey(x509);
		X509_free(x509);
	} else {
		::rewind(fp);
		pkey = PEM_read_PUBKEY(fp, NULL, NULL, NULL);
	}

	if(pkey == NULL){
		log_error("get pub key error");
	}

	fclose(fp);
	return pkey;
}

EVP_PKEY* SignHelper::import_private_key(const char* keyfile){

    EVP_PKEY* pkey = NULL;
    FILE *fp = fopen(keyfile, "r");
    if(fp == NULL) {
        log_error("load private key file failed");
        return false;
    }
    ::rewind(fp);
    pkey = PEM_read_PrivateKey(fp, NULL, NULL, NULL);
    if (!pkey)
        log_error("load private key error!");

    fclose(fp);

    return pkey;
}

// 我赌通过SSL_setup，这些都是线程安全的
int SignHelper::sha1_with_rsa_verify(const EVP_PKEY* key, const std::string& buffer, const std::string& sig)
{
	int nBCDEResultLen;
	unsigned char result[512+1];
	commonutil::BCD_Decode(sig.c_str(), sig.length(), result, sizeof(result), &nBCDEResultLen);

	unsigned char ubuffer[2048];
	memset(ubuffer, 0, sizeof(ubuffer));
	memcpy(ubuffer, buffer.c_str(), buffer.length());

	unsigned char sha1result[20];
	EVP_MD_CTX ctx;
	EVP_MD_CTX_init(&ctx);
	EVP_SignInit(&ctx, EVP_sha1());
	EVP_SignUpdate(&ctx, ubuffer, buffer.length());

	unsigned int nShalResultLen = sizeof(sha1result);
	EVP_DigestFinal(&ctx, sha1result, &nShalResultLen);
	EVP_MD_CTX_cleanup(&ctx);
	if (!RSA_verify(NID_sha1, sha1result, sizeof(sha1result), result, nBCDEResultLen, key->pkey.rsa)) {
		return -1;
	}

	return 0;
}

std::string SignHelper::sha1_with_rsa_sign(unsigned char* pBuffer, unsigned int nLen) {

	unsigned char signresult[128];

	unsigned int nSignResultLen = sizeof(signresult);
	RSA_sign(NID_sha1, pBuffer, nLen, signresult, &nSignResultLen, tibank_platform_private_key_->pkey.rsa);

	char signresulthex[256+1] = {0};
	int nBase64ResultLen = 0;
	(void)commonutil::Base64_Encode(signresult, nSignResultLen, signresulthex, sizeof(signresulthex), &nBase64ResultLen);
	signresulthex[nBase64ResultLen] = 0;

	return signresulthex;
}

int SignHelper::calc_sign(const std::map<std::string, std::string>& mapParam, std::string& sign) {

    std::ostringstream osForSHA1;

    for (std::map<std::string, std::string>::const_iterator iter = mapParam.begin(); iter != mapParam.end(); iter++) {
        if (iter->first == "sign"){
            log_error("we cannot sign object with sign provide!");
            return -1;
        }

        osForSHA1 << iter->first;
        osForSHA1 << "=";
        osForSHA1 << iter->second;
        osForSHA1 << "&";
    }

    std::string strDataForSHA1 = osForSHA1.str();
	strDataForSHA1 = strDataForSHA1.substr(0, strDataForSHA1.size() - 1); //删除结尾的&
    // log_trace("strDataForSHA1:%s", strDataForSHA1.c_str());

    unsigned char sha1value[20];
	EVP_MD_CTX ctx;
	EVP_MD_CTX_init(&ctx);
	EVP_SignInit(&ctx, EVP_sha1());
	EVP_SignUpdate(&ctx, strDataForSHA1.c_str(), strDataForSHA1.size());
	unsigned int nShalResultLen = sizeof(sha1value);
	EVP_DigestFinal(&ctx, sha1value, &nShalResultLen);
	EVP_MD_CTX_cleanup(&ctx);

	sign = sha1_with_rsa_sign((unsigned char*)sha1value, 20);

    return 0;
}

int SignHelper::check_sign(const std::map<std::string, std::string>& mapParam) {

	std::string strSign;
	std::ostringstream osForSHA1;

	for (std::map<std::string, std::string>::const_iterator iter = mapParam.begin(); iter != mapParam.end(); iter++) {
		if (iter->first == "sign") {
			strSign = iter->second;
			continue;
		}

		osForSHA1 << iter->first;
		osForSHA1 << "=";
		osForSHA1 << iter->second;
		osForSHA1 << "&";
	}

	if (strSign.empty()) {
		log_error("sign not found or empty");
		return -2;
	}

	std::string strDataForSHA1 = osForSHA1.str();
	std::string strNeedSHA1Data = strDataForSHA1;
	log_error("strNeedSHA1Data:%s", strNeedSHA1Data.c_str());

	if(sha1_with_rsa_verify(tibank_app_public_key_, strNeedSHA1Data, strSign)){
		log_error("str: %s, sign: %s ", strNeedSHA1Data.c_str(), strSign.c_str());
		return -4;
	}

	return 0;
}


int SignHelper::check_sign(const std::string& strJson) {

	Json::Value root;
	Json::Reader reader;

    if(!reader.parse(strJson, root) || root.isNull()) {
        log_error("parse error for: %s", strJson.c_str());
        return -1;
    }

	std::map<std::string, std::string> mapInput;
	Json::Value::Members mem = root.getMemberNames();
	for (Json::Value::Members::const_iterator iter = mem.begin(); iter != mem.end(); iter++) {
        mapInput[*iter] = root[*iter].asString();
    }

	return check_sign(mapInput);
}
