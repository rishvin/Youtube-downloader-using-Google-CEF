#ifndef _YTD_HPP_
#define _YTD_HPP_

#include <iostream>
#include <string>
#include <vector>

namespace YTD {

  #define KB 0
  #define MB 1
  #define GB 2

  typedef std::pair<void*, size_t> DOWNLOADER_PAIR;
  typedef void (*FnUpdate)(DOWNLOADER_PAIR pair);

  #define DOWNLOADER_PAIR_KEY(PAIR) (PAIR.first)
  #define DOWNLOADER_PAIR_VALUE(PAIR) (PAIR.second)
 
  class Videos {
    public:
      Videos(std::string &title, std::string &url, size_t &iTag, size_t len)
          :title(title), url(url), iTag(iTag), len(len) {
      }
      
      inline std::string &GetTitle() { return title; }
      inline std::string &GetUrl() { return url; }
      inline size_t &GetiTag() { return iTag; }
      inline size_t &GetLength() { return len; }

    private:
        std::string title, url;
        size_t iTag, len;
    };

  
  class App {
    public:
      static int GetVideos(const char* pUrl, std::vector<Videos> &urls);
      static size_t Download(const char *pUrl, const char *pFile, 
          void *pKey, FnUpdate pfnUpdate, int &interrupt);
    };

  std::string iTagToQuality(size_t &iTag);
  void GetDownloadUrl(Videos &video, std::string &response);
  size_t ConvertToSize(int iType, size_t size);
}

#endif
