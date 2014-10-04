#include <assert.h>
#include "Html.hpp"

namespace YTD {
   
  Html::Html(): pCurl(NULL) {
      pCurl = curl_easy_init();
      assert(pCurl);
      curl_easy_setopt(pCurl, CURLOPT_FOLLOWLOCATION, 1L);
      curl_easy_setopt(pCurl, CURLOPT_NOSIGNAL, 1);     
      curl_easy_setopt(pCurl, CURLOPT_ACCEPT_ENCODING, "deflate");
  }
  
  Html::~Html() {
      curl_easy_cleanup(pCurl);
  }
  
  void Html::DecodeUrl(const char *pUrl, int len, std::string &outPage) {
      int outLen = 0;
      const char *pDecodedUrl = curl_easy_unescape(pCurl, (char *)pUrl, len, &outLen);
      if(NULL != pDecodedUrl) {
          outPage.assign(pDecodedUrl, outLen);
      }
      else {
          outPage.clear();
      }
  }
} 


