/*
 * PROJECT:         ReactOS HDAudio Driver
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Internal definitions
 * COPYRIGHT:       Copyright 2025 Johannes Anderwald <johannes.anderwald@reactos.org>
 *                  Copyright 2025-2026 Oleg Dubinskiy <oleg.dubinskiy@reactos.org>
 */

#pragma once

#include <ntddk.h>
#include <ks.h>
#include <portcls.h>
#include <ksmedia.h>
#include <hdaudio.h>
#include <ddk/ntstrsafe.h>

#include "hda_verbs.h"
#include "tables.h"

#define TAG_HDAUDIO 'UADH'
#define HDAUDIO_MAX_FORMAT_RANGES 3

PVOID
__cdecl
operator new(size_t Size, POOL_TYPE PoolType, ULONG Tag);

typedef struct
{
    UCHAR AudioFormatSupported32Bit : 1;
    UCHAR AudioFormatSupported24Bit : 1;
    UCHAR AudioFormatSupported20Bit : 1;
    UCHAR AudioFormatSupported16Bit : 1;
    UCHAR AudioFormatSupported8Bit : 1;
    UCHAR Supported8Khz : 1;
    UCHAR Supported11Khz : 1;
    UCHAR Supported16Khz : 1;
    UCHAR Supported22Khz : 1;
    UCHAR Supported32Khz : 1;
    UCHAR Supported44Khz : 1;
    UCHAR Supported48Khz : 1;
    UCHAR Supported88Khz : 1;
    UCHAR Supported96Khz : 1;
    UCHAR Supported176Khz : 1;
    UCHAR Supported192Khz : 1;
    UCHAR Supported384Khz : 1;
    UCHAR AC3FormatSupported : 1;
    UCHAR Float32FormatSupported : 1;
    UCHAR PCMFormatSupported : 1;
}NODE_PCM_RATES, *PNODE_PCM_RATES;

typedef struct
{
    UCHAR PortConnectivity : 2;
    UCHAR Location : 6;
    UCHAR DefaultDevice : 4;
    UCHAR ConnectionType :4 ;
    UCHAR Color : 4;
    UCHAR Misc : 4;
    UCHAR DefaultAssociation : 4;
    UCHAR Sequence : 4;

}PIN_CONFIGURATION_DEFAULT, *PPIN_CONFIGURATION_DEFAULT;

typedef struct
{
    UCHAR MuteCapable : 1;
    UCHAR Steps : 7;
    UCHAR NumSteps : 7;
    UCHAR Offset : 7;
}AMPLIFIER_CAPABILITIES, *PAMPLIFIER_CAPABILITIES;

typedef struct
{
    UCHAR Output;
    UCHAR Input;
    UCHAR Left;
    UCHAR Right;
    UCHAR Index;
    UCHAR Mute;
    UCHAR Gain;
}AMPLIFIER_GAIN_MUTE_SET, *PAMPLIFIER_GAIN_MUTE_SET;

/* widget types */
#define HDA_NODE_OUTPUT     0x00
#define HDA_NODE_INPUT      0x01
#define HDA_NODE_MIXER      0x02
#define HDA_NODE_SELECTOR   0x03
#define HDA_NODE_PIN        0x04

/* longest path between a pin and a converter */
#define HDA_MAX_PATH_LENGTH 10

typedef struct
{
    LIST_ENTRY ListEntry;
    UCHAR Visited;
    UCHAR Claimed;      /* converter used by a filter */
    UCHAR Digital;
    UCHAR NodeId;
    UCHAR PinDefaultAssociation;
    UCHAR ChannelCount;
    UCHAR NodeType;
    UCHAR UnsolCapable;
    UCHAR ConnectionCount;
    USHORT Connections[1];
}NODE_CONTEXT, *PNODE_CONTEXT;

typedef struct
{
    UCHAR EAPDCapable : 1;
    UCHAR InputCapable : 1;
    UCHAR OutputCapable : 1;
    UCHAR HeadphoneDriveCapable : 1;
    UCHAR PresenceDetectCapable : 1;
} PIN_CAPABILITIES, *PPIN_CAPABILITIES;

#define HDAUDIO_MAX_WAVE_PINS \
    (RTL_NUMBER_OF(WaveInPins) > RTL_NUMBER_OF(WaveOutPins) ? RTL_NUMBER_OF(WaveInPins) : RTL_NUMBER_OF(WaveOutPins))

/* one wave / topology filter pair, shared by its miniports */
typedef struct _HDAUDIO_FILTER_CONTEXT
{
    volatile LONG Ref;
    BOOLEAN Capture;
    ULONG PinCount;
    PULONG Pins;                /* pin widgets, in sequence order */
    ULONG ConverterCount;
    PULONG Converters;          /* converters connected to the pins */
    KSDATARANGE_AUDIO DataRange[HDAUDIO_MAX_FORMAT_RANGES];
    PKSDATARANGE DataRanges[HDAUDIO_MAX_FORMAT_RANGES];
    PCPIN_DESCRIPTOR WavePins[HDAUDIO_MAX_WAVE_PINS];
    PCFILTER_DESCRIPTOR WaveDescription;
} HDAUDIO_FILTER_CONTEXT, *PHDAUDIO_FILTER_CONTEXT;

PHDAUDIO_FILTER_CONTEXT
HDAUDIO_AllocateFilterContext(
    IN BOOLEAN Capture,
    IN ULONG PinCount,
    IN PULONG Pins,
    IN ULONG ConverterCount,
    IN PULONG Converters);

VOID
HDAUDIO_ReferenceFilterContext(
    IN PHDAUDIO_FILTER_CONTEXT Context);

VOID
HDAUDIO_ReleaseFilterContext(
    IN PHDAUDIO_FILTER_CONTEXT Context);

template <typename... Interfaces> class CUnknownImpl : public Interfaces...
{
  private:
    volatile LONG m_Ref;

  protected:
    CUnknownImpl() : m_Ref(0)
    {
    }
    virtual ~CUnknownImpl()
    {
    }

  public:
    STDMETHODIMP_(ULONG) AddRef()
    {
        ULONG Ref = InterlockedIncrement(&m_Ref);
        ASSERT(Ref < 0x10000);
        return Ref;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        ULONG Ref = InterlockedDecrement(&m_Ref);
        ASSERT(Ref < 0x10000);
        if (!Ref)
        {
            delete this;
            return 0;
        }
        return Ref;
    }
};

class CAdapterCommon : public CUnknownImpl<IAdapterPowerManagement>
{
  public:
    STDMETHODIMP QueryInterface(REFIID InterfaceId, PVOID *Interface);
    NTSTATUS NTAPI Initialize(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PRESOURCELIST ResourceList);
    NTSTATUS NTAPI AcquireBusInterface(IN PDEVICE_OBJECT DeviceObject);
    VOID NTAPI GetResourceInformation();
    NTSTATUS NTAPI GetHardwareIds();
    NTSTATUS NTAPI TransferInitVerbs(IN PDEVICE_OBJECT DeviceObject);
    NTSTATUS NTAPI ReadRegistryKey(IN PDEVICE_OBJECT DeviceObject, IN LPWSTR KeyName, IN ULONG BufferLength, OUT PREGISTRYKEY * ParentKey, OUT PULONG Type, OUT PVOID Value);
    NTSTATUS NTAPI TransferVerb(ULONG Verb, PULONG Response);
    NTSTATUS NTAPI InstallDevice(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp,
        IN LPWSTR Name,
        IN BOOLEAN bTopology,
        IN REFGUID PortClassId,
        IN PRESOURCELIST ResourceList,
        IN PHDAUDIO_FILTER_CONTEXT FilterContext,
        IN PVOID FunctionGroupNode,
        IN PPCFILTER_DESCRIPTOR FilterDescription,
        OUT PUNKNOWN *OutPortUnknown);
    NTSTATUS
    NTAPI
    ChooseSubdeviceName(
        UCHAR DefaultDevice,
        UCHAR Instance,
        UCHAR Wave,
        OUT LPWSTR *OutSubdeviceName);
    NTSTATUS
    NTAPI
    BuildWaveFormat(
        IN PVOID Node,
        IN PHDAUDIO_FILTER_CONTEXT FilterContext);
    NTSTATUS
    NTAPI
    SetupPath(
        IN PVOID Node,
        IN BOOLEAN bOutput,
        IN ULONG PinNodeId,
        IN UCHAR DefaultDevice,
        IN UCHAR Digital,
        OUT PULONG ConverterNodeId);
    NTSTATUS
    NTAPI
    BuildInstallFilter(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp,
        IN ULONG bOutput,
        IN PVOID OutNode,
        IN PRESOURCELIST ResourceList,
        IN ULONG AssociatedPinCount,
        IN PULONG AssociatedPins,
        IN UCHAR DefaultDevice,
        IN UCHAR Digital);
    VOID NTAPI ClearRef(IN ULONG RefValue, IN ULONG NodeCount, IN PULONG Nodes);
    NTSTATUS
    NTAPI
    ProcessOutputNodes(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp,
        IN PRESOURCELIST ResourceList,
        IN ULONG PinNodeCount,
        IN PULONG PinNodes,
        IN PVOID Node);
    NTSTATUS
    NTAPI
    ProcessInputNodes(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp,
        IN PRESOURCELIST ResourceList,
        IN ULONG PinNodeCount,
        IN PULONG PinNodes,
        IN PVOID Node);

    NTSTATUS
    NTAPI
    AssociatePins(
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp,
        IN PRESOURCELIST ResourceList,
        IN ULONG PinNodeId,
        IN PPIN_CONFIGURATION_DEFAULT PinConfiguration,
        IN PVOID Node,
        IN ULONG PinNodeCount,
        IN PULONG PinNodes,
        IN UCHAR Digital);

    NTSTATUS
    NTAPI
    ExecuteVendorSpecificQuirks(
        IN PVOID Node);

    NTSTATUS NTAPI
    GetInterface(PHDAUDIO_BUS_INTERFACE_V2 Interface);

    UCHAR
    GetCodecAddress()
    {
        return m_CodecAddress;
    }

    IMP_IAdapterPowerManagement;
    CAdapterCommon(IUnknown* OuterUnknown)
    {
        RtlZeroMemory(m_SubdeviceCount, sizeof(m_SubdeviceCount));
    }
    virtual ~CAdapterCommon()
    {
    }

    friend NTSTATUS NTAPI PropertyHandler_JackDescription(IN PPCPROPERTY_REQUEST PropertyRequest);
    friend NTSTATUS NTAPI PropertyHandler_ChannelConfig(IN PPCPROPERTY_REQUEST PropertyRequest);
    friend NTSTATUS NTAPI PropertyHandler_SpeakerGeometry(IN PPCPROPERTY_REQUEST PropertyRequest);
    friend NTSTATUS NTAPI PropertyHandler_InputVolume(IN PPCPROPERTY_REQUEST PropertyRequest);
    friend NTSTATUS NTAPI PropertyHandler_OutputVolume(IN PPCPROPERTY_REQUEST PropertyRequest);
    friend NTSTATUS NTAPI PropertyHandler_InputMute(IN PPCPROPERTY_REQUEST PropertyRequest);
    friend NTSTATUS NTAPI PropertyHandler_OutputMute(IN PPCPROPERTY_REQUEST PropertyRequest);
    friend NTSTATUS NTAPI EventHandler_Volume(IN PPCEVENT_REQUEST EventRequest);

  private:
    HDAUDIO_BUS_INTERFACE_V2 Interface;
    UCHAR m_CodecAddress;
    UCHAR m_FunctionGroupStartNode;
    UINT16 DeviceId;
    UINT16 RevisionId;
    UINT16 VendorId;
    UCHAR m_Tags[64];
    ULONG m_TagIndex;
    UCHAR m_SubdeviceCount[16];     /* filters per default device type, keeps names unique */
};

class CFunctionGroupNode
{
  public:
    CFunctionGroupNode(UCHAR CodecAddress, UCHAR NodeId, CAdapterCommon *AdapterCommon)
        : m_Adapter(AdapterCommon), m_StartNodeId(NodeId), m_CodecAddress(CodecAddress), m_SubNodeCount(0)
    {
        InitializeListHead(&m_Nodes);
    }
    virtual ~CFunctionGroupNode()
    {
        while (!IsListEmpty(&m_Nodes))
        {
            PLIST_ENTRY Entry = RemoveHeadList(&m_Nodes);
            PNODE_CONTEXT NodeContext = CONTAINING_RECORD(Entry, NODE_CONTEXT, ListEntry);
            ExFreePoolWithTag(NodeContext, TAG_HDAUDIO);
        }
    }

    NTSTATUS NTAPI QueryWidgetCount();
    NTSTATUS NTAPI EnumWidgets();
    NTSTATUS NTAPI EnumConnections();
    VOID NTAPI PrintNodeInfo(UCHAR NodeType, ULONG NodeId);
    NTSTATUS NTAPI GetNodesWithType(IN UCHAR NodeType, OUT PULONG NodeCount, OUT PULONG *NodesAddress);
    NTSTATUS NTAPI GetSupportedPCMSizeRates(ULONG NodeId, OUT PNODE_PCM_RATES OutRates);
    NTSTATUS NTAPI GetPinConfigurationDefault(IN ULONG NodeId, IN PPIN_CONFIGURATION_DEFAULT PinConfiguration);
    NTSTATUS NTAPI AddNode(ULONG NodeId);
    PNODE_CONTEXT NTAPI FindNodeId(ULONG NodeId);
    NTSTATUS NTAPI GetAmplifierDetails(IN ULONG NodeId, IN ULONG Input, OUT PAMPLIFIER_CAPABILITIES Caps);
    NTSTATUS NTAPI GetAmplifierGainMute(IN ULONG NodeId, IN UCHAR Input, IN UCHAR Left, IN UCHAR Index, OUT PUCHAR Mute, OUT PUCHAR Gain);
    NTSTATUS NTAPI SetAmplifierGainMute(IN ULONG NodeId, IN PAMPLIFIER_GAIN_MUTE_SET Set);
    NTSTATUS NTAPI GetPinCapabilities(IN ULONG NodeId, OUT PPIN_CAPABILITIES PinCaps);
    NTSTATUS NTAPI GetPinSense(IN ULONG NodeId, IN PULONG DevicePresent);
    NTSTATUS NTAPI GetVolumeKnob(IN ULONG NodeId, OUT PUCHAR Direct, OUT PUCHAR Volume);
    NTSTATUS NTAPI SetVolumeKnob(IN ULONG NodeId, IN UCHAR Direct, IN UCHAR Volume);
    NTSTATUS NTAPI GetConnectionIndex(IN ULONG NodeId, OUT PUCHAR Index);
    NTSTATUS NTAPI SetConnectionIndex(IN ULONG NodeId, IN UCHAR Index);
    NTSTATUS NTAPI GetVolumeCapabilities(IN ULONG NodeId, IN PUCHAR Delta, IN PUCHAR NumSteps);
    NTSTATUS NTAPI GetPinNodesWithDefaultAssociation(IN UCHAR DefaultAssociation, IN UCHAR Digital, OUT PULONG NodeCount, OUT PULONG * Nodes);
    VOID NTAPI ClearVisitedState();

    NTSTATUS
    NTAPI
    GetFunctionGroupType(
        IN ULONG NodeId,
        OUT PUCHAR UnsolCapable,
        OUT PUCHAR NodeType);

    NTSTATUS
    NTAPI
    EnableUnsolicitedResponse(
        IN ULONG NodeId,
        IN UCHAR Enable);

    NTSTATUS
    NTAPI
    SetPowerState(IN ULONG NodeId, IN UCHAR Enable);

    NTSTATUS
    NTAPI
    SetPinWidgetControl(
        IN ULONG NodeId,
        IN UCHAR HPhnEnable,
        IN UCHAR OutEnable,
        IN UCHAR InEnable);

    NTSTATUS
    NTAPI
    GetPinWidgetControlValue(IN ULONG NodeId, OUT PUCHAR Value);

    NTSTATUS
    NTAPI
    SetPinWidgetControlValue(IN ULONG NodeId, IN UCHAR Value);

    NTSTATUS
    NTAPI
    SetStreamFormat(IN ULONG NodeId, IN USHORT Format);

    NTSTATUS
    NTAPI
    SetConverterStream(IN ULONG NodeId, IN UCHAR StreamId, IN UCHAR Channel);

    NTSTATUS
    NTAPI
    GetEAPD(IN ULONG NodeId, OUT PUCHAR Value);

    NTSTATUS
    NTAPI
    SetEAPD(IN ULONG NodeId, IN UCHAR Enable);

    NTSTATUS
    NTAPI
    SetEAPDPower(IN UCHAR Value);

    NTSTATUS
    NTAPI
    GetGPIO(
        IN ULONG NodeId,
        OUT PULONG GPIOData,
        OUT PULONG GPIOEnableMask,
        OUT PULONG GPIODirection);

    NTSTATUS
    NTAPI
    SetGPIO(
        IN ULONG NodeId,
        IN ULONG Delay,
        IN ULONG GPIOData,
        IN ULONG GPIOEnableMask,
        IN ULONG GPIODirection);

    NTSTATUS
    NTAPI
    SetCustomVerb(
        IN ULONG NodeId,
        IN ULONG VerbId,
        IN UCHAR Value);

    UCHAR
    GetStripeBit()
    {
        return m_Stripe;
    }

    VOID
    SetStripeBit(UCHAR Stripe)
    {
        m_Stripe = Stripe;
    }

    AMPLIFIER_CAPABILITIES
    GetCachedAmplifierCapabilities()
    {
        return m_CachedAmplifierCapabilities;
    }

    VOID
    SetCachedAmplifierCapabilities(PAMPLIFIER_CAPABILITIES AmplifierCapabilities)
    {
        m_CachedAmplifierCapabilities = *AmplifierCapabilities;
    }

    UCHAR
    GetCachedInputGain()
    {
        return m_CachedInputGain;
    }

    VOID
    SetCachedInputGain(UCHAR Gain)
    {
        m_CachedInputGain = Gain;
    }

    UCHAR
    GetCachedInputMute()
    {
        return m_CachedInputMute;
    }

    VOID
    SetCachedInputMute(UCHAR Mute)
    {
        m_CachedInputMute = Mute;
    }

    UCHAR
    GetCachedOutputGain()
    {
        return m_CachedOutputGain;
    }

    VOID
    SetCachedOutputGain(UCHAR Gain)
    {
        m_CachedOutputGain = Gain;
    }

    UCHAR
    GetCachedOutputMute()
    {
        return m_CachedOutputMute;
    }

    VOID
    SetCachedOutputMute(UCHAR Mute)
    {
        m_CachedOutputMute = Mute;
    }

  private:
    CAdapterCommon *m_Adapter;
    BOOLEAN m_Stripe;
    UCHAR m_StartNodeId;
    UCHAR m_CodecAddress;
    ULONG m_StartSubNode;
    ULONG m_SubNodeCount;
    LIST_ENTRY m_Nodes;
    AMPLIFIER_CAPABILITIES m_CachedAmplifierCapabilities;
    UCHAR m_CachedInputGain;
    UCHAR m_CachedInputMute;
    UCHAR m_CachedOutputGain;
    UCHAR m_CachedOutputMute;
};

class CMiniportTopology : public CUnknownImpl<IMiniportTopology>
{
  public:
    STDMETHODIMP QueryInterface(REFIID InterfaceId, PVOID *Interface);
    IMP_IMiniportTopology;
    CMiniportTopology(
        IUnknown *OuterUnknown,
        PHDAUDIO_FILTER_CONTEXT FilterContext,
        CFunctionGroupNode *Node,
        CAdapterCommon *Adapter,
        PPCFILTER_DESCRIPTOR Filter)
        : m_Port(0), m_PortEvents(0), m_FilterContext(FilterContext), m_Node(Node),
          m_Adapter(Adapter), m_FilterDescription(Filter)
    {
        HDAUDIO_ReferenceFilterContext(m_FilterContext);
    }
    virtual ~CMiniportTopology()
    {
        if (m_PortEvents)
            m_PortEvents->Release();
        if (m_Port)
            m_Port->Release();
        HDAUDIO_ReleaseFilterContext(m_FilterContext);
    }

    VOID NTAPI AddEventToEventList(IN PKSEVENT_ENTRY EventEntry);
    VOID NTAPI GenerateEventList(_In_opt_ GUID *Set,
                                 _In_ ULONG EventId,
                                 _In_ BOOL PinEvent,
                                 _In_ ULONG PinId,
                                 _In_ BOOL NodeEvent,
                                 _In_ ULONG NodeId);

    CFunctionGroupNode*
    GetNode()
    {
        return m_Node;
    }

  private:
    PPORTTOPOLOGY m_Port;
    PPORTEVENTS m_PortEvents;
    PHDAUDIO_FILTER_CONTEXT m_FilterContext;
    CFunctionGroupNode *m_Node;
    CAdapterCommon *m_Adapter;
    PPCFILTER_DESCRIPTOR m_FilterDescription;
    PIN_CONFIGURATION_DEFAULT m_PinConfiguration;
    HDAUDIO_BUS_INTERFACE_V2 m_Interface;
};

class CMiniportWaveRT : public CUnknownImpl<IMiniportWaveRT>
{
  public:
    STDMETHODIMP QueryInterface(REFIID InterfaceId, PVOID *Interface);
    IMP_IMiniportWaveRT;
    CMiniportWaveRT(
        IUnknown *OuterUnknown,
        PHDAUDIO_FILTER_CONTEXT FilterContext,
        CFunctionGroupNode *Node,
        CAdapterCommon *Adapter,
        PPCFILTER_DESCRIPTOR Filter)
        : m_Port(0), m_FilterContext(FilterContext), m_Node(Node),
          m_Adapter(Adapter), m_FilterDescription(Filter)
    {
        HDAUDIO_ReferenceFilterContext(m_FilterContext);
    }
    virtual ~CMiniportWaveRT()
    {
        if (m_Port)
            m_Port->Release();
        HDAUDIO_ReleaseFilterContext(m_FilterContext);
    }

    CFunctionGroupNode*
    GetNode()
    {
        return m_Node;
    }

  private:
    PPORTWAVERT m_Port;
    PHDAUDIO_FILTER_CONTEXT m_FilterContext;
    CFunctionGroupNode *m_Node;
    CAdapterCommon *m_Adapter;
    PPCFILTER_DESCRIPTOR m_FilterDescription;
    PIN_CONFIGURATION_DEFAULT m_PinConfiguration;
    HDAUDIO_BUS_INTERFACE_V2 m_Interface;
};

class CMiniportWaveRTStream : public CUnknownImpl<IMiniportWaveRTStreamNotification>
{
  public:
    STDMETHODIMP
    QueryInterface(REFIID InterfaceId, PVOID *Interface);

    CMiniportWaveRTStream(IUnknown *OuterUnknown, CAdapterCommon * Adapter, CFunctionGroupNode * OutNode,
        ULONG Pin,
        BOOLEAN Capture,
        PHDAUDIO_STREAM_FORMAT StreamFormat,
        PHDAUDIO_CONVERTER_FORMAT Converter,
        PHDAUDIO_BUS_INTERFACE_V2 Interface,
        HANDLE DmaEngine,
        ULONG NodeCount,
        PULONG Nodes,
        PPCFILTER_DESCRIPTOR FilterDescription)
        : m_Adapter(Adapter), m_OutNode(OutNode), m_Pin(Pin), m_Capture(Capture), m_DmaEngine(DmaEngine),
          m_NodeCount(NodeCount), m_Nodes(Nodes), m_FilterDescription(FilterDescription)
    {
       RtlCopyMemory(&m_Interface, Interface, sizeof(HDAUDIO_BUS_INTERFACE_V2));
       RtlCopyMemory(&m_StreamFormat, StreamFormat, sizeof(HDAUDIO_STREAM_FORMAT));
       RtlCopyMemory(&m_Converter, Converter, sizeof(HDAUDIO_CONVERTER_FORMAT));
    }
    virtual ~CMiniportWaveRTStream()
    {
        /* detach the converters from the stream before its tag is reused */
        for (ULONG Index = 0; Index < m_NodeCount; Index++)
            m_OutNode->SetConverterStream(m_Nodes[Index], 0, 0);

        m_Interface.FreeDmaEngine(m_Interface.Context, m_DmaEngine);

        if (m_Nodes)
            ExFreePoolWithTag(m_Nodes, TAG_HDAUDIO);
    }

    IMP_IMiniportWaveRTStreamNotification;

  private:
    CAdapterCommon *m_Adapter;
    CFunctionGroupNode *m_OutNode;
    ULONG m_Pin;
    BOOLEAN m_Capture;
    HDAUDIO_STREAM_FORMAT m_StreamFormat;
    HDAUDIO_CONVERTER_FORMAT m_Converter;
    HANDLE m_DmaEngine;
    HDAUDIO_BUS_INTERFACE_V2 m_Interface;
    UCHAR m_StreamId;
    ULONG m_FifoSize;
    ULONG m_NodeCount;
    PULONG m_Nodes;
    PPCFILTER_DESCRIPTOR m_FilterDescription;
};

NTSTATUS
HDAUDIO_AllocateCommonAdapter(
    OUT CAdapterCommon ** OutAdapter);

NTSTATUS
HDAUDIO_InitializeCommonAdapter(
    CAdapterCommon* CommonAdapter,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PRESOURCELIST ResourceList
);

typedef struct
{
    ULONG_PTR Reserved[4]; // used by portcls
    CAdapterCommon *AdapterCommon;
}HDAUDIO_DEVICE_EXTENSION, *PHDAUDIO_DEVICE_EXTENSION;

NTSTATUS
HDAUDIO_NewMiniportTopology(
    OUT PMINIPORTTOPOLOGY* OutMiniport,
    IN PHDAUDIO_FILTER_CONTEXT FilterContext,
    IN CFunctionGroupNode * Node,
    IN CAdapterCommon * Adapter,
    IN PPCFILTER_DESCRIPTOR FilterDescription);

NTSTATUS
HDAUDIO_NewMiniportWaveRT(
    OUT PMINIPORTWAVERT *OutMiniport,
    IN PHDAUDIO_FILTER_CONTEXT FilterContext,
    IN CFunctionGroupNode * Node,
    IN CAdapterCommon * Adapter,
    IN PPCFILTER_DESCRIPTOR FilterDescription);

NTSTATUS
HDAUDIO_EnumFunctionGroupWidgets(
    IN UCHAR CodecAddress,
    IN UCHAR FunctionGroupStartNodeId,
    IN CAdapterCommon *Adapter,
    OUT CFunctionGroupNode **OutFunctionGroupNode);

NTSTATUS
HDAUDIO_AllocateStream(
    OUT PMINIPORTWAVERTSTREAM *Stream,
    IN CAdapterCommon *Adapter,
    IN CFunctionGroupNode *Node,
    IN ULONG Pin,
    IN BOOLEAN Capture,
    IN PKSDATAFORMAT DataFormat,
    IN ULONG NodeCount,
    IN PULONG Nodes,
    IN PPCFILTER_DESCRIPTOR FilterDescription);

NTSTATUS
HDAUDIO_ValidateDataFormat(
    IN PKSDATAFORMAT DataFormat,
    IN PKSDATARANGE_AUDIO PinRange,
    OUT PHDAUDIO_STREAM_FORMAT StreamFormat OPTIONAL);

NTSTATUS
HDAUDIO_ValidateNodeFormats(
    IN CFunctionGroupNode *Node,
    IN ULONG NodeCount,
    IN PULONG Nodes,
    IN PHDAUDIO_STREAM_FORMAT StreamFormat);
