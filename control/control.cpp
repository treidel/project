
#include "common.h"
#include "control.h"

#include "proto/v1.pb.h"

#include <google/protobuf/message_lite.h>

#include <log4cxx/logger.h>

///////////////////////////////////////////////////////////////////////////////
// macros
///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////
// type defintions
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// constants
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// module variables
///////////////////////////////////////////////////////////////////////////////


static log4cxx::LoggerPtr g_logger(log4cxx::Logger::getLogger("control.v1"));

///////////////////////////////////////////////////////////////////////////////
// private function declarations
///////////////////////////////////////////////////////////////////////////////

static APPManager::Message *populate_response(::google::protobuf::MessageLite& message);

///////////////////////////////////////////////////////////////////////////////
// public function implementations
///////////////////////////////////////////////////////////////////////////////

Control::Control(AUDIOProcessor *processor_p, APPManager::NotificationHandler *handler_p) :
    m_processor_p(processor_p),
    m_handler_p(handler_p)
{
    LOG4CXX_DEBUG(g_logger, "Control::Control enter " << handler_p);

    GOOGLE_PROTOBUF_VERIFY_VERSION;

    // add ourselves as a handler
    m_processor_p->add_handler(this);

    LOG4CXX_DEBUG(g_logger, "Control::Control exit");
}

Control::~Control()
{
    LOG4CXX_DEBUG(g_logger, "Control::Contol enter");

    // remove ourselves as a handler
    m_processor_p->remove_handler(this);

    LOG4CXX_DEBUG(g_logger, "Control::Contol exit");
}


///////////////////////////////////////////////////////////////////////////////
// APPManager::RequestHandler implementation
///////////////////////////////////////////////////////////////////////////////

ResultCode Control::handle_request(APPManager::Message *request_p, APPManager::Message **response_pp)
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
        if (false == request.ParseFromArray(request_p->get_data_p(), request_p->get_length()))
        {
            LOG4CXX_ERROR(g_logger, "unable to parse request protocol buffer len=" << request_p->get_length());
            result_code = RESULT_CODE_ERROR;
            break;
        }
        // get the response message ready
        v1::ResponseOrNotification responseornotification;
        v1::Response *response_p = responseornotification.mutable_response();
        responseornotification.set_type(v1::ResponseOrNotification_ResponseOrNotificationType_RESPONSE);

        // assume failure
        response_p->set_success(false);

        // send back the same response type as the request
        response_p->set_type(request.type());

        // see what kind of request we got
        switch (request.type())
        {
        case v1::SETLEVEL:
        {
            // get the request
            const ::v1::SetLevelRequest& setlevel = request.setlevel();
            // validate the channel
            AUDIOChannel *channel_p = AUDIOCaptureManager::get_instance()->find_channel((AUDIOChannel::Index)setlevel.channel());
            if (NULL == channel_p)
            {
                LOG4CXX_ERROR(g_logger, "invalid channel=" << setlevel.channel() << " received from client");
                result_code = RESULT_CODE_ERROR;
                break;
            }
            // validate the requested level type
            switch (setlevel.type())
            {
            case v1::NONE:
            case v1::PEAK:
            case v1::VU:
            {
                // set the type in the audio processor
                m_processor_p->	set_level_type_for_channel((AUDIOProcessor::LevelType)setlevel.type(), channel_p);
                // populate the response
                v1::SetLevelResponse *sl_p = response_p->mutable_setlevel();
            }
            // success
            response_p->set_success(true);
            break;

            default:
                LOG4CXX_ERROR(g_logger, "unknown level type=" << setlevel.type());
                break;
            }
        }
        break;

        case v1::QUERYAUDIOCHANNELS:
        {
            // do something to query the channels
            v1::QueryAudioChannelsResponse *qac_p = response_p->mutable_queryaudiochannels();
            AUDIOCaptureManager *manager_p = AUDIOCaptureManager::get_instance();
            for (AUDIOCaptureManager::ChannelIterator it = manager_p->begin();
                    it != manager_p->end();
                    it++)
            {
                AUDIOChannel *channel_p = it->second;
                qac_p->add_channels(channel_p->get_index());
            }
            response_p->set_success(true);
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

ResultCode Control::handle_results(const size_t num_results, const AUDIOProcessor::ResultData results[])
{
    LOG4CXX_DEBUG(g_logger, "Control::handle_results enter " << results);

    // get the response message ready
    v1::ResponseOrNotification responseornotification;
    v1::Notification *notification_p = responseornotification.mutable_notification();
    responseornotification.set_type(v1::ResponseOrNotification_ResponseOrNotificationType_NOTIFICATION);

    // setup the notification
    notification_p->set_type(v1::LEVEL);

    // get the level notification 
    v1::LevelNotification *level_p =  notification_p->mutable_level();
    
    // iterate through all results
    for (int counter = 0; counter < num_results; counter++)
    {
        // create the record
        v1::LevelRecord* record_p = level_p->add_records();
        // populate the record
        record_p->set_channel(results[counter].channel);
        // fill the special stuff
        switch (results[counter].type)
        {
            case AUDIOProcessor::LEVEL_TYPE_PEAK:
                record_p->set_type(v1::PEAK);
                record_p->set_peakindb(results[counter].values.peakInDB);
                break;
            case AUDIOProcessor::LEVEL_TYPE_VU:
                record_p->set_type(v1::VU);
                record_p->set_powerindb(results[counter].values.powerInDB);
                break;
            default:
                LOG4CXX_ERROR(g_logger, "unknown level type=" + to_string(results[counter].type));
                return RESULT_CODE_ERROR;
        }
    }

    // encode the notification
    APPManager::Message *message_p = populate_response(responseornotification);

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

APPManager::Message *populate_response(::google::protobuf::MessageLite& message)
{
    LOG4CXX_DEBUG(g_logger, "Control::populate_response enter " << &message);

    // allocate the memory for the response
    APPManager::Message *response_p = new APPManager::Message(message.ByteSize());
    // serialize the response
    if (false == message.SerializeToArray((void *)(response_p->get_data_p()), response_p->get_length()))
    {
        LOG4CXX_ERROR(g_logger, "unable to serialize response to protocol buffer");
        delete response_p;
        return NULL;
    }
    // done
    LOG4CXX_DEBUG(g_logger, "Control::populate_response exit " << response_p);
    return response_p;
}

