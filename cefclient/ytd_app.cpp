#include <string>
#include <sstream>
#include "include/cef_v8.h"
#include "ytd_app.h"
#include "cefclient/ytd_inc.h"

namespace YTD {

  CefRefPtr<CefV8Context> gContext;
  CefRefPtr<CefV8Value> gCbFunc;

  const char *kTestUrl = YTD_URL;
  const char *kMessageName = MSG_DOMAIN;
  
  void SendToUI(CefString msgName, CefRefPtr<CefV8Value> &args) {
      ASSERT(gCbFunc);
      EnterMsgContext();
      CefV8ValueList arguments;
      CefString name(msgName);
      arguments.push_back(CefV8Value::CreateString(name));  
      arguments.push_back(args);
      CefRefPtr<CefV8Value> retval = gCbFunc->ExecuteFunction(NULL, arguments);
      ExitMsgContext();
  }


  void EnterMsgContext() {
      ASSERT(gContext);
      gContext->Enter();
  }

  void ExitMsgContext() {
      gContext->Exit();
  }
  
  namespace {

    class ClientDOMEventListener : public CefDOMEventListener {
     public:    
      virtual void HandleEvent(CefRefPtr<CefDOMEvent> event) OVERRIDE {
          CefRefPtr<CefDOMDocument> document = event->GetDocument();
          ASSERT(document.get());
          CefRefPtr<CefDOMNode> node = event->GetTarget();
          ASSERT(node.get());
          SetMsg(document, "");
          if(node->HasElementAttribute(ID)) {
              CefString nodeValue = node->GetElementAttribute(ID);
             
              if(IsEqual(nodeValue, GET)) {
                  YtdIntf::ClearVideo(document);
                  YtdIntf::GetRequest(document);
              }
              else if(IsEqual(nodeValue, DOWNLOAD)) {
                  YtdIntf::DownloadRequest(document);
              }
        }
      }
    
    IMPLEMENT_REFCOUNTING(ClientDOMEventListener);
    
    };
    
    class ClientDOMVisitor : public CefDOMVisitor {
     public:
       virtual void Visit(CefRefPtr<CefDOMDocument> document) OVERRIDE {
    	   CefRefPtr<CefDOMNode> getButton = document->GetElementById(GET);
           ASSERT(getButton.get());
  
           CefRefPtr<CefDOMNode> downloadButton = document->GetElementById(DOWNLOAD);
           ASSERT(downloadButton.get());
           
           getButton->AddEventListener("click", new ClientDOMEventListener(), false);
           downloadButton->AddEventListener("click", new ClientDOMEventListener(), false);       
      }
    
      IMPLEMENT_REFCOUNTING(ClientDOMVisitor);
    };
    
    class RenderDelegate : public ClientApp::RenderDelegate {
     public:   
       virtual bool OnProcessMessageReceived(
           CefRefPtr<ClientApp> app,
           CefRefPtr<CefBrowser> browser,
           CefProcessId source_process,
           CefRefPtr<CefProcessMessage> message) OVERRIDE {
           CefString msgName = message->GetName();
           if (msgName == kMessageName) {
               browser->GetMainFrame()->VisitDOM(new ClientDOMVisitor);
               return true;
           }
           return false;
       }
    
     private:
      IMPLEMENT_REFCOUNTING(RenderDelegate);
    };

    class ProcessMessageDelegate : public ClientHandler::ProcessMessageDelegate {
      public: 
        virtual bool OnProcessMessageReceived(
            CefRefPtr<ClientHandler> handler,
            CefRefPtr<CefBrowser> browser,
            CefProcessId source_process,
            CefRefPtr<CefProcessMessage> message) OVERRIDE {
            CefString msgName = message->GetName();
            if(msgName == kMessageName) {
                std::stringstream st;
            st << source_process;
            CefString s(st.str());
                CefRefPtr<CefListValue> args = message->GetArgumentList();
                if (args->GetSize() > 0) {
                    int32 id = args->GetInt(0);
                    YtdIntf::CancelRequest(id);
                    return true;
                }
            }
			return false;
        }
      IMPLEMENT_REFCOUNTING(ProcessMessageDelegate);
  };
    
  }  // namespace
  
  void CreateRenderDelegates(ClientApp::RenderDelegateSet& delegates) {
      delegates.insert(new RenderDelegate);
  }

  void CreateProcessMessageDelegates(ClientHandler::ProcessMessageDelegateSet& delegates) {
      delegates.insert(new ProcessMessageDelegate);
  }
  
  void OnLoadEnd(CefRefPtr<CefBrowser> browser) {
    // Send a message to the render process to continue the test setup.
    browser->SendProcessMessage(PID_RENDERER,
        CefProcessMessage::Create(kMessageName));
  }

  void SetMessageCallback(CefRefPtr<CefV8Context> context,
                              CefRefPtr<CefV8Value> function) {   
      gContext = context;
      gCbFunc = function;
  }
  
}  // namespace YtdApp
