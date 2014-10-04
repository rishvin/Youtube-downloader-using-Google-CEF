#include <sstream>
#include "Ytd.hpp"
#include "Common.hpp"

#define URL_ENCODED_HEAD_LEN 26
#define TITLE_LEN             7
#define META_NAME_LEN         9
#define CONTENT_LEN           9
#define URL_LEN               4
#define U0026_LEN             6
#define iTAG_LEN              5

namespace YTD {
    
     std::string iTagToQuality(size_t &iTag) {
         switch(iTag) {
             case 5:     return "flv[320]";     
             case 6:     return "flv[480]";     
             case 17:    return "3gp[176]";     
             case 18:    return "mp4[640]";     
             case 22:    return "mp4[1280]";    
             case 34:    return "flv[640]";     
             case 35:    return "flv[854]";     
             case 36:    return "3gp[320]";     
             case 37:    return "mp4[1920]";    
             case 38:    return "mp4[2048]";    
             case 43:    return "webm[640]";    
             case 44:    return "webm[854]";    
             case 45:    return "webm[1280]";   
             case 46:    return "webm[1920]";   
             case 82:    return "3D.mp4[480]";  
             case 83:    return "3D.mp4[640]";  
             case 84:    return "3D.mp4[1280]"; 
             case 85:    return "3D.mp4[1920]"; 
             case 100:   return "3D.webm[640]"; 
             case 101:   return "3D.webm[640]"; 
             case 102:   return "3D.webm[1280]"; 
             case 120:   return "live.flv[1280]";
             default:    return "Custom";        
         }
         return "";
    }
    
    size_t GetiTag(std::string &url) {
        size_t pos = url.find("itag=");
        size_t iTag = 0;
        if(std::string::npos != pos)
        {
            pos += iTAG_LEN;
            while(1) {
                char ch = url[pos++];
                if(ch >= '0' && ch <= '9') {
                    iTag = (iTag * 10) + (ch - '0');
                }
                else {
                    break;
                }
            }
        }
        return iTag;
    }

    void GetTitle(std::string &page, size_t &outPos, size_t &outLen) {               
        size_t pos = page.find("meta name");        
        outLen = 0;
        if(std::string::npos != pos)
        {
            pos += META_NAME_LEN;
            pos = page.find("title", pos);
            if(std::string::npos != pos)
            {
                pos += TITLE_LEN;
                pos = page.find("content=\"", pos);
                pos += CONTENT_LEN;                
                size_t end = page.find("\">", pos);
                if(std::string::npos != end)
                {
                    outLen = end - pos;
                    outPos = pos;
                }
             }
        }
    }

    size_t GetUrl(std::string &page, size_t start, size_t &outPos, size_t & outLen) {
        size_t pos = start;
        size_t end = std::string::npos;
        outLen = 0;
        pos = page.find("url=", pos);
        if(std::string::npos != pos)
        {
            pos += URL_LEN;
            end = page.find("\\u0026", pos);
    		if(std::string::npos != end)
    		{
    		    outPos = pos;
    		    outLen = end - pos;
                end += U0026_LEN;
            }
        }
        return end + outLen;
    }

    void UpateToUnsecureUrl(std::string &url) {
        if(url[4] == 's') {
            url.erase(4, 1);
        }
    }

    size_t GetUrlSection(std::string &page) {
        size_t pos = page.find("url_encoded_fmt_stream_map");
        if(std::string::npos != pos) {
            pos += URL_ENCODED_HEAD_LEN;
        }
        return pos;
    }

    std::string GetDownloadUrl(std::string &url, std::string &title) {
        return(url + "&title=" + title);
    }

    size_t ConvertToSize(int iType, size_t size) {
        size_t s = size;
        switch(iType) {
            case GB: s /= 1024;
            case MB: s /= 1024;
            case KB: s /= 1024;
        }
        return s;
    }
}

