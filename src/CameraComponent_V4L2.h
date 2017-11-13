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
#pragma once
#include <map>
#include <string>
#include <vector>

#include "CameraComponent.h"
#include "log.h"

class CameraComponent_V4L2 final : public CameraComponent {
public:
    CameraComponent_V4L2(std::string dev_path);
    CameraComponent_V4L2(std::string dev_path, std::string uri);
    ~CameraComponent_V4L2();
    int getParamType(const char *param_id, size_t id_size);
    int getParam(const char *param_id, size_t id_size, char *param_value /*o/p Byte Array*/,
                 size_t value_size);
    int setParam(const char *param_id, size_t id_size, const char *param_value /*i/p Byte Array*/,
                 size_t value_size, int param_type);
    int setParam(std::string param_id, float param_value);
    int setParam(std::string param_id, int32_t param_value);
    int setParam(std::string param_id, uint32_t param_value);
    int setParam(std::string param_id, uint8_t param_value);
    int setCameraMode(uint32_t mode);
    int getCameraMode();

private:
    std::string dev_path;
    int mCamMode;
    void initCameraInfo();
    void initStorageInfo();
    void initSupportedValues();
    void initDefaultValues();
    int setImazeSize(uint32_t wb_value);
    int setImageFormat(uint32_t wb_value);
    int setPixelFormat(uint32_t wb_value);
    int setSceneMode(uint32_t wb_value);
    int setVideoSize(uint32_t wb_value);
    int setVideoFrameFormat(uint32_t wb_value);
    std::string toString(const char *buf, size_t buf_size);
};
