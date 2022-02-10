/*
 * Copyright (C) 2017-2018, 2020,2022 NXP
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "NCILxDebugDecoder.h"
#include "phOsal_Posix.h"

#include <netinet/in.h>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>

using namespace std;

/*
#define LOG_FUNCTION_ENTRY \
            phOsal_LogFunctionEntry((const uint8_t*) \
                 "LxDecoder",(const uint8_t*)__FUNCTION__)
#define LOG_FUNCTION_EXIT \
            phOsal_LogFunctionExit((const uint8_t*) \
                  "LxDecoder",(const uint8_t*)__FUNCTION__)
*/

#define LOG_FUNCTION_ENTRY
#define LOG_FUNCTION_EXIT

NCI_LxDebug_Decoder* NCI_LxDebug_Decoder::mLxDbgDecoder = nullptr;

/*******************************************************************************
 **
 ** Function:        NCI_LxDebug_Decoder()
 **
 ** Description:
 **
 ** Returns:         nothing
 **
 ******************************************************************************/
NCI_LxDebug_Decoder::NCI_LxDebug_Decoder()
    : mL2DebugMode(false),        // bit:0 Byte0
      mFelicaRFDebugMode(false),  // bit:1 Byte0
      mFelicaSCDebugMode(false),  // bit:2 Byte0
      mL1DebugMode(false),        // bit:4 Byte0
      m7816DebugMode(false),      // bit:6 Byte0
      mRssiDebugMode(false) {     // bit:0 Byte1
}

/*******************************************************************************
 **
 ** Function:        ~NCI_LxDebug_Decoder()
 **
 ** Description:
 **
 ** Returns:         nothing
 **
 ******************************************************************************/
NCI_LxDebug_Decoder::~NCI_LxDebug_Decoder() { mLxDbgDecoder = nullptr; }

/*******************************************************************************
 **
 ** Function:        getInstance()
 **
 ** Description:     This function return the singleton of LxDebug Decoder
 **
 ** Returns:         return singleton object
 **
 ******************************************************************************/
NCI_LxDebug_Decoder& NCI_LxDebug_Decoder::getInstance() {
  if (mLxDbgDecoder == nullptr) {
    mLxDbgDecoder = new NCI_LxDebug_Decoder;
    return (*mLxDbgDecoder);
  } else
    return (*mLxDbgDecoder);
}

/*******************************************************************************
 **
 ** Function:        setLxDebugModes(uint8_t *, uint16_t)
 **
 ** Description:     This function sets the proper Lx Debug level decoding
 **
 ** Returns:         void
 **
 ******************************************************************************/
void NCI_LxDebug_Decoder::setLxDebugModes(uint8_t* pNciPkt,
                                          __attribute__((unused))
                                          uint16_t pktLen) {
  mL2DebugMode = (pNciPkt[7] & 0x01) ? true : false;        // bit:0 Byte0
  mFelicaRFDebugMode = (pNciPkt[7] & 0x02) ? true : false;  // bit:1 Byte0
  mFelicaSCDebugMode = (pNciPkt[7] & 0x04) ? true : false;  // bit:2 Byte0
  mL1DebugMode = (pNciPkt[7] & 0x10) ? true : false;        // bit:4 Byte0
  m7816DebugMode = (pNciPkt[7] & 0x40) ? true : false;      // bit:6 Byte0
  mRssiDebugMode = (pNciPkt[8] & 0x01) ? true : false;      // bit:0 Byte1

  phOsal_LogInfoU32d((const uint8_t*)"LxDecoder> L1 Debug Enable",
                     mL1DebugMode);
  phOsal_LogInfoU32d((const uint8_t*)"LxDecoder> L2 Debug Enable",
                     mL2DebugMode);
  phOsal_LogInfoU32d((const uint8_t*)"LxDecoder> FelicaRF Enable",
                     mFelicaRFDebugMode);
  phOsal_LogInfoU32d((const uint8_t*)"LxDecoder> FelicaSC Enable",
                     mFelicaSCDebugMode);
  phOsal_LogInfoU32d((const uint8_t*)"LxDecoder> 7816-4RC Enable",
                     m7816DebugMode);
  phOsal_LogInfoU32d((const uint8_t*)"LxDecoder> RSSI Dbg Enable",
                     mRssiDebugMode);
}

/*******************************************************************************
 **
 ** Function:        processLxDbgNciPkt(uint8_t *,uint16_t)
 **
 ** Description:     This function process Lx NTF, finds the type of it whether
 **                  L1 or L2 and then calls the appropriate decoding function
 **
 ** Returns:         void
 **
 ******************************************************************************/
void NCI_LxDebug_Decoder::processLxDbgNciPkt(uint8_t* pNciPkt,
                                             uint16_t pktLen) {
  LOG_FUNCTION_ENTRY;

  sLxNtfCoded_t sLxNtfCoded = {.pLxNtf = nullptr, .LxNtfLen = 0};
  psLxNtfCoded_t psLxNtfCoded = &sLxNtfCoded;

  sLxNtfDecoded_t sLxNtfDecoded;
  sL1NtfDecoded_t sL1NtfDecoded;
  sL2NtfDecoded_t sL2NtfDecoded;

  memset((uint8_t*)&sLxNtfDecoded, 0, sizeof(sLxNtfDecoded_t));
  memset((uint8_t*)&sL1NtfDecoded, 0, sizeof(sL1NtfDecoded_t));
  memset((uint8_t*)&sL2NtfDecoded, 0, sizeof(sL2NtfDecoded_t));

  psLxNtfDecoded_t psLxNtfDecoded = &sLxNtfDecoded;
  psLxNtfDecoded->psL1NtfDecoded = &sL1NtfDecoded;
  psLxNtfDecoded->psL2NtfDecoded = &sL2NtfDecoded;

  if ((pNciPkt != nullptr) && (pktLen != 0)) {
    if (pNciPkt[0] == 0x6F && (pNciPkt[1] == 0x35 || pNciPkt[1] == 0x36)) {
      psLxNtfCoded->pLxNtf = pNciPkt;
      psLxNtfCoded->LxNtfLen = pktLen;
    } else if ((pNciPkt[0] == 0x20) && (pNciPkt[1] == 0x02)) {
      if ((pNciPkt[4] == 0xA0) && (pNciPkt[5] == 0x1D)) {
        setLxDebugModes(pNciPkt, pktLen);
      }
    } else
      return;
  } else
    return;

  if ((psLxNtfCoded->pLxNtf != nullptr) && (psLxNtfCoded->LxNtfLen != 0)) {
    if (psLxNtfCoded->pLxNtf[1] == SYSTEM_DEBUG_STATE_L1_MESSAGE) {
      psLxNtfDecoded->level = SYSTEM_DEBUG_STATE_L1_MESSAGE;
      parseL1DbgNtf(psLxNtfCoded, psLxNtfDecoded);
    } else if (psLxNtfCoded->pLxNtf[1] == SYSTEM_DEBUG_STATE_L2_MESSAGE) {
      psLxNtfDecoded->level = SYSTEM_DEBUG_STATE_L2_MESSAGE;
      parseL2DbgNtf(psLxNtfCoded, psLxNtfDecoded);
    }

    printLxDebugInfo(psLxNtfDecoded);
  } else
    return;

  LOG_FUNCTION_EXIT;
}

/*******************************************************************************
 **
 ** Function:        parseL1DebugNtf(psLxNtfCoded_t, psLxNtfDecoded_t)
 **
 ** Description:     This function parse L1 NTF and decodes the values.
 **
 ** Returns:         void
 **
 ******************************************************************************/
void NCI_LxDebug_Decoder::parseL1DbgNtf(psLxNtfCoded_t psLxNtfCoded,
                                        psLxNtfDecoded_t psLxNtfDecoded) {
  LOG_FUNCTION_ENTRY;

  sLxNtfDecodingInfo_t sLxNtfDecodingInfo;
  memset((uint8_t*)&sLxNtfDecodingInfo, 0, sizeof(sLxNtfDecodingInfo_t));
  psLxNtfDecodingInfo_t psLxNtfDecodingInfo = &sLxNtfDecodingInfo;

  psLxNtfDecodingInfo->baseIndex = 3;  // Entry Start Index

  psLxNtfDecodingInfo->milliSecOffset = 0;  // 3rd byte 4th byte
  psLxNtfDecodingInfo->microSecOffset = 2;  // 5th byte 6th byte
  psLxNtfDecodingInfo->rawRSSIOffset = 0;   // RSSI dbg enable
  psLxNtfDecodingInfo->intrpltdRSSIOffset = 4;
  psLxNtfDecodingInfo->apcOffset = 4;
  psLxNtfDecodingInfo->cliffStateTriggerTypeOffset = 6;
  psLxNtfDecodingInfo->cliffStateRFTechModeOffset = 6;
  psLxNtfDecodingInfo->eddOffset = 7;
  psLxNtfDecodingInfo->retCode78164Offset = 8;

  if (psLxNtfCoded->pLxNtf[2] == L1_EVT_LEN)  // 0x07 == Normal L1 Event Entry
  {
    // TODO: Remove common code
    if (mRssiDebugMode) {
      decodeCLIFFState(psLxNtfCoded, psLxNtfDecodingInfo, psLxNtfDecoded);
      decodeRSSIValues(psLxNtfCoded, psLxNtfDecodingInfo, psLxNtfDecoded);
    } else {
      decodeTimeStamp(psLxNtfCoded, psLxNtfDecodingInfo, psLxNtfDecoded);
      decodeCLIFFState(psLxNtfCoded, psLxNtfDecodingInfo, psLxNtfDecoded);
      if ((psLxNtfDecoded->psL1NtfDecoded->sInfo
               .pCliffStateTriggerTypeDirection) &&
          (!strcmp((const char*)psLxNtfDecoded->psL1NtfDecoded->sInfo
                       .pCliffStateTriggerTypeDirection,
                   (const char*)"CLF_EVT_RX")))
        decodeRSSIValues(psLxNtfCoded, psLxNtfDecodingInfo, psLxNtfDecoded);
      else {
        decodeAPCTable(psLxNtfCoded, psLxNtfDecodingInfo, psLxNtfDecoded);
        calculateTxVpp(psLxNtfDecoded);
      }
    }
  } else if (psLxNtfCoded->pLxNtf[2] ==
             L1_EVT_EXTRA_DBG_LEN)  // 0x08 == L1 Event Entry + EDD
  {
    if (mRssiDebugMode) {
      decodeCLIFFState(psLxNtfCoded, psLxNtfDecodingInfo, psLxNtfDecoded);
      decodeRSSIValues(psLxNtfCoded, psLxNtfDecodingInfo, psLxNtfDecoded);
    } else {
      decodeTimeStamp(psLxNtfCoded, psLxNtfDecodingInfo, psLxNtfDecoded);
      decodeCLIFFState(psLxNtfCoded, psLxNtfDecodingInfo, psLxNtfDecoded);
      if ((psLxNtfDecoded->psL1NtfDecoded->sInfo
               .pCliffStateTriggerTypeDirection) &&
          (!strcmp((const char*)psLxNtfDecoded->psL1NtfDecoded->sInfo
                       .pCliffStateTriggerTypeDirection,
                   (const char*)"CLF_EVT_RX")))
        decodeRSSIValues(psLxNtfCoded, psLxNtfDecodingInfo, psLxNtfDecoded);
      else {
        decodeAPCTable(psLxNtfCoded, psLxNtfDecodingInfo, psLxNtfDecoded);
        calculateTxVpp(psLxNtfDecoded);
      }
    }
    decodeExtraDbgData(psLxNtfCoded, psLxNtfDecodingInfo, psLxNtfDecoded);
  } else if (psLxNtfCoded->pLxNtf[2] == L1_EVT_7816_RET_CODE_LEN ||
             psLxNtfCoded->pLxNtf[2] ==
                 L1_EVT_7816_RET_CODE_EXTRA_DBG_LEN)  // 0xA0 == L1 Event Entry
                                                      // + EDD + 7816-4 SW1SW2
  {
    if (mRssiDebugMode) {
      decodeCLIFFState(psLxNtfCoded, psLxNtfDecodingInfo, psLxNtfDecoded);
      decodeRSSIValues(psLxNtfCoded, psLxNtfDecodingInfo, psLxNtfDecoded);
    } else {
      decodeTimeStamp(psLxNtfCoded, psLxNtfDecodingInfo, psLxNtfDecoded);
      decodeCLIFFState(psLxNtfCoded, psLxNtfDecodingInfo, psLxNtfDecoded);
      if ((psLxNtfDecoded->psL1NtfDecoded->sInfo
               .pCliffStateTriggerTypeDirection) &&
          (!strcmp((const char*)psLxNtfDecoded->psL1NtfDecoded->sInfo
                       .pCliffStateTriggerTypeDirection,
                   (const char*)"CLF_EVT_RX")))
        decodeRSSIValues(psLxNtfCoded, psLxNtfDecodingInfo, psLxNtfDecoded);
      else {
        decodeAPCTable(psLxNtfCoded, psLxNtfDecodingInfo, psLxNtfDecoded);
        calculateTxVpp(psLxNtfDecoded);
      }
    }
    if (psLxNtfCoded->pLxNtf[2] == L1_EVT_7816_RET_CODE_EXTRA_DBG_LEN) {
      psLxNtfDecodingInfo->eddOffset = 9;
      decodeExtraDbgData(psLxNtfCoded, psLxNtfDecodingInfo, psLxNtfDecoded);
    }
    decode78164RetCode(psLxNtfCoded, psLxNtfDecodingInfo, psLxNtfDecoded);
  } else {
    phOsal_LogDebug((const uint8_t*)"LxDecoder> Invalid Length !");
  }

  LOG_FUNCTION_EXIT;
}

/*******************************************************************************
 **
 ** Function:        parseL2DbgNtf(psLxNtfCoded_t, psLxNtfDecoded_t)
 **
 ** Description:     This functions parses L2 NTF, finds event TAG and decodes
 **                  them one by one.
 **
 ** Returns:         void
 **
 ******************************************************************************/
void NCI_LxDebug_Decoder::parseL2DbgNtf(psLxNtfCoded_t psLxNtfCoded,
                                        psLxNtfDecoded_t psLxNtfDecoded) {
  LOG_FUNCTION_ENTRY;

  uint8_t tlvCount = 0;
  int32_t totalTLVlength = 0;
  uint8_t tlvIndex = 3;
  sLxNtfDecodingInfo_t sLxNtfDecodingInfo;
  psLxNtfDecodingInfo_t psLxNtfDecodingInfo = &sLxNtfDecodingInfo;

  memset((uint8_t*)&sLxNtfDecodingInfo, 0, sizeof(sLxNtfDecodingInfo_t));
  totalTLVlength = psLxNtfCoded->pLxNtf[2];

  phOsal_LogDebugU32d((const uint8_t*)"LxDecoder> Total TLV length",
                      totalTLVlength);
  do {
    psL2DecodedInfo_t psL2DecodedInfo =
        &(psLxNtfDecoded->psL2NtfDecoded->sTlvInfo[tlvCount]);
    memset(psL2DecodedInfo, 0, sizeof(sL2DecodedInfo_t));

    psLxNtfDecodingInfo->baseIndex = tlvIndex;
    uint8_t baseIndex = psLxNtfDecodingInfo->baseIndex + 1;
    uint8_t* ptr = psLxNtfCoded->pLxNtf + baseIndex;

    uint8_t tag = psLxNtfCoded->pLxNtf[tlvIndex] & 0xF0;
    uint8_t len = psLxNtfCoded->pLxNtf[tlvIndex] & 0x0F;
    uint8_t milliSec[2] = {0};
    uint8_t microSec[2] = {0};

    milliSec[1] = *ptr++;
    milliSec[0] = *ptr++;
    microSec[1] = *ptr++;
    microSec[0] = *ptr;

    psL2DecodedInfo->tag = tag;
    psL2DecodedInfo->len = len;
    psL2DecodedInfo->timeStampMs = ntohs(*((uint16_t*)milliSec));
    psL2DecodedInfo->timeStampUs = ntohs(*((uint16_t*)microSec));
    // Decode sub events inside L2 Dbg Ntf
    switch (tag) {
      case L2_EVT_TAG_ID:
        decodeL2Event(psLxNtfCoded, psLxNtfDecodingInfo, psL2DecodedInfo);
        break;
      case L2_EVT_FELICA_CMD_TAG_ID:
        decodeFelicaCmdEvent(psLxNtfCoded, psLxNtfDecodingInfo,
                             psL2DecodedInfo);
        break;
      case L2_EVT_FELICA_SYS_CODE_TAG_ID:
        decodeFelicaSysCodeEvent(psLxNtfCoded, psLxNtfDecodingInfo,
                                 psL2DecodedInfo);
        break;
      case L2_EVT_FELICA_RSP_CODE_TAG_ID:
        decodeFelicaRspEvent(psLxNtfCoded, psLxNtfDecodingInfo,
                             psL2DecodedInfo);
        break;
      case L2_EVT_FELICA_MISC_TAG_ID:
        decodeFelicaMiscEvent(psLxNtfCoded, psLxNtfDecodingInfo,
                              psL2DecodedInfo);
        break;
      case L2_EVT_CMA_TAG_ID:
        decodeCMAEvent(psLxNtfCoded, psLxNtfDecodingInfo, psL2DecodedInfo);
        break;
      default:
        phOsal_LogDebugU32d((const uint8_t*)"LxDecoder> L2_EVT_UNKNOWN",
                            __LINE__);
        break;
    };
    totalTLVlength = totalTLVlength - (len + 1);
    tlvIndex = tlvIndex + (len + 1);

  } while ((++tlvCount < MAX_TLV) && (totalTLVlength > 0));

  psLxNtfDecoded->psL2NtfDecoded->tlvCount = tlvCount;

  LOG_FUNCTION_EXIT;
}

/*******************************************************************************
 **
 ** Function:        decodeL2Event(psLxNtfCoded_t psLxNtfCoded,
 **                                psLxNtfDecodingInfo_t psLxLtfDecodingInfo,
 **                                psL2DecodedInfo_t psL2DecodedInfo)
 **
 ** Description:     Decodes L2 event in L2 Dbg Ntf.
 **
 ** Returns:         void
 **
 ******************************************************************************/

void NCI_LxDebug_Decoder::decodeL2Event(
    psLxNtfCoded_t psLxNtfCoded, psLxNtfDecodingInfo_t psLxNtfDecodingInfo,
    psL2DecodedInfo_t psL2DecodedInfo) {
  int32_t baseIndex = psLxNtfDecodingInfo->baseIndex + L2_SUB_EVT_OFFSET;
  uint8_t* ptr = psLxNtfCoded->pLxNtf + baseIndex;

  if (psL2DecodedInfo->len == L2_EVT_RX_TAG_ID_LEN ||
      psL2DecodedInfo->len == L2_EVT_RX_TAG_ID_EXTRA_DBG_LEN) {
    // Non Tx Event
    sL2RxEvent_t* l2RxEvt = &(psL2DecodedInfo->u.sL2RxEvent);

    l2RxEvt->rssi[0] = *ptr++;
    l2RxEvt->rssi[1] = *ptr++;
    l2RxEvt->clifState = *ptr++;

    if (psL2DecodedInfo->len == L2_EVT_RX_TAG_ID_EXTRA_DBG_LEN) {
      l2RxEvt->extraData = *ptr;
    }
  } else {
    // Tx Event
    sL2TxEvent_t* l2TxEvt = &(psL2DecodedInfo->u.sL2TxEvent);
    l2TxEvt->dlma[0] = *ptr++;
    l2TxEvt->dlma[1] = *ptr++;
    l2TxEvt->dlma[2] = *ptr++;
    l2TxEvt->dlma[3] = *ptr++;

    l2TxEvt->clifState = *ptr++;
    if (psL2DecodedInfo->len == L2_EVT_TX_TAG_ID_EXTRA_DBG_LEN) {
      l2TxEvt->extraData = *ptr;
    }
  }
}

/*******************************************************************************
 **
 ** Function:        decodeFelicaCmdEvent(psLxNtfCoded_t psLxNtfCoded,
 **                                       psLxNtfDecodingInfo_t
 **                                       psLxLtfDecodingInfo,
 **                                       psL2DecodedInfo_t psL2DecodedInfo)
 **
 ** Description:     Decodes Felica cmd event in L2 Dbg Ntf.
 **
 ** Returns:         void
 **
 ******************************************************************************/

void NCI_LxDebug_Decoder::decodeFelicaCmdEvent(
    psLxNtfCoded_t psLxNtfCoded, psLxNtfDecodingInfo_t psLxNtfDecodingInfo,
    psL2DecodedInfo_t psL2DecodedInfo) {
  sL2FelicaCmdEvent_t* felicaEvt = &(psL2DecodedInfo->u.sL2FelicaCmdEvent);
  int32_t baseIndex = psLxNtfDecodingInfo->baseIndex + L2_SUB_EVT_OFFSET;
  uint8_t* ptr = psLxNtfCoded->pLxNtf + baseIndex;

  felicaEvt->rssi[0] = *ptr++;
  felicaEvt->rssi[1] = *ptr++;
  felicaEvt->felicaCmd = *ptr++;

  if (psL2DecodedInfo->len == L2_EVT_FELICA_CMD_TAG_ID_EXTRA_DBG_LEN) {
    felicaEvt->extraData = *ptr;
  }
}

/*******************************************************************************
 **
 ** Function:        decodeFelicaSysCodeEvent(psLxNtfCoded_t psLxNtfCoded,
 **                                           psLxNtfDecodingInfo_t
 **                                           psLxLtfDecodingInfo,
 **                                           psL2DecodedInfo_t psL2DecodedInfo)
 **
 ** Description:     Decodes Felica system code event in L2 Dbg Ntf.
 **
 ** Returns:         void
 **
 ******************************************************************************/

void NCI_LxDebug_Decoder::decodeFelicaSysCodeEvent(
    psLxNtfCoded_t psLxNtfCoded, psLxNtfDecodingInfo_t psLxNtfDecodingInfo,
    psL2DecodedInfo_t psL2DecodedInfo) {
  sL2FelicaSysCodeEvent_t* sysCodeEvt =
      &(psL2DecodedInfo->u.sL2FelicaSysCodeEvent);
  int32_t baseIndex = psLxNtfDecodingInfo->baseIndex + L2_SUB_EVT_OFFSET;
  uint8_t* ptr = psLxNtfCoded->pLxNtf + baseIndex;

  sysCodeEvt->felicaCmd[0] = *ptr++;
  sysCodeEvt->felicaCmd[1] = *ptr;
}

/*******************************************************************************
 **
 ** Function:        decodeFelicaRspEvent(psLxNtfCoded_t psLxNtfCoded,
 **                                       psLxNtfDecodingInfo_t
 **                                       psLxLtfDecodingInfo,
 **                                       psL2DecodedInfo_t psL2DecodedInfo)
 **
 ** Description:     Decodes Felica response event in L2 Dbg Ntf.
 **
 ** Returns:         void
 **
 ******************************************************************************/

void NCI_LxDebug_Decoder::decodeFelicaRspEvent(
    psLxNtfCoded_t psLxNtfCoded, psLxNtfDecodingInfo_t psLxNtfDecodingInfo,
    psL2DecodedInfo_t psL2DecodedInfo) {
  sL2FelicaRspEvent_t* felicaRspEvt = &(psL2DecodedInfo->u.sL2FelicaRspEvent);
  int32_t baseIndex = psLxNtfDecodingInfo->baseIndex + L2_SUB_EVT_OFFSET;
  uint8_t* ptr = psLxNtfCoded->pLxNtf + baseIndex;

  felicaRspEvt->dlma[0] = *ptr++;
  felicaRspEvt->dlma[1] = *ptr++;
  felicaRspEvt->dlma[2] = *ptr++;
  felicaRspEvt->dlma[3] = *ptr++;

  felicaRspEvt->felicaRsp = *ptr++;

  felicaRspEvt->responseStatus[0] = *ptr++;
  felicaRspEvt->responseStatus[1] = *ptr++;

  if (psL2DecodedInfo->len == L2_EVT_FELICA_RSP_CODE_TAG_ID_EXTRA_DBG_LEN) {
    felicaRspEvt->extraData = *ptr;
  }
}

/*******************************************************************************
 **
 ** Function:        decodeFelicaMiscEvent(psLxNtfCoded_t psLxNtfCoded,
 **                                        psLxNtfDecodingInfo_t
 **                                        psLxLtfDecodingInfo,
 **                                        psL2DecodedInfo_t psL2DecodedInfo)
 **
 ** Description:     Decodes Felica MISC event in L2 Dbg Ntf.
 **
 ** Returns:         void
 **
 ******************************************************************************/

void NCI_LxDebug_Decoder::decodeFelicaMiscEvent(
    psLxNtfCoded_t psLxNtfCoded, psLxNtfDecodingInfo_t psLxNtfDecodingInfo,
    psL2DecodedInfo_t psL2DecodedInfo) {
  sL2FelicaMiscEvent_t* miscEvt = &(psL2DecodedInfo->u.sL2FelicaMiscEvent);
  int32_t baseIndex = psLxNtfDecodingInfo->baseIndex + L2_SUB_EVT_OFFSET;
  uint8_t* ptr = psLxNtfCoded->pLxNtf + baseIndex;

  miscEvt->trigger = *ptr++;
  if (psL2DecodedInfo->len == L2_EVT_FELICA_MISC_TAG_ID_EXTRA_DBG_LEN) {
    miscEvt->extraData = *ptr;
  }
}

/*******************************************************************************
 **
 ** Function:        decodeCMAEvent(psLxNtfCoded_t psLxNtfCoded,
 **                                 psLxNtfDecodingInfo_t psLxLtfDecodingInfo,
 **                                 psL2DecodedInfo_t psL2DecodedInfo)
 **
 ** Description:     Decodes Card mode access event in L2 Dbg Ntf.
 **
 ** Returns:         void
 **
 ******************************************************************************/

void NCI_LxDebug_Decoder::decodeCMAEvent(
    psLxNtfCoded_t psLxNtfCoded, psLxNtfDecodingInfo_t psLxNtfDecodingInfo,
    psL2DecodedInfo_t psL2DecodedInfo) {
  sL2CmaEvent_t* cmaEvt = &(psL2DecodedInfo->u.sL2CmaEvent);
  int32_t baseIndex = psLxNtfDecodingInfo->baseIndex + L2_SUB_EVT_OFFSET;
  uint8_t* ptr = psLxNtfCoded->pLxNtf + baseIndex;

  cmaEvt->trigger = *ptr++;

  if (psL2DecodedInfo->len > L2_EVT_CMA_TAG_ID_MIN_LEN) {
    memcpy(&(cmaEvt->extraData), ptr,
           (psL2DecodedInfo->len - L2_EVT_CMA_TAG_ID_MIN_LEN));
  }
}

/*******************************************************************************
 **
 ** Function:        printLxDebugInfo(psLxNtfDecoded)
 **
 ** Description:     This function prints the decoded information
 **
 ** Returns:         void
 **
 ******************************************************************************/
void NCI_LxDebug_Decoder::printLxDebugInfo(psLxNtfDecoded_t psLxNtfDecoded) {
  if (psLxNtfDecoded != nullptr) {
    if (psLxNtfDecoded->level == SYSTEM_DEBUG_STATE_L1_MESSAGE) {
      phOsal_LogDebug((
          const uint8_t*)"---------------------L1 Debug "
                         "Information--------------------");
      phOsal_LogDebugU32dd((const uint8_t*)"Time Stamp",
                          psLxNtfDecoded->psL1NtfDecoded->sInfo.timeStampMs,
                          psLxNtfDecoded->psL1NtfDecoded->sInfo.timeStampUs);
      phOsal_LogDebugString(
          (const uint8_t*)"Trigger Type",
          psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateTriggerType);
      phOsal_LogDebugString(
          (const uint8_t*)"RF Tech and Mode",
          psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateRFTechNMode);
      phOsal_LogDebugString((const uint8_t*)"Event Type",
                           psLxNtfDecoded->psL1NtfDecoded->sInfo
                               .pCliffStateTriggerTypeDirection);
      if (mRssiDebugMode) {
        phOsal_LogDebugU32h((const uint8_t*)"Raw RSSI ADC",
                           psLxNtfDecoded->psL1NtfDecoded->sInfo.rawRSSIADC);
        phOsal_LogDebugU32h((const uint8_t*)"Raw RSSI AGC",
                           psLxNtfDecoded->psL1NtfDecoded->sInfo.rawRSSIAGC);
      } else
        phOsal_LogDebugU32hh(
            (const uint8_t*)"Interpolated RSSI",
            psLxNtfDecoded->psL1NtfDecoded->sInfo.intrpltdRSSI[0],
            psLxNtfDecoded->psL1NtfDecoded->sInfo.intrpltdRSSI[1]);
      phOsal_LogDebugU32hh((const uint8_t*)"Auto Power Control",
                          psLxNtfDecoded->psL1NtfDecoded->sInfo.APC[0],
                          psLxNtfDecoded->psL1NtfDecoded->sInfo.APC[1]);
      phOsal_LogDebugString((const uint8_t*)"L1 Error EDD",
                           psLxNtfDecoded->psL1NtfDecoded->sInfo.pEddL1Error);
      phOsal_LogDebugString((const uint8_t*)"L1 RxNak EDD",
                           psLxNtfDecoded->psL1NtfDecoded->sInfo.pEddL1RxNak);
      phOsal_LogDebugString((const uint8_t*)"L1 TxErr EDD",
                           psLxNtfDecoded->psL1NtfDecoded->sInfo.pEddL1TxErr);
      phOsal_LogDebugU32hh(
          (const uint8_t*)"L1 7816-4 Ret Code",
          psLxNtfDecoded->psL1NtfDecoded->sInfo.eddL178164RetCode[0],
          psLxNtfDecoded->psL1NtfDecoded->sInfo.eddL178164RetCode[1]);
      if ((psLxNtfDecoded->psL1NtfDecoded->sInfo
               .pCliffStateTriggerTypeDirection) &&
          (!strcmp((const char*)psLxNtfDecoded->psL1NtfDecoded->sInfo
                       .pCliffStateTriggerTypeDirection,
                   (const char*)"CLF_EVT_TX"))) {
        phOsal_LogDebugU32d(
            (const uint8_t*)"Residual Carrier",
            psLxNtfDecoded->psL1NtfDecoded->sInfo.residualCarrier);
        phOsal_LogDebugU32d((const uint8_t*)"Number Driver",
                           psLxNtfDecoded->psL1NtfDecoded->sInfo.numDriver);
        phOsal_LogInfo32f((const uint8_t*)"Vtx AMP",
                          psLxNtfDecoded->psL1NtfDecoded->sInfo.vtxAmp);
        phOsal_LogInfo32f((const uint8_t*)"Vtx LDO",
                          psLxNtfDecoded->psL1NtfDecoded->sInfo.vtxLDO);
        phOsal_LogDebugU32d((const uint8_t*)"Tx Vpp",
                           psLxNtfDecoded->psL1NtfDecoded->sInfo.txVpp);
      }
      phOsal_LogDebug((
          const uint8_t*)"----------------------------------------"
                         "---------------------");
    } else if (psLxNtfDecoded->level == SYSTEM_DEBUG_STATE_L2_MESSAGE) {
      uint8_t tlvCount = psLxNtfDecoded->psL2NtfDecoded->tlvCount;

      phOsal_LogDebug((
          const uint8_t*)"---------------------L2 Debug "
                         "Information--------------------");
      for (int tlv = 0; tlv < tlvCount; tlv++) {
        std::ostringstream oss;
        psL2DecodedInfo_t decodedInfo =
            &(psLxNtfDecoded->psL2NtfDecoded->sTlvInfo[tlv]);
        oss << "TLV " << tlv << " {";
        oss << "TimeStamp = " << decodedInfo->timeStampMs << "."
            << decodedInfo->timeStampUs;

        switch (decodedInfo->tag) {
          case L2_EVT_TAG_ID: {
            oss << ", TAG = L2 EVENT";
            if (decodedInfo->len <= L2_EVT_RX_TAG_ID_EXTRA_DBG_LEN) {
              sL2RxEvent_t* l2RxEvt = &(decodedInfo->u.sL2RxEvent);
              l2RxEvt->toString(oss);
            } else {
              sL2TxEvent_t* l2TxEvt = &(decodedInfo->u.sL2TxEvent);
              l2TxEvt->toString(oss);
            }
            break;
          }
          case L2_EVT_FELICA_CMD_TAG_ID: {
            oss << ", TAG = FELICA CMD";
            sL2FelicaCmdEvent_t* felicaCmdEvt =
                &(decodedInfo->u.sL2FelicaCmdEvent);
            felicaCmdEvt->toString(oss);
            break;
          }
          case L2_EVT_FELICA_SYS_CODE_TAG_ID: {
            oss << ", TAG = FELICA SYSTEM CODE";
            sL2FelicaSysCodeEvent_t* sysCodeEvt =
                &(decodedInfo->u.sL2FelicaSysCodeEvent);
            sysCodeEvt->toString(oss);
            break;
          }
          case L2_EVT_FELICA_RSP_CODE_TAG_ID: {
            oss << ", TAG = FELICA RSP";
            sL2FelicaRspEvent_t* felicaRspEvt =
                &(decodedInfo->u.sL2FelicaRspEvent);
            felicaRspEvt->toString(oss);
            break;
          }
          case L2_EVT_FELICA_MISC_TAG_ID: {
            oss << ", TAG = FELICA MISC";
            sL2FelicaMiscEvent_t* miscEvt =
                &(decodedInfo->u.sL2FelicaMiscEvent);
            miscEvt->toString(oss);
            break;
          }
          case L2_EVT_CMA_TAG_ID: {
            oss << ", TAG = CMA";
            sL2CmaEvent_t* cmaEvt = &(decodedInfo->u.sL2CmaEvent);
            cmaEvt->toString(oss);
            break;
          }
          default:
            oss << ", TAG = RFU";
            break;
        }
        oss << " }" << std::endl;
        phOsal_LogDebug((const uint8_t*)oss.str().c_str());
      }
      phOsal_LogDebug((
          const uint8_t*)"----------------------------------"
                         "---------------------------");
    }
  }
}

/*******************************************************************************
 **
 ** Function:        decodeTimeStamp(psLxNtfCoded_t, psLxNtfDecodingInfo_t,
 **                  psLxNtfDecoded_t)
 **
 ** Description:     This function decodes the time stamp.
 **
 ** Returns:         void
 **
 ******************************************************************************/
void NCI_LxDebug_Decoder::decodeTimeStamp(
    psLxNtfCoded_t psLxNtfCoded, psLxNtfDecodingInfo_t psLxNtfDecodingInfo,
    psLxNtfDecoded_t psLxNtfDecoded) {
  uint8_t base = 0;
  uint8_t offsetMilliSec = 0;
  uint8_t offsetMicroSec = 0;
  uint8_t milliSec[2] = {0};
  uint8_t microSec[2] = {0};

  if (psLxNtfDecodingInfo != nullptr) {
    base = psLxNtfDecodingInfo->baseIndex;
    offsetMilliSec = psLxNtfDecodingInfo->milliSecOffset;
    offsetMicroSec = psLxNtfDecodingInfo->microSecOffset;
  } else
    return;

  /*Converting Little Endian to Big Endian*/
  milliSec[1] = psLxNtfCoded->pLxNtf[base + offsetMilliSec];
  offsetMilliSec++;
  milliSec[0] = psLxNtfCoded->pLxNtf[base + offsetMilliSec];

  /*Converting Little Endian to Big Endian*/
  microSec[1] = psLxNtfCoded->pLxNtf[base + offsetMicroSec];
  offsetMicroSec++;
  microSec[0] = psLxNtfCoded->pLxNtf[base + offsetMicroSec];

  if (psLxNtfCoded->pLxNtf[1] == SYSTEM_DEBUG_STATE_L1_MESSAGE) {
    psLxNtfDecoded->psL1NtfDecoded->sInfo.timeStampMs =
        ntohs(*((uint16_t*)milliSec));
    psLxNtfDecoded->psL1NtfDecoded->sInfo.timeStampUs =
        ntohs(*((uint16_t*)microSec));
  }
}

/*******************************************************************************
 **
 ** Function:        decodeRSSIandAPC(psLxNtfCoded_t, psLxNtfDecodingInfo_t,
 **                  psLxNtfDecoded_t)
 **
 ** Description:     This function decodes RSSI values in case RSSI mode enabled
 **
 ** Returns:         void
 **
 ******************************************************************************/
void NCI_LxDebug_Decoder::decodeRSSIValues(
    psLxNtfCoded_t psLxNtfCoded, psLxNtfDecodingInfo_t psLxNtfDecodingInfo,
    psLxNtfDecoded_t psLxNtfDecoded) {
  uint8_t base = 0;
  uint8_t offsetRawRSSI = 0;
  uint8_t offsetIntrpltdRSSI = 0;
  uint8_t rawRSSIAGC = 0;
  uint8_t rawRSSIADC = 0;
  uint8_t intrpltdRSSI[2] = {0};

  if (psLxNtfDecodingInfo != nullptr) {
    base = psLxNtfDecodingInfo->baseIndex;
    offsetRawRSSI = psLxNtfDecodingInfo->rawRSSIOffset;
    offsetIntrpltdRSSI = psLxNtfDecodingInfo->intrpltdRSSIOffset;
  } else
    return;

  rawRSSIAGC = psLxNtfCoded->pLxNtf[base + offsetRawRSSI];
  offsetRawRSSI++;
  rawRSSIADC = psLxNtfCoded->pLxNtf[base + offsetRawRSSI];

  /*Converting Little Endian to Big Endian*/
  intrpltdRSSI[0] = psLxNtfCoded->pLxNtf[base + offsetIntrpltdRSSI];
  offsetIntrpltdRSSI++;
  intrpltdRSSI[1] = psLxNtfCoded->pLxNtf[base + offsetIntrpltdRSSI];

  if (psLxNtfCoded->pLxNtf[1] == SYSTEM_DEBUG_STATE_L1_MESSAGE) {
    if (mRssiDebugMode) {
      psLxNtfDecoded->psL1NtfDecoded->sInfo.rawRSSIADC = rawRSSIADC;
      psLxNtfDecoded->psL1NtfDecoded->sInfo.rawRSSIAGC = rawRSSIAGC;
    }
    psLxNtfDecoded->psL1NtfDecoded->sInfo.intrpltdRSSI[0] = intrpltdRSSI[0];
    psLxNtfDecoded->psL1NtfDecoded->sInfo.intrpltdRSSI[1] = intrpltdRSSI[1];
  }
}

/*******************************************************************************
 **
 ** Function:       decodeRSSIandAPC(psLxNtfCoded_t, psLxNtfDecodingInfo_t,
 **                 psLxNtfDecoded_t)
 **
 ** Description:    X X X X X X |  X X X  |  X X   |  X  | X X X X X
 **                     RFU     |  VtxLDO | VtxAmp | NuD | ResCarrier
 **
 ** Returns:        void
 **
 ******************************************************************************/
void NCI_LxDebug_Decoder::decodeAPCTable(
    psLxNtfCoded_t psLxNtfCoded, psLxNtfDecodingInfo_t psLxNtfDecodingInfo,
    psLxNtfDecoded_t psLxNtfDecoded) {
  uint8_t base = 0;
  uint8_t offsetAPC = 0;
  uint8_t APC[2] = {0};

  if (psLxNtfDecodingInfo != nullptr) {
    base = psLxNtfDecodingInfo->baseIndex;
    offsetAPC = psLxNtfDecodingInfo->apcOffset;
  } else
    return;

  /*Converting Little Endian to Big Endian*/
  APC[1] = psLxNtfCoded->pLxNtf[base + offsetAPC];
  offsetAPC++;
  APC[0] = psLxNtfCoded->pLxNtf[base + offsetAPC];

  if (psLxNtfCoded->pLxNtf[1] == SYSTEM_DEBUG_STATE_L1_MESSAGE) {
    psLxNtfDecoded->psL1NtfDecoded->sInfo.APC[0] = APC[0];
    psLxNtfDecoded->psL1NtfDecoded->sInfo.APC[1] = APC[1];
    psLxNtfDecoded->psL1NtfDecoded->sInfo.residualCarrier =
        mLOOKUP_RESCARRIER[(APC[1] & mLOOKUP_RESCARRIER_BITMASK)];
    psLxNtfDecoded->psL1NtfDecoded->sInfo.numDriver =
        mLOOKUP_NUMDRIVER[(APC[1] & mLOOKUP_NUMDRIVER_BITMASK)];
    psLxNtfDecoded->psL1NtfDecoded->sInfo.vtxAmp =
        mLOOKUP_VTXAMP[(APC[1] & mLOOKUP_VTXAMP_BITMASK)];
    psLxNtfDecoded->psL1NtfDecoded->sInfo.vtxLDO =
        mLOOKUP_VTXLDO[(APC[0] & mLOOKUP_VTXLDO_BITMASK)];
  }
}

/*******************************************************************************
 **
 ** Function:        calculateTxVpp(psLxNtfDecoded_t)
 **
 ** Description:     This function calculates TxVpp based on formula
 **                  ( (VtxLDO + VtxAmp) * (1 - ((ResCarrier*1.61)/2)/100) ) *
 **                  (2 - NumDriver + 1);
 **
 ** Returns:         void
 **
 ******************************************************************************/
void NCI_LxDebug_Decoder::calculateTxVpp(psLxNtfDecoded_t psLxNtfDecoded) {
  uint8_t residualCarrier = 0;
  uint8_t numDriver = 0;
  float vtxAmp = 0;
  float vtxLDO = 0;
  uint16_t txVpp = 0;

  // Vpp = ( (VtxLDO + VtxAmp) * (1 - (ResCarrier*0.00805)) ) * (2 - NumDriver +
  // 1);

  if (psLxNtfDecoded->level == SYSTEM_DEBUG_STATE_L1_MESSAGE) {
    residualCarrier = psLxNtfDecoded->psL1NtfDecoded->sInfo.residualCarrier;
    numDriver = psLxNtfDecoded->psL1NtfDecoded->sInfo.numDriver;
    vtxAmp = psLxNtfDecoded->psL1NtfDecoded->sInfo.vtxAmp;
    vtxLDO = psLxNtfDecoded->psL1NtfDecoded->sInfo.vtxLDO;
    txVpp = ((vtxLDO + vtxAmp) * (1 - (residualCarrier * 0.00805))) *
            (2 - numDriver + 1);
    psLxNtfDecoded->psL1NtfDecoded->sInfo.txVpp = txVpp;
  }
}

/*******************************************************************************
 **
 ** Function:        decodeCLIFFState(psLxNtfCoded_t, psLxNtfDecodingInfo_t,
 **                  psLxNtfDecoded_t)
 **
 ** Description:     This functions decodes the current CLIFF state.
 **
 ** Returns:         void
 **
 ******************************************************************************/
void NCI_LxDebug_Decoder::decodeCLIFFState(
    psLxNtfCoded_t psLxNtfCoded, psLxNtfDecodingInfo_t psLxNtfDecodingInfo,
    psLxNtfDecoded_t psLxNtfDecoded) {
  if (psLxNtfDecodingInfo != nullptr) {
    decodeTriggerType(psLxNtfCoded, psLxNtfDecodingInfo, psLxNtfDecoded);
    decodeRFTechMode(psLxNtfCoded, psLxNtfDecodingInfo, psLxNtfDecoded);
  } else
    return;
}

/*******************************************************************************
 **
 ** Function:        decodeTriggerType(psLxNtfCoded_t, psLxNtfDecodingInfo_t,
 **                  psLxNtfDecoded_t)
 **
 ** Description:     This function decodes the CLIFF trigger type.
 **
 ** Returns:         void
 **
 ******************************************************************************/
void NCI_LxDebug_Decoder::decodeTriggerType(
    psLxNtfCoded_t psLxNtfCoded, psLxNtfDecodingInfo_t psLxNtfDecodingInfo,
    psLxNtfDecoded_t psLxNtfDecoded) {
  uint8_t base = psLxNtfDecodingInfo->baseIndex;
  uint8_t offsetTriggerType = psLxNtfDecodingInfo->cliffStateTriggerTypeOffset;

  if (psLxNtfCoded->pLxNtf[1] == SYSTEM_DEBUG_STATE_L1_MESSAGE) {
    switch ((psLxNtfCoded->pLxNtf[base + offsetTriggerType] & 0x0F)) {
      case CLF_L1_EVT_ACTIVATED:  // APC
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateTriggerType =
            mCLF_STAT_L1_TRIG_TYPE[1];
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateTriggerTypeDirection =
            mCLF_EVT_DIRECTION[0];
        break;
      case CLF_L1_EVT_DATA_RX:  // RSSI
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateTriggerType =
            mCLF_STAT_L1_TRIG_TYPE[2];
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateTriggerTypeDirection =
            mCLF_EVT_DIRECTION[0];
        break;
      case CLF_L1_EVT_RX_DESLECT:  // RSSI
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateTriggerType =
            mCLF_STAT_L1_TRIG_TYPE[3];
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateTriggerTypeDirection =
            mCLF_EVT_DIRECTION[0];
        break;
      case CLF_L1_EVT_RX_WTX:  // RSSI
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateTriggerType =
            mCLF_STAT_L1_TRIG_TYPE[4];
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateTriggerTypeDirection =
            mCLF_EVT_DIRECTION[0];
        break;
      case CLF_L1_EVT_ERROR:  // RSSI, EDD Error Types
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateTriggerType =
            mCLF_STAT_L1_TRIG_TYPE[5];
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateTriggerTypeDirection =
            mCLF_EVT_DIRECTION[0];
        break;
      case CLF_L1_EVT_RX_ACK:  // RSSI
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateTriggerType =
            mCLF_STAT_L1_TRIG_TYPE[6];
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateTriggerTypeDirection =
            mCLF_EVT_DIRECTION[0];
        break;
      case CLF_L1_EVT_RX_NACK:  // RSSI, IOT1, IOT2, IOT3, IOT4, IOT5
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateTriggerType =
            mCLF_STAT_L1_TRIG_TYPE[7];
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateTriggerTypeDirection =
            mCLF_EVT_DIRECTION[0];
        break;
      case CLF_L1_EVT_DATA_TX:  // APC , DPLL
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateTriggerType =
            mCLF_STAT_L1_TRIG_TYPE[8];
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateTriggerTypeDirection =
            mCLF_EVT_DIRECTION[1];
        break;
      case CLF_L1_EVT_WTX_AND_DATA_TX:  // APC, DPLL
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateTriggerType =
            mCLF_STAT_L1_TRIG_TYPE[9];
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateTriggerTypeDirection =
            mCLF_EVT_DIRECTION[1];
        break;
      case CLF_L1_EVT_TX_DESELECT:  // APC, DPLL
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateTriggerType =
            mCLF_STAT_L1_TRIG_TYPE[10];
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateTriggerTypeDirection =
            mCLF_EVT_DIRECTION[1];
        break;
      case CLF_L1_EVT_TX_WTX:  // APC, DPLL
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateTriggerType =
            mCLF_STAT_L1_TRIG_TYPE[11];
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateTriggerTypeDirection =
            mCLF_EVT_DIRECTION[1];
        break;
      case CLF_L1_EVT_TX_ACK:  // APC, DPLL
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateTriggerType =
            mCLF_STAT_L1_TRIG_TYPE[12];
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateTriggerTypeDirection =
            mCLF_EVT_DIRECTION[1];
        break;
      case CLF_L1_EVT_TX_NAK:  // APC, DPLL
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateTriggerType =
            mCLF_STAT_L1_TRIG_TYPE[13];
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateTriggerTypeDirection =
            mCLF_EVT_DIRECTION[1];
        break;
      case CLF_L1_EVT_EXTENDED:  // APC, 7816-4 Return Code
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateTriggerType =
            mCLF_STAT_L1_TRIG_TYPE[14];
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateTriggerTypeDirection =
            mCLF_EVT_DIRECTION[1];
        break;
      default:
        break;
    }
  }
}

/*******************************************************************************
 **
 ** Function:        decodeRFTechMode(psLxNtfCoded_t, psLxNtfDecodingInfo_t,
 **                  psLxNtfDecoded_t)
 **
 ** Description:     This function decodes CLIFF RF Tech & Mode
 **
 ** Returns:         void
 **
 ******************************************************************************/
void NCI_LxDebug_Decoder::decodeRFTechMode(
    psLxNtfCoded_t psLxNtfCoded, psLxNtfDecodingInfo_t psLxNtfDecodingInfo,
    psLxNtfDecoded_t psLxNtfDecoded) {
  uint8_t base = psLxNtfDecodingInfo->baseIndex;
  uint8_t offsetRFTechMode = psLxNtfDecodingInfo->cliffStateRFTechModeOffset;

  if (psLxNtfCoded->pLxNtf[1] == SYSTEM_DEBUG_STATE_L1_MESSAGE) {
    switch ((psLxNtfCoded->pLxNtf[base + offsetRFTechMode] & 0xF0)) {
      case CLF_STATE_TECH_CE_A:  // APC for Tx Events
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateRFTechNMode =
            mCLF_STAT_RF_TECH_MODE[1];
        break;
      case CLF_STATE_TECH_CE_B:  // APC for Tx Events
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateRFTechNMode =
            mCLF_STAT_RF_TECH_MODE[2];
        break;
      case CLF_STATE_TECH_CE_F:  // APC for Tx Events
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateRFTechNMode =
            mCLF_STAT_RF_TECH_MODE[3];
        break;
      case CLF_STATE_TECH_NFCIP1_TARGET_PASSIVE_A:
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateRFTechNMode =
            mCLF_STAT_RF_TECH_MODE[4];
        break;
      case CLF_STATE_TECH_NFCIP1_TARGET_PASSIVE_F:
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateRFTechNMode =
            mCLF_STAT_RF_TECH_MODE[5];
        break;
      case CLF_STATE_TECH_NFCIP1_TARGET_ACTIVE_A:
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateRFTechNMode =
            mCLF_STAT_RF_TECH_MODE[6];
        break;
      case CLF_STATE_TECH_NFCIP1_TARGET_ACTIVE_F:
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateRFTechNMode =
            mCLF_STAT_RF_TECH_MODE[7];
        break;
      case CLF_STATE_TECH_RM_A:
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateRFTechNMode =
            mCLF_STAT_RF_TECH_MODE[8];
        break;
      case CLF_STATE_TECH_RM_B:
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateRFTechNMode =
            mCLF_STAT_RF_TECH_MODE[9];
        break;
      case CLF_STATE_TECH_RM_F:
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateRFTechNMode =
            mCLF_STAT_RF_TECH_MODE[10];
        break;
      case CLF_STATE_TECH_NFCIP1_INITIATOR_PASSIVE_A:
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateRFTechNMode =
            mCLF_STAT_RF_TECH_MODE[11];
        break;
      case CLF_STATE_TECH_NFCIP1_INITIATOR_PASSIVE_B:
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateRFTechNMode =
            mCLF_STAT_RF_TECH_MODE[12];
        break;
      case CLF_STATE_TECH_NFCIP1_INITIATOR_PASSIVE_F:
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateRFTechNMode =
            mCLF_STAT_RF_TECH_MODE[13];
        break;
      default:
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pCliffStateRFTechNMode =
            mCLF_STAT_RF_TECH_MODE[0];
        break;
    }
  }
}

/*******************************************************************************
 **
 ** Function:        decodeExtraDbgData(psLxNtfCoded_t, psLxNtfDecodingInfo_t,
 **                  psLxNtfDecoded_t)
 **
 ** Description:     This function decodes Extra Debug Data in case of Errors
 **
 ** Returns:         void
 **
 ******************************************************************************/
void NCI_LxDebug_Decoder::decodeExtraDbgData(
    psLxNtfCoded_t psLxNtfCoded, psLxNtfDecodingInfo_t psLxNtfDecodingInfo,
    psLxNtfDecoded_t psLxNtfDecoded) {
  uint8_t base = 0;
  uint8_t offsetEDD = 0;
  // uint8_t offsetEDDFelica = 0;

  if (psLxNtfDecodingInfo != nullptr) {
    base = psLxNtfDecodingInfo->baseIndex;
  } else
    return;

  if (psLxNtfCoded->pLxNtf[1] == SYSTEM_DEBUG_STATE_L1_MESSAGE) {
    offsetEDD = psLxNtfDecodingInfo->eddOffset;

    switch (psLxNtfCoded->pLxNtf[base + offsetEDD]) {
      case L1_ERROR_EDD_RF_TIMEOUT:
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pEddL1Error = mEDD_L1_ERROR[0];
        break;
      case L1_ERROR_EDD_RF_CRC_ERROR:
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pEddL1Error = mEDD_L1_ERROR[1];
        break;
      case L1_ERROR_EDD_RF_COLLISION:
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pEddL1Error = mEDD_L1_ERROR[2];
        break;
      case L1_ERROR_EDD_RX_DATA_OVERFLOW:
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pEddL1Error = mEDD_L1_ERROR[3];
        break;
      case L1_ERROR_EDD_RX_PROTOCOL_ERROR:
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pEddL1Error = mEDD_L1_ERROR[4];
        break;
      case L1_ERROR_EDD_TX_NO_DATA_ERROR:
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pEddL1Error = mEDD_L1_ERROR[5];
        break;
      case L1_ERROR_EDD_EXTERNAL_FIELD_ERROR:
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pEddL1Error = mEDD_L1_ERROR[6];
        break;
      case L1_ERROR_EDD_RXDATA_LENGTH_ERROR:
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pEddL1Error = mEDD_L1_ERROR[7];
        break;
      case L1_TX_EVT_EDD_DPLL_UNLOCKED:
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pEddL1TxErr = mEDD_L1_TX_ERROR;
        break;
      case L1_RX_NACK_EDD_IOT_STAGE1:
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pEddL1RxNak = mEDD_L1_RX_NAK[0];
        break;
      case L1_RX_NACK_EDD_IOT_STAGE2:
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pEddL1RxNak = mEDD_L1_RX_NAK[1];
        break;
      case L1_RX_NACK_EDD_IOT_STAGE3:
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pEddL1RxNak = mEDD_L1_RX_NAK[2];
        break;
      case L1_RX_NACK_EDD_IOT_STAGE4:
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pEddL1RxNak = mEDD_L1_RX_NAK[3];
        break;
      case L1_RX_NACK_EDD_IOT_STAGE5:
        psLxNtfDecoded->psL1NtfDecoded->sInfo.pEddL1RxNak = mEDD_L1_RX_NAK[4];
        break;
      default:
        break;
    }
  }
}

/*******************************************************************************
 **
 ** Function:        decode78164RetCode(psLxNtfCoded_t, psLxNtfDecodingInfo_t,
 **                  psLxNtfDecoded_t)
 **
 ** Description:     This function decodes the ISO-7816-4 Return Code.
 **
 ** Returns:         void
 **
 ******************************************************************************/
void NCI_LxDebug_Decoder::decode78164RetCode(
    psLxNtfCoded_t psLxNtfCoded, psLxNtfDecodingInfo_t psLxNtfDecodingInfo,
    psLxNtfDecoded_t psLxNtfDecoded) {
  uint8_t base = 0;
  uint8_t offsetRetCode78164 = 0;
  uint8_t retCode78164[2] = {0};

  if (psLxNtfDecodingInfo != nullptr) {
    base = psLxNtfDecodingInfo->baseIndex;
    offsetRetCode78164 = psLxNtfDecodingInfo->retCode78164Offset;
  } else
    return;

  retCode78164[1] = psLxNtfCoded->pLxNtf[base + offsetRetCode78164];
  offsetRetCode78164++;
  retCode78164[0] = psLxNtfCoded->pLxNtf[base + offsetRetCode78164];

  psLxNtfDecoded->psL1NtfDecoded->sInfo.eddL178164RetCode[0] = retCode78164[0];
  psLxNtfDecoded->psL1NtfDecoded->sInfo.eddL178164RetCode[1] = retCode78164[1];
}

/*******************************************************************************
 **
 ** Function:        toString(std::ostringstream & oss)
 **
 ** Description:     Converts this object to ostringstream
 **
 ** Returns:         void
 **
 ******************************************************************************/
void sL2RxEvent_t::toString(std::ostringstream& oss) {
  // Get pointer to container of this sL2RxEvent object
  psL2DecodedInfo_t decodedInfo =
      (psL2DecodedInfo_t)((char*)this -
                          ((size_t) & ((psL2DecodedInfo_t)0)->u.sL2RxEvent));
  std::ios::fmtflags flgs(oss.flags());

  oss << ", RSSI = 0x";
  for (size_t i = 0; i < NUM_RSSI_BYTES; i++) {
    oss << std::setfill('0') << std::setw(2) << std::uppercase << std::hex
        << (int)this->rssi[i];
  }
  oss << ", Trigger Type = 0x" << std::uppercase << std::hex
      << (int)(this->clifState & 0x0F);
  oss << ", RF Tech and Mode = 0x" << std::uppercase << std::hex
      << (int)((this->clifState & 0xF0) >> 4);
  if (decodedInfo->len == L2_EVT_RX_TAG_ID_EXTRA_DBG_LEN) {
    oss << ", Error Info = 0x" << std::setfill('0') << std::setw(2)
        << std::uppercase << std::hex << (int)this->extraData;
  }
  oss.flags(flgs);
}

/*******************************************************************************
 **
 ** Function:        toString(std::ostringstream & oss)
 **
 ** Description:     Converts this object to ostringstream
 **
 ** Returns:         void
 **
 ******************************************************************************/
void sL2TxEvent_t::toString(std::ostringstream& oss) {
  // Get pointer to container of this sL2TxEvent object
  psL2DecodedInfo_t decodedInfo =
      (psL2DecodedInfo_t)((char*)this -
                          ((size_t) & ((psL2DecodedInfo_t)0)->u.sL2TxEvent));
  std::ios::fmtflags flgs(oss.flags());

  oss << ", DLMA = 0x";
  for (size_t i = 0; i < NUM_DLMA_BYTES; i++) {
    oss << std::setfill('0') << std::setw(2) << std::uppercase << std::hex
        << (int)this->dlma[i];
  }
  oss << ", Trigger Type = 0x" << std::uppercase << std::hex
      << (int)(this->clifState & 0x0F);
  oss << ", RF Tech and Mode = 0x" << std::uppercase << std::hex
      << (int)((this->clifState & 0xF0) >> 4);
  if (decodedInfo->len == L2_EVT_TX_TAG_ID_EXTRA_DBG_LEN) {
    oss << ", Error Info = 0x" << std::setfill('0') << std::setw(2)
        << std::uppercase << std::hex << (int)this->extraData;
  }
  oss.flags(flgs);
}

/*******************************************************************************
 **
 ** Function:        toString(std::ostringstream & oss)
 **
 ** Description:     Converts this object to ostringstream
 **
 ** Returns:         void
 **
 ******************************************************************************/
void sL2FelicaCmdEvent_t::toString(std::ostringstream& oss) {
  // Get pointer to container of this sL2FelicaCmdEvent object
  psL2DecodedInfo_t decodedInfo =
      (psL2DecodedInfo_t)((char*)this -
                          ((size_t) &
                           ((psL2DecodedInfo_t)0)->u.sL2FelicaCmdEvent));
  std::ios::fmtflags flgs(oss.flags());

  oss << ", RSSI = 0x";
  for (size_t i = 0; i < NUM_RSSI_BYTES; i++) {
    oss << std::setfill('0') << std::setw(2) << std::uppercase << std::hex
        << (int)this->rssi[i];
  }
  oss << ", Felica Cmd = 0x" << std::setfill('0') << std::setw(2)
      << std::uppercase << std::hex << (int)this->felicaCmd;
  if (decodedInfo->len == L2_EVT_FELICA_CMD_TAG_ID_EXTRA_DBG_LEN) {
    oss << ", Extra Info = 0x" << std::setfill('0') << std::setw(2)
        << std::uppercase << std::hex << (int)this->extraData;
  }
  oss.flags(flgs);
}

/*******************************************************************************
 **
 ** Function:        toString(std::ostringstream & oss)
 **
 ** Description:     Converts this object to ostringstream
 **
 ** Returns:         void
 **
 ******************************************************************************/
void sL2FelicaSysCodeEvent_t::toString(std::ostringstream& oss) {
  std::ios::fmtflags flgs(oss.flags());
  oss << ", System Code = 0x";
  for (size_t i = 0; i < NUM_SYSCODE_BYTES; i++) {
    oss << std::setfill('0') << std::setw(2) << std::uppercase << std::hex
        << (int)this->felicaCmd[i];
  }
  oss.flags(flgs);
}

/*******************************************************************************
 **
 ** Function:        toString(std::ostringstream & oss)
 **
 ** Description:     Converts this object to ostringstream
 **
 ** Returns:         void
 **
 ******************************************************************************/
void sL2FelicaRspEvent_t::toString(std::ostringstream& oss) {
  // Get pointer to container of this sL2FelicaRspEvent object
  psL2DecodedInfo_t decodedInfo =
      (psL2DecodedInfo_t)((char*)this -
                          ((size_t) &
                           ((psL2DecodedInfo_t)0)->u.sL2FelicaRspEvent));
  std::ios::fmtflags flgs(oss.flags());

  oss << ", DLMA = 0x";
  for (size_t i = 0; i < NUM_DLMA_BYTES; i++) {
    oss << std::setfill('0') << std::setw(2) << std::uppercase << std::hex
        << (int)this->dlma[i];
  }
  oss << ", Felica response = 0x" << std::setfill('0') << std::setw(2)
      << std::uppercase << std::hex << (int)this->felicaRsp;

  oss << ", Response status flag = 0x";
  for (size_t i = 0; i < NUM_FELICA_RES_STATUS_BYTES; i++) {
    oss << std::setfill('0') << std::setw(2) << std::uppercase << std::hex
        << (int)this->responseStatus[i];
  }
  if (decodedInfo->len == L2_EVT_FELICA_RSP_CODE_TAG_ID_EXTRA_DBG_LEN) {
    oss << ", Extra data = 0x" << std::setfill('0') << std::setw(2)
        << std::uppercase << std::hex << (int)this->extraData;
  }
  oss.flags(flgs);
}

/*******************************************************************************
 **
 ** Function:        toString(std::ostringstream & oss)
 **
 ** Description:     Converts this object to ostringstream
 **
 ** Returns:         void
 **
 ******************************************************************************/
void sL2FelicaMiscEvent_t::toString(std::ostringstream& oss) {
  // Get pointer to container of this sL2FelicaMiscEvent object
  psL2DecodedInfo_t decodedInfo =
      (psL2DecodedInfo_t)((char*)this -
                          ((size_t) &
                           ((psL2DecodedInfo_t)0)->u.sL2FelicaMiscEvent));
  std::ios::fmtflags flgs(oss.flags());

  oss << ", Event trigger = 0x" << std::setfill('0') << std::setw(2)
      << std::uppercase << std::hex << (int)this->trigger;
  if (decodedInfo->len == L2_EVT_FELICA_MISC_TAG_ID_EXTRA_DBG_LEN) {
    oss << ", Error = 0x" << std::setfill('0') << std::setw(2) << std::uppercase
        << std::hex << (int)this->extraData;
  }
  oss.flags(flgs);
}

/*******************************************************************************
 **
 ** Function:        toString(std::ostringstream & oss)
 **
 ** Description:     Converts this object to ostringstream
 **
 ** Returns:         void
 **
 ******************************************************************************/
void sL2CmaEvent_t::toString(std::ostringstream& oss) {
  // Get pointer to container of this sL2CmaEvent object
  psL2DecodedInfo_t decodedInfo =
      (psL2DecodedInfo_t)((char*)this -
                          ((size_t) & ((psL2DecodedInfo_t)0)->u.sL2CmaEvent));
  std::ios::fmtflags flgs(oss.flags());

  oss << ", Event trigger = 0x" << std::setfill('0') << std::setw(2)
      << std::uppercase << std::hex << (int)this->trigger;
  if (decodedInfo->len > L2_EVT_CMA_TAG_ID_MIN_LEN) {
    oss << ", Extra data = 0x";
    for (size_t i = 0; i < (decodedInfo->len - L2_EVT_CMA_TAG_ID_MIN_LEN);
         i++) {
      oss << std::setfill('0') << std::setw(2) << std::uppercase << std::hex
          << (int)this->extraData[i];
    }
  }
  oss.flags(flgs);
}
