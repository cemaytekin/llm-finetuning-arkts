/*
 * Copyright (c) 2023 Your Organization
 * Licensed under the Apache License, Version 2.0
 */

#ifndef SAMPLE_BITMAP_H
#define SAMPLE_BITMAP_H

#include <ace/xcomponent/native_interface_xcomponent.h>
#include <native_window/external_window.h>
#include <native_drawing/drawing_bitmap.h>
#include <native_drawing/drawing_color.h>
#include <native_drawing/drawing_canvas.h>
#include <native_drawing/drawing_pen.h>
#include <native_drawing/drawing_brush.h>
#include <native_drawing/drawing_path.h>
#include <native_drawing/drawing_bitmap.h>
#include <native_drawing/drawing_color.h>
#include <native_drawing/drawing_canvas.h>
#include <native_drawing/drawing_pen.h>
#include <native_drawing/drawing_brush.h>
#include <native_drawing/drawing_path.h>
#include <native_drawing/drawing_text_typography.h>
#include "napi/native_api.h"
#include <string>

// Forward declarations for callbacks
void OnSurfaceCreatedCB(OH_NativeXComponent* component, void* window);
void OnSurfaceChangedCB(OH_NativeXComponent* component, void* window);
void OnSurfaceDestroyedCB(OH_NativeXComponent* component, void* window);
void DispatchTouchEventCB(OH_NativeXComponent* component, void* window);

class SampleBitMap {
public:
    SampleBitMap();
    ~SampleBitMap() noexcept;

    // Static methods for instance management
    static SampleBitMap* GetInstance(const std::string& id);
    static void Release(const std::string& id);

    // Setters for window and dimensions
    void SetNativeWindow(OHNativeWindow* window);
    void SetWidth(uint64_t width);
    void SetHeight(uint64_t height);

    // Register callbacks with XComponent
    void RegisterCallback(OH_NativeXComponent* nativeXComponent);

    // Drawing methods
    void DrawPattern();
    void DrawText();

    // Export NAPI interface
    void Export(napi_env env, napi_value exports);

    // NAPI methods for JavaScript
    static napi_value NapiDrawPattern(napi_env env, napi_callback_info info);
    static napi_value NapiDrawText(napi_env env, napi_callback_info info);

private:
    // Helper methods for drawing
    bool PrepareDrawing();
    void FinishDrawing();
    void ReleaseBitmapResources();

    // XComponent callback structure
    OH_NativeXComponent_Callback renderCallback_;

    // Window dimensions
    uint64_t width_;
    uint64_t height_;

    // Drawing resources
    OH_Drawing_Bitmap* cBitmap_;
    OH_Drawing_Canvas* cCanvas_;
    OH_Drawing_Path* cPath_;
    OH_Drawing_Brush* cBrush_;
    OH_Drawing_Pen* cPen_;

    // Native window resources
    OHNativeWindow* nativeWindow_;
    uint32_t* mappedAddr_;
    BufferHandle* bufferHandle_;
    struct NativeWindowBuffer* buffer_;
    int fenceFd_;
};

#endif // SAMPLE_BITMAP_H