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
#include <cstring>
#include <sstream>
#include <string.h>

#include "CameraParameters.h"
#include "log.h"

const char CameraParameters::CAMERA_MODE[] = "camera-mode";
const char CameraParameters::BRIGHTNESS[] = "brightness";
const char CameraParameters::CONTRAST[] = "contrast";
const char CameraParameters::SATURATION[] = "saturation";
const char CameraParameters::HUE[] = "hue";
const char CameraParameters::WHITE_BALANCE_MODE[] = "wb-mode";
const char CameraParameters::GAMMA[] = "gamma";
const char CameraParameters::GAIN[] = "gain";
const char CameraParameters::POWER_LINE_FREQ_MODE[] = "power-mode";
const char CameraParameters::WHITE_BALANCE_TEMPERATURE[] = "wb-temp";
const char CameraParameters::SHARPNESS[] = "sharpness";
const char CameraParameters::BACKLIGHT_COMPENSATION[] = "backlight";
const char CameraParameters::EXPOSURE_MODE[] = "exp-mode";
const char CameraParameters::EXPOSURE_ABSOLUTE[] = "exp-absolute";
const char CameraParameters::IMAGE_SIZE[] = "image-size";
const char CameraParameters::IMAGE_FORMAT[] = "image-format";
const char CameraParameters::PIXEL_FORMAT[] = "pixel-format";
const char CameraParameters::SCENE_MODE[] = "scene-mode";
const char CameraParameters::VIDEO_SIZE[] = "video-size";
const char CameraParameters::VIDEO_FRAME_FORMAT[] = "video-format";
const char CameraParameters::VIDEO_SNAPSHOT_SUPPORTED[] = "video-snapshot";

/* ++Need to check if required, as we have defined values as enums++ */
const char CameraParameters::CAMERA_MODE_STILL[] = "still";
const char CameraParameters::CAMERA_MODE_VIDEO[] = "video";
const char CameraParameters::CAMERA_MODE_PREVIEW[] = "preview";

const char CameraParameters::IMAGE_FORMAT_JPEG[] = "jpg";
const char CameraParameters::IMAGE_FORMAT_PNG[] = "png";

const char CameraParameters::PIXEL_FORMAT_YUV422SP[] = "yuv422sp";
const char CameraParameters::PIXEL_FORMAT_YUV420SP[] = "yuv420sp";
const char CameraParameters::PIXEL_FORMAT_YUV422I[] = "yuv422i";
const char CameraParameters::PIXEL_FORMAT_YUV420P[] = "yuv420p";
const char CameraParameters::PIXEL_FORMAT_RGB565[] = "rgb565";
const char CameraParameters::PIXEL_FORMAT_RGBA8888[] = "rgba8888";

// Values for white balance settings.
const char CameraParameters::WHITE_BALANCE_AUTO[] = "auto";
const char CameraParameters::WHITE_BALANCE_INCANDESCENT[] = "incandescent";
const char CameraParameters::WHITE_BALANCE_FLUORESCENT[] = "fluorescent";
const char CameraParameters::WHITE_BALANCE_WARM_FLUORESCENT[] = "warm-fluorescent";
const char CameraParameters::WHITE_BALANCE_DAYLIGHT[] = "daylight";
const char CameraParameters::WHITE_BALANCE_CLOUDY_DAYLIGHT[] = "cloudy-daylight";
const char CameraParameters::WHITE_BALANCE_TWILIGHT[] = "twilight";
const char CameraParameters::WHITE_BALANCE_SHADE[] = "shade";
/* --Need to check if required-- */

CameraParameters::CameraParameters()
{
    initParamIdType();
}

CameraParameters::~CameraParameters()
{
}

int CameraParameters::setParameterSupported(std::string key, std::string value)
{
    set(paramValuesSupported, key, value);
    return 1;
}

int CameraParameters::setParameter(std::string key, std::string value)
{
    set(paramValue, key, value);
    return 1;
}

std::string CameraParameters::getParameter(std::string key)
{
    return get(paramValue, key);
}

int CameraParameters::getParameterID(std::string param)
{
    // TODO :: Create a map of paramString to paramID
    int ret = -1;
    std::map<std::string, std::pair<int, int>>::iterator it = paramIdType.find(param);
    if (it != paramIdType.end()) {
        ret = it->second.first;
    } else
        ret = -1;
    return ret;
}

int CameraParameters::getParameterType(std::string param)
{
    // TODO :: Create a map of paramString to paramType
    int ret = -1;
    std::map<std::string, std::pair<int, int>>::iterator it = paramIdType.find(param);
    if (it != paramIdType.end()) {
        ret = it->second.second;
    } else
        ret = -1;
    return ret;
}

void CameraParameters::initParamIdType()
{
    set(paramIdType, CAMERA_MODE, std::make_pair(PARAM_ID_CAMERA_MODE, PARAM_TYPE_UINT32));
    set(paramIdType, BRIGHTNESS, std::make_pair(PARAM_ID_BRIGHTNESS, PARAM_TYPE_UINT32));
    set(paramIdType, CONTRAST, std::make_pair(PARAM_ID_CONTRAST, PARAM_TYPE_UINT32));
    set(paramIdType, SATURATION, std::make_pair(PARAM_ID_SATURATION, PARAM_TYPE_UINT32));
    set(paramIdType, HUE, std::make_pair(PARAM_ID_HUE, PARAM_TYPE_INT32));
    set(paramIdType, WHITE_BALANCE_MODE,
        std::make_pair(PARAM_ID_WHITE_BALANCE_MODE, PARAM_TYPE_UINT32));
    set(paramIdType, GAMMA, std::make_pair(PARAM_ID_GAMMA, PARAM_TYPE_UINT32));
    set(paramIdType, GAIN, std::make_pair(PARAM_ID_GAIN, PARAM_TYPE_UINT32));
    set(paramIdType, POWER_LINE_FREQ_MODE,
        std::make_pair(PARAM_ID_POWER_LINE_FREQ_MODE, PARAM_TYPE_UINT32));
    set(paramIdType, WHITE_BALANCE_TEMPERATURE,
        std::make_pair(PARAM_ID_WHITE_BALANCE_TEMPERATURE, PARAM_TYPE_UINT32));
    set(paramIdType, SHARPNESS, std::make_pair(PARAM_ID_SHARPNESS, PARAM_TYPE_UINT32));
    set(paramIdType, BACKLIGHT_COMPENSATION,
        std::make_pair(PARAM_ID_BACKLIGHT_COMPENSATION, PARAM_TYPE_UINT32));
    set(paramIdType, EXPOSURE_MODE, std::make_pair(PARAM_ID_EXPOSURE_MODE, PARAM_TYPE_UINT32));
    set(paramIdType, EXPOSURE_ABSOLUTE,
        std::make_pair(PARAM_ID_EXPOSURE_ABSOLUTE, PARAM_TYPE_UINT32));
    set(paramIdType, IMAGE_SIZE, std::make_pair(PARAM_ID_IMAGE_SIZE, PARAM_TYPE_UINT32));
    set(paramIdType, IMAGE_FORMAT, std::make_pair(PARAM_ID_IMAGE_FORMAT, PARAM_TYPE_UINT32));
    set(paramIdType, PIXEL_FORMAT, std::make_pair(PARAM_ID_PIXEL_FORMAT, PARAM_TYPE_UINT32));
    set(paramIdType, WHITE_BALANCE_MODE,
        std::make_pair(PARAM_ID_WHITE_BALANCE_MODE, PARAM_TYPE_UINT32));
    set(paramIdType, SCENE_MODE, std::make_pair(PARAM_ID_SCENE_MODE, PARAM_TYPE_UINT32));
    set(paramIdType, VIDEO_SIZE, std::make_pair(PARAM_ID_VIDEO_SIZE, PARAM_TYPE_UINT32));
    set(paramIdType, VIDEO_FRAME_FORMAT,
        std::make_pair(PARAM_ID_VIDEO_FRAME_FORMAT, PARAM_TYPE_UINT32));
    set(paramIdType, VIDEO_SNAPSHOT_SUPPORTED,
        std::make_pair(PARAM_ID_VIDEO_SNAPSHOT_SUPPORTED, PARAM_TYPE_UINT32));
}

void CameraParameters::set(std::map<std::string, std::string> &pMap, std::string key,
                           std::string value)
{
    pMap.insert(std::make_pair(key, value));
}

void CameraParameters::set(std::map<std::string, std::pair<int, int>> &pMap, std::string key,
                           std::pair<int, int> value)
{
    pMap.insert(std::make_pair(key, value));
}

std::string CameraParameters::get(std::map<std::string, std::string> &pMap, std::string key)
{
    std::map<std::string, std::string>::iterator it = pMap.find(key);
    if (it != pMap.end())
        return it->second;
    else
        return "";
}
