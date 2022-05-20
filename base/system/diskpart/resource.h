/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/resource.h
 * PURPOSE:         Manages all the partitions of the OS in an interactive way
 * PROGRAMMERS:     Lee Schroeder
 */

#pragma once

#define IDS_NONE -1

#define IDS_APP_HEADER        0
#define IDS_APP_USAGE         1
#define IDS_APP_LICENSE       2
#define IDS_APP_CURR_COMPUTER 3
#define IDS_APP_LEAVING       4
#define IDS_APP_PROMPT        5

#define IDS_DETAIL_INFO_DISK_ID        1107
#define IDS_DETAIL_INFO_TYPE           1108
#define IDS_DETAIL_INFO_STATUS         1109
#define IDS_DETAIL_INFO_PATH           1110
#define IDS_DETAIL_INFO_TARGET         1111
#define IDS_DETAIL_INFO_LUN_ID         1112
#define IDS_DETAIL_INFO_LOC_PATH       1113
#define IDS_DETAIL_INFO_CURR_RO_STATE  1114
#define IDS_DETAIL_INFO_RO             1115
#define IDS_DETAIL_INFO_BOOT_DSK       1116
#define IDS_DETAIL_INFO_PAGE_FILE_DSK  1117
#define IDS_DETAIL_INFO_HIBER_FILE_DSK 1118
#define IDS_DETAIL_INFO_CRASH_DSK      1119
#define IDS_DETAIL_INFO_CLST_DSK       1120

#define IDS_DETAIL_PARTITION_NUMBER    1130
#define IDS_DETAIL_PARTITION_TYPE      1131
#define IDS_DETAIL_PARTITION_HIDDEN    1132
#define IDS_DETAIL_PARTITION_ACTIVE    1133
#define IDS_DETAIL_PARTITION_OFFSET    1134

#define IDS_HELP_FORMAT_STRING         1200

#define IDS_LIST_DISK_HEAD             3300
#define IDS_LIST_DISK_LINE             3301
#define IDS_LIST_DISK_FORMAT           3302
#define IDS_LIST_PARTITION_HEAD        3303
#define IDS_LIST_PARTITION_LINE        3304
#define IDS_LIST_PARTITION_FORMAT      3305
#define IDS_LIST_PARTITION_NO_DISK     3306
#define IDS_LIST_VOLUME_HEAD           3307
#define IDS_LIST_VOLUME_LINE           3308
#define IDS_LIST_VOLUME_FORMAT         3309

#define IDS_RESCAN_START               4100
#define IDS_RESCAN_END                 4101

#define IDS_SELECT_NO_DISK             4400
#define IDS_SELECT_DISK                4401
#define IDS_SELECT_DISK_INVALID        4402
#define IDS_SELECT_NO_PARTITION        4403
#define IDS_SELECT_PARTITION           4404
#define IDS_SELECT_PARTITION_NO_DISK   4405
#define IDS_SELECT_PARTITION_INVALID   4406
#define IDS_SELECT_NO_VOLUME           4407
#define IDS_SELECT_VOLUME              4408
#define IDS_SELECT_VOLUME_INVALID      4409

#define IDS_STATUS_YES          31
#define IDS_STATUS_NO           32
#define IDS_STATUS_DISK_HEALTHY 33
#define IDS_STATUS_DISK_SICK    34
#define IDS_STATUS_UNAVAILABLE  35
#define IDS_STATUS_ONLINE       36
#define IDS_STATUS_OFFLINE      37
#define IDS_STATUS_NO_MEDIA     38

#define IDS_MSG_ARG_SYNTAX_ERROR   41

#define IDS_HELP_ACTIVE                     58
#define IDS_HELP_ADD                        59
#define IDS_HELP_ASSIGN                     60
#define IDS_HELP_ATTRIBUTES                 61
#define IDS_HELP_ATTACH                     62
#define IDS_HELP_AUTOMOUNT                  63
#define IDS_HELP_BREAK                      64
#define IDS_HELP_CLEAN                      65
#define IDS_HELP_COMPACT                    66
#define IDS_HELP_CONVERT                    67

#define IDS_HELP_CREATE                     68
#define IDS_HELP_CREATE_PARTITION           69
#define IDS_HELP_CREATE_PARTITION_EFI       70
#define IDS_HELP_CREATE_PARTITION_EXTENDED  71
#define IDS_HELP_CREATE_PARTITION_LOGICAL   72
#define IDS_HELP_CREATE_PARTITION_MSR       73
#define IDS_HELP_CREATE_PARTITION_PRIMARY   74
#define IDS_HELP_CREATE_VOLUME              75
#define IDS_HELP_CREATE_VDISK               76

#define IDS_HELP_DELETE                     77
#define IDS_HELP_DETACH                     78

#define IDS_HELP_DETAIL                     79
#define IDS_HELP_DETAIL_DISK                80
#define IDS_HELP_DETAIL_PARTITION           81
#define IDS_HELP_DETAIL_VOLUME              82

#define IDS_HELP_EXIT                       83
#define IDS_HELP_EXPAND                     84
#define IDS_HELP_EXTEND                     85
#define IDS_HELP_FILESYSTEMS                86
#define IDS_HELP_FORMAT                     87
#define IDS_HELP_GPT                        88
#define IDS_HELP_HELP                       89
#define IDS_HELP_IMPORT                     90
#define IDS_HELP_INACTIVE                   91

#define IDS_HELP_LIST                       92
#define IDS_HELP_LIST_DISK                  93
#define IDS_HELP_LIST_PARTITION             94
#define IDS_HELP_LIST_VOLUME                95
#define IDS_HELP_LIST_VDISK                 96

#define IDS_HELP_MERGE                      97
#define IDS_HELP_ONLINE                     98
#define IDS_HELP_OFFLINE                    99
#define IDS_HELP_RECOVER                   100
#define IDS_HELP_REM                       101
#define IDS_HELP_REMOVE                    102
#define IDS_HELP_REPAIR                    103
#define IDS_HELP_RESCAN                    104
#define IDS_HELP_RETAIN                    105
#define IDS_HELP_SAN                       106

#define IDS_HELP_SELECT                    107
#define IDS_HELP_SELECT_DISK               108
#define IDS_HELP_SELECT_PARTITION          109
#define IDS_HELP_SELECT_VOLUME             110
#define IDS_HELP_SELECT_VDISK              111

#define IDS_HELP_SETID                     112
#define IDS_HELP_SHRINK                    113

#define IDS_HELP_UNIQUEID                  114
#define IDS_HELP_UNIQUEID_DISK             115


#define IDS_COMMAND_ACTIVE                     1000
#define IDS_COMMAND_ADD                        1001
#define IDS_COMMAND_ASSIGN                     1002
#define IDS_COMMAND_ATTACH                     1003
#define IDS_COMMAND_ATTRIBUTES                 1004
#define IDS_COMMAND_AUTOMOUNT                  1005
#define IDS_COMMAND_BREAK                      1006
#define IDS_COMMAND_CLEAN                      1007
#define IDS_COMMAND_COMPACT                    1008
#define IDS_COMMAND_CONVERT                    1009
#define IDS_COMMAND_CREATE_PARTITION_EFI       1010
#define IDS_COMMAND_CREATE_PARTITION_EXTENDED  1011
#define IDS_COMMAND_CREATE_PARTITION_LOGICAL   1012
#define IDS_COMMAND_CREATE_PARTITION_MSR       1013
#define IDS_COMMAND_CREATE_PARTITION_PRIMARY   1014
#define IDS_COMMAND_DELETE                     1015
#define IDS_COMMAND_DETACH                     1016
#define IDS_COMMAND_DETAIL_DISK                1017
#define IDS_COMMAND_DETAIL_PARTITION           1018
#define IDS_COMMAND_DETAIL_VOLUME              1019
#define IDS_COMMAND_EXIT                       1020
#define IDS_COMMAND_EXPAND                     1021
#define IDS_COMMAND_EXTEND                     1022
#define IDS_COMMAND_FILESYSTEMS                1023
#define IDS_COMMAND_FORMAT                     1024
#define IDS_COMMAND_GPT                        1025
#define IDS_COMMAND_HELP                       1026
#define IDS_COMMAND_IMPORT                     1027
#define IDS_COMMAND_INACTIVE                   1028
#define IDS_COMMAND_LIST_DISK                  1029
#define IDS_COMMAND_LIST_PARTITION             1030
#define IDS_COMMAND_LIST_VOLUME                1031
#define IDS_COMMAND_LIST_VDISK                 1032
#define IDS_COMMAND_MERGE                      1033
#define IDS_COMMAND_ONLINE                     1034
#define IDS_COMMAND_OFFLINE                    1035
#define IDS_COMMAND_RECOVER                    1036
#define IDS_COMMAND_REM                        1037
#define IDS_COMMAND_REMOVE                     1038
#define IDS_COMMAND_REPAIR                     1039
#define IDS_COMMAND_RESCAN                     1040
#define IDS_COMMAND_RETAIN                     1041
#define IDS_COMMAND_SAN                        1042
#define IDS_COMMAND_SELECT_DISK                1043
#define IDS_COMMAND_SELECT_PARTITION           1044
#define IDS_COMMAND_SELECT_VOLUME              1045
#define IDS_COMMAND_SELECT_VDISK               1046
#define IDS_COMMAND_SETID                      1047
#define IDS_COMMAND_SHRINK                     1048
#define IDS_COMMAND_UNIQUEID_DISK              1049

#define IDS_ERROR_MSG_NO_SCRIPT  2000
#define IDS_ERROR_MSG_BAD_ARG    2001
#define IDS_ERROR_INVALID_ARGS   2002
