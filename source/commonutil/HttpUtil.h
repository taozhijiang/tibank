#ifndef HTTPUTIL_HTTPUTIL_H
#define HTTPUTIL_HTTPUTIL_H

#include <vector>
#include <string>
#include <list>
#include <map>
#include <boost/noncopyable.hpp>

namespace HttpUtil
{

void InitHttpEnvironment();

class HttpClient : boost::noncopyable
{
public:
	HttpClient();
	~HttpClient();

	int PostByHttp(const std::string& strUrl, const std::string& strData);
	int PostByHttp(const std::string& strUrl, const std::string& strData, const std::list<std::string> &headers, long nTimeout = 120);
    int GetByHttp(const std::string& strUrl, bool bTrace = true, long nConnTimeout = 200, long nTimeout = 600);

	char* GetData();

private:
	static size_t WriteCallback(void* ptr, size_t size, size_t nmemb, void* data);
private:
	std::vector<char> m_vecResponseData;
};

}

#endif // HTTPUTIL_HTTPUTIL_H
