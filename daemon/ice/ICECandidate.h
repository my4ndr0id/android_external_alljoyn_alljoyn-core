/**
 * @file ICECandidate.h
 *
 */

/******************************************************************************
 * Copyright 2009,2012 Qualcomm Innovation Center, Inc.
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

#ifndef _ICECANDIDATE_H
#define _ICECANDIDATE_H

#include <qcc/platform.h>

#include <string>
#include <qcc/IPAddress.h>
#include <qcc/SocketTypes.h>
#include <qcc/Config.h>
#include <qcc/Thread.h>
#include <qcc/ManagedObj.h>
#include <Stun.h>

using namespace std;
using namespace qcc;

namespace ajn {

// forward declarations
class Component;
class _ICECandidate;
class ICECandidatePair;
class StunActivity;

// Types
typedef qcc::ManagedObj<_ICECandidate>  ICECandidate;

class FoundationAttrs {
  public:
    ICECandidate IceCandidate;
    uint16_t candidateType;
    IPAddress baseAddr;
    IPAddress serverAddr;
    SocketType transportProtocol;

    FoundationAttrs(
        const ICECandidate& IceCandidate,
        uint16_t candidateType,
        IPAddress baseAddr,
        IPAddress serverAddr,
        SocketType transportProtocol) :
        IceCandidate(IceCandidate),
        candidateType(candidateType),
        baseAddr(baseAddr),
        serverAddr(serverAddr),
        transportProtocol(transportProtocol) { }

    FoundationAttrs() :
        IceCandidate(),
        candidateType(),
        baseAddr(),
        serverAddr(),
        transportProtocol() { }

    /**
     * Compare inequality between 2 FoundationAttr's.
     *
     * @param other     The other to compare against.
     *
     * @return  'true' if they are not the same, 'false' otherwise.
     */
    bool operator!=(const FoundationAttrs& other) const
    {
        return (this->candidateType != other.candidateType ||
                this->baseAddr != other.baseAddr ||
                this->serverAddr != other.serverAddr ||
                this->transportProtocol != other.transportProtocol);
    }


};

/**
 * ICECandidate is a local or remote address and port that is a potential point
 * of contact for receipt of media.
 */
class _ICECandidate {
  public:
    /** Candidate Type */
    /*  Important! Do not change the order of these enumerations. Used in
     *  assigning default candidates.
     */
    typedef enum {
        Invalid_Candidate = 0,       /**< Invalid candidate */
        Host_Candidate,              /**< Direct host candidate */
        ServerReflexive_Candidate,   /**< Outermost NAT candidate */
        PeerReflexive_Candidate,     /**< Intermediate NAT candidate learned during peer connection check */
        Relayed_Candidate            /**< TURN server candidate */
    } ICECandidateType;

    static ICECandidateType StrToCandidateType(String& s) {
        ICECandidateType type = Host_Candidate;
        if (s == "srflx") {
            type = ServerReflexive_Candidate;
        } else if (s == "relay") {
            type = Relayed_Candidate;
        } else if (s == "prflx") {
            type = PeerReflexive_Candidate;
        }
        return type;
    }

    /**
     * ICECandidate constructor
     *
     * @param type        Candidate type.
     * @param priority    Candidate priority.
     * @param endPoint    IP address and port by which candidate is known to other agents.
     * @param base        The base of a server reflexive candidate is the host candidate
     *                    from which it was derived.  A host candidate is also said to have
     *                    a base, equal to that candidate itself.  Similarly, the base of a
     *                    relayed candidate is that candidate itself.
     *
     * @param foundation  Identifier used determine whether candidates share common path (used for Freezing).
     */

    // Local Host/Server-Reflexive/Peer-Reflexive
    _ICECandidate(ICECandidateType type, IPEndpoint endPoint, IPEndpoint base, Component* component, SocketType transportProtocol, StunActivity* stunActivity, String interfaceName);

    // Local Relayed Candidate
    _ICECandidate(IPEndpoint endPoint, IPEndpoint base, IPEndpoint mappedAddress, uint32_t grantedAllocationLifetimeSecs,
                  Component* component, SocketType transportProtocol, StunActivity* stunActivity, StunActivity* permissionStunActivity);

    // Remote Candidate
    _ICECandidate(ICECandidateType type, IPEndpoint endPoint, Component* component, SocketType transportProtocol, uint32_t priority, String foundation);


    // Default Candidate
    _ICECandidate();

    ~_ICECandidate(void);

    bool operator==(const _ICECandidate& other) const { return this == &other; }

    Component* GetComponent(void) const { return component; }

    void SetMappedAddress(IPEndpoint addr) { mappedAddress = addr; }

    void SetAllocationLifetimeSeconds(uint32_t seconds) { grantedAllocationLifetimeSecs = seconds; }

    uint32_t GetAllocationLifetimeSeconds(void) const { return grantedAllocationLifetimeSecs; }

    void SetFoundation(String foundation) { this->foundation = foundation; }
    String GetFoundation(void) const { return foundation; }

    void SetPriority(uint32_t priority) { this->priority = priority; }
    uint32_t GetPriority(void) const { return priority; }
    String GetPriorityString(void) const;

    String GetTypeString(void) const;

    ICECandidateType GetType(void) const { return type; }

    IPEndpoint GetEndpoint(void) const { return endPoint; }
    IPEndpoint GetBase(void) const { return base; }
    IPEndpoint GetMappedAddress(void) const { return mappedAddress; }

    IPAddress GetServer(void) const;

    String GetTURNUserName(void) const;

    SocketType GetTransportProtocol(void) const { return transportProtocol; }

    FoundationAttrs* GetFoundationAttrs(void)
    {
        ICECandidate candidate(this);
        return new FoundationAttrs(candidate, (uint16_t)GetType(), GetBase().addr, GetServer(), GetTransportProtocol());
    }

    StunActivity* GetStunActivity(void) const { return stunActivity; }

    StunActivity* GetPermissionStunActivity(void) const { return permissionStunActivity; }

    QStatus StartListener(void);

    QStatus StopCheckListener(void);

    String GetInterfaceName(void) const { return InterfaceName; };


  private:
    /** Candiate type */
    ICECandidateType type;

    /** Candidate priority */
    uint32_t priority;

    /** IP Endpoint of this candidate */
    IPEndpoint endPoint;

    /** Base Address */
    IPEndpoint base;

    /** for a relayed-candidate, this is the associated mapped-address */
    IPEndpoint mappedAddress;

    /** for a relayed-candidate, this is the number of seconds until the
        TURN server expires the allocation */
    uint32_t grantedAllocationLifetimeSecs;

    /** ICECandidate foundation */
    String foundation;

    /** Component for which this is a candidate */
    Component*component;

    SocketType transportProtocol;

    StunActivity* stunActivity; // allocated/deallocated in Component

    StunActivity* permissionStunActivity;

    bool terminating;

    ICECandidate* sharedStunRelayedCandidate;
    ICECandidate* sharedStunServerReflexiveCandidate;

    class ICECandidateThread : public qcc::Thread {
      public:
        ICECandidateThread(ICECandidate* candidate) :
            qcc::Thread("iceCand"), candidate(candidate) { }

        ThreadReturn STDCALL Run(void* arg)
        {
            (*candidate)->AwaitRequestsAndResponses();
            return 0;
        }

        void ThreadExit(qcc::Thread* p)
        {
            /* Cleanup Candidate */
            (*candidate)->candidateThread = NULL;

            /* Decrement candidate reference */
            delete candidate;

            /* Remove self */
            delete this;
        }

      private:
        ICECandidate* candidate;
    };

    ICECandidateThread* candidateThread;

    /* Private copy constructor */
    _ICECandidate(const _ICECandidate& other);

    /* Private assignment operator */
    const _ICECandidate& operator=(const _ICECandidate& other);

    // The following methods only used by a Host_Candidate
    void AwaitRequestsAndResponses(void);

    QStatus ReadReceivedMessage(uint32_t timeoutMsec);

    QStatus SendResponse(uint16_t checkStatus, IPEndpoint& dest, bool usingTurn, StunTransactionID tid);


    String InterfaceName;

};

} // namespace ajn

#endif
