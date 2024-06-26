/*
 * Copyright 2011 Joakim Sindholt <opensource@zhasha.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef _NINE_ADAPTER9_H_
#define _NINE_ADAPTER9_H_

#include "iunknown.h"

#include "d3dadapter/d3dadapter9.h"

struct pipe_screen;
struct pipe_resource;

struct d3dadapter9_context
{
    struct pipe_screen *hal, *ref;
    D3DADAPTER_IDENTIFIER9 identifier;
    BOOL linear_framebuffer;
    BOOL throttling;
    int throttling_value;
    int vblank_mode;
    BOOL thread_submit;
    BOOL discard_delayed_release;
    BOOL tearfree_discard;
    int csmt_force;
    BOOL dynamic_texture_workaround;
    BOOL shader_inline_constants;
    int memfd_virtualsizelimit;
    int override_vram_size;
    BOOL force_emulation;

    void (*destroy)( struct d3dadapter9_context *ctx );
};

struct NineAdapter9
{
    struct NineUnknown base;
    
    struct d3dadapter9_context *ctx;
};
static inline struct NineAdapter9 *
NineAdapter9( void *data )
{
    return (struct NineAdapter9 *)data;
}

HRESULT
NineAdapter9_new( struct d3dadapter9_context *pCTX,
                  struct NineAdapter9 **ppOut );

HRESULT
NineAdapter9_ctor( struct NineAdapter9 *This,
                   struct NineUnknownParams *pParams,
                   struct d3dadapter9_context *pCTX );

void
NineAdapter9_dtor( struct NineAdapter9 *This );

HRESULT NINE_WINAPI
NineAdapter9_GetAdapterIdentifier( struct NineAdapter9 *This,
                                   DWORD Flags,
                                   D3DADAPTER_IDENTIFIER9 *pIdentifier );

HRESULT NINE_WINAPI
NineAdapter9_CheckDeviceType( struct NineAdapter9 *This,
                              D3DDEVTYPE DevType,
                              D3DFORMAT AdapterFormat,
                              D3DFORMAT BackBufferFormat,
                              BOOL bWindowed );

HRESULT NINE_WINAPI
NineAdapter9_CheckDeviceFormat( struct NineAdapter9 *This,
                                D3DDEVTYPE DeviceType,
                                D3DFORMAT AdapterFormat,
                                DWORD Usage,
                                D3DRESOURCETYPE RType,
                                D3DFORMAT CheckFormat );

HRESULT NINE_WINAPI
NineAdapter9_CheckDeviceMultiSampleType( struct NineAdapter9 *This,
                                         D3DDEVTYPE DeviceType,
                                         D3DFORMAT SurfaceFormat,
                                         BOOL Windowed,
                                         D3DMULTISAMPLE_TYPE MultiSampleType,
                                         DWORD *pQualityLevels );

HRESULT NINE_WINAPI
NineAdapter9_CheckDepthStencilMatch( struct NineAdapter9 *This,
                                     D3DDEVTYPE DeviceType,
                                     D3DFORMAT AdapterFormat,
                                     D3DFORMAT RenderTargetFormat,
                                     D3DFORMAT DepthStencilFormat );

HRESULT NINE_WINAPI
NineAdapter9_CheckDeviceFormatConversion( struct NineAdapter9 *This,
                                          D3DDEVTYPE DeviceType,
                                          D3DFORMAT SourceFormat,
                                          D3DFORMAT TargetFormat );

HRESULT NINE_WINAPI
NineAdapter9_GetDeviceCaps( struct NineAdapter9 *This,
                            D3DDEVTYPE DeviceType,
                            D3DCAPS9 *pCaps );

HRESULT NINE_WINAPI
NineAdapter9_CreateDevice( struct NineAdapter9 *This,
                           UINT RealAdapter,
                           D3DDEVTYPE DeviceType,
                           HWND hFocusWindow,
                           DWORD BehaviorFlags,
                           D3DPRESENT_PARAMETERS *pPresentationParameters,
                           IDirect3D9 *pD3D9,
                           ID3DPresentGroup *pPresentationGroup,
                           IDirect3DDevice9 **ppReturnedDeviceInterface );

HRESULT NINE_WINAPI
NineAdapter9_CreateDeviceEx( struct NineAdapter9 *This,
                             UINT RealAdapter,
                             D3DDEVTYPE DeviceType,
                             HWND hFocusWindow,
                             DWORD BehaviorFlags,
                             D3DPRESENT_PARAMETERS *pPresentationParameters,
                             D3DDISPLAYMODEEX *pFullscreenDisplayMode,
                             IDirect3D9Ex *pD3D9Ex,
                             ID3DPresentGroup *pPresentationGroup,
                             IDirect3DDevice9Ex **ppReturnedDeviceInterface );

#endif /* _NINE_ADAPTER9_H_ */
