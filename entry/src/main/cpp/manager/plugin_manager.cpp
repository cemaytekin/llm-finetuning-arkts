// cpp/manager/plugin_manager.cpp

#include "plugin_manager.h"
#include <cstdio>

#define DRAWING_LOGI(...) printf("INFO: " __VA_ARGS__)
#define DRAWING_LOGE(...) printf("ERROR: " __VA_ARGS__)

// Initialize the static instance pointer
PluginManager* PluginManager::instance_ = nullptr;

PluginManager* PluginManager::GetInstance()
{
    if (instance_ == nullptr) {
        instance_ = new PluginManager();
    }
    return instance_;
}

PluginManager::~PluginManager()
{
    // Clean up all SampleBitMap instances in the map
    for (auto& pair : pluginRenderMap_) {
        if (pair.second != nullptr) {
            delete pair.second;
            pair.second = nullptr;
        }
    }
    pluginRenderMap_.clear();

    // Note: We don't delete nativeXComponent objects as they are owned by the system
    nativeXComponentMap_.clear();
    
    // Reset the static instance
    instance_ = nullptr;
}

void PluginManager::SetNativeXComponent(std::string& id, OH_NativeXComponent* nativeXComponent)
{
    if (nativeXComponent == nullptr) {
        DRAWING_LOGE("SetNativeXComponent: nativeXComponent is null\n");
        return;
    }

    nativeXComponentMap_[id] = nativeXComponent;

    // Create a SampleBitMap for this component if it doesn't exist
    if (pluginRenderMap_.find(id) == pluginRenderMap_.end()) {
        pluginRenderMap_[id] = SampleBitMap::GetInstance(id);
    }
}

SampleBitMap* PluginManager::GetRender(std::string& id)
{
    auto it = pluginRenderMap_.find(id);
    if (it != pluginRenderMap_.end()) {
        return it->second;
    }
    
    // If not found, create a new instance and store it
    SampleBitMap* render = SampleBitMap::GetInstance(id);
    pluginRenderMap_[id] = render;
    return render;
}

void PluginManager::Export(napi_env env, napi_value exports)
{
    if ((env == nullptr) || (exports == nullptr)) {
        DRAWING_LOGE("Export: env or exports is null\n");
        return;
    }

    napi_value exportInstance = nullptr;
    if (napi_get_named_property(env, exports, OH_NATIVE_XCOMPONENT_OBJ, &exportInstance) != napi_ok) {
        DRAWING_LOGE("Export: napi_get_named_property fail\n");
        return;
    }

    OH_NativeXComponent* nativeXComponent = nullptr;
    if (napi_unwrap(env, exportInstance, reinterpret_cast<void**>(&nativeXComponent)) != napi_ok) {
        DRAWING_LOGE("Export: napi_unwrap fail\n");
        return;
    }

    char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {'\0'};
    uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;
    if (OH_NativeXComponent_GetXComponentId(nativeXComponent, idStr, &idSize) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
        DRAWING_LOGE("Export: OH_NativeXComponent_GetXComponentId fail\n");
        return;
    }

    std::string id(idStr);
    auto context = PluginManager::GetInstance();
    if ((context != nullptr) && (nativeXComponent != nullptr)) {
        context->SetNativeXComponent(id, nativeXComponent);
        auto render = context->GetRender(id);
        if (render != nullptr) {
            render->RegisterCallback(nativeXComponent);
            render->Export(env, exports);
        } else {
            DRAWING_LOGE("Export: render is nullptr\n");
        }
    }
}