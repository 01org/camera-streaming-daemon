/*
 * This file is part of the Camera Streaming Daemon
 *
 * Copyright (C) 2017  Intel Corporation. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <assert.h>
#include <mavlink.h>
#include <sys/socket.h>
#include <unistd.h>

#include "log.h"
#include "mainloop.h"
#include "mavlink_server.h"
#include "util.h"

#define DEFAULT_MAVLINK_PORT 14550
#define DEFAULT_SYSID 1
#define DEFAULT_MAVLINK_BROADCAST_ADDR "255.255.255.255"
#define DEFAULT_RTSP_SERVER_ADDR "0.0.0.0"
#define MAX_MAVLINK_MESSAGE_SIZE 1024

MavlinkServer::MavlinkServer(ConfFile &conf, std::vector<std::unique_ptr<Stream>> &streams,
                             RTSPServer &rtsp)
    : _streams(streams)
    , _is_running(false)
    , _timeout_handler(0)
    , _broadcast_addr{}
    , _system_id(DEFAULT_SYSID)
    , _comp_id(MAV_COMP_ID_CAMERA)
    , _rtsp_server_addr(nullptr)
    , _rtsp(rtsp)
{
    struct options {
        unsigned long int port;
        int sysid;
        int compid;
        char *rtsp_server_addr;
        char broadcast[17];
    } opt = {};
    static const ConfFile::OptionsTable option_table[] = {
        {"port", false, ConfFile::parse_ul, OPTIONS_TABLE_STRUCT_FIELD(options, port)},
        {"system_id", false, ConfFile::parse_i, OPTIONS_TABLE_STRUCT_FIELD(options, sysid)},
        {"component_id", false, ConfFile::parse_i, OPTIONS_TABLE_STRUCT_FIELD(options, compid)},
        {"rtsp_server_addr", false, ConfFile::parse_str_dup, OPTIONS_TABLE_STRUCT_FIELD(options, rtsp_server_addr)},
        {"broadcast_addr", false, ConfFile::parse_str_buf, OPTIONS_TABLE_STRUCT_FIELD(options, broadcast)},
    };
    conf.extract_options("mavlink", option_table, ARRAY_SIZE(option_table), (void *)&opt);

    if (opt.port)
        _broadcast_addr.sin_port = htons(opt.port);
    else
        _broadcast_addr.sin_port = htons(DEFAULT_MAVLINK_PORT);

    if (opt.sysid) {
        if (opt.sysid <= 1 || opt.sysid >= 255)
            log_error("Invalid System ID for MAVLink communication (%d). Using default (%d)",
                      opt.sysid, DEFAULT_SYSID);
        else
            _system_id = opt.sysid;
    }

    if (opt.compid) {
        if (opt.compid <= 1 || opt.compid >= 255)
            log_error("Invalid Component ID for MAVLink communication (%d). Using default "
                      "MAV_COMP_ID_CAMERA (%d)", opt.compid, MAV_COMP_ID_CAMERA);
        else
            _comp_id = opt.compid;
    }

    if (opt.broadcast[0])
        _broadcast_addr.sin_addr.s_addr = inet_addr(opt.broadcast);
    else
        _broadcast_addr.sin_addr.s_addr = inet_addr(DEFAULT_MAVLINK_BROADCAST_ADDR);
    _broadcast_addr.sin_family = AF_INET;
    _rtsp_server_addr = opt.rtsp_server_addr;
}

MavlinkServer::~MavlinkServer()
{
    stop();
    free(_rtsp_server_addr);
}

void MavlinkServer::_send_ack(const struct sockaddr_in &addr, int cmd, int comp_id, bool success)
{
    mavlink_message_t msg;

    mavlink_msg_command_ack_pack(_system_id, comp_id, &msg, cmd,
                                 success ? MAV_RESULT_ACCEPTED : MAV_RESULT_FAILED, 255);

    if (!_send_mavlink_message(&addr, msg)) {
        log_error("Sending ack failed.");
        return;
    }
}

void MavlinkServer::_handle_request_camera_information(const struct sockaddr_in &addr,
                                                       mavlink_command_long_t &cmd)
{
    log_debug("%s", __func__);

    mavlink_message_t msg;
    bool success = false;

    if (cmd.param1 != 1) {
        _send_ack(addr, cmd.command, cmd.target_component, true);
        return;
    }

    CameraComponent *tgtComp = getCameraComponent(cmd.target_component);
    if (tgtComp) {
        const CameraInfo &camInfo = tgtComp->getCameraInfo();
        mavlink_msg_camera_information_pack(
            _system_id, cmd.target_component, &msg, 0, (const uint8_t *)camInfo.vendorName,
            (const uint8_t *)camInfo.modelName, camInfo.firmware_version, camInfo.focal_length,
            camInfo.sensor_size_h, camInfo.sensor_size_v, camInfo.resolution_h,
            camInfo.resolution_v, camInfo.lens_id, camInfo.flags, camInfo.cam_definition_version,
            (const char *)camInfo.cam_definition_uri);

        if (!_send_mavlink_message(&addr, msg)) {
            log_error("Sending camera information failed for camera %d.", cmd.target_component);
            return;
        }

        success = true;
    }

    _send_ack(addr, cmd.command, cmd.target_component, success);
}

void MavlinkServer::_handle_request_camera_settings(const struct sockaddr_in &addr,
                                                    mavlink_command_long_t &cmd)
{
    log_debug("%s", __func__);

    if (cmd.param1 != 1) {
        _send_ack(addr, cmd.command, cmd.target_component, true);
        return;
    }

    mavlink_message_t msg;
    bool success = false;

    CameraComponent *tgtComp = getCameraComponent(cmd.target_component);
    if (tgtComp) {
        // TODO:: Fill with appropriate mode value
        mavlink_msg_camera_settings_pack(_system_id, cmd.target_component, &msg, 0,
                                         1 /*video mode*/);

        if (!_send_mavlink_message(&addr, msg)) {
            log_error("Sending camera setting failed for camera %d.", cmd.target_component);
            return;
        }

        success = true;
    }

    _send_ack(addr, cmd.command, cmd.target_component, success);
}

void MavlinkServer::_handle_request_storage_information(const struct sockaddr_in &addr,
                                                        mavlink_command_long_t &cmd)
{
    log_debug("%s", __func__);

    if (cmd.param1 != 1) {
        _send_ack(addr, cmd.command, cmd.target_component, true);
        return;
    }

    mavlink_message_t msg;
    bool success = false;

    CameraComponent *tgtComp = getCameraComponent(cmd.target_component);
    if (tgtComp) {
        // TODO:: Fill with appropriate value
        mavlink_msg_storage_information_pack(
            _system_id, cmd.target_component, &msg, 0, 1 /*storage_id*/, 1 /*storage_count*/,
            2 /*status- formatted*/, 50.0 /*total_capacity*/, 0.0 /*used_capacity*/,
            50.0 /*available_capacity*/, 128 /*read_speed*/, 128 /*write_speed*/);

        if (!_send_mavlink_message(&addr, msg)) {
            log_error("Sending storage information failed for camera %d.", cmd.target_component);
            return;
        }

        success = true;
    }

    _send_ack(addr, cmd.command, cmd.target_component, success);
}

void MavlinkServer::_handle_camera_video_stream_request(const struct sockaddr_in &addr, int command,
                                                        unsigned int camera_id, unsigned int action)
{
    log_debug("%s", __func__);

    mavlink_message_t msg;
    char query[35] = "";

    if (action != 1)
        return;

    for (auto const &s : _streams) {
        if (camera_id == 0 || camera_id == s->id) {
            const Stream::FrameSize *fs = s->sel_frame_size
                ? s->sel_frame_size
                : _find_best_frame_size(*s, UINT32_MAX, UINT32_MAX);

            if (s->sel_frame_size) {
                int ret = snprintf(query, sizeof(query), "?width=%d&height=%d",
                                   s->sel_frame_size->width, s->sel_frame_size->height);
                if (ret > (int)sizeof(query)) {
                    log_error("Invalid requested resolution. Aborting request.");
                    return;
                }
            }

            mavlink_msg_video_stream_information_pack(
                _system_id, _comp_id, &msg, s->id, s->is_streaming /* Status */,
                0 /* FPS */, fs->width, fs->height, 0 /* bitrate */, 0 /* Rotation */,
                _rtsp.get_rtsp_uri(_rtsp_server_addr, *s, query).c_str());
            if (!_send_mavlink_message(&addr, msg)) {
                log_error("Sending camera information failed for camera %d.", s->id);
                return;
            }
        }
    }
}

const Stream::FrameSize *MavlinkServer::_find_best_frame_size(Stream &s, uint32_t w, uint32_t h)
{
    // Using strategy of getting the higher frame size that is lower than WxH, if the
    // exact resolution is not found
    const Stream::FrameSize *best = nullptr;
    for (auto const &f : s.formats) {
        for (auto const &fs : f.frame_sizes) {
            if (fs.width == w && fs.height == h)
                return &fs;
            else if (!best || (fs.width <= w && fs.width >= best->width && fs.height <= h
                               && fs.height >= best->height))
                best = &fs;
        }
    }
    return best;
}

void MavlinkServer::_handle_camera_set_video_stream_settings(const struct sockaddr_in &addr,
                                                             mavlink_message_t *msg)
{
    log_debug("%s", __func__);

    mavlink_set_video_stream_settings_t settings;
    Stream *stream = nullptr;

    mavlink_msg_set_video_stream_settings_decode(msg, &settings);
    for (auto const &s : _streams) {
        if (s->id == settings.camera_id) {
            stream = &*s;
        }
    }
    if (!stream) {
        log_debug("SET_VIDEO_STREAM request in an invalid camera (camera_id = %d)",
                  settings.camera_id);
        return;
    }

    if (settings.resolution_h == 0 || settings.resolution_v == 0)
        stream->sel_frame_size = nullptr;
    else
        stream->sel_frame_size
            = _find_best_frame_size(*stream, settings.resolution_h, settings.resolution_v);
}

void MavlinkServer::_handle_param_ext_request_read(const struct sockaddr_in &addr,
                                                   mavlink_message_t *msg)
{
    log_debug("%s", __func__);

    mavlink_message_t msg2;
    mavlink_param_ext_request_read_t param_ext_read;
    mavlink_param_ext_value_t param_ext_value;
    bool ret = false;
    mavlink_msg_param_ext_request_read_decode(msg, &param_ext_read);
    CameraComponent *tgtComp = getCameraComponent(param_ext_read.target_component);
    if (tgtComp) {
        // Read parameter value from camera component
        ret = tgtComp->getParam(param_ext_read.param_id, param_ext_value.param_value);
        if (ret) {
            // Send the param value to GCS
            param_ext_value.param_count = 1;
            param_ext_value.param_index = 0;
            strncpy(param_ext_value.param_id, param_ext_read.param_id, 16);
            param_ext_value.param_type = tgtComp->getParamType(param_ext_value.param_id);
            mavlink_msg_param_ext_value_encode(_system_id, param_ext_read.target_component, &msg2,
                                               &param_ext_value);
        } else {
            // Send param ack error to GCS
            mavlink_param_ext_ack_t param_ext_ack;
            strncpy(param_ext_ack.param_id, param_ext_read.param_id, 16);
            // strncpy(param_ext_ack.param_value, , 128);
            param_ext_ack.param_type = tgtComp->getParamType(param_ext_value.param_id);
            ;
            param_ext_ack.param_result = PARAM_ACK_FAILED;
            mavlink_msg_param_ext_ack_encode(_system_id, param_ext_read.target_component, &msg2,
                                             &param_ext_ack);
        }
        if (!_send_mavlink_message(&addr, msg2)) {
            log_error("Sending response to param request read failed %d.",
                      param_ext_read.target_component);
            return;
        }
    }
}
void MavlinkServer::_handle_param_ext_request_list(const struct sockaddr_in &addr,
                                                   mavlink_message_t *msg)
{
    log_debug("%s", __func__);

    int idx = 0;
    mavlink_message_t msg2;
    mavlink_param_ext_request_list_t param_list;
    mavlink_param_ext_value_t param_ext_value;
    mavlink_msg_param_ext_request_list_decode(msg, &param_list);
    CameraComponent *tgtComp = getCameraComponent(param_list.target_component);
    if (tgtComp) {
        // Get the list of parameter from camera component
        const std::map<std::string, std::string> &paramIdtoValue = tgtComp->getParamList();
        param_ext_value.param_count = paramIdtoValue.size();

        // Send each param,value to GCS
        for (auto &x : paramIdtoValue) {
            param_ext_value.param_index = idx++;
            strncpy(param_ext_value.param_id, x.first.c_str(), 16);
            strncpy(param_ext_value.param_value, x.second.c_str(), 128);
            param_ext_value.param_type = tgtComp->getParamType(param_ext_value.param_id);
            mavlink_msg_param_ext_value_encode(_system_id, param_list.target_component, &msg2,
                                               &param_ext_value);
            if (!_send_mavlink_message(&addr, msg2)) {
                log_error("Sending response to param request list failed %d.", idx);
            }
        }
    }
}

void MavlinkServer::_handle_param_ext_set(const struct sockaddr_in &addr, mavlink_message_t *msg)
{
    log_debug("%s", __func__);

    bool ret = false;
    mavlink_message_t msg2;
    mavlink_param_ext_set_t param_set;
    mavlink_param_ext_ack_t param_ext_ack;
    mavlink_msg_param_ext_set_decode(msg, &param_set);
    CameraComponent *tgtComp = getCameraComponent(param_set.target_component);
    if (tgtComp) {
        // Set parameter
        ret = tgtComp->setParam(param_set.param_id, param_set.param_value, param_set.param_type);
        strncpy(param_ext_ack.param_id, param_set.param_id, 16);
        param_ext_ack.param_type = param_set.param_type;
        if (ret) {
            // Send response to GCS
            strncpy(param_ext_ack.param_value, param_set.param_value, 128);
            param_ext_ack.param_result = PARAM_ACK_ACCEPTED;
        } else {
            // Send error alongwith current value of the param to GCS
            tgtComp->getParam(param_ext_ack.param_id, param_ext_ack.param_value);
            param_ext_ack.param_result = PARAM_ACK_FAILED;
        }

        mavlink_msg_param_ext_ack_encode(_system_id, param_set.target_component, &msg2,
                                         &param_ext_ack);
        if (!_send_mavlink_message(&addr, msg2)) {
            log_error("Sending response to param set failed %d.", param_set.target_component);
            return;
        }
    }
}

void MavlinkServer::_handle_mavlink_message(const struct sockaddr_in &addr, mavlink_message_t *msg)
{
    // log_debug("Message received: (sysid: %d compid: %d msgid: %d)", msg->sysid, msg->compid,
    //          msg->msgid);

    if (msg->msgid == MAVLINK_MSG_ID_COMMAND_LONG) {
        mavlink_command_long_t cmd;
        mavlink_msg_command_long_decode(msg, &cmd);
        log_debug("Command received: (sysid: %d compid: %d msgid: %d)", cmd.target_system,
                  cmd.target_component, cmd.command);

        // Get the target component
        // CameraComponent *tgt_comp = getCameraComponent(cmd.target_component);

        if (cmd.target_system != _system_id || cmd.target_component < MAV_COMP_ID_CAMERA
            || cmd.target_component > MAV_COMP_ID_CAMERA6)
            return;
        switch (cmd.command) {
        case MAV_CMD_REQUEST_CAMERA_INFORMATION:
            this->_handle_request_camera_information(addr, cmd);
            break;
        case MAV_CMD_REQUEST_VIDEO_STREAM_INFORMATION:
            this->_handle_camera_video_stream_request(addr, cmd.command, cmd.param1 /* Camera ID */,
                                                      cmd.param2 /* Action */);
            break;
        case MAV_CMD_REQUEST_CAMERA_SETTINGS:
            this->_handle_request_camera_settings(addr, cmd);
            break;
        case MAV_CMD_REQUEST_CAMERA_CAPTURE_STATUS:
            log_debug("MAV_CMD_REQUEST_CAMERA_CAPTURE_STATUS");
            break;
        case MAV_CMD_RESET_CAMERA_SETTINGS:
            log_debug("MAV_CMD_RESET_CAMERA_SETTINGS");
            break;
        case MAV_CMD_REQUEST_STORAGE_INFORMATION:
            this->_handle_request_storage_information(addr, cmd);
            break;
        case MAV_CMD_STORAGE_FORMAT:
            log_debug("MAV_CMD_STORAGE_FORMAT");
            break;
        case MAV_CMD_SET_CAMERA_MODE:
        case MAV_CMD_IMAGE_START_CAPTURE:
        case MAV_CMD_IMAGE_STOP_CAPTURE:
        case MAV_CMD_REQUEST_CAMERA_IMAGE_CAPTURE:
        case MAV_CMD_DO_TRIGGER_CONTROL:
        case MAV_CMD_VIDEO_START_CAPTURE:
        case MAV_CMD_VIDEO_STOP_CAPTURE:
        case MAV_CMD_VIDEO_START_STREAMING:
        case MAV_CMD_VIDEO_STOP_STREAMING:
        default:
            log_debug("Command %d unhandled. Discarding.", cmd.command);
            break;
        }
    } else {
        switch (msg->msgid) {
        case MAVLINK_MSG_ID_SET_VIDEO_STREAM_SETTINGS:
            this->_handle_camera_set_video_stream_settings(addr, msg);
            break;
        case MAVLINK_MSG_ID_PARAM_EXT_REQUEST_READ:
            this->_handle_param_ext_request_read(addr, msg);
            break;
        case MAVLINK_MSG_ID_PARAM_EXT_REQUEST_LIST:
            this->_handle_param_ext_request_list(addr, msg);
            break;
        case MAVLINK_MSG_ID_PARAM_EXT_SET:
            this->_handle_param_ext_set(addr, msg);
            break;
        default:
            // log_debug("Message %d unhandled, Discarding", msg->msgid);
            break;
        }
    }
}

void MavlinkServer::_message_received(const struct sockaddr_in &sockaddr, const struct buffer &buf)
{
    mavlink_message_t msg;
    mavlink_status_t status;

    for (unsigned int i = 0; i < buf.len; ++i) {
        //TOOD: Parse mavlink message all at once, instead of using mavlink_parse_char
        if (mavlink_parse_char(MAVLINK_COMM_0, buf.data[i], &msg, &status))
            _handle_mavlink_message(sockaddr, &msg);
    }
}

bool MavlinkServer::_send_mavlink_message(const struct sockaddr_in *addr, mavlink_message_t &msg)
{
    uint8_t buffer[MAX_MAVLINK_MESSAGE_SIZE];
    struct buffer buf = {0, buffer};

    buf.len = mavlink_msg_to_send_buffer(buf.data, &msg);

    if (addr)
        return buf.len > 0 && _udp.write(buf, *addr) > 0;
    return buf.len > 0 && _udp.write(buf, _broadcast_addr) > 0;
}

bool _heartbeat_cb(void *data)
{
    // log_debug("%s", __func__);
    assert(data);
    MavlinkServer *server = (MavlinkServer *)data;
    mavlink_message_t msg;

    for (std::map<int, CameraComponent *>::iterator it = server->compIdToObj.begin();
         it != server->compIdToObj.end(); it++) {
        // log_debug("Sending heartbeat for component :%d",it->first);
        mavlink_msg_heartbeat_pack(server->_system_id, it->first, &msg, MAV_TYPE_GENERIC,
                                   MAV_AUTOPILOT_INVALID, MAV_MODE_PREFLIGHT, 0, MAV_STATE_ACTIVE);
        if (!server->_send_mavlink_message(nullptr, msg))
            log_error("Sending HEARTBEAT failed.");
    }
    return true;
}

void MavlinkServer::start()
{
    log_error("MAVLINK START\n");
    if (_is_running)
        return;
    _is_running = true;

    _udp.open(true);
    _udp.set_read_callback([this](const struct buffer &buf, const struct sockaddr_in &sockaddr) {
        this->_message_received(sockaddr, buf);
    });
    _timeout_handler = Mainloop::get_mainloop()->add_timeout(1000, _heartbeat_cb, this);
}

void MavlinkServer::stop()
{
    if (!_is_running)
        return;
    _is_running = false;

    if (_timeout_handler > 0)
        Mainloop::get_mainloop()->del_timeout(_timeout_handler);
}

int MavlinkServer::addCameraComponent(CameraComponent *camComp)
{
    log_debug("%s", __func__);
    int id = MAV_COMP_ID_CAMERA;
    while (id < MAV_COMP_ID_CAMERA6 + 1) {
        if (compIdToObj.find(id) == compIdToObj.end()) {
            compIdToObj.insert(std::make_pair(id, camComp));
            break;
        }
        id++;
    }

    return id;
}

void MavlinkServer::removeCameraComponent(CameraComponent *camComp)
{
    log_debug("%s", __func__);

    if (!camComp)
        return;

    for (std::map<int, CameraComponent *>::iterator it = compIdToObj.begin();
         it != compIdToObj.end(); it++) {
        if ((it->second) == camComp) {
            compIdToObj.erase(it);
            break;
        }
    }

    return;
}

CameraComponent *MavlinkServer::getCameraComponent(int compID)
{
    if (compID < MAV_COMP_ID_CAMERA || compID > MAV_COMP_ID_CAMERA6)
        return NULL;

    std::map<int, CameraComponent *>::iterator it = compIdToObj.find(compID);
    if (it != compIdToObj.end())
        return it->second;
    else
        return NULL;
}
