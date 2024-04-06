#ifndef __ADSP_INTERFACE
#define __ADSP_INTERFACE
#include <hdaudio.h>

//
// The ADSP_BUS_INTERFACE interface GUID
//
// {752A2CAE-3455-4D18-A184-8B34B22632CD}
DEFINE_GUID(GUID_ADSP_BUS_INTERFACE,
    0x752a2cae, 0x3455, 0x4d18, 0xa1, 0x84, 0x8b, 0x34, 0xb2, 0x26, 0x32, 0xcd);

typedef struct _NHLT_INFO {
    PVOID nhlt;
    UINT64 nhltSz;
} NHLT_INFO, *PNHLT_INFO;

typedef _Must_inspect_result_ NTSTATUS(NTAPI *PGET_ADSP_RESOURCES) (_In_ PVOID _context, _Out_ _PCI_BAR* hdaBar, _Out_ _PCI_BAR* adspBar, PVOID* ppcap, PNHLT_INFO nhltInfo, _Out_ BUS_INTERFACE_STANDARD* pciConfig);
typedef _Must_inspect_result_ BOOL(NTAPI *PADSP_INTERRUPT_CALLBACK)(PVOID context);
typedef _Must_inspect_result_ NTSTATUS(NTAPI *PREGISTER_ADSP_INTERRUPT) (_In_ PVOID _context, _In_ PADSP_INTERRUPT_CALLBACK callback, _In_ PVOID callbackContext);
typedef _Must_inspect_result_ NTSTATUS(NTAPI *PUNREGISTER_ADSP_INTERRUPT) (_In_ PVOID _context);
typedef _Must_inspect_result_ NTSTATUS(NTAPI *PGET_STREAM)(_In_ PVOID _context, HDAUDIO_STREAM_FORMAT StreamFormat, PHANDLE Handle, _Out_ UINT8 *streamTag);
typedef _Must_inspect_result_ NTSTATUS(NTAPI *PFREE_STREAM)(_In_ PVOID _context, _In_ HANDLE Handle);
typedef _Must_inspect_result_ NTSTATUS(NTAPI *PDSP_PREPARE_STREAM)(_In_ PVOID _context, _In_ HANDLE Handle, _In_ unsigned int ByteSize, _In_ int NumBlocks, _Out_ PVOID* bdlBuf);
typedef _Must_inspect_result_ NTSTATUS(NTAPI *PDSP_CLEANUP_STREAM)(_In_ PVOID _context, _In_ HANDLE Handle);
typedef _Must_inspect_result_ UINT32(NTAPI *PDSP_STREAM_POSITION)(_In_ PVOID _context, _In_ HANDLE Handle);
typedef void (NTAPI *PDSP_START_STOP_STREAM)(_In_ PVOID _context, _In_ HANDLE Handle, BOOL startStop);
typedef void (NTAPI *PDSP_ENABLE_SPIB)(_In_ PVOID _context, _In_ HANDLE Handle, UINT32 value);
typedef void (NTAPI *PDSP_DISABLE_SPIB)(_In_ PVOID _context, _In_ HANDLE Handle);

typedef _Must_inspect_result_ NTSTATUS(NTAPI *PDSP_GET_LINK)(_In_ PVOID _context, _In_ int addr, _Out_ PHANDLE Handle);
typedef void (NTAPI *PDSP_LINK_STREAM)(_In_ HANDLE Handle, _In_ int stream);

typedef struct _ADSP_BUS_INTERFACE
{
    //
    // First we define the standard INTERFACE structure ...
    //
    USHORT                    Size;
    USHORT                    Version;
    PVOID                     Context;
    PINTERFACE_REFERENCE      InterfaceReference;
    PINTERFACE_DEREFERENCE    InterfaceDereference;

    //
    // Then we expand the structure with the ADSP_BUS_INTERFACE stuff.

    UINT16 CtlrDevId;
    PGET_ADSP_RESOURCES           GetResources;
    PREGISTER_ADSP_INTERRUPT      RegisterInterrupt;
    PUNREGISTER_ADSP_INTERRUPT    UnregisterInterrupt;
    PGET_STREAM                   GetRenderStream;
    PGET_STREAM                   GetCaptureStream;
    PFREE_STREAM                  FreeStream;
    PDSP_PREPARE_STREAM           PrepareDSP;
    PDSP_CLEANUP_STREAM           CleanupDSP;
    PDSP_START_STOP_STREAM        TriggerDSP;
    PDSP_STREAM_POSITION          StreamPosition;

    PDSP_ENABLE_SPIB              DSPEnableSPIB;
    PDSP_DISABLE_SPIB             DSPDisableSPIB;
} ADSP_BUS_INTERFACE, * PADSP_BUS_INTERFACE;

#ifndef ADSP_DECL
ADSP_BUS_INTERFACE ADSP_BusInterface(PVOID Context);
#endif
#endif
