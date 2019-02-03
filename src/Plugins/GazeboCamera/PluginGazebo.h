/*
 * This file is part of the Dronecode Camera Manager
 *
 * Copyright (C) 2018  Intel Corporation. All rights reserved.
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

#include <string>
#include <vector>

#include "CameraDevice.h"
#include "PluginBase.h"

class PluginGazebo final : public PluginBase {
public:
    PluginGazebo();
    ~PluginGazebo();

    std::vector<std::string> getCameraDevices();
    std::shared_ptr<CameraDevice> createCameraDevice(std::string);

private:
    std::vector<std::string> mCamList;
    void discoverCameras(std::vector<std::string> &camList);
};
