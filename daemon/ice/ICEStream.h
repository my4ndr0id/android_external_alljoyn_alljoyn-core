#ifndef _ICESTREAM_H
#define _ICESTREAM_H
/**
 * @file ICEStream.h
 *
 * ICEStream contains the state for a single media stream
 * for a session.  (Each session contains one or more streams)
 * A stream contains one or more components. e.g. RTP and RTCP
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

#include <list>
#include <queue>
#include <qcc/IPAddress.h>
#include <qcc/Thread.h>
#include <qcc/Mutex.h>
#include "ICECandidatePair.h"
#include "Status.h"
#include "RendezvousServerInterface.h"

using namespace qcc;

/** @internal */
#define QCC_MODULE "ICESTREAM"

using namespace std;

namespace ajn {

// Forward Declaration
class ICESession;

class ICEStream {
  public:

    /** ICE checks state for stream */
    typedef enum {
        CheckStateInitial,         /**< Checks have not yet begun.      */
        CheckStateRunning,         /**< Checks are still in progress.   */
        CheckStateCompleted,       /**< Checks have produced nominated pair(s) for
                                   **  each component of the stream. ICE has
                                   **  succeeded and media can be sent. */
        CheckStateFailed           /**< Checks have finished unsuccessfully. */
    } ICEStreamCheckListState;

    /** RTP 'RR and RS' Bandwidth values */
    typedef enum {
        Unspecified,
        BothAreZero,
        NotBothAreZero
    } BandwidthSpecifierType;   /**< Bandwidth specifier for RTP stream */


    ICEStream(uint16_t streamNumber, ICESession* session, STUNServerInfo stunInfo, const uint8_t* key, size_t keyLen, BandwidthSpecifierType bwSpec = Unspecified) :
        streamNumber(streamNumber),
        componentList(),
        session(session),
        bandwidthSpecifier(bwSpec),
        checkListState(CheckStateInitial),
        checkList(),
        checkListDispatcherThread(NULL),
        terminating(false),
        STUNInfo(stunInfo),
        hmacKey(key),
        hmacKeyLen(keyLen)
    { }


    ~ICEStream(void);

    QStatus AddComponent(AddressFamily af, SocketType socketType,
                         Component*& component,
                         Component*& implicitComponent);


    /// const_iterator typedef.
    typedef vector<Component*>::const_iterator const_iterator;
    typedef vector<Component*>::iterator iterator;

    const_iterator Begin(void) const { return componentList.begin(); }
    const_iterator End(void) const { return componentList.end(); }

    uint16_t GetICEStreamNumber(void) { return streamNumber; }

    ICESession* GetSession(void) const { return session; }

    void SortAndPruneCandidatePairs(void);

    QStatus ActivateCheckList(void);

    void AddCandidatePair(ICECandidatePair* checkPair);

    void AddCandidatePairByPriority(ICECandidatePair* checkPair);

    void AddRemoteCandidate(const ICECandidate& remoteCandidate);

    void ProcessCheckEvent(ICECandidatePair& requestPair,
                           ICECandidatePair::CheckStatus status,
                           IPEndpoint& mappedAddress);

    /*A check list with at least one pair that is Waiting is
       called an active check list, and a check list with all pairs frozen
       is called a frozen check list.
     */
    bool CheckListIsActive(void);

    bool CheckListIsFrozen(void);

    void CancelChecks(void);

    ICECandidate MatchRemoteCandidate(IPEndpoint& source, String& uniqueFoundation);

    ICECandidatePair* MatchCheckList(IPEndpoint& remoteEndpoint, StunTransactionID& tid);

    ICECandidatePair* MatchCheckListEndpoint(IPEndpoint& localBase, IPEndpoint& remoteEndpoint);

    ICEStreamCheckListState GetCheckListState(void) const { return checkListState; }

    void SetCheckListState(ICEStreamCheckListState state) { checkListState = state; }

    void RemoveWaitFrozenPairsForComponent(Component* component);

    void CeaseRetransmissions(Component* component, uint64_t lowestPairPriority);

    void SetTerminate(void);

    /// const_iterator typedef.
    typedef list<ICECandidate>::const_iterator constRemoteListIterator;
    constRemoteListIterator RemoteListBegin(void) const { return remoteCandidateList.begin(); }
    constRemoteListIterator RemoteListEnd(void) const { return remoteCandidateList.end(); }

    typedef list<ICECandidatePair*>::iterator checkListIterator;

    checkListIterator CheckListBegin(void) { return checkList.begin(); }
    checkListIterator CheckListEnd(void) { return checkList.end(); }

  private:

    bool ChecksFinished(void);

    void UpdateCheckListAndTimerState(void);

    QStatus StartCheckListDispatcher(void);

    void CheckListDispatcher(void);

    // Each active check list will have a thread with this
    // function as its entry point.
    static ThreadReturn STDCALL CheckListDispatcherThreadStub(void* pThis)
    {
        ICEStream* thisPtr = (ICEStream*)pThis;
        thisPtr->CheckListDispatcher();

        return 0;
    }

    ICECandidatePair* GetNextCheckPair(void);

    void UpdatePairStates(ICECandidatePair* pair);

    bool DiscoverPeerReflexive(IPEndpoint& mappedAddress, ICECandidatePair* pair, ICECandidate& peerReflexiveCandidate);

    void UnfreezeMatchingPairs(String foundation);

    void UnfreezeMatchingPairs(ICEStream* stream, Component* component);

    bool AtLeastOneMatchingPair(ICEStream* stream, Component* component, vector<ICECandidatePair*>& matchingList);

    void SetPairsWaiting(void);

    void GetAllReadyTriggeredCheckPairs(list<ICECandidatePair*>& foundList);

    void GetAllReadyOrdinaryCheckPairs(list<ICECandidatePair*>& foundList, bool& noWaitingPairs);

#ifndef NDEBUG
    void DumpChecklist(void);
#endif

    uint16_t streamNumber;

    vector<Component*> componentList;

    ICESession* session;

    BandwidthSpecifierType bandwidthSpecifier;

    ICEStreamCheckListState checkListState;

    list<ICECandidatePair*> checkList;

    Thread* checkListDispatcherThread;

    bool terminating;

    Mutex lock;

    list<ICECandidate> remoteCandidateList;

    typedef list<ICECandidate>::iterator remoteListIterator;

    STUNServerInfo STUNInfo;

    const uint8_t* hmacKey;

    size_t hmacKeyLen;
};

} //namespace ajn

#undef QCC_MODULE

#endif
