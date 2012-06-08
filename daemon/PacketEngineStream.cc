/**
 * @file
 * PacketEngineStream is an implementation of qcc::Stream used by PacketEngine.
 */

/******************************************************************************
 * Copyright 2011-2012, Qualcomm Innovation Center, Inc.
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

#include <qcc/platform.h>

#include <algorithm>
#include <limits>

#include "PacketEngineStream.h"
#include "PacketEngine.h"

#define QCC_MODULE "PACKET"

/* Constants */


using namespace std;
using namespace qcc;

namespace ajn {

PacketEngineStream::PacketEngineStream() :
    engine(NULL),
    chanId(0),
    sourceEvent(NULL),
    sinkEvent(NULL),
    sendTimeout(Event::WAIT_FOREVER)
{
}

PacketEngineStream::PacketEngineStream(PacketEngine& engine, uint32_t chanId, Event& sourceEvent, Event& sinkEvent) :
    engine(&engine),
    chanId(chanId),
    sourceEvent(&sourceEvent),
    sinkEvent(&sinkEvent),
    sendTimeout(Event::WAIT_FOREVER)
{
}

PacketEngineStream::PacketEngineStream(const PacketEngineStream& other) :
    engine(other.engine),
    chanId(other.chanId),
    sourceEvent(other.sourceEvent),
    sinkEvent(other.sinkEvent),
    sendTimeout(other.sendTimeout)
{
}

PacketEngineStream& PacketEngineStream::operator=(const PacketEngineStream& other)
{
    engine = other.engine;
    chanId = other.chanId;
    sourceEvent = other.sourceEvent;
    sinkEvent = other.sinkEvent;
    sendTimeout = other.sendTimeout;
    return *this;
}

bool PacketEngineStream::operator==(const PacketEngineStream& other) const
{
    return (chanId == other.chanId) && (engine == other.engine);
}


PacketEngineStream::~PacketEngineStream()
{
}

QStatus PacketEngineStream::PullBytes(void* buf, size_t reqBytes, size_t& actualBytes, uint32_t timeout)
{
    PacketEngine::ChannelInfo* ci = engine->AcquireChannelInfo(chanId);
    if (!ci) {
        return ER_SOCK_OTHER_END_CLOSED;
    }

    /* Check for expired messages and skip them */
    QStatus status = ER_OK;
    ci->rxLock.Lock();
    uint64_t now = GetTimestamp64();
    uint16_t drain = ci->rxDrain;
    bool inExpiredMsg = false;
    while (true) {
        Packet*& p = ci->rxPackets[drain % ci->windowSize];
        if (drain == ci->rxFill) {
            /* Wait for more packets to arrive */
            sourceEvent->ResetEvent();
            ci->rxLock.Unlock();
            status = Event::Wait(*sourceEvent, timeout);
            ci->rxLock.Lock();
            if (status == ER_OK) {
                if (ci->state == PacketEngine::ChannelInfo::OPEN) {
                    drain = ci->rxDrain;
                } else {
                    status = ER_SOCK_OTHER_END_CLOSED;
                    break;
                }
            } else {
                break;
            }
        } else if (!p) {
            inExpiredMsg = true;
            ci->rxDrain = ++drain;
        } else {
            if (p->flags & PACKET_FLAG_BOM) {
                inExpiredMsg = (p->expireTs < now);
                if (inExpiredMsg) {
                    engine->pool.ReturnPacket(p);
                    p = NULL;
                }
                ci->rxDrain = drain;
            } else if (inExpiredMsg) {
                engine->pool.ReturnPacket(p);
                p = NULL;
                ci->rxDrain = drain;
            }
            if (!inExpiredMsg && (p->flags & PACKET_FLAG_EOM)) {
                break;
            }
            drain++;
        }
        /* Check whether we need to send XON */
        /* Flow on is triggered if packet that caused flow off is not at very last position in rcv window or if rcv buf is empty */
        if (ci->rxFlowOff && ((ci->rxDrain == ci->rxAck) || IN_WINDOW(uint16_t, ci->rxDrain, ci->windowSize - 2 - XON_THRESHOLD, ci->rxFlowSeqNum))) {
            ci->rxFlowOff = false;
            engine->SendXOn(*ci);
            engine->txPacketThread.Alert();
        }
    }

    /* Copy packets starting at rxDrain to user's buffer */
    if (status == ER_OK) {
        bool wasLast = false;
        actualBytes = 0;
        while (reqBytes > actualBytes) {
            Packet*& p = ci->rxPackets[ci->rxDrain % ci->windowSize];
            assert(p);
            size_t copyLen = ::min(reqBytes - actualBytes, p->payloadLen - ci->rxPayloadOffset);
            ::memcpy(buf, reinterpret_cast<uint8_t*>(p->payload) + ci->rxPayloadOffset, copyLen);
            actualBytes += copyLen;
            buf = static_cast<uint8_t*>(buf) + copyLen;
            ci->rxPayloadOffset += copyLen;
            if (ci->rxPayloadOffset >= p->payloadLen) {
                wasLast = p->flags & PACKET_FLAG_EOM;
                engine->pool.ReturnPacket(p);
                p = NULL;
                ci->rxPayloadOffset = 0;
                ci->rxDrain++;
                if ((ci->rxDrain == ci->rxFill) || wasLast) {
                    break;
                }
            }
        }
        if (actualBytes > 0) {
            ci->rxIsMidMessage = !wasLast;
        }

        /* Clear data-available event in source if empty */
        if (ci->rxDrain == ci->rxFill) {
            sourceEvent->ResetEvent();
        }
    } else if (status == ER_ALERTED_THREAD) {
        status = ER_NONE;
    }
    /* Check whether we need to send XON */
    /* Flow on is triggered if packet that caused flow off is not at very last position in rcv window or if rcv buf is empty */
    if (ci->rxFlowOff && ((ci->rxDrain == ci->rxAck) || IN_WINDOW(uint16_t, ci->rxDrain, ci->windowSize - 2 - XON_THRESHOLD, ci->rxFlowSeqNum))) {
        ci->rxFlowOff = false;
        engine->SendXOn(*ci);
        engine->txPacketThread.Alert();
    }
    ci->rxLock.Unlock();
    engine->ReleaseChannelInfo(*ci);

    return status;
}

QStatus PacketEngineStream::PushBytes(const void* buf, size_t numBytes, size_t& numSent, uint32_t ttl)
{
    PacketEngine::ChannelInfo* ci = engine->AcquireChannelInfo(chanId);
    if (!ci) {
        return ER_SOCK_OTHER_END_CLOSED;
    }

    uint64_t now = GetTimestamp64();
    QStatus status = ER_OK;
    numSent = 0;

    /* Check size of caller's message */
    size_t maxPayload = ::min(ci->packetStream.GetSinkMTU(), (size_t)engine->pool.GetMTU()) - Packet::payloadOffset;
    size_t numPackets = (numBytes + maxPayload - 1) / maxPayload;
    if (numPackets >= ci->windowSize) {
        return ER_PACKET_TOO_LARGE;
    }

    /* Make sure that there is room for the ENTIRE message before writing any part of it */
    ci->txLock.Lock();
    while (status == ER_OK) {
        uint16_t delta = ci->txFill - ci->txDrain;
        if (delta > ci->windowSize) {
            delta += ci->windowSize;
        }
        uint16_t room = ci->windowSize - delta - 1;
        if (room < numPackets) {
            sinkEvent->ResetEvent();
            ci->txLock.Unlock();
            uint32_t waitMs = (ttl != 0) ? ::min(ttl, sendTimeout) : sendTimeout;
            if (waitMs > 0) {
                status = Event::Wait(*sinkEvent, waitMs);
                /* Treat a ttl expiration as a successfully sent packet */
                if ((status == ER_TIMEOUT) && (waitMs == ttl)) {
                    engine->ReleaseChannelInfo(*ci);
                    numSent = numBytes;
                    return ER_OK;
                } else if ((status == ER_OK) && (ci->state != PacketEngine::ChannelInfo::OPEN)) {
                    engine->ReleaseChannelInfo(*ci);
                    return ER_SOCK_OTHER_END_CLOSED;
                }
            } else {
                status = ER_TIMEOUT;
            }
            ci->txLock.Lock();
        } else {
            break;
        }
    }

    /* Write packets */
    bool isFirst = true;
    while ((status == ER_OK) && (numSent < numBytes)) {
        Packet* p = engine->pool.GetPacket();
        size_t pLen = ::min(maxPayload, numBytes - numSent);
        p->SetPayload(reinterpret_cast<const uint8_t*>(buf) + numSent, pLen);
        p->chanId = ci->id;
        p->seqNum = ci->txFill;
        p->flags = isFirst ? PACKET_FLAG_BOM : 0;
        p->flags |= (numBytes - numSent) <= maxPayload ? PACKET_FLAG_EOM : 0;
        p->expireTs = (ttl == 0) ? numeric_limits<uint64_t>::max() : now + ttl;
        ci->txPackets[ci->txFill % ci->windowSize] = p;
        ci->txFill++;
        numSent += pLen;
        isFirst = false;
    }
    if (status == ER_OK) {
        engine->txPacketThread.Alert();
    }
    ci->txLock.Unlock();
    engine->ReleaseChannelInfo(*ci);

    return status;
}

}
