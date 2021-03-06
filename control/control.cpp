
#include "common.h"
#include "control.h"
#include "log.h"

#include "proto/v1.pb.h"

#include <google/protobuf/message_lite.h>

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


static LogInstance g_logger("control.v1");

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
    LOG_GENERATE_TRACE(g_logger, "Control::Control enter this=%p handler_p=%p", this, handler_p);

    GOOGLE_PROTOBUF_VERIFY_VERSION;

    // add ourselves as a handler
    m_processor_p->add_handler(this);

    LOG_GENERATE_TRACE(g_logger, "Control::Control exit");
}

Control::~Control()
{
    LOG_GENERATE_TRACE(g_logger, "Control::Contol enter this=%p", this);

    // remove ourselves as a handler
    m_processor_p->remove_handler(this);

    LOG_GENERATE_TRACE(g_logger, "Control::Contol exit");
}


///////////////////////////////////////////////////////////////////////////////
// APPManager::RequestHandler implementation
///////////////////////////////////////////////////////////////////////////////

ResultCode Control::handle_request(APPManager::Message *request_p, APPManager::Message **response_pp)
{
    LOG_GENERATE_TRACE(g_logger, "Control::handler_request enter this=%p request_p=%p response_pp=%p", this, request_p, response_pp);

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
            LOG_GENERATE_ERROR(g_logger, "unable to parse request protocol buffer len=%d", request_p->get_length());
            result_code = RESULT_CODE_ERROR;
            break;
        }
        // get the response message ready
        v1::ResponseOrNotification responseornotification;
        v1::Response *response_p = responseornotification.mutable_response();
        responseornotification.set_type(v1::ResponseOrNotification_ResponseOrNotificationType_RESPONSE);

        // assume success
        response_p->set_success(true);

        // send back the same response type as the request
        response_p->set_type(request.type());

        // see what kind of request we got
        switch (request.type())
        {
            case v1::ECHO:
            {
                const ::v1::EchoRequest &echo = request.echo();
                LOG_GENERATE_DEBUG(g_logger, "processing ECHO request");
                response_p->mutable_echo();
            }
            break;

            case v1::SETLEVEL:
            {
                // get the request
                const ::v1::SetLevelRequest& setlevel = request.setlevel();

                LOG_GENERATE_INFO(g_logger, "processing SETLEVEL request, channel=%d level=%d", setlevel.channel(), setlevel.type());

                // validate the channel
                AUDIOChannel *channel_p = AUDIOCaptureManager::get_instance()->find_channel((AUDIOChannel::Index)setlevel.channel());
                if (NULL == channel_p)
                {
                    LOG_GENERATE_ERROR(g_logger, "invalid channel=%d received from client", setlevel.channel());
                    result_code = RESULT_CODE_ERROR;
                    break;
                }
                // validate the requested level type
                switch (setlevel.type())
                {
                    case v1::NONE:
                        // remove the existing meter (if one exists)
                        m_processor_p->clear_meter(channel_p);
                        break;

                    case v1::PPM:
                    {
                        // create the meter
                        AUDIOProcessor::PeakMeter *meter_p = new AUDIOProcessor::PPMMeter(channel_p);
                        // add the hold time if it's been configured
                        if (true == setlevel.has_holdtime())
                        {
                            meter_p->set_hold_time(setlevel.holdtime());
                        }
                        // add the meter
                        m_processor_p->add_meter(meter_p);
                    }
                    break;

                    case v1::DIGITALPEAK:
                    {
                        // create the meter
                        AUDIOProcessor::PeakMeter *meter_p = new AUDIOProcessor::DigitalPeakMeter(channel_p);
                        // add the hold time if it's been configured
                        if (true == setlevel.has_holdtime())
                        {
                            meter_p->set_hold_time(setlevel.holdtime());
                        }
                        // add the meter
                        m_processor_p->add_meter(meter_p);
                    }
                    break;
            
                    case v1::VU:
                    {
                        // create the meter
                        AUDIOProcessor::Meter *meter_p = new AUDIOProcessor::VUMeter(channel_p);
                        m_processor_p->add_meter(meter_p);
                    }
                    break;

                    default:
                        LOG_GENERATE_ERROR(g_logger, "invalid type=%d received from client", setlevel.type());
                        result_code = RESULT_CODE_ERROR;
                        break;
                }
                // populate the response unless we've set an error code
                if(RESULT_CODE_OK == result_code)
                {
                    v1::SetLevelResponse *sl_p = response_p->mutable_setlevel();
                }
                break;
            }
            break;

            case v1::QUERYAUDIOCHANNELS:
            {
                LOG_GENERATE_INFO(g_logger, "processing QUERYAUDIOCHANNELS request");
                // do something to query the channels
                v1::QueryAudioChannelsResponse *qac_p = response_p->mutable_queryaudiochannels();
                AUDIOCaptureManager *manager_p = AUDIOCaptureManager::get_instance();
                for(AUDIOCaptureManager::ChannelIterator it = manager_p->begin();
                    it != manager_p->end();
                    it++)
                {
                    AUDIOChannel *channel_p = it->second;
                    qac_p->add_channels(channel_p->get_index());
                }
            }
            break;

            default:
                LOG_GENERATE_ERROR(g_logger, "unknown request type=%d", request.type());
                result_code = RESULT_CODE_ERROR;
                break;
        }
        
        // set the failure flag is necessary
        if (RESULT_CODE_OK != result_code)
        {
            response_p->set_success(false);
        }

        // populate the response
        *response_pp = populate_response(responseornotification);
    }
    while (false);

    // done
    LOG_GENERATE_TRACE(g_logger, "Control::handler_request exit result_code=%d", result_code);
    return result_code;
}



///////////////////////////////////////////////////////////////////////////////
// AUDIOProcessor::Handler implementation
///////////////////////////////////////////////////////////////////////////////

ResultCode Control::handle_results(const size_t num_results, const AUDIOProcessor::ResultData results[])
{
    LOG_GENERATE_TRACE(g_logger, "Control::handle_results enter this=%p num_result=%d results=%p", this, num_results, results);

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
            case AUDIOProcessor::LEVEL_TYPE_PPM:
                record_p->set_type(v1::PPM);
                record_p->set_peakindb(results[counter].values.peak.peakInDB);
                record_p->set_holdindb(results[counter].values.peak.holdInDB);
                break;            
            case AUDIOProcessor::LEVEL_TYPE_DIGITALPEAK:
                record_p->set_type(v1::DIGITALPEAK);
                record_p->set_peakindb(results[counter].values.peak.peakInDB);
                record_p->set_holdindb(results[counter].values.peak.holdInDB);
                break;
            case AUDIOProcessor::LEVEL_TYPE_VU:
                record_p->set_type(v1::VU);
                record_p->set_vuinunits(results[counter].values.vuInUnits);
                break;
            default:
                LOG_GENERATE_ERROR(g_logger, "unknown level type=%d", results[counter].type);
                return RESULT_CODE_ERROR;
        }
    }

    // encode the notification
    APPManager::Message *message_p = populate_response(responseornotification);

    // off she goes
    ResultCode rc = m_handler_p->send_notification(&message_p);
    if (RESULT_CODE_OK != rc)
    {
        LOG_GENERATE_ERROR(g_logger, "unable to send notification rc=%d", rc);
        free(message_p);
    }
    else
    {
        ASSERT(NULL == message_p);
    }

    LOG_GENERATE_TRACE(g_logger, "Control::handle_results exit rc=%d", rc);
    return rc;
}



///////////////////////////////////////////////////////////////////////////////
// private function implementations
///////////////////////////////////////////////////////////////////////////////

APPManager::Message *populate_response(::google::protobuf::MessageLite& message)
{
    LOG_GENERATE_TRACE(g_logger, "Control::populate_response enter message=%p", &message);

    // allocate the memory for the response
    APPManager::Message *response_p = new APPManager::Message(message.ByteSize());
    // serialize the response
    if (false == message.SerializeToArray((void *)(response_p->get_data_p()), response_p->get_length()))
    {
        LOG_GENERATE_ERROR(g_logger, "unable to serialize response to protocol buffer");
        delete response_p;
        return NULL;
    }
    // done
    LOG_GENERATE_TRACE(g_logger, "Control::populate_response exit response_p=%p", response_p);
    return response_p;
}

