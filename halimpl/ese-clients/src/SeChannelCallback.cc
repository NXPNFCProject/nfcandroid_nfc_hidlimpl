#include <stdlib.h>
#include <cutils/log.h>
#include "SeChannelCallback.h"
#include "phNxpEse_Api.h"
  /** abstract class having pure virtual functions to be implemented be each
   * client  - spi, nfc etc**/

    /*******************************************************************************
    **
    ** Function:        Open
    **
    ** Description:     Initialize the channel.
    **
    ** Returns:         True if ok.
    **
    *******************************************************************************/
    int16_t SeChannelCallback :: open() { return SESTATUS_SUCCESS; }

    /*******************************************************************************
    **
    ** Function:        close
    **
    ** Description:     Close the channel.
    **
    ** Returns:         True if ok.
    **
    *******************************************************************************/
    bool SeChannelCallback::close(int16_t mHandle) {
      if (mHandle != 0)
        return true;
      else
        return false;
    }

    /*******************************************************************************
    **
    ** Function:        transceive
    **
    ** Description:     Send data to the secure element; read it's response.
    **                  xmitBuffer: Data to transmit.
    **                  xmitBufferSize: Length of data.
    **                  recvBuffer: Buffer to receive response.
    **                  recvBufferMaxSize: Maximum size of buffer.
    **                  recvBufferActualSize: Actual length of response.
    **                  timeoutMillisec: timeout in millisecond
    **
    ** Returns:         True if ok.
    **
    *******************************************************************************/
    bool SeChannelCallback::transceive(uint8_t* xmitBuffer, int32_t xmitBufferSize,
                               __attribute__((unused)) uint8_t* recvBuffer, int32_t recvBufferMaxSize,
                               __attribute__((unused)) int32_t& recvBufferActualSize,
                               int32_t timeoutMillisec) {
      phNxpEse_data cmdData;

      cmdData.len = xmitBufferSize;
      cmdData.p_data = xmitBuffer;

      recvBufferMaxSize++;
      timeoutMillisec++;
      return true;
    }

    /*******************************************************************************
    **
    ** Function:        doEseHardReset
    **
    ** Description:     Power OFF and ON to eSE during JCOP Update
    **
    ** Returns:         None.
    **
    *******************************************************************************/
    void SeChannelCallback::doEseHardReset() { return ;}

    /*******************************************************************************
    **
    ** Function:        getInterfaceInfo
    **
    ** Description:     NFCC and eSE feature flags
    **
    ** Returns:         None.
    **
    *******************************************************************************/
    uint8_t SeChannelCallback :: getInterfaceInfo() { return INTF_NFC; }
