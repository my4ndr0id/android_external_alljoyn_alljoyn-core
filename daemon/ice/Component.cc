/**
 * @file Component.cpp
 *
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

#include <qcc/Config.h>
#include <Stun.h>
#include <ICECandidate.h>
#include <StunActivity.h>
#include <Component.h>
#include <StunCredential.h>
#include <ICEStream.h>
#include "RendezvousServerInterface.h"

#define QCC_MODULE "COMPONENT"

namespace ajn {

Component::~Component(void)
{
    EmptyActivityList();

    /* Disable the ICEComponents' threads that are managed by this component */
    list<ICECandidate>::iterator it = candidateList.begin();
    while (it != candidateList.end()) {
        (*it)->StopCheckListener();
        ++it;
    }
}

void Component::EmptyActivityList(void)
{
    while (!stunActivityList.empty()) {
        StunActivity* stunActivity = stunActivityList.front();

        // Because a stun object can be shared among candidates,
        // we only delete it once - here, from the Host_Candidate that allocated it.
        if (_ICECandidate::Host_Candidate == stunActivity->candidate->GetType()) {

            if (stunActivity->stun) {
                delete stunActivity->stun;
                stunActivity->stun = NULL;
            }
        }

        delete stunActivity;
        stunActivityList.pop_front();
    }
}


QStatus Component::AddCandidate(const ICECandidate& candidate)
{
    QStatus status = ER_OK;

    candidateList.push_back(candidate);

    return status;
}


QStatus Component::RemoveCandidate(ICECandidate& candidate)
{
    QStatus status = ER_FAIL;

    iterator iter;
    for (iter = candidateList.begin(); iter != candidateList.end(); ++iter) {
        if (candidate == (*iter)) {
            candidateList.erase(iter);
            status = ER_OK;
            break;
        }
    }

    return status;
}


void Component::AddToStunActivityList(StunActivity* stunActivity)
{
    stunActivityList.push_back(stunActivity);
}


QStatus Component::CreateHostCandidate(qcc::SocketType socketType, const qcc::IPAddress& addr, uint16_t port, qcc::String interfaceName)
{
    QStatus status = ER_OK;
    QCC_DbgTrace(("Component::CreateHostCandidate(socketType = %d, &addr = %s, port = %d interfaceName = %s)", socketType, addr.ToString().c_str(), port, interfaceName.c_str()));

    this->socketType = socketType;

#ifndef USE_SPECIFIED_PORTS_FOR_HOST_CANDIDATES
    // override specified port and request OS to assign an ephemeral port for us
    port = 0;
#endif
    Stun* stun = NULL;
    status = AddStun(addr, port, stun);

    if (ER_OK == status) {
        qcc::IPEndpoint host;
        host.addr = addr;
        host.port = port;

        StunActivity* stunActivity = new StunActivity(stun);
        AddToStunActivityList(stunActivity);

        _ICECandidate::ICECandidateType type = _ICECandidate::Host_Candidate;
        ICECandidate candidate(type, host, host, this, socketType, stunActivity, interfaceName);
        status = AddCandidate(candidate);

        if (ER_OK == status) {
            candidate->StartListener();
        }
    }

    return status;
}


void Component::AssignDefaultCandidate(const ICECandidate& candidate)
{
    if (candidate->GetType() > defaultCandidate->GetType()) {
        defaultCandidate = candidate;
    }
}


QStatus Component::AddStun(const qcc::IPAddress& address, uint16_t& port, Stun*& stun)
{
    QStatus status = ER_OK;

    QCC_DbgTrace(("Component::AddStun(&address = %s, &port = %d, *&stun = <>)", address.ToString().c_str(), port));

    stun = new Stun(socketType, this, STUNInfo, hmacKey, hmacKeyLen);

    status = stun->OpenSocket(af);
    if (ER_OK == status) {
        status = stun->Bind(address, port);
        if (ER_OK == status) {
            // see what port we were assigned
            qcc::IPAddress ignored;
            status = stun->GetLocalAddress(ignored, port);
        }
    }

    if (ER_OK == status) {
        QCC_DbgPrintf(("Add Stun: %s:%d", address.ToString().c_str(), port));
    } else {
        delete stun;
    }
    return status;
}

void Component::GetStunTurnServerAddress(qcc::String& address) const
{
    address = STUNInfo.address.ToString();
}


uint16_t Component::GetStunTurnServerPort(void) const
{
    uint16_t port = 0;

    port = STUNInfo.port;

    return port;
}


void Component::AddToValidList(ICECandidatePair* validPair)
{
    QCC_DbgPrintf(("AddToValidList isValid(current): %s, hasValidPair(current): %s, [local addr = %s port = %d], [remote addr = %s port = %d]",
                   (validPair->isValid ? "true" : "false"),
                   (hasValidPair ? "true" : "false"),
                   validPair->local->GetEndpoint().addr.ToString().c_str(),
                   validPair->local->GetEndpoint().port,
                   validPair->remote->GetEndpoint().addr.ToString().c_str(),
                   validPair->remote->GetEndpoint().port));

    validPair->isValid = true;

    // ensure exactly one instance
    validList.remove(validPair);
    validList.push_back(validPair);

    hasValidPair = true;
}

bool Component::FoundationMatchesValidPair(const qcc::String& foundation)
{
    bool match = false;
    const_iterator_validList iter;

    for (iter = validList.begin(); iter != validList.end(); ++iter) {
        if ((*iter)->GetFoundation() == foundation) {
            match = true;
            break;
        }
    }

    return match;
}


const uint8_t* Component::GetHmacKey(void) const {
    return hmacKey;
}

const size_t Component::GetHmacKeyLength(void) const {
    return hmacKeyLen;
}


void Component::SetSelectedIfHigherPriority(ICECandidatePair* pair)
{
    if (NULL == selectedPair ||
        (pair->GetPriority() > selectedPair->GetPriority())) {
        selectedPair = pair;
    }
}


QStatus Component::GetSelectedCandidatePair(ICECandidatePair*& pair)
{
    QStatus status = ER_OK;

    if (stream->GetCheckListState() != ICEStream::CheckStateCompleted) {
        status = ER_ICE_CHECKS_INCOMPLETE;
        pair = NULL;
    } else {
        // By definition, this is the highest priority nominated pair from the valid list;
        pair = selectedPair;
    }

    return status;
}


Retransmit* Component::GetRetransmitByTransaction(const StunTransactionID& tid) const
{
    Retransmit* retransmit = NULL;

    // iterate stunActivityList looking for activity
    list<StunActivity*>::const_iterator it;
    for (it = stunActivityList.begin(); it != stunActivityList.end(); ++it) {
        StunTransactionID transaction;
        if ((*it)->retransmit.GetTransactionID(transaction) &&
            transaction == tid) {
            retransmit = &(*it)->retransmit;
            break;
        }
    }

    return retransmit;
}


CheckRetry* Component::GetCheckRetryByTransaction(StunTransactionID tid) const
{
    CheckRetry* retransmit = NULL;

    // iterate checkLists looking for activity
    ICEStream::checkListIterator checkListIter;
    for (checkListIter = stream->CheckListBegin(); checkListIter != stream->CheckListEnd(); ++checkListIter) {
        if (NULL != (retransmit = (*checkListIter)->GetCheckRetryByTransaction(tid))) {
            break;
        }
    }

    return retransmit;
}

} //namespace ajn
