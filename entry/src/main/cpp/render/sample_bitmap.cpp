
/*
 * Copyright (c) 2023 Your Organization
 * Licensed under the Apache License, Version 2.0
 */
// sample bitmap header:
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
 
 
// sample_bitmap for drawing the cpp file
#include "sample_bitmap.h"
#include <unordered_map>
#include <stdint.h>
#include <sys/mman.h>
#include <cmath>
#include <algorithm>

#define DRAWING_LOGI(...) printf("INFO: " __VA_ARGS__)
#define DRAWING_LOGE(...) printf("ERROR: " __VA_ARGS__)

// Static map to store instances
static std::unordered_map<std::string, SampleBitMap*> instanceMap;

SampleBitMap::SampleBitMap()
    : width_(0),
      height_(0),
      cBitmap_(nullptr),
      cCanvas_(nullptr),
      cPath_(nullptr),
      cBrush_(nullptr),
      cPen_(nullptr),
      nativeWindow_(nullptr),
      mappedAddr_(nullptr),
      bufferHandle_(nullptr),
      buffer_(nullptr),
      fenceFd_(0)
{
    // Initialize the callback structure
    renderCallback_.OnSurfaceCreated = nullptr;
    renderCallback_.OnSurfaceChanged = nullptr;
    renderCallback_.OnSurfaceDestroyed = nullptr;
    renderCallback_.DispatchTouchEvent = nullptr;
}

SampleBitMap::~SampleBitMap() noexcept
{
    // Release all resources
    ReleaseBitmapResources();

    if (nativeWindow_ != nullptr) {
        // The native window is owned by the system, no need to destroy it
        nativeWindow_ = nullptr;
    }
}

SampleBitMap* SampleBitMap::GetInstance(const std::string& id)
{
    auto iter = instanceMap.find(id);
    if (iter != instanceMap.end()) {
        return iter->second;
    }

    SampleBitMap* instance = new SampleBitMap();
    instanceMap[id] = instance;
    return instance;
}

void SampleBitMap::Release(const std::string& id)
{
    auto iter = instanceMap.find(id);
    if (iter != instanceMap.end()) {
        delete iter->second;
        instanceMap.erase(iter);
    }
}

void SampleBitMap::SetNativeWindow(OHNativeWindow* window)
{
    nativeWindow_ = window;
}

void SampleBitMap::SetWidth(uint64_t width)
{
    width_ = width;
}

void SampleBitMap::SetHeight(uint64_t height)
{
    height_ = height;
}

void SampleBitMap::RegisterCallback(OH_NativeXComponent* nativeXComponent)
{
    if (nativeXComponent == nullptr) {
        DRAWING_LOGE("RegisterCallback: nativeXComponent is null\n");
        return;
    }

    // Set up the callback structure
    renderCallback_.OnSurfaceCreated = OnSurfaceCreatedCB;
    renderCallback_.OnSurfaceChanged = OnSurfaceChangedCB;
    renderCallback_.OnSurfaceDestroyed = OnSurfaceDestroyedCB;
    renderCallback_.DispatchTouchEvent = DispatchTouchEventCB;

    // Register the callback with the XComponent
    OH_NativeXComponent_RegisterCallback(nativeXComponent, &renderCallback_);
}

void SampleBitMap::ReleaseBitmapResources()
{
    // Unmap the memory if previously mapped
    if (mappedAddr_ != nullptr && bufferHandle_ != nullptr) {
        int result = munmap(mappedAddr_, bufferHandle_->size);
        if (result == -1) {
            DRAWING_LOGE("munmap failed!\n");
        }
        mappedAddr_ = nullptr;
    }

    // Destroy the created drawing objects
    if (cBrush_ != nullptr) {
        OH_Drawing_BrushDestroy(cBrush_);
        cBrush_ = nullptr;
    }

    if (cPen_ != nullptr) {
        OH_Drawing_PenDestroy(cPen_);
        cPen_ = nullptr;
    }

    if (cPath_ != nullptr) {
        OH_Drawing_PathDestroy(cPath_);
        cPath_ = nullptr;
    }

    if (cCanvas_ != nullptr) {
        OH_Drawing_CanvasDestroy(cCanvas_);
        cCanvas_ = nullptr;
    }

    if (cBitmap_ != nullptr) {
        OH_Drawing_BitmapDestroy(cBitmap_);
        cBitmap_ = nullptr;
    }
}

bool SampleBitMap::PrepareDrawing()
{
    // Clean up any previous resources
    ReleaseBitmapResources();

    if (nativeWindow_ == nullptr) {
        DRAWING_LOGE("PrepareDrawing: nativeWindow is null\n");
        return false;
    }

    // Request a buffer from the native window
    int32_t ret = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow_, &buffer_, &fenceFd_);
    if (ret != 0) {
        DRAWING_LOGE("PrepareDrawing: RequestBuffer failed, ret = %d\n", ret);
        return false;
    }

    // Get the buffer handle
    bufferHandle_ = OH_NativeWindow_GetBufferHandleFromNative(buffer_);
    if (bufferHandle_ == nullptr) {
        DRAWING_LOGE("PrepareDrawing: GetBufferHandleFromNative failed\n");
        return false;
    }

    // Map the buffer memory
    mappedAddr_ = static_cast<uint32_t*>(
        mmap(bufferHandle_->virAddr, bufferHandle_->size, PROT_READ | PROT_WRITE, MAP_SHARED, bufferHandle_->fd, 0));
    if (mappedAddr_ == MAP_FAILED) {
        DRAWING_LOGE("PrepareDrawing: mmap failed\n");
        mappedAddr_ = nullptr;
        return false;
    }

    // Create a bitmap for drawing
    cBitmap_ = OH_Drawing_BitmapCreate();
    if (cBitmap_ == nullptr) {
        DRAWING_LOGE("PrepareDrawing: BitmapCreate failed\n");
        return false;
    }

    // Define the pixel format of the bitmap
    OH_Drawing_BitmapFormat cFormat {COLOR_FORMAT_RGBA_8888, ALPHA_FORMAT_OPAQUE};
    
    // Build the bitmap with the specified format
    OH_Drawing_BitmapBuild(cBitmap_, width_, height_, &cFormat);

    // Create a canvas for drawing
    cCanvas_ = OH_Drawing_CanvasCreate();
    if (cCanvas_ == nullptr) {
        DRAWING_LOGE("PrepareDrawing: CanvasCreate failed\n");
        return false;
    }

    // Bind the bitmap to the canvas
    OH_Drawing_CanvasBind(cCanvas_, cBitmap_);

    // Clear the canvas with white
    OH_Drawing_CanvasClear(cCanvas_, OH_Drawing_ColorSetArgb(0xFF, 0xFF, 0xFF, 0xFF));

    return true;
}

void SampleBitMap::FinishDrawing()
{
    if (cBitmap_ == nullptr || mappedAddr_ == nullptr) {
        DRAWING_LOGE("FinishDrawing: bitmap or mappedAddr is null\n");
        return;
    }

    // Get the pixel data from the bitmap
    void* bitmapAddr = OH_Drawing_BitmapGetPixels(cBitmap_);
    if (bitmapAddr == nullptr) {
        DRAWING_LOGE("FinishDrawing: BitmapGetPixels failed\n");
        return;
    }

    // Copy the bitmap pixels to the native window buffer
    uint32_t* value = static_cast<uint32_t*>(bitmapAddr);
    uint32_t* pixel = static_cast<uint32_t*>(mappedAddr_);
    for (uint32_t x = 0; x < width_; x++) {
        for (uint32_t y = 0; y < height_; y++) {
            *pixel++ = *value++;
        }
    }

    // Flush the buffer to display it on the screen
    Region region {nullptr, 0};
    OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow_, buffer_, fenceFd_, region);

    // Release resources
    ReleaseBitmapResources();
}

void SampleBitMap::DrawPattern()
{
    DRAWING_LOGI("DrawPattern: Starting with width=%lu, height=%lu\n", width_, height_);
    
    if (!PrepareDrawing()) {
        DRAWING_LOGE("DrawPattern: PrepareDrawing failed\n");
        return;
    }
    DRAWING_LOGI("DrawPattern: PrepareDrawing succeeded\n");

    // Calculate pentagon vertices
    int len = height_ / 4;
    float aX = width_ / 2;
    float aY = height_ / 4;
    float dX = aX - len * std::sin(18.0f);
    float dY = aY + len * std::cos(18.0f);
    float cX = aX + len * std::sin(18.0f);
    float cY = dY;
    float bX = aX + (len / 2.0);
    float bY = aY + std::sqrt((cX - dX) * (cX - dX) + (len / 2.0) * (len / 2.0));
    float eX = aX - (len / 2.0);
    float eY = bY;

    // Create a path object for the pentagon
    cPath_ = OH_Drawing_PathCreate();
    
    // Specify the start point of the path
    OH_Drawing_PathMoveTo(cPath_, aX, aY);
    
    // Draw line segments for the pentagon
    OH_Drawing_PathLineTo(cPath_, bX, bY);
    OH_Drawing_PathLineTo(cPath_, cX, cY);
    OH_Drawing_PathLineTo(cPath_, dX, dY);
    OH_Drawing_PathLineTo(cPath_, eX, eY);
    
    // Close the path
    OH_Drawing_PathClose(cPath_);

    // Create a pen for outlining
    cPen_ = OH_Drawing_PenCreate();
    OH_Drawing_PenSetAntiAlias(cPen_, true);
    OH_Drawing_PenSetColor(cPen_, OH_Drawing_ColorSetArgb(0xFF, 0xFF, 0x00, 0x00)); // Red
    OH_Drawing_PenSetWidth(cPen_, 10.0);
    OH_Drawing_PenSetJoin(cPen_, LINE_ROUND_JOIN);
    
    // Attach the pen to the canvas
    OH_Drawing_CanvasAttachPen(cCanvas_, cPen_);

    // Create a brush for filling
    cBrush_ = OH_Drawing_BrushCreate();
    OH_Drawing_BrushSetColor(cBrush_, OH_Drawing_ColorSetArgb(0xFF, 0x00, 0xFF, 0x00)); // Green
    
    // Attach the brush to the canvas
    OH_Drawing_CanvasAttachBrush(cCanvas_, cBrush_);

    // Draw the pentagon
    OH_Drawing_CanvasDrawPath(cCanvas_, cPath_);

    DRAWING_LOGI("DrawPattern: Finished drawing, calling FinishDrawing()\n");
    // Finish drawing and display the result
    FinishDrawing();
    DRAWING_LOGI("DrawPattern: FinishDrawing completed\n");
}

void SampleBitMap::DrawText()
{
    DRAWING_LOGI("DrawText: Starting with width=%lu, height=%lu\n", width_, height_);
    
    if (!PrepareDrawing()) {
        DRAWING_LOGE("DrawText: PrepareDrawing failed\n");
        return;
    }
    DRAWING_LOGI("DrawText: PrepareDrawing succeeded\n");

    // Start with a gray background for better contrast
    OH_Drawing_CanvasClear(cCanvas_, OH_Drawing_ColorSetArgb(0xFF, 0xE0, 0xE0, 0xE0)); // Light gray background
    
    // Draw a blue rectangle to help visualize the drawing area
    OH_Drawing_Pen* rectPen = OH_Drawing_PenCreate();
    OH_Drawing_PenSetColor(rectPen, OH_Drawing_ColorSetArgb(0xFF, 0x00, 0x00, 0xFF)); // Blue
    OH_Drawing_PenSetWidth(rectPen, 5.0);
    OH_Drawing_CanvasAttachPen(cCanvas_, rectPen);
    
    OH_Drawing_Brush* rectBrush = OH_Drawing_BrushCreate();
    OH_Drawing_BrushSetColor(rectBrush, OH_Drawing_ColorSetArgb(0x40, 0x00, 0x00, 0xFF)); // Semi-transparent blue
    OH_Drawing_CanvasAttachBrush(cCanvas_, rectBrush);
    
    OH_Drawing_Path* rectPath = OH_Drawing_PathCreate();
    float x = width_ / 4;
    float y = height_ / 4;
    float w = width_ / 2;
    float h = height_ / 2;
    OH_Drawing_PathMoveTo(rectPath, x, y);
    OH_Drawing_PathLineTo(rectPath, x + w, y);
    OH_Drawing_PathLineTo(rectPath, x + w, y + h);
    OH_Drawing_PathLineTo(rectPath, x, y + h);
    OH_Drawing_PathClose(rectPath);
    OH_Drawing_CanvasDrawPath(cCanvas_, rectPath);
    
    // Clean up the rectangle resources
    OH_Drawing_PathDestroy(rectPath);
    OH_Drawing_BrushDestroy(rectBrush);
    OH_Drawing_PenDestroy(rectPen);

    // ----------------
    // ALTERNATIVE TEXT DRAWING METHOD
    // ----------------
    // Instead of using the typography API, draw text manually using paths
    // Create red pen and brush for the text
    OH_Drawing_Pen* textPen = OH_Drawing_PenCreate();
    OH_Drawing_PenSetColor(textPen, OH_Drawing_ColorSetArgb(0xFF, 0xFF, 0x00, 0x00)); // Red
    OH_Drawing_PenSetWidth(textPen, 5.0);
    OH_Drawing_PenSetAntiAlias(textPen, true);
    OH_Drawing_CanvasAttachPen(cCanvas_, textPen);
    
    OH_Drawing_Brush* textBrush = OH_Drawing_BrushCreate();
    OH_Drawing_BrushSetColor(textBrush, OH_Drawing_ColorSetArgb(0xFF, 0xFF, 0x00, 0x00)); // Red
    OH_Drawing_CanvasAttachBrush(cCanvas_, textBrush);
    
    // Starting position for text
    float textX = x + 40;
    float textY = y + 100;
    float letterHeight = h / 2;
    float letterWidth = w / 8;
    float spacing = letterWidth / 3;
    
    DRAWING_LOGI("DrawText: Drawing manual text at position: %f, %f\n", textX, textY);
    
    // Draw "HELLO" manually using paths
    
    // Draw "H"
    OH_Drawing_Path* letterH = OH_Drawing_PathCreate();
    OH_Drawing_PathMoveTo(letterH, textX, textY);
    OH_Drawing_PathLineTo(letterH, textX, textY + letterHeight);
    OH_Drawing_PathMoveTo(letterH, textX, textY + letterHeight/2);
    OH_Drawing_PathLineTo(letterH, textX + letterWidth, textY + letterHeight/2);
    OH_Drawing_PathMoveTo(letterH, textX + letterWidth, textY);
    OH_Drawing_PathLineTo(letterH, textX + letterWidth, textY + letterHeight);
    OH_Drawing_CanvasDrawPath(cCanvas_, letterH);
    OH_Drawing_PathDestroy(letterH);
    textX += letterWidth + spacing;
    
    // Draw "E"
    OH_Drawing_Path* letterE = OH_Drawing_PathCreate();
    OH_Drawing_PathMoveTo(letterE, textX, textY);
    OH_Drawing_PathLineTo(letterE, textX, textY + letterHeight);
    OH_Drawing_PathMoveTo(letterE, textX, textY);
    OH_Drawing_PathLineTo(letterE, textX + letterWidth, textY);
    OH_Drawing_PathMoveTo(letterE, textX, textY + letterHeight/2);
    OH_Drawing_PathLineTo(letterE, textX + letterWidth, textY + letterHeight/2);
    OH_Drawing_PathMoveTo(letterE, textX, textY + letterHeight);
    OH_Drawing_PathLineTo(letterE, textX + letterWidth, textY + letterHeight);
    OH_Drawing_CanvasDrawPath(cCanvas_, letterE);
    OH_Drawing_PathDestroy(letterE);
    textX += letterWidth + spacing;
    
    // Draw "L"
    OH_Drawing_Path* letterL = OH_Drawing_PathCreate();
    OH_Drawing_PathMoveTo(letterL, textX, textY);
    OH_Drawing_PathLineTo(letterL, textX, textY + letterHeight);
    OH_Drawing_PathMoveTo(letterL, textX, textY + letterHeight);
    OH_Drawing_PathLineTo(letterL, textX + letterWidth, textY + letterHeight);
    OH_Drawing_CanvasDrawPath(cCanvas_, letterL);
    OH_Drawing_PathDestroy(letterL);
    textX += letterWidth + spacing;
    
    // Draw another "L"
    OH_Drawing_Path* letterL2 = OH_Drawing_PathCreate();
    OH_Drawing_PathMoveTo(letterL2, textX, textY);
    OH_Drawing_PathLineTo(letterL2, textX, textY + letterHeight);
    OH_Drawing_PathMoveTo(letterL2, textX, textY + letterHeight);
    OH_Drawing_PathLineTo(letterL2, textX + letterWidth, textY + letterHeight);
    OH_Drawing_CanvasDrawPath(cCanvas_, letterL2);
    OH_Drawing_PathDestroy(letterL2);
    textX += letterWidth + spacing;
    
    // Draw "O"
    OH_Drawing_Path* letterO = OH_Drawing_PathCreate();
    // Draw a circle for the letter O
    float oRadius = letterWidth / 2;
    float oCenterX = textX + oRadius;
    float oCenterY = textY + letterHeight / 2;
    
    // Approximating a circle with lines
    const int numSegments = 20;
    float angle = 0.0f;
    float angleIncrement = 2.0f * M_PI / numSegments;
    
    float startX = oCenterX + oRadius * cos(angle);
    float startY = oCenterY + oRadius * sin(angle);
    OH_Drawing_PathMoveTo(letterO, startX, startY);
    
    for (int i = 1; i <= numSegments; i++) {
        angle += angleIncrement;
        float x = oCenterX + oRadius * cos(angle);
        float y = oCenterY + oRadius * sin(angle);
        OH_Drawing_PathLineTo(letterO, x, y);
    }
    
    OH_Drawing_CanvasDrawPath(cCanvas_, letterO);
    OH_Drawing_PathDestroy(letterO);
    
    // Clean up text resources
    OH_Drawing_BrushDestroy(textBrush);
    OH_Drawing_PenDestroy(textPen);
    
    DRAWING_LOGI("DrawText: All text resources cleaned up, calling FinishDrawing()\n");
    // Finish drawing and display the result
    FinishDrawing();
    DRAWING_LOGI("DrawText: FinishDrawing completed\n");
}

// NAPI methods for JavaScript/TypeScript interop
napi_value SampleBitMap::NapiDrawPattern(napi_env env, napi_callback_info info)
{
    // Get the current instance
    void* data = nullptr;
    napi_get_cb_info(env, info, nullptr, nullptr, nullptr, &data);
    
    napi_value thisArg;
    napi_get_cb_info(env, info, nullptr, nullptr, &thisArg, nullptr);
    
    // Get the component ID from XComponent
    char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {' '};
    uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;
    napi_value exportInstance;
    napi_get_named_property(env, thisArg, OH_NATIVE_XCOMPONENT_OBJ, &exportInstance);
    
    OH_NativeXComponent* nativeXComponent = nullptr;
    napi_unwrap(env, exportInstance, reinterpret_cast<void**>(&nativeXComponent));
    
    OH_NativeXComponent_GetXComponentId(nativeXComponent, idStr, &idSize);
    std::string id(idStr);
    
    // Get the SampleBitMap instance and draw the pattern
    auto render = SampleBitMap::GetInstance(id);
    if (render != nullptr) {
        render->DrawPattern();
    } else {
        DRAWING_LOGE("NapiDrawPattern: render is nullptr\n");
    }
    
    napi_value result;
    napi_get_undefined(env, &result);
    return result;
}

napi_value SampleBitMap::NapiDrawText(napi_env env, napi_callback_info info)
{
    // Get the current instance
    void* data = nullptr;
    napi_get_cb_info(env, info, nullptr, nullptr, nullptr, &data);
    
    napi_value thisArg;
    napi_get_cb_info(env, info, nullptr, nullptr, &thisArg, nullptr);
    
    // Get the component ID from XComponent
    char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {' '};
    uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;
    napi_value exportInstance;
    napi_get_named_property(env, thisArg, OH_NATIVE_XCOMPONENT_OBJ, &exportInstance);
    
    OH_NativeXComponent* nativeXComponent = nullptr;
    napi_unwrap(env, exportInstance, reinterpret_cast<void**>(&nativeXComponent));
    
    OH_NativeXComponent_GetXComponentId(nativeXComponent, idStr, &idSize);
    std::string id(idStr);
    
    // Get the SampleBitMap instance and draw text
    auto render = SampleBitMap::GetInstance(id);
    if (render != nullptr) {
        render->DrawText();
    } else {
        DRAWING_LOGE("NapiDrawText: render is nullptr\n");
    }
    
    napi_value result;
    napi_get_undefined(env, &result);
    return result;
}

void SampleBitMap::Export(napi_env env, napi_value exports)
{
    if ((env == nullptr) || (exports == nullptr)) {
        DRAWING_LOGE("Export: env or exports is null\n");
        return;
    }

    // Define JavaScript methods
    napi_property_descriptor desc[] = {
        {"drawPattern", nullptr, SampleBitMap::NapiDrawPattern, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"drawText", nullptr, SampleBitMap::NapiDrawText, nullptr, nullptr, nullptr, napi_default, nullptr}
    };

    // Register methods
    if (napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc) != napi_ok) {
        DRAWING_LOGE("Export: napi_define_properties failed\n");
    }
}

// Callback functions
void OnSurfaceCreatedCB(OH_NativeXComponent* component, void* window)
{
    if ((component == nullptr) || (window == nullptr)) {
        DRAWING_LOGE("OnSurfaceCreatedCB: component or window is null\n");
        return;
    }
    
    // Obtain an OHNativeWindow instance
    OHNativeWindow* nativeWindow = static_cast<OHNativeWindow*>(window);
    
    // Get the XComponent ID
    char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {' '};
    uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;
    if (OH_NativeXComponent_GetXComponentId(component, idStr, &idSize) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
        DRAWING_LOGE("OnSurfaceCreatedCB: Unable to get XComponent id\n");
        return;
    }
    
    std::string id(idStr);
    auto render = SampleBitMap::GetInstance(id);
    render->SetNativeWindow(nativeWindow);
    
    // Get the size of the XComponent
    uint64_t width;
    uint64_t height;
    int32_t xSize = OH_NativeXComponent_GetXComponentSize(component, window, &width, &height);
    if ((xSize == OH_NATIVEXCOMPONENT_RESULT_SUCCESS) && (render != nullptr)) {
        render->SetHeight(height);
        render->SetWidth(width);
        DRAWING_LOGI("xComponent width = %lu, height = %lu\n", width, height);
    }
}

void OnSurfaceChangedCB(OH_NativeXComponent* component, void* window)
{
    if ((component == nullptr) || (window == nullptr)) {
        DRAWING_LOGE("OnSurfaceChangedCB: component or window is null\n");
        return;
    }
    
    // Get the XComponent ID
    char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {' '};
    uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;
    if (OH_NativeXComponent_GetXComponentId(component, idStr, &idSize) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
        DRAWING_LOGE("OnSurfaceChangedCB: Unable to get XComponent id\n");
        return;
    }
    
    std::string id(idStr);
    auto render = SampleBitMap::GetInstance(id);
    
    // Get the new size of the XComponent
    uint64_t width;
    uint64_t height;
    int32_t xSize = OH_NativeXComponent_GetXComponentSize(component, window, &width, &height);
    if ((xSize == OH_NATIVEXCOMPONENT_RESULT_SUCCESS) && (render != nullptr)) {
        render->SetHeight(height);
        render->SetWidth(width);
        DRAWING_LOGI("Surface Changed: xComponent width = %lu, height = %lu\n", width, height);
    }
}

void OnSurfaceDestroyedCB(OH_NativeXComponent* component, void* window)
{
    if ((component == nullptr) || (window == nullptr)) {
        DRAWING_LOGE("OnSurfaceDestroyedCB: component or window is null\n");
        return;
    }
    
    // Get the XComponent ID
    char idStr[OH_XCOMPONENT_ID_LEN_MAX + 1] = {' '};
    uint64_t idSize = OH_XCOMPONENT_ID_LEN_MAX + 1;
    if (OH_NativeXComponent_GetXComponentId(component, idStr, &idSize) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
        DRAWING_LOGE("OnSurfaceDestroyedCB: Unable to get XComponent id\n");
        return;
    }
    
    std::string id(idStr);
    SampleBitMap::Release(id);
    DRAWING_LOGI("OnSurfaceDestroyedCB: Released instance for id %s\n", id.c_str());
}

void DispatchTouchEventCB(OH_NativeXComponent* component, void* window)
{
    // This callback is not implemented in this example
    // It would handle touch events if needed
}
