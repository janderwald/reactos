1 stdcall accept(long ptr ptr)
2 stdcall bind(long ptr long)
3 stdcall closesocket(long)
4 stdcall connect(long ptr long)
5 stdcall getpeername(long ptr ptr)
6 stdcall getsockname(long ptr ptr)
7 stdcall getsockopt(long long long ptr ptr)
8 stdcall htonl(long)
9 stdcall htons(long)
10 stdcall ioctlsocket(long long ptr)
11 stdcall inet_addr(str)
12 stdcall inet_ntoa(ptr)
13 stdcall listen(long long)
14 stdcall ntohl(long)
15 stdcall ntohs(long)
16 stdcall recv(long ptr long long)
17 stdcall recvfrom(long ptr long long ptr ptr)
18 stdcall select(long ptr ptr ptr ptr)
19 stdcall send(long ptr long long)
20 stdcall sendto(long ptr long long ptr long)
21 stdcall setsockopt(long long long ptr long)
22 stdcall shutdown(long long)
23 stdcall socket(long long long)
24 stdcall WSApSetPostRoutine(ptr)
25 stdcall FreeAddrInfoW(ptr) freeaddrinfo
26 stdcall GetAddrInfoW(wstr wstr ptr ptr)
27 stdcall GetNameInfoW(ptr long wstr long wstr long long)
28 stdcall WPUCompleteOverlappedRequest(long ptr long long ptr)
29 stdcall WSAAccept(long ptr ptr ptr long)
30 stdcall WSAAddressToStringA(ptr long ptr ptr ptr)
31 stdcall WSAAddressToStringW(ptr long ptr ptr ptr)
32 stdcall WSACloseEvent(long)
33 stdcall WSAConnect(long ptr long ptr ptr ptr ptr)
34 stdcall WSACreateEvent ()
35 stdcall WSADuplicateSocketA(long long ptr)
36 stdcall WSADuplicateSocketW(long long ptr)
37 stdcall WSAEnumNameSpaceProvidersA(ptr ptr)
38 stdcall WSAEnumNameSpaceProvidersW(ptr ptr)
39 stdcall WSAEnumNetworkEvents(long long ptr)
40 stdcall WSAEnumProtocolsA(ptr ptr ptr)
41 stdcall WSAEnumProtocolsW(ptr ptr ptr)
42 stdcall WSAEventSelect(long long long)
43 stdcall WSAGetOverlappedResult(long ptr ptr long ptr)
44 stdcall WSAGetQOSByName(long ptr ptr)
45 stdcall WSAGetServiceClassInfoA(ptr ptr ptr ptr)
46 stdcall WSAGetServiceClassInfoW(ptr ptr ptr ptr)
47 stdcall WSAGetServiceClassNameByClassIdA(ptr ptr ptr)
48 stdcall WSAGetServiceClassNameByClassIdW(ptr ptr ptr)
49 stdcall WSAHtonl(long long ptr)
50 stdcall WSAHtons(long long ptr)
51 stdcall gethostbyaddr(ptr long long)
52 stdcall gethostbyname(str)
53 stdcall getprotobyname(str)
54 stdcall getprotobynumber(long)
55 stdcall getservbyname(str str)
56 stdcall getservbyport(long str)
57 stdcall gethostname(ptr long)
58 stdcall WSAInstallServiceClassA(ptr)
59 stdcall WSAInstallServiceClassW(ptr)
60 stdcall WSAIoctl(long long ptr long ptr long ptr ptr ptr)
61 stdcall WSAJoinLeaf(long ptr long ptr ptr ptr ptr long)
62 stdcall WSALookupServiceBeginA(ptr long ptr)
63 stdcall WSALookupServiceBeginW(ptr long ptr)
64 stdcall WSALookupServiceEnd(long)
65 stdcall WSALookupServiceNextA(long long ptr ptr)
66 stdcall WSALookupServiceNextW(long long ptr ptr)
67 stdcall WSANSPIoctl(ptr long ptr long ptr long ptr ptr)
68 stdcall WSANtohl(long long ptr)
69 stdcall WSANtohs(long long ptr)
70 stdcall WSAProviderConfigChange(ptr ptr ptr)
71 stdcall WSARecv(long ptr long ptr ptr ptr ptr)
72 stdcall WSARecvDisconnect(long ptr)
73 stdcall WSARecvFrom(long ptr long ptr ptr ptr ptr ptr ptr )
74 stdcall WSARemoveServiceClass(ptr)
75 stdcall WSAResetEvent(long) kernel32.ResetEvent
76 stdcall WSASend(long ptr long ptr long ptr ptr)
77 stdcall WSASendDisconnect(long ptr)
78 stdcall WSASendTo(long ptr long ptr long ptr long ptr ptr)
79 stdcall WSASetEvent(long) kernel32.SetEvent
80 stdcall WSASetServiceA(ptr long long)
81 stdcall WSASetServiceW(ptr long long)
82 stdcall WSASocketA(long long long ptr long long)
83 stdcall WSASocketW(long long long ptr long long)
84 stdcall WSAStringToAddressA(str long ptr ptr ptr)
85 stdcall WSAStringToAddressW(wstr long ptr ptr ptr)
86 stdcall WSAWaitForMultipleEvents(long ptr long long long) kernel32.WaitForMultipleObjectsEx
87 stdcall WSCDeinstallProvider(ptr ptr)
88 stdcall WSCEnableNSProvider(ptr long)
89 stdcall WSCEnumProtocols(ptr ptr ptr ptr)
90 stdcall WSCGetProviderPath(ptr ptr ptr ptr)
91 stdcall WSCInstallNameSpace(wstr wstr long long ptr)
92 stdcall WSCInstallProvider(ptr wstr ptr long ptr)
93 stdcall WSCUnInstallNameSpace(ptr)
94 stdcall WSCUpdateProvider(ptr ptr ptr long ptr)
95 stdcall WSCWriteNameSpaceOrder(ptr long)
96 stdcall WSCWriteProviderOrder(ptr long)
97 stdcall freeaddrinfo(ptr)
98 stdcall getaddrinfo(str str ptr ptr)
99 stdcall getnameinfo(ptr long ptr long ptr long long)
101 stdcall WSAAsyncSelect(long long long long)
102 stdcall WSAAsyncGetHostByAddr(long long ptr long long ptr long)
103 stdcall WSAAsyncGetHostByName(long long str ptr long)
104 stdcall WSAAsyncGetProtoByNumber(long long long ptr long)
105 stdcall WSAAsyncGetProtoByName(long long str ptr long)
106 stdcall WSAAsyncGetServByPort(long long long str ptr long)
107 stdcall WSAAsyncGetServByName(long long str str ptr long)
108 stdcall WSACancelAsyncRequest(long)
109 stdcall WSASetBlockingHook(ptr)
110 stdcall WSAUnhookBlockingHook()
111 stdcall WSAGetLastError()
112 stdcall WSASetLastError(long)
113 stdcall WSACancelBlockingCall()
114 stdcall WSAIsBlocking()
115 stdcall WSAStartup(long ptr)
116 stdcall WSACleanup()
151 stdcall __WSAFDIsSet(long ptr)
500 stdcall WEP()