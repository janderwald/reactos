/*
 * Video Renderer (Fullscreen and Windowed using Direct Draw)
 *
 * Copyright 2004 Christian Costa
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "quartz_private.h"

#include "uuids.h"
#include "vfwmsgs.h"
#include "amvideo.h"
#include "windef.h"
#include "winbase.h"
#include "dshow.h"
#include "evcode.h"
#include "strmif.h"
#include "ddraw.h"
#include "dvdmedia.h"

#include <assert.h>
#include "wine/debug.h"
#include <reactos/libs/libjpeg/jpeglib.h>
#include <reactos/libs/libjpeg/jerror.h>

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

struct video_renderer
{
    struct strmbase_renderer renderer;
    struct video_window window;

    IOverlay IOverlay_iface;

    LONG VideoWidth;
    LONG VideoHeight;
    LONG FullScreenMode;

    DWORD saved_style;
};

static inline struct video_renderer *impl_from_video_window(struct video_window *iface)
{
    return CONTAINING_RECORD(iface, struct video_renderer, window);
}

static inline struct video_renderer *impl_from_strmbase_renderer(struct strmbase_renderer *iface)
{
    return CONTAINING_RECORD(iface, struct video_renderer, renderer);
}

static inline struct video_renderer *impl_from_IVideoWindow(IVideoWindow *iface)
{
    return CONTAINING_RECORD(iface, struct video_renderer, window.IVideoWindow_iface);
}

static const BITMAPINFOHEADER *get_bitmap_header(const AM_MEDIA_TYPE *mt)
{
    if (IsEqualGUID(&mt->formattype, &FORMAT_VideoInfo))
        return &((VIDEOINFOHEADER *)mt->pbFormat)->bmiHeader;
    else
        return &((VIDEOINFOHEADER2 *)mt->pbFormat)->bmiHeader;
}

static void VideoRenderer_AutoShowWindow(struct video_renderer *This)
{
    if (This->window.AutoShow)
        ShowWindow(This->window.hwnd, SW_SHOW);
}


/* Memory data source for libjpeg */
typedef struct
{
    struct jpeg_source_mgr pub;
    const BYTE* data;
    size_t size;
    size_t pos;
} mem_source_mgr;

// ── Custom error handler that uses longjmp instead of exit() ──────────────
struct JpegErrorMgr {
    struct jpeg_error_mgr  pub;        // must be first
    jmp_buf         setjmpBuf;
    char            msg[JMSG_LENGTH_MAX];
};

static void init_source(j_decompress_ptr cinfo)
{
    mem_source_mgr* src = (mem_source_mgr*)cinfo->src;
    src->pos = 0;
}

static boolean fill_input_buffer(j_decompress_ptr cinfo)
{
    mem_source_mgr* src = (mem_source_mgr*)cinfo->src;
    if (src->pos >= src->size)
        return FALSE;
    src->pub.next_input_byte = src->data + src->pos;
    src->pub.bytes_in_buffer = src->size - src->pos;
    src->pos = src->size;
    return TRUE;
}

static void skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
    mem_source_mgr* src = (mem_source_mgr*)cinfo->src;
    if (num_bytes > 0)
    {
        src->pub.next_input_byte += num_bytes;
        src->pub.bytes_in_buffer -= num_bytes;
    }
}

static void term_source(j_decompress_ptr cinfo)
{
}


static void JpegErrorExit(j_common_ptr cinfo)
{
    struct JpegErrorMgr * myErr = (struct JpegErrorMgr*)(cinfo->err);
    // Format the error message
    (*cinfo->err->format_message)(cinfo, myErr->msg);
    // Jump back to the setjmp point
    longjmp(myErr->setjmpBuf, 1);
}

static HRESULT jpeg_to_bgra32(struct video_renderer * This, const BYTE* jpegData, DWORD jpegSize, int w, int h)
{
    char            msg[JMSG_LENGTH_MAX];
    mem_source_mgr src;
    JSAMPARRAY buffer;
    int row_stride;
    int x, y;
    BYTE* dst;
    BYTE* bgra32;
    BITMAPINFO bmi;
    HDC dc;
    RECT srcRect = This->window.src, dstRect = This->window.dst;

    // HACK
    w = 640;
    h = 480;

    TRACE("Decoding JPEG: %u bytes, target %dx%d", jpegSize, w, h);
    // ── Set up libjpeg decompressor ────────────────────────────────────────
    struct jpeg_decompress_struct cinfo = {0};
    struct JpegErrorMgr           errMgr = {0};

    cinfo.err = jpeg_std_error(&errMgr.pub);
    errMgr.pub.error_exit = JpegErrorExit;

    // If libjpeg calls error_exit, we land here
    if (setjmp(errMgr.setjmpBuf)) {
        jpeg_destroy_decompress(&cinfo);
        strcpy(msg, errMgr.msg);
        TRACE("Decoding JPEG:failed with %s\n", msg);
        return S_FALSE;
    }

    /* Allocate display buffer (BGRA32) */
    bgra32 = CoTaskMemAlloc(w * h * 4);
    if (!bgra32)
    {
        TRACE("failed to allocate buffer\n");
        return S_FALSE;
    }
    /* Initialize BITMAPINFO for display */
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = 640;
    bmi.bmiHeader.biHeight = -480;  /* Negative for top-down */
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    /* Create decompressor */
    jpeg_create_decompress(&cinfo);

    /* Set up memory source */
    ZeroMemory(&src, sizeof(src));
    src.pub.init_source = init_source;
    src.pub.fill_input_buffer = fill_input_buffer;
    src.pub.skip_input_data = skip_input_data;
    src.pub.resync_to_restart = jpeg_resync_to_restart;
    src.pub.term_source = term_source;
    src.pub.next_input_byte = jpegData;
    src.pub.bytes_in_buffer = jpegSize;
    src.data = jpegData;
    src.size = jpegSize;
    src.pos = 0;

    cinfo.src = (struct jpeg_source_mgr *)&src;

    /* Read header */
    if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
        jpeg_destroy_decompress(&cinfo);
        TRACE("JPEG header read failed");
        return FALSE;
    }

    /* Set output format to RGB */
    cinfo.out_color_space = JCS_RGB;

    /* Start decompression */
    jpeg_start_decompress(&cinfo);

    TRACE("[APP] JPEG image: %u x %u, components: %u\n",
               cinfo.output_width, cinfo.output_height, cinfo.output_components);

    /* Allocate scanline buffer */
    row_stride = cinfo.output_width * cinfo.output_components;
    buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);

    dst = bgra32;

    /* Read and convert scanlines */
    while (cinfo.output_scanline < cinfo.output_height && cinfo.output_scanline < (UINT)h)
    {
        BYTE *src_row;
        int x;

        jpeg_read_scanlines(&cinfo, buffer, 1);
        src_row = buffer[0];

        /* Convert RGB to BGRA32 */
        for (x = 0; x < w && x < (int)cinfo.output_width; x++)
        {
            BYTE r = src_row[x * 3 + 0];
            BYTE g = src_row[x * 3 + 1];
            BYTE b = src_row[x * 3 + 2];

            *dst++ = b;  /* B */
            *dst++ = g;  /* G */
            *dst++ = r;  /* R */
            *dst++ = 0xFF;  /* A */
        }

        /* Pad remaining columns with black */
        for (; x < w; x++)
        {
            *dst++ = 0;
            *dst++ = 0;
            *dst++ = 0;
            *dst++ = 0xFF;
        }
    }

    /* Fill remaining rows with black */
    for (y = cinfo.output_scanline; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            *dst++ = 0;
            *dst++ = 0;
            *dst++ = 0;
            *dst++ = 0xFF;
        }
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    dc = GetDC(This->window.hwnd);

    StretchDIBits(dc, dstRect.left, dstRect.top, dstRect.right - dstRect.left, dstRect.bottom - dstRect.top,
            srcRect.left, srcRect.top, srcRect.right - srcRect.left, srcRect.bottom - srcRect.top, bgra32,
            (BITMAPINFO *)get_bitmap_header(&This->renderer.sink.pin.mt), DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(This->window.hwnd, dc);

    return S_OK;
}

static HRESULT video_renderer_render(struct strmbase_renderer *iface, IMediaSample *pSample)
{
    struct video_renderer *filter = impl_from_strmbase_renderer(iface);
    RECT src = filter->window.src, dst = filter->window.dst;
    LPBYTE pbSrcStream = NULL;
    HRESULT hr;
    AM_MEDIA_TYPE * mediaType;
    HDC dc;
    BITMAPINFOHEADER *bmiHeader;


    TRACE("filter %p, sample %p.\n", filter, pSample);

    hr = IMediaSample_GetPointer(pSample, &pbSrcStream);
    if (FAILED(hr))
    {
        ERR("Failed to get buffer pointer, hr %#lx.\n", hr);
        return hr;
    }
    hr = IMediaSample_GetMediaType(pSample, &mediaType);
    if (FAILED(hr))
    {
        ERR("Failed to get media type, hr %#lx.\n", hr);
        return hr;
    }
     if (IsEqualIID(&mediaType->formattype, &FORMAT_VideoInfo))
    {
        bmiHeader = &((VIDEOINFOHEADER *)mediaType->pbFormat)->bmiHeader;
    }
    else if (IsEqualIID(&mediaType->formattype, &FORMAT_VideoInfo2))
    {
        bmiHeader = &((VIDEOINFOHEADER2 *)mediaType->pbFormat)->bmiHeader;
    }
    else
    {
        FIXME("Unknown type %s\n", debugstr_guid(&mediaType->subtype));
        return VFW_E_RUNTIME_ERROR;
    }

    if (IsEqualGUID(&mediaType->subtype, &MEDIASUBTYPE_MJPG))
    {
        return jpeg_to_bgra32(filter, pbSrcStream, IMediaSample_GetSize(pSample), bmiHeader->biWidth, bmiHeader->biHeight);
    }

    dc = GetDC(filter->window.hwnd);
    StretchDIBits(dc, dst.left, dst.top, dst.right - dst.left, dst.bottom - dst.top,
            src.left, src.top, src.right - src.left, src.bottom - src.top, pbSrcStream,
            (BITMAPINFO *)get_bitmap_header(&filter->renderer.sink.pin.mt), DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(filter->window.hwnd, dc);

    return S_OK;
}

static HRESULT video_renderer_query_accept(struct strmbase_renderer *iface, const AM_MEDIA_TYPE *mt)
{
    if (!IsEqualGUID(&mt->majortype, &MEDIATYPE_Video))
        return S_FALSE;

    if (!IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_RGB32)
            && !IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_RGB24)
            && !IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_RGB565)
            && !IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_RGB8)
            && !IsEqualGUID(&mt->subtype, &MEDIASUBTYPE_MJPG))
        return S_FALSE;

    if (!IsEqualGUID(&mt->formattype, &FORMAT_VideoInfo)
            && !IsEqualGUID(&mt->formattype, &FORMAT_VideoInfo2))
        return S_FALSE;

    return S_OK;
}

static void video_renderer_destroy(struct strmbase_renderer *iface)
{
    struct video_renderer *filter = impl_from_strmbase_renderer(iface);

    video_window_cleanup(&filter->window);
    strmbase_renderer_cleanup(&filter->renderer);
    free(filter);
}

static HRESULT video_renderer_query_interface(struct strmbase_renderer *iface, REFIID iid, void **out)
{
    struct video_renderer *filter = impl_from_strmbase_renderer(iface);

    if (IsEqualGUID(iid, &IID_IBasicVideo))
        *out = &filter->window.IBasicVideo_iface;
    else if (IsEqualGUID(iid, &IID_IVideoWindow))
        *out = &filter->window.IVideoWindow_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT video_renderer_pin_query_interface(struct strmbase_renderer *iface, REFIID iid, void **out)
{
    struct video_renderer *filter = impl_from_strmbase_renderer(iface);

    if (IsEqualGUID(iid, &IID_IOverlay))
        *out = &filter->IOverlay_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static void video_renderer_stop_stream(struct strmbase_renderer *iface)
{
    struct video_renderer *This = impl_from_strmbase_renderer(iface);

    TRACE("(%p)->()\n", This);

    if (This->window.AutoShow)
        /* Black it out */
        RedrawWindow(This->window.hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
}

static void video_renderer_init_stream(struct strmbase_renderer *iface)
{
    struct video_renderer *filter = impl_from_strmbase_renderer(iface);

    VideoRenderer_AutoShowWindow(filter);
}

static HRESULT video_renderer_connect(struct strmbase_renderer *iface, const AM_MEDIA_TYPE *mt)
{
    struct video_renderer *filter = impl_from_strmbase_renderer(iface);
    const BITMAPINFOHEADER *bitmap_header = get_bitmap_header(mt);
    HWND window = filter->window.hwnd;
    RECT rect;

    filter->VideoWidth = bitmap_header->biWidth;
    filter->VideoHeight = abs(bitmap_header->biHeight);
    SetRect(&rect, 0, 0, filter->VideoWidth, filter->VideoHeight);
    filter->window.src = rect;

    AdjustWindowRectEx(&rect, GetWindowLongW(window, GWL_STYLE), FALSE,
            GetWindowLongW(window, GWL_EXSTYLE));
    SetWindowPos(window, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    GetClientRect(window, &filter->window.dst);

    return S_OK;
}

static void video_renderer_get_default_rect(struct video_window *iface, RECT *rect)
{
    struct video_renderer *filter = impl_from_video_window(iface);

    SetRect(rect, 0, 0, filter->VideoWidth, filter->VideoHeight);
}

static const struct strmbase_renderer_ops renderer_ops =
{
    .renderer_query_accept = video_renderer_query_accept,
    .renderer_render = video_renderer_render,
    .renderer_init_stream = video_renderer_init_stream,
    .renderer_stop_stream = video_renderer_stop_stream,
    .renderer_destroy = video_renderer_destroy,
    .renderer_query_interface = video_renderer_query_interface,
    .renderer_pin_query_interface = video_renderer_pin_query_interface,
    .renderer_connect = video_renderer_connect,
};

static HRESULT video_renderer_get_current_image(struct video_window *iface, LONG *size, LONG *image)
{
    struct video_renderer *filter = impl_from_video_window(iface);
    const BITMAPINFOHEADER *bih;
    size_t image_size;
    BYTE *sample_data;

    EnterCriticalSection(&filter->renderer.filter.stream_cs);

    bih = get_bitmap_header(&filter->renderer.sink.pin.mt);
    image_size = bih->biWidth * bih->biHeight * bih->biBitCount / 8;

    if (!image)
    {
        LeaveCriticalSection(&filter->renderer.filter.stream_cs);
        *size = sizeof(BITMAPINFOHEADER) + image_size;
        return S_OK;
    }

    if (filter->renderer.filter.state != State_Paused)
    {
        LeaveCriticalSection(&filter->renderer.filter.stream_cs);
        return VFW_E_NOT_PAUSED;
    }

    if (!filter->renderer.current_sample)
    {
        LeaveCriticalSection(&filter->renderer.filter.stream_cs);
        return E_UNEXPECTED;
    }

    if (*size < sizeof(BITMAPINFOHEADER) + image_size)
    {
        LeaveCriticalSection(&filter->renderer.filter.stream_cs);
        return E_OUTOFMEMORY;
    }

    memcpy(image, bih, sizeof(BITMAPINFOHEADER));
    IMediaSample_GetPointer(filter->renderer.current_sample, &sample_data);
    memcpy((char *)image + sizeof(BITMAPINFOHEADER), sample_data, image_size);

    LeaveCriticalSection(&filter->renderer.filter.stream_cs);
    return S_OK;
}

static const struct video_window_ops window_ops =
{
    .get_default_rect = video_renderer_get_default_rect,
    .get_current_image = video_renderer_get_current_image,
};

static HRESULT WINAPI VideoWindow_get_FullScreenMode(IVideoWindow *iface,
                                                     LONG *FullScreenMode)
{
    struct video_renderer *This = impl_from_IVideoWindow(iface);

    TRACE("window %p, fullscreen %p.\n", This, FullScreenMode);

    if (!FullScreenMode)
        return E_POINTER;

    *FullScreenMode = This->FullScreenMode;

    return S_OK;
}

static HRESULT WINAPI VideoWindow_put_FullScreenMode(IVideoWindow *iface, LONG fullscreen)
{
    struct video_renderer *filter = impl_from_IVideoWindow(iface);
    HWND window = filter->window.hwnd;

    FIXME("filter %p, fullscreen %ld.\n", filter, fullscreen);

    if (fullscreen == filter->FullScreenMode)
        return S_FALSE;

    if (fullscreen)
    {
        filter->saved_style = GetWindowLongW(window, GWL_STYLE);
        ShowWindow(window, SW_HIDE);
        SetParent(window, NULL);
        SetWindowLongW(window, GWL_STYLE, WS_POPUP);
        SetWindowPos(window, HWND_TOP, 0, 0,
                GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_SHOWWINDOW);
        GetWindowRect(window, &filter->window.dst);
    }
    else
    {
        ShowWindow(window, SW_HIDE);
        SetParent(window, filter->window.hwndOwner);
        SetWindowLongW(window, GWL_STYLE, filter->saved_style);
        GetClientRect(window, &filter->window.dst);
        SetWindowPos(window, 0, filter->window.dst.left, filter->window.dst.top,
                filter->window.dst.right, filter->window.dst.bottom, SWP_NOZORDER | SWP_SHOWWINDOW);
    }
    filter->FullScreenMode = fullscreen;

    return S_OK;
}

static const IVideoWindowVtbl IVideoWindow_VTable =
{
    BaseControlWindowImpl_QueryInterface,
    BaseControlWindowImpl_AddRef,
    BaseControlWindowImpl_Release,
    BaseControlWindowImpl_GetTypeInfoCount,
    BaseControlWindowImpl_GetTypeInfo,
    BaseControlWindowImpl_GetIDsOfNames,
    BaseControlWindowImpl_Invoke,
    BaseControlWindowImpl_put_Caption,
    BaseControlWindowImpl_get_Caption,
    BaseControlWindowImpl_put_WindowStyle,
    BaseControlWindowImpl_get_WindowStyle,
    BaseControlWindowImpl_put_WindowStyleEx,
    BaseControlWindowImpl_get_WindowStyleEx,
    BaseControlWindowImpl_put_AutoShow,
    BaseControlWindowImpl_get_AutoShow,
    BaseControlWindowImpl_put_WindowState,
    BaseControlWindowImpl_get_WindowState,
    BaseControlWindowImpl_put_BackgroundPalette,
    BaseControlWindowImpl_get_BackgroundPalette,
    BaseControlWindowImpl_put_Visible,
    BaseControlWindowImpl_get_Visible,
    BaseControlWindowImpl_put_Left,
    BaseControlWindowImpl_get_Left,
    BaseControlWindowImpl_put_Width,
    BaseControlWindowImpl_get_Width,
    BaseControlWindowImpl_put_Top,
    BaseControlWindowImpl_get_Top,
    BaseControlWindowImpl_put_Height,
    BaseControlWindowImpl_get_Height,
    BaseControlWindowImpl_put_Owner,
    BaseControlWindowImpl_get_Owner,
    BaseControlWindowImpl_put_MessageDrain,
    BaseControlWindowImpl_get_MessageDrain,
    BaseControlWindowImpl_get_BorderColor,
    BaseControlWindowImpl_put_BorderColor,
    VideoWindow_get_FullScreenMode,
    VideoWindow_put_FullScreenMode,
    BaseControlWindowImpl_SetWindowForeground,
    BaseControlWindowImpl_NotifyOwnerMessage,
    BaseControlWindowImpl_SetWindowPosition,
    BaseControlWindowImpl_GetWindowPosition,
    BaseControlWindowImpl_GetMinIdealImageSize,
    BaseControlWindowImpl_GetMaxIdealImageSize,
    BaseControlWindowImpl_GetRestorePosition,
    BaseControlWindowImpl_HideCursor,
    BaseControlWindowImpl_IsCursorHidden
};

static inline struct video_renderer *impl_from_IOverlay(IOverlay *iface)
{
    return CONTAINING_RECORD(iface, struct video_renderer, IOverlay_iface);
}

static HRESULT WINAPI overlay_QueryInterface(IOverlay *iface, REFIID iid, void **out)
{
    struct video_renderer *filter = impl_from_IOverlay(iface);
    return IPin_QueryInterface(&filter->renderer.sink.pin.IPin_iface, iid, out);
}

static ULONG WINAPI overlay_AddRef(IOverlay *iface)
{
    struct video_renderer *filter = impl_from_IOverlay(iface);
    return IPin_AddRef(&filter->renderer.sink.pin.IPin_iface);
}

static ULONG WINAPI overlay_Release(IOverlay *iface)
{
    struct video_renderer *filter = impl_from_IOverlay(iface);
    return IPin_Release(&filter->renderer.sink.pin.IPin_iface);
}

static HRESULT WINAPI overlay_GetPalette(IOverlay *iface, DWORD *count, PALETTEENTRY **palette)
{
    FIXME("iface %p, count %p, palette %p, stub!\n", iface, count, palette);
    return E_NOTIMPL;
}

static HRESULT WINAPI overlay_SetPalette(IOverlay *iface, DWORD count, PALETTEENTRY *palette)
{
    FIXME("iface %p, count %lu, palette %p, stub!\n", iface, count, palette);
    return E_NOTIMPL;
}

static HRESULT WINAPI overlay_GetDefaultColorKey(IOverlay *iface, COLORKEY *key)
{
    FIXME("iface %p, key %p, stub!\n", iface, key);
    return E_NOTIMPL;
}

static HRESULT WINAPI overlay_GetColorKey(IOverlay *iface, COLORKEY *key)
{
    FIXME("iface %p, key %p, stub!\n", iface, key);
    return E_NOTIMPL;
}

static HRESULT WINAPI overlay_SetColorKey(IOverlay *iface, COLORKEY *key)
{
    FIXME("iface %p, key %p, stub!\n", iface, key);
    return E_NOTIMPL;
}

static HRESULT WINAPI overlay_GetWindowHandle(IOverlay *iface, HWND *window)
{
    struct video_renderer *filter = impl_from_IOverlay(iface);

    TRACE("filter %p, window %p.\n", filter, window);

    *window = filter->window.hwnd;
    return S_OK;
}

static HRESULT WINAPI overlay_GetClipList(IOverlay *iface, RECT *source, RECT *dest, RGNDATA **region)
{
    FIXME("iface %p, source %p, dest %p, region %p, stub!\n", iface, source, dest, region);
    return E_NOTIMPL;
}

static HRESULT WINAPI overlay_GetVideoPosition(IOverlay *iface, RECT *source, RECT *dest)
{
    FIXME("iface %p, source %p, dest %p, stub!\n", iface, source, dest);
    return E_NOTIMPL;
}

static HRESULT WINAPI overlay_Advise(IOverlay *iface, IOverlayNotify *sink, DWORD flags)
{
    FIXME("iface %p, sink %p, flags %#lx, stub!\n", iface, sink, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI overlay_Unadvise(IOverlay *iface)
{
    FIXME("iface %p, stub!\n", iface);
    return E_NOTIMPL;
}

static const IOverlayVtbl overlay_vtbl =
{
    overlay_QueryInterface,
    overlay_AddRef,
    overlay_Release,
    overlay_GetPalette,
    overlay_SetPalette,
    overlay_GetDefaultColorKey,
    overlay_GetColorKey,
    overlay_SetColorKey,
    overlay_GetWindowHandle,
    overlay_GetClipList,
    overlay_GetVideoPosition,
    overlay_Advise,
    overlay_Unadvise,
};

HRESULT video_renderer_create(IUnknown *outer, IUnknown **out)
{
    struct video_renderer *object;
    HRESULT hr;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    strmbase_renderer_init(&object->renderer, outer, &CLSID_VideoRenderer, L"In", &renderer_ops);
    wcscpy(object->renderer.sink.pin.name, L"Input");

    object->IOverlay_iface.lpVtbl = &overlay_vtbl;

    video_window_init(&object->window, &IVideoWindow_VTable,
            &object->renderer.filter, &object->renderer.sink.pin, &window_ops);

    if (FAILED(hr = video_window_create_window(&object->window)))
    {
        video_window_cleanup(&object->window);
        strmbase_renderer_cleanup(&object->renderer);
        free(object);
        return hr;
    }

    TRACE("Created video renderer %p.\n", object);
    *out = &object->renderer.filter.IUnknown_inner;
    return S_OK;
}

HRESULT video_renderer_default_create(IUnknown *outer, IUnknown **out)
{
    HRESULT hr;

    if (SUCCEEDED(hr = vmr7_create(outer, out)))
        return hr;

    return video_renderer_create(outer, out);
}
