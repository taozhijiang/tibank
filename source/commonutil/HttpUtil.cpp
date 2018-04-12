#include "HttpUtil.h"
#include "Log.h"


#include <curl/curl.h>
#include <sstream>
#include <string.h>
#include <pthread.h>

#include <boost/smart_ptr/make_shared.hpp>

namespace {

class CurlInit {
public:
	void SetShareHandle(CURL* handle) {
		curl_easy_setopt(handle, CURLOPT_SHARE, _share_handle);
		curl_easy_setopt(handle, CURLOPT_DNS_CACHE_TIMEOUT, 60*5);
	}

public:
	CurlInit() {

		curl_global_init(CURL_GLOBAL_ALL);
        pthread_rwlock_init(&rwlock_, NULL);

		_share_handle = curl_share_init();
		curl_share_setopt(_share_handle, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
		curl_share_setopt(_share_handle, CURLSHOPT_LOCKFUNC, dnsLock);
		curl_share_setopt(_share_handle, CURLSHOPT_UNLOCKFUNC, dnsUnlock);
		curl_share_setopt(_share_handle, CURLSHOPT_USERDATA, this);
	}

	~CurlInit() {
		curl_global_cleanup();
        pthread_rwlock_destroy(&rwlock_);
	}

private:
	 CURLSH* _share_handle;
	 pthread_rwlock_t rwlock_;

static void dnsLock(CURL *h, curl_lock_data data, curl_lock_access access, void *userptr){
	if (data == CURL_LOCK_DATA_DNS){
		if ( access == CURL_LOCK_ACCESS_SHARED ){
			CurlInit *init = (CurlInit *)userptr;
			if (init) {
				pthread_rwlock_rdlock(&init->rwlock_);
			}
		} else if( access == CURL_LOCK_ACCESS_SINGLE ){
			CurlInit *init = (CurlInit *)userptr;
			if (init) {
				pthread_rwlock_wrlock(&init->rwlock_);
			}
		} else {
            log_err("!!! error: unknown access type: %d", access);
        }
	}
}
static void dnsUnlock(CURL *handle, curl_lock_data data, void *userptr){
	if (data == CURL_LOCK_DATA_DNS){
		CurlInit *init = (CurlInit *)userptr;
		if (init) {
			pthread_rwlock_unlock(&init->rwlock_);
		}
	}
}
};

typedef boost::shared_ptr<CurlInit> CurlInitPtr;

}

static CurlInitPtr g_ptrCurlInit;

void HttpUtil::InitHttpEnvironment() {
	g_ptrCurlInit = boost::make_shared<CurlInit>();
}

HttpUtil::HttpClient::HttpClient()
{
	if (!g_ptrCurlInit) {
		HttpUtil::InitHttpEnvironment();
	}
}

HttpUtil::HttpClient::~HttpClient() {
}

int HttpUtil::HttpClient::GetByHttp(const std::string& strUrl, bool bTrace, long nConnTimeout, long nTimeout)
{
	if (bTrace) {
		log_info("strUrl=%s", strUrl.c_str());
	}

	m_vecResponseData.clear();

	try {
		CURL *curl;
		CURLcode res;
        long nStatusCode = 500;
		curl = curl_easy_init();
		curl_easy_setopt(curl, CURLOPT_ENCODING , "UTF-8");
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1);
		curl_easy_setopt(curl, CURLOPT_POST, 0);
		curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, nConnTimeout);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, nTimeout);
		g_ptrCurlInit->SetShareHandle(curl);

        res = curl_easy_perform(curl);
        m_vecResponseData.push_back(0);
        if (res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &nStatusCode);
        }
        curl_easy_cleanup(curl);
        if (res == CURLE_OK && nStatusCode == 200  ) {
            if (bTrace) {
                log_info("Back:%s", (char*)&m_vecResponseData[0]);
            }
            return 0;
        } else {
            log_err("nStatusCode:%ld res=%d,error:%s", nStatusCode, res, curl_easy_strerror(res));
            return -1;
        }
	} catch (...) {
		return -1;
	}
}


int HttpUtil::HttpClient::PostByHttp(const std::string& strUrl, const std::string& strData)
{
	log_info("strUrl=%s,strData=%s", strUrl.c_str(), strData.c_str());

	m_vecResponseData.clear();

	try {
		CURL *curl;
		CURLcode res;
        long nStatusCode = 500;
		curl = curl_easy_init();
		curl_easy_setopt(curl, CURLOPT_ENCODING , "UTF-8");
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1);
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strData.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 200);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120);
		g_ptrCurlInit->SetShareHandle(curl);

        res = curl_easy_perform(curl);
        m_vecResponseData.push_back(0);

        if (res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &nStatusCode);
        }
        curl_easy_cleanup(curl);
        if (res == CURLE_OK && nStatusCode == 200){
        	log_info("Back:%s", (char*)&m_vecResponseData[0]);
            return 0;
        } else {
            log_err("nStatusCode:%ld res=%d,error:%s", nStatusCode, res, curl_easy_strerror(res));
            return -1;
        }
	} catch (...) {
		return -1;
	}
}


int HttpUtil::HttpClient::PostByHttp(const std::string& strUrl, const std::string& strData, const std::list<std::string> &headers, long nTimeout)
{
	log_info("strUrl=%s,strData=%s", strUrl.c_str(), strData.c_str());

	m_vecResponseData.clear();

	try {
		CURL *curl;
		CURLcode res;
        long nStatusCode = 500;
		curl = curl_easy_init();
		curl_easy_setopt(curl, CURLOPT_ENCODING , "UTF-8");
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1);
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, strData.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, nTimeout);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, nTimeout + 5);

		struct curl_slist *s_headers=NULL;
	    std::list<std::string>::const_iterator it;
	    for (it=headers.begin(); it!=headers.end(); it++) {
	        s_headers = curl_slist_append(s_headers, it->c_str());
	    }
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, s_headers);

		g_ptrCurlInit->SetShareHandle(curl);


        res = curl_easy_perform(curl);
        curl_slist_free_all(s_headers);
        m_vecResponseData.push_back(0);

        if (res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &nStatusCode);
        }
        curl_easy_cleanup(curl);
        if (res == CURLE_OK && nStatusCode == 200) {
        	log_info("Back:%s", (char*)&m_vecResponseData[0]);
            return 0;
        } else {
            log_err("nStatusCode:%ld res=%d,error:%s", nStatusCode, res, curl_easy_strerror(res));
            return -1;
        }
	} catch (...) {
		return -1;
	}
}



char* HttpUtil::HttpClient::GetData() {
	return &m_vecResponseData[0];
}

size_t HttpUtil::HttpClient::WriteCallback(void* ptr, size_t size, size_t nmemb, void* data)
{
	HttpUtil::HttpClient* pHttp = (HttpUtil::HttpClient*)data;
	std::vector<char>& vecResponseData = pHttp->m_vecResponseData;
	std::vector<char>::size_type pos = vecResponseData.size();
	size_t len = size*nmemb;
	vecResponseData.resize(pos + len);
	memcpy(&vecResponseData[pos], ptr, len);
	return len;
}

