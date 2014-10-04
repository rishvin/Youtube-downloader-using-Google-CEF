#ifndef _YTD_APP_H_
#define _YTD_APP_H_
#pragma once

#include "include/cef_base.h"
#include "cefclient/client_app.h"
#include "cefclient/client_handler.h"

namespace YTD {

extern const char *kTestUrl;
extern const char *kMessageName;

void CreateRenderDelegates(ClientApp::RenderDelegateSet& delegates);
void CreateProcessMessageDelegates(ClientHandler::ProcessMessageDelegateSet& delegates); 
void OnLoadEnd(CefRefPtr<CefBrowser> browser);
void SetMessageCallback(CefRefPtr<CefV8Context> context,
                        CefRefPtr<CefV8Value> function);
}

#endif 
