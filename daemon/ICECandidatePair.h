/**
 * @file ICECandidatePair.h
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

#ifndef _ICECANDIDATEPAIR_H
#define _ICECANDIDATEPAIR_H

#include <qcc/Thread.h>
#include <ICECandidate.h>
#include <Stun.h>
#include <StunRetry.h>

using namespace std;
using namespace qcc;

/** @internal */
#define QCC_MODULE "ICECANDIDATEPAIR"

namespace ajn {

/**
 * An ICECandidatePair describes a local and a remote ICECandidate used during an ICE Connection check.
 */
class ICECandidatePair {
  public:
    /** Connection check state for candidate pair */
    typedef enum {
        Frozen,          /**< Connection check is deferred until another check completes */
        Waiting,         /**< Connection check is ready to be performed but has not been started */
        InProgress,      /**< Connection check is in progress */
        Failed,          /**< Connection check has failed */
        Succeeded        /**< Connection check has succeeded */
    } ICEPairConnectionState;

    /** Error Code for check result (Keep CheckStatusToString consistent) */
    typedef enum {
        CheckSucceeded,
        CheckTimeout,
        CheckRoleConflict,
        CheckInconsistentTransactionID,
        CheckGenericFailed,
        CheckUnknownPair,
        CheckResponseSent
    } CheckStatus;

    String CheckStatusToString(CheckStatus status);


    /**
     * Constructor
     *
     * @param local     Local candidate used in pair.
     * @param remote    Remote candidate used in pair.
     */
    ICECandidatePair(const ICECandidate& local, const ICECandidate& remote, bool isDefault, uint64_t priority) :
        local(local), remote(remote), state(Frozen), isValid(false), checkRetry(),
        canceledRetry(), priority(priority),
        isDefault(isDefault), isNominated(false), isNominatedContingent(false), foundation(),
        useAggressiveNomination(), regularlyNominated(), controlTieBreaker(),
        bindRequestPriority(), isTriggered() {
        // the colon disambiguates "12" + "345" from "123" + "45"
        foundation = local.GetFoundation() + ":" + remote.GetFoundation();
    }

    ~ICECandidatePair(void);

    uint64_t GetPriority(void) const { return priority; };
    uint32_t GetBindRequestPriority(void) const { return bindRequestPriority; };

    void GetFoundation(String& foundation) const { foundation = this->foundation; }

    String GetFoundation(void) const { return foundation; }

    QStatus InitChecker(const uint64_t& controlTieBreaker,
                        bool useAggressiveNomination, const uint32_t& bindRequestPriority);

    QStatus InitChecker(ICECandidatePair& originalPair);

    void Check(void);

    StunTransactionID GetTransactionID(void) const { return checkRetry->GetTransactionID(); }

    bool EqualsCanceledTransactionID(const StunTransactionID& tid) const
    {
        return (NULL != canceledRetry &&
                canceledRetry->IsTransactionValid() && (tid == canceledRetry->GetTransactionID()));
    }

    uint64_t GetControlTieBreaker(void) const { return controlTieBreaker; }

    void SetNominated(void);

    void SetNominatedContingent(void) { isNominatedContingent = true; }

    bool IsNominated(void) const { return isNominated; }

    void AddTriggered(void);

    bool IsTriggered(void) { return isTriggered; }

    void RemoveTriggered(void) { isTriggered = false; }

    bool IsWorkRemaining(void);

    bool RetryTimedOut(void) { return checkRetry->RetryTimedOut(); }

    bool RetryAvailable(void) { return checkRetry->RetryAvailable(); }

    uint32_t GetQueuedTimeOffset() { return checkRetry->GetQueuedTimeOffset(); }

    ICECandidatePair* IncrementRetryAttempt(void);

    void UpdateNominatedFlag(void);

    void SetCanceled(void);

    CheckRetry* GetCheckRetryByTransaction(StunTransactionID& tid);


    /** Local ICECandidate */
    const ICECandidate& local;

    /** Remote ICECandidate */
    const ICECandidate& remote;

    /** Connection check state for this ICECandidatePair */
    ICEPairConnectionState state;

    bool isValid;

  private:

    CheckRetry* checkRetry;

    CheckRetry* canceledRetry;

    /** Connection check priority of this ICECandidatePair */
    uint64_t priority;

    bool isDefault;

    bool isNominated;

    bool isNominatedContingent;

    String foundation;

    bool useAggressiveNomination;

    bool regularlyNominated;

    uint64_t controlTieBreaker;

    uint32_t bindRequestPriority;

    bool isTriggered;

};

} //namespace ajn

#undef QCC_MODULE
#endif
