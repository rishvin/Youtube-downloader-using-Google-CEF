#ifndef _YTD_INC_H_
#define _YTD_INC_H_

#include "cefclient/client_app.h"
#include "cefclient/util.h"
#include "include/youtubeDownloader/Ytd.hpp"

namespace YTD {

  #define URL         "url"
  #define ID          "id"
  #define GET         "get"
  #define MSG         "msg"
  #define FILE        "file"
  #define SET         "set"
  #define DOWNLOAD    "download"
  #define LABEL       "label"
  #define VIDEO_URL   "video_url"
  #define VIDEO_TITLE "video_title"
  #define MSG_DOMAIN  "YTD"
  #define POPULATE_LIST "populate_list"  
  #define NEW_DOWNLOAD "new_download"
  #define UPDATE_DOWNLOAD "update_download"
  #define END_DOWNLOAD "end_download"
  #define CANCEL_DOWNLOAD "cancel_download"
  #define YTD_URL     "http://tests/ytd"
  
  #define VIDEO_ELEMENT   4
  #define DOWNLOAD_ELEMENT 2

  typedef std::pair<CefString, CefString> YTD_PAIR;

  #define YTD_PAIR_KEY(PAIR)    (PAIR.first)
  #define YTD_PAIR_VALUE(PAIR)  (PAIR.second)
  

  // This will route msg to Renderer thread
  class RouteMsg: public CefTask {
    public:
      typedef void (*Dispatch)(CefString msgName, YTD_PAIR &pair);
      RouteMsg(CefString msgName, YTD_PAIR pair, Dispatch pFnDispatch)
          :msgName(msgName), pair(pair), pFnDispatch(pFnDispatch) {
      }
        
      inline void Execute() {
          pFnDispatch(msgName, pair);
      }
      
      IMPLEMENT_REFCOUNTING(RouteMsg);
    private:
      CefString msgName;
      YTD_PAIR pair;
      Dispatch pFnDispatch;
  };

  class YtdIntf {
    public:
      static bool GetRequest(CefRefPtr<CefDOMDocument> &document);
      static void DownloadRequest(CefRefPtr<CefDOMDocument> &document);
      static void ClearVideo(CefRefPtr<CefDOMDocument> &document);
      static void GetVideo(CefRefPtr<CefDOMDocument> &document, CefString &url, CefString &title);
      static void CreatePopulateListMsg(std::vector<Videos> &data, CefRefPtr<CefV8Value> &args);
      static void CreateDownloadMsg(CefString param1, CefString param2, CefRefPtr<CefV8Value> &args) ;
      static void DownloaderUpdates(uint32 state, YTD_PAIR pair);
      static void Dispatch(CefString msgName, YTD_PAIR &pair);
      static void CancelRequest(int id);
  };
        
  inline bool IsEqual(CefString str, const char *pcComp) {
      return 0 == str.compare(pcComp);
  }

  CefString ToString(uint32 value);
  YTD_PAIR FormatData(CefString str, uint32 value);
  YTD_PAIR FormatData(uint32 val1, uint32 val2);
  void SetMsg( CefRefPtr<CefDOMDocument> &document, std::string msg);
  void SetList(int index, CefString source, CefRefPtr<CefV8Value> &target);
  void SetList(int index, uint32 source, CefRefPtr<CefV8Value> &target);
  void SendToUI(CefString msgName, CefRefPtr<CefV8Value> &args);
  void EnterMsgContext();
  void ExitMsgContext();
  void ExtractFileName(CefString &innerHtml, YTD_PAIR &fileInfo);
  bool GetFileSaveLocation(CefString &file, YTD_PAIR &fileInfo);

}
#endif
