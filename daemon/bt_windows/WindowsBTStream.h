/**
 * @file
 *
 * Stream class for reading and writing data to the Windows Bluetooth driver.
 */

/******************************************************************************
 *
 *
 * Copyright 2011, Qualcomm Innovation Center, Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 ******************************************************************************/

#ifndef _ALLJOYN_WINDOWSBTSTREAM_H
#define _ALLJOYN_WINDOWSBTSTREAM_H

#include <qcc/Stream.h>

#include <WinIoCtl.h>
#include "userKernelComm.h"

#include "BTAccessor.h"

using namespace qcc;

namespace ajn {

/**
 * Stream is a virtual class that defines a standard interface for a streaming source and sink.
 * WindowsBTStream is the implementation of this class for the Windows Bluetooth interface.
 */
class WindowsBTStream : public qcc::Stream {
  public:
    WindowsBTStream(BTH_ADDR address,
                    BTTransport::BTAccessor* accessor) :
        remoteDeviceAddress(address),
        sourceBytesWaiting(0),
        channelHandle(0),
        connectionStatus(ER_OK),
        btAccessor(accessor)
    {
    }

    /** Destructor */
    virtual ~WindowsBTStream()
    {
        btAccessor = NULL;
        channelHandle = 0;
        connectionStatus = ER_SOCK_OTHER_END_CLOSED;
    }

    /**
     * Pull bytes from the source.
     * The source is exhausted when ER_NONE is returned.
     *
     * @param buf          Buffer to store pulled bytes
     * @param reqBytes     Number of bytes requested to be pulled from source.
     * @param actualBytes  Actual number of bytes retrieved from source.
     * @param timeout      Time to wait to pull the requested bytes.
     * @return   ER_OK if successful. ER_NONE if source is exhausted. Otherwise an error.
     */
    virtual QStatus PullBytes(void* buf,
                              size_t reqBytes,
                              size_t& actualBytes,
                              uint32_t timeout = Event::WAIT_FOREVER);

    /**
     * Push zero or more bytes into the sink.
     *
     * @param buf          Buffer to store pulled bytes
     * @param numBytes     Number of bytes from buf to send to sink.
     * @param numSent      Number of bytes actually consumed by sink.
     * @return   ER_OK if successful.
     */
    virtual QStatus PushBytes(const void* buf, size_t numBytes, size_t& numSent);

    /**
     * Get the Event indicating that data is available when signaled.
     *
     * @return Event that is signaled when data is available.
     */
    virtual Event& GetSourceEvent() { return dataAvailable; }

    /**
     * Get the BTAccessor which created this stream.
     *
     * @return A pointer the BTAccessor which created this stream.
     */
    BTTransport::BTAccessor* GetAccessor() const { return btAccessor; }

    /**
     * Set the pointer to the BTAccessor which created this stream to NULL.
     * This is needed when the stream has not yet been deleted but the BTAccessor is
     * in the process of being deleted.
     */
    void OrphanStream(void) { btAccessor = NULL; }

    L2CAP_CHANNEL_HANDLE GetChannelHandle() const { return channelHandle; }

    void SetChannelHandle(L2CAP_CHANNEL_HANDLE channel) { channelHandle = channel; }

    BTH_ADDR GetRemoteDeviceAddress() const { return remoteDeviceAddress; }

    /**
     * Set the number of bytes waiting in the kernel buffer for this endpoint. If the
     * number is greater than zero then the Event dataAvailable is set to indicate
     * such. If it is zero then dataAvailable is reset.
     *
     * IMPORTANT: This method should ONLY be called via a message from the kernel. This
     * is because only the kernel knows how many bytes are there. Even if all the data
     * had just pulled out there could have been more bytes put in by a remote device in
     * a different thread.
     *
     * @param bytesWaiting The number of bytes waiting in the kernel buffer for this
     * channel.
     * @param status The status of the connection.
     */
    void SetSourceBytesWaiting(size_t bytesWaiting, QStatus status);

  private:
    mutable qcc::Mutex dataLock;

    BTTransport::BTAccessor* btAccessor;
    BTH_ADDR remoteDeviceAddress;
    L2CAP_CHANNEL_HANDLE channelHandle;
    volatile size_t sourceBytesWaiting;
    volatile QStatus connectionStatus;
    Event dataAvailable;
};

} /* namespace */

#endif