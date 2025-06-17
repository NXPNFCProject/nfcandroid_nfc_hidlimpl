/**
 *
 *  Copyright 2025 NXP
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 **/

#ifndef _PHNXPNCIDEF_H_
#define _PHNXPNCIDEF_H_

// #define NCI_MT_MASK 0xE0
// #define NCI_MT_CMD 0x20
// #define NCI_MT_RSP 0x40
// #define NCI_MT_NTF 0x60

/* GID: Group Identifier (byte 0) */
#define NCI_GID_MASK 0x0F
#define NCI_GID_CORE 0x00      /* 0000b NCI Core group */
#define NCI_GID_RF_MANAGE 0x01 /* 0001b RF Management group */
#define NCI_GID_EE_MANAGE 0x02 /* 0010b NFCEE Management group */
#define NCI_GID_PROP 0x0F      /* 1111b Proprietary */
/* 0111b - 1110b RFU */

/* OID: Opcode Identifier (byte 1) */
#define NCI_OID_MASK 0x3F
#define NCI_OID_SHIFT 0
#define SUB_GID_MASK 0xF0
#define SUB_OID_MASK 0x0F

/* builds byte0 of NCI Command and Notification packet */
#define NCI_MSG_BLD_HDR0(p, mt, gid) \
  *(p)++ = (uint8_t)(((mt) << NCI_MT_SHIFT) | (gid));

/* builds byte1 of NCI Command and Notification packet */
#define NCI_MSG_BLD_HDR1(p, oid) *(p)++ = (uint8_t)(((oid) << NCI_OID_SHIFT));

/* parse byte0 of NCI packet */
#define NCI_MSG_PRS_HDR0(p, mt, pbf, gid)         \
  (mt) = (*(p) & NCI_MT_MASK) >> NCI_MT_SHIFT;    \
  (pbf) = (*(p) & NCI_PBF_MASK) >> NCI_PBF_SHIFT; \
  (gid) = *(p)++ & NCI_GID_MASK;

/* parse byte1 of NCI Cmd/Ntf */
#define NCI_MSG_PRS_HDR1(p, oid) \
  (oid) = (*(p) & NCI_OID_MASK); \
  (p)++;

/**********************************************
 * NCI Core Group Opcode        - 0
 **********************************************/
// #define NCI_MSG_CORE_RESET 0
// #define NCI_MSG_CORE_INIT 1
#define NCI_MSG_CORE_SET_CONFIG 2
#define NCI_MSG_CORE_GET_CONFIG 3
#define NCI_MSG_CORE_CONN_CREATE 4
#define NCI_MSG_CORE_CONN_CLOSE 5
#define NCI_MSG_CORE_CONN_CREDITS 6
#define NCI_MSG_CORE_GEN_ERR_STATUS 7
#define NCI_MSG_CORE_INTF_ERR_STATUS 8
#define NCI_MSG_CORE_SET_POWER_SUB_STATE 9

/**********************************************
 * RF MANAGEMENT Group Opcode    - 1
 **********************************************/
#define NCI_MSG_RF_DISCOVER_MAP 0
#define NCI_MSG_RF_SET_ROUTING 1
#define NCI_MSG_RF_GET_ROUTING 2
#define NCI_MSG_RF_DISCOVER 3
#define NCI_MSG_RF_DISCOVER_SELECT 4
#define NCI_MSG_RF_INTF_ACTIVATED 5
#define NCI_MSG_RF_DEACTIVATE 6
#define NCI_MSG_RF_FIELD 7
#define NCI_MSG_RF_T3T_POLLING 8
#define NCI_MSG_RF_EE_ACTION 9
#define NCI_MSG_RF_EE_DISCOVERY_REQ 10
#define NCI_MSG_RF_PARAMETER_UPDATE 11
#define NCI_MSG_RF_INTF_EXT_START 12
#define NCI_MSG_RF_INTF_EXT_STOP 13
#define NCI_MSG_RF_INTF_PROP_NTF 15  // Ntag Prop notification
#define NCI_MSG_RF_ISO_DEP_NAK_PRESENCE 16
#define NCI_MSG_RF_REMOVAL_DETECTION 18
#define NCI_MSG_WPT_START 21

/**********************************************
 * NCI Deactivation Type
 **********************************************/
#define NCI_DEACTIVATE_TYPE_IDLE 0      /* Idle Mode     */
#define NCI_DEACTIVATE_TYPE_SLEEP 1     /* Sleep Mode    */
#define NCI_DEACTIVATE_TYPE_SLEEP_AF 2  /* Sleep_AF Mode */
#define NCI_DEACTIVATE_TYPE_DISCOVERY 3 /* Discovery     */

#endif /* _PHNXPNCIDEF_H_ */
