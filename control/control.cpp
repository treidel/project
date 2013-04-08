
#include "common.h"
#include "control.h"

#include "control.pb.h"

#include <google/protobuf/message_lite.h>
#include <math.h>

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


///////////////////////////////////////////////////////////////////////////////
// private function declarations
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// public function implementations
///////////////////////////////////////////////////////////////////////////////

Control::Control(APPManager::NotificationHandler *handler_p) :
	m_handler_p(handler_p),
	m_state(STATE_HELLO)
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;
}

Control::~Control()
{
}


///////////////////////////////////////////////////////////////////////////////
// APPManager::RequestHandler implementation
///////////////////////////////////////////////////////////////////////////////

ResultCode Control::handle_request(const APPManager::Message *request_p, APPManager::Message **response_pp)
{
	// assume success
	ResultCode result_code = RESULT_CODE_OK;
    
	// assume we don't have a response unless we explicitly send one below
	*response_pp = NULL;

	switch (m_state)
	{
		case STATE_HELLO:
		{
            		// decode the hello
			project::HelloRequest request;
			if (false == request.ParseFromArray((void *)request_p->data_p, request_p->length))
			{
				// TBD: error log
				result_code = RESULT_CODE_ERROR;
				break;			
			}

			// setup the response
			project::HelloResponse response;

	    		// if the version is wrong send back a nasty gram
            		if (EXPECTED_MESSAGE_VERSION != request.version())
            		{
                		response.set_success(false);
				*response_pp = populate_response(response);
                		result_code = RESULT_CODE_ERROR;
				break;
            		}
			
			// success
			response.set_success(true);
			*response_pp = populate_response(response);

			// now connected
			m_state = STATE_CONNECTED;
		}
		break;

		case STATE_CONNECTED:
		{
			// decode the message
        		project::RequestV1 request;
        		if (false == request.ParseFromArray((void *)request_p->data_p, request_p->length))
        		{
            			// TBD: error log
            			result_code = RESULT_CODE_ERROR;
            			break;
        		}
			// get the response message ready
        		project::ResponseV1 response;

			// do some processing

			// populate the response
			*response_pp = populate_response(response);        		
		}
		break;

		default:
			// unknown state
			// TBD: error log
			return RESULT_CODE_ERROR;
        }
    
	// done
    	return result_code;
}



///////////////////////////////////////////////////////////////////////////////
// AUDIOProcessor::Handler implementation
///////////////////////////////////////////////////////////////////////////////

ResultCode Control::handle_results(const AUDIOProcessor::Data results[])
{
	// setup the notification
	project::NotificationV1 notification;
	
	// only support level for now
	project::LevelNotificationV1 *level_p =  notification.mutable_level();
	
	// iterate through all results
	for (int counter = 0; counter < sizeof results; counter++)
	{
		// get the peak sample's level
		AUDIOChannel::Sample peak = results[counter].peak;
		// assume the level is zero for now
		int32_t levelInDB = c_zero_level_in_db;
		// if this is zero then the level is dB is negative infinity
		if (AUDIO_CHANNEL_ZERO_LEVEL != peak)
		{
			// convert to normalized form
			AUDIOChannel::NormalizedSample normalized = AUDIOChannel::convertToNormalized(peak);
			// do the dBm calcuation
			levelInDB = (int32_t)(20.0 * log10(normalized));
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
		// TBD: error log
		free(message_p);

	}
	else
	{
		ASSERT(NULL == message_p);
	}
	return rc;
}



///////////////////////////////////////////////////////////////////////////////
// private function implementations
///////////////////////////////////////////////////////////////////////////////

APPManager::Message *Control::populate_response(::google::protobuf::MessageLite& message)
{
	// allocate the memory for the response
       	APPManager::Message *response_p = (APPManager::Message *)calloc(1, sizeof(APPManager::Message));
        // calculate how large the response will be 
        response_p->length = (uint16_t)message.ByteSize();
        // allocate a buffer for the response
        response_p->data_p = (uint8_t *)malloc(response_p->length);
        // serialize the response
        if (false == message.SerializeToArray((void *)(response_p->data_p), response_p->length))
        {
            	// TBD: error log
            	free(response_p->data_p);
            	free(response_p);
		return NULL;
        }
	// done
	return response_p;
}
