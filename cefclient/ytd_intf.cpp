#include "ytd_resource.h"

namespace YTD {

  MTDownloader gDownloader(YtdIntf::DownloaderUpdates);
          
  bool YtdIntf::GetRequest(CefRefPtr<CefDOMDocument> &document) {
      CefRefPtr<CefDOMNode> urlNode = document->GetElementById(URL);
      ASSERT(urlNode.get());

      std::string url(urlNode->GetValue().ToString());
      if(url.empty()) {
        SetMsg(document, "Invalid Text");
        return false;
      }
      else {
        SetMsg(document, "Please wait, Getting videos");
      }
      std::vector<Videos> vidList;
      if(0 != App::GetVideos(url.c_str(), vidList)) {
          SetMsg(document, "Failed to get videos");
          return false;
      }
      
      CefRefPtr<CefV8Value> args;
      CreatePopulateListMsg(vidList, args);
      SendToUI(POPULATE_LIST, args);
      SetMsg(document, "Choose a video and then click the Download button");
      return true;
  }

  void YtdIntf::DownloadRequest(CefRefPtr<CefDOMDocument> &document) {

      CefString videoUrl, videoTitle;
      GetVideo(document, videoUrl, videoTitle);
      
      if(videoUrl.empty() || videoTitle.empty()) {
          SetMsg(document, "Please select video to download");
          return;
      }
      
      CefString file;
      YTD_PAIR fileInfo;
      ExtractFileName(videoTitle, fileInfo);
	  if(YTD_PAIR_KEY(fileInfo).empty()) {
          SetMsg(document, "Unknown video extension");
          return;
      }
      
      if(GetFileSaveLocation(file, fileInfo)) {
          SetMsg(document, "Your file will be saved in " + file.ToString());
      }
      else {
          SetMsg(document, "Downlaoding did not happened");
          return;
      }
      gDownloader.Download(videoUrl, file);
      SetMsg(document, "Downloading video. Meantime you may like to download another video");
  }
 
  void YtdIntf::ClearVideo(CefRefPtr<CefDOMDocument> &document) {
      CefRefPtr<CefDOMNode> videoUrl = document->GetElementById(VIDEO_URL);
      ASSERT(videoUrl.get());
    
      CefRefPtr<CefDOMNode> videoTitle = document->GetElementById(VIDEO_TITLE);
      ASSERT(videoTitle.get());

      videoUrl->SetValue("");
      videoTitle->SetValue("");   
  }

  void YtdIntf::GetVideo(CefRefPtr<CefDOMDocument> &document, CefString &url, CefString &title) {
      CefRefPtr<CefDOMNode> videoUrl = document->GetElementById(VIDEO_URL);
      ASSERT(videoUrl.get());
    
      CefRefPtr<CefDOMNode> videoTitle = document->GetElementById(VIDEO_TITLE);
      ASSERT(videoTitle.get());

      url = videoUrl->GetValue();
      title = videoTitle->GetValue();     
  }

  
  void YtdIntf::CreatePopulateListMsg(std::vector<Videos> &data, CefRefPtr<CefV8Value> &args) {
      EnterMsgContext();
      args = CefV8Value::CreateArray(static_cast<int>(data.size()));
      for(size_t i = 0; i < data.size(); ++i) {
          CefRefPtr<CefV8Value> val = CefV8Value::CreateArray(VIDEO_ELEMENT);
          args->SetValue(i, val);
          SetList(0, data[i].GetTitle(), val);
          SetList(1, data[i].GetUrl(), val);
          SetList(2, iTagToQuality(data[i].GetiTag()), val);
          SetList(3, data[i].GetLength(), val);
      }
      ExitMsgContext();
  }

  void YtdIntf::CreateDownloadMsg(CefString param1, CefString param2, CefRefPtr<CefV8Value> &args) {
      EnterMsgContext();
      args = CefV8Value::CreateArray(DOWNLOAD_ELEMENT);
      SetList(0, param1, args);
      SetList(1, param2, args);
      ExitMsgContext();
  }

  // Downloader updates will not be called from the context of renderer
  // and so we won't be able to send the message from V8 as V8 exits in renderer context.
  // So we need to post message to renderer thread to process it, we are using 
  // CefTaskRunner for that.
  void YtdIntf::DownloaderUpdates(uint32 state, YTD_PAIR pair) {
      CefRefPtr<CefV8Value> args;
      CefRefPtr<CefTask>  msg;
      
    switch(state) {
        case MT_INIT: 
            msg = new RouteMsg(NEW_DOWNLOAD, pair, Dispatch);
            break;
        
        case MT_PROGRESS: 
            msg = new RouteMsg(UPDATE_DOWNLOAD, pair, Dispatch);
            break;
        
        case MT_DONE: 
            msg = new RouteMsg(END_DOWNLOAD, pair, Dispatch);
           // MessageBox(NULL, node.param2.c_str(), node.param2.c_str(), MB_OK); 
            break;
           
        case MT_CANCEL:
            msg = new RouteMsg(CANCEL_DOWNLOAD, pair, Dispatch);
            break;
    }
    
    CefRefPtr<CefTaskRunner> MsgRouter = CefTaskRunner::GetForThread(TID_RENDERER);
    MsgRouter->PostTask(msg);
  }

  void YtdIntf::Dispatch(CefString msgName, YTD_PAIR &pair) {
      CefRefPtr<CefV8Value> args;
      YtdIntf::CreateDownloadMsg(YTD_PAIR_KEY(pair), YTD_PAIR_VALUE(pair), args);
      SendToUI(msgName, args);
  }

  void YtdIntf::CancelRequest(int id) {
      gDownloader.CancelDownload(id);
  }
}

