
#include "common.h"
#include "control.h"

#include "proto/v1.pb.h"

#include <google/protobuf/message_lite.h>
#include <math.h>

#include <log4cxx/logger.h>

///////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////

#define EXPECTED_MESSAGE_VERSION (1)

///////////////////////////////////////////////////////////////////////////////
// type defintions
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// constants
///////////////////////////////////////////////////////////////////////////////

static const int32_t c_zero_level_in_db = -98;

///////////////////////////////////////////////////////////////////////////////
// module variables
///////////////////////////////////////////////////////////////////////////////


static log4cxx::LoggerPtr g_logger(log4cxx::Logger::getLogger("control.v1"));

///////////////////////////////////////////////////////////////////////////////
// private function declarations
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// public function implementations
///////////////////////////////////////////////////////////////////////////////

Control::Control(APPManager::NotificationHandler *handler_p) :
	m_handler_p(handler_p)
{
	LOG4CXX_DEBUG(g_logger, "Control::Control enter " << handler_p);

	GOOGLE_PROTOBUF_VERIFY_VERSION;

	LOG4CXX_DEBUG(g_logger, "Control::Control exit");
}

Control::~Control()
{
	LOG4CXX_DEBUG(g_logger, "Control::Contol enter");
	LOG4CXX_DEBUG(g_logger, "Control::Contol exit");
}


///////////////////////////////////////////////////////////////////////////////
// APPManager::RequestHandler implementation
///////////////////////////////////////////////////////////////////////////////

ResultCode Control::handle_request(const APPManager::Message *request_p, APPManager::Message **response_pp)
{
	LOG4CXX_DEBUG(g_logger, "Control::handler_request enter " << request_p << " " << response_pp);

	// assume success
	ResultCode result_code = RESULT_CODE_OK;
    
	// assume we don't have a response unless we explicitly send one below
	*response_pp = NULL;

	do 
	{
		// decode the message
        	v1::Request request;
        	if (false == request.ParseFromArray((void *)request_p->data_p, request_p->length))
        	{
			LOG4CXX_ERROR(g_logger, "unable to parse request protocol buffer len=" << request_p->length);
            		result_code = RESULT_CODE_ERROR;
			break;
        	}
		// get the response message ready
        	v1::ResponseOrNotification responseornotification;
		v1::Response *response_p = responseornotification.mutable_response();

		// assume failure 
		response_p->set_success(false);

		// see what kind of request we got
		switch (request.type())
		{
			case v1::Request_RequestType_SETLEVEL:
			{
			  	// do something to set the level type	
			}
			break;
			
			case v1::Request_RequestType_QUERYAUDIOCHANNELS:
			{
				// do something to query the channels
				v1::QueryAudioChannelsResponse *qac_p = response_p->mutable_queryaudiochannels();
				qac_p->add_channels(1);
				qac_p->add_channels(2);
			}	
			break;
			
			default:
				LOG4CXX_ERROR(g_logger, "unknown request type=" << request.type());
				break;

		}
		

		// populate the response
		*response_pp = populate_response(responseornotification);        		
	} 
	while (false);
    
	// done
	LOG4CXX_DEBUG(g_logger, "Control::handler_request exit " << result_code);
    	return result_code;
}



///////////////////////////////////////////////////////////////////////////////
// AUDIOProcessor::Handler implementation
///////////////////////////////////////////////////////////////////////////////

ResultCode Control::handle_results(const size_t num_results, const AUDIOProcessor::Data results[])
{
	LOG4CXX_DEBUG(g_logger, "Control::handle_results enter " << results);

	// setup the notification
	v1::Notification notification;
	
	// only support level for now
	v1::LevelNotification *level_p =  notification.mutable_level();
	
	// iterate through all results
	for (int counter = 0; counter < num_results; counter++)
	{
		// get the peak sample's level
		AUDIOChannel::Sample peak = results[counter].peak;
		// assume the level is zero for now
		int32_t levelInDB = c_zero_level_in_db;
		// if this is zero then the level is dB is negative infinity
		if (AUDIO_CHANNEL_ZERO_LEVEL != peak)
		{
			// do the dBm calcuation
			levelInDB = (int32_t)(20.0 * log10(peak));
		}
		// add the level to the notification
		level_p->add_levelindecibels(levelInDB);
	}
	
	// encode the notification
	APPManager::Message *message_p = populate_response(notification);

	// off she goes
	ResultCode rc = m_handler_p->send_notification(&message_p);
	if (RESULT_CODE_OK != rc)
	{
		LOG4CXX_ERROR(g_logger, "unable to send notification rc=" << rc);
		free(message_p);
	}
	else
	{
		ASSERT(NULL == message_p);
	}

	LOG4CXX_DEBUG(g_logger, "Control::handle_results exit " << rc);

	return rc;
}



///////////////////////////////////////////////////////////////////////////////
// private function implementations
///////////////////////////////////////////////////////////////////////////////

APPManager::Message *Control::populate_response(::google::protobuf::MessageLite& message)
{
	LOG4CXX_DEBUG(g_logger, "Control::populate_response enter " << &message);

	// allocate the memory for the response
       	APPManager::Message *response_p = (APPManager::Message *)calloc(1, sizeof(APPManager::Message));
        // calculate how large the response will be 
        response_p->length = (uint16_t)message.ByteSize();
        // allocate a buffer for the response
        response_p->data_p = (uint8_t *)malloc(response_p->length);
        // serialize the response
        if (false == message.SerializeToArray((void *)(response_p->data_p), response_p->length))
        {
		LOG4CXX_ERROR(g_logger, "unable to serialize response to protocol buffer");
            	free(response_p->data_p);
            	free(response_p);
		return NULL;
        }
	// done
	LOG4CXX_DEBUG(g_logger, "Control::populate_response exit " << response_p);
	return response_p;
}
