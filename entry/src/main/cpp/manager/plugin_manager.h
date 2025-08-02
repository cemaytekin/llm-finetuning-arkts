/*
 * Copyright (c) 2023 Your Organization
 * Licensed under the Apache License, Version 2.0
 */

#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include <unordered_map>
#include <string>
#include <ace/xcomponent/native_interface_xcomponent.h>
#include "napi/native_api.h"
#include "render/sample_bitmap.h"


class PluginManager {
public:
    ~PluginManager();
    static PluginManager* GetInstance();
    
    void SetNativeXComponent(std::string& id, OH_NativeXComponent* nativeXComponent);
    SampleBitMap* GetRender(std::string& id);
    void Export(napi_env env, napi_value exports);

private:
    PluginManager() = default;
    static PluginManager* instance_;
    std::unordered_map<std::string, OH_NativeXComponent*> nativeXComponentMap_;
    std::unordered_map<std::string, SampleBitMap*> pluginRenderMap_;
};

#endif // PLUGIN_MANAGER_H