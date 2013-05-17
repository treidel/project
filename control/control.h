#ifndef _CONTROL_H_
#define _CONTROL_H_


///////////////////////////////////////////////////////////////////////////////
// includes
///////////////////////////////////////////////////////////////////////////////

#include "common.h"

#include "app/app_mgr.h"
#include "audio/audio_processor.h"

#include <google/protobuf/message_lite.h>

//////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////
// forward declarations
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// class definition
///////////////////////////////////////////////////////////////////////////////

class Control : public APPManager::RequestHandler, public AUDIOProcessor::Handler
{

///////////////////////////////////////////////////////////////////////////////
// type definitions
///////////////////////////////////////////////////////////////////////////////

private:


///////////////////////////////////////////////////////////////////////////////
// public function declarations
///////////////////////////////////////////////////////////////////////////////

public:
    Control(AUDIOProcessor *processor_p, APPManager::NotificationHandler *handler_p);
    virtual ~Control();

///////////////////////////////////////////////////////////////////////////////
// APPManager::RequestHandler declarations
///////////////////////////////////////////////////////////////////////////////

public:
    ResultCode handle_request(const APPManager::Message *request_p, APPManager::Message **response_pp);

///////////////////////////////////////////////////////////////////////////////
// AUDIOProcessor::Handler declarations
///////////////////////////////////////////////////////////////////////////////

public:
    ResultCode handle_results(const size_t num_results, const AUDIOProcessor::ResultData results[]);


///////////////////////////////////////////////////////////////////////////////
// private function declarations
///////////////////////////////////////////////////////////////////////////////

private:
    //APPManager::Message *populate_response(::google::protobuf::MessageLite& message);

///////////////////////////////////////////////////////////////////////////////
// private variable definitions
///////////////////////////////////////////////////////////////////////////////

private:
    AUDIOProcessor *m_processor_p;
    APPManager::NotificationHandler *m_handler_p;

};
#endif
