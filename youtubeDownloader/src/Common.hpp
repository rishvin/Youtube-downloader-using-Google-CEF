#ifndef _COMMON_HPP_
#define _COMMON_HPP_

#include <string>

#define ERR_OK 0
#define ERR_TITLE -1
#define ERR_DECODE_URL -2
#define ERR_PAGE_NOT_FOUND -3


#define CHECK(EXP, CMD)          \
    if(EXP) {                    \
        CMD;                     \
    }

namespace YTD {
        size_t WritePage(void *ptr, size_t size, size_t nmemb, void *stream);
        size_t GetiTag(std::string &url);
        void GetTitle(std::string &page, size_t &outPos, size_t &outLen);
        size_t GetUrl(std::string &page, size_t start, size_t &outPos, size_t & outLen);
        void UpateToUnsecureUrl(std::string &url);
        size_t GetUrlSection(std::string &page);
        std::string GetDownloadUrl(std::string &url, std::string &title);
}

#endif
