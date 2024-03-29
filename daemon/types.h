#ifndef _TYPES_H
#define _TYPES_H
/**
 * @file
 *
 * This file defines the STUN Message attribute types.
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


/**
 * Enumeration of STUN message attribute type IDs.  These are defined in IETF
 * RFC 5389.
 */
enum StunAttrType {
    /******************************************************
     * Attributes defined in RFC 5389
     */
    STUN_ATTR_MAPPED_ADDRESS =      0x0001,  ///< Reflexive address of the client.
    STUN_ATTR_USERNAME =            0x0006,  ///< USERNAME
    STUN_ATTR_MESSAGE_INTEGRITY =   0x0008,  ///< HMAC-SHA1 of the STUN message.
    STUN_ATTR_ERROR_CODE =          0x0009,  ///< Error indication about a failed request.
    STUN_ATTR_UNKNOWN_ATTRIBUTES =  0x000A,  ///< List of STUN attributes not understood by server.
    STUN_ATTR_XOR_MAPPED_ADDRESS =  0x0020,  ///< Reflexive address of the client XOR'd with the
                                             ///  magic cookie and the Transaction ID as described
                                             ///  in RFC 5289.
    STUN_ATTR_SOFTWARE =            0x8022,  ///< Textual description of software sending the message.
    STUN_ATTR_ALTERNATE_SERVER =    0x8023,  ///< Alternate STUN/TURN server client should try.
    STUN_ATTR_FINGERPRINT =         0x8028,  ///< CRC-32 (XOR'd w/ 0x5354554e) of the STUN message.


    /******************************************************
     * Attributes defined in DRAFT-IETF-MMUSIC-ICE-19
     */
    STUN_ATTR_PRIORITY =            0x0024,  ///< Peer reflexive address priority.
    STUN_ATTR_USE_CANDIDATE =       0x0025,  ///< Use candidate pair from this check.
    STUN_ATTR_ICE_CONTROLLED =      0x8029,  ///< ICE client believes it's in the controlled role.
    STUN_ATTR_ICE_CONTROLLING =     0x802A,  ///< ICE client believes it's in the controlling role.


    /******************************************************
     * Attributes defined in BEHAVE-TURN-13
     */
    STUN_ATTR_CHANNEL_NUMBER =      0x000C,  ///< Channel number.
    STUN_ATTR_LIFETIME =            0x000D,  ///< Number os second to maintain allocation.
    STUN_ATTR_XOR_PEER_ADDRESS =    0x0012,  ///< Peer's reflexive address XOR'd with the
                                             ///  magic cookie and the Transaction ID as described
                                             ///  in RFC 5289.
    STUN_ATTR_DATA =                0x0013,  ///< Application data to be relayed.
    STUN_ATTR_XOR_RELAYED_ADDRESS = 0x0016,  ///< Allocated TURN server address for use by this agent.
    STUN_ATTR_EVEN_PORT =           0x0018,  ///< Ensure that TURN server reserves an even port number and optionally port number + 1.
    STUN_ATTR_REQUESTED_TRANSPORT = 0x0019,  ///< Transport protocol for the allocated TURN resources.
    STUN_ATTR_DONT_FRAGMENT =       0x001A,  ///< Tells the TURN server to not fragment the data.
    STUN_ATTR_RESERVATION_TOKEN =   0x0022   ///< Token referencing allocated resources on the TURN server.

};


/**
 * Stun message error codes defined in RFC 5389 and BEHAVE-TURN-13.
 */
enum StunErrorCodes {
    // Error codes defined in RFC 5389
    STUN_ERR_CODE_TRY_ALTERNATE =                  300, ///< Try alternate server.
    STUN_ERR_CODE_BAD_REQUEST =                    400, ///< Request is malformed.
    STUN_ERR_CODE_UNAUTHORIZED =                   401, ///< Invalid credentials.
    STUN_ERR_CODE_UNKNOWN_ATTRIBUTE =              420, ///< Server did not understand an attribute.
    STUN_ERR_CODE_SERVER_ERROR =                   500, ///< Server has a temporary error.

    // Error codes defined in DRAFT-IETF-MMUSIC-ICE-19
    STUN_ERR_CODE_ROLE_CONFLICT =                  487, ///< Client needs to switch roles.

    // Error codes defined in BEHAVE-TURN-13
    STUN_ERR_CODE_FORBIDDEN =                      403, ///< Request denied.
    STUN_ERR_CODE_ALLOCATION_MISMATCH =            437, ///< Allocation mismatch.
    STUN_ERR_CODE_WRONG_CREDENTIALS =              441, ///< Credentials given does not match credentials used for allocation.
    STUN_ERR_CODE_UNSUPPORTED_TRANSPORT_PROTOCOL = 442, ///< Server does not support transport protocoal.
    STUN_ERR_CODE_ALLOCATION_QUOTA_REACHED =       486, ///< Allocation limit for username reached.
    STUN_ERR_CODE_INSUFFICIENT_CAPACITY =          508  ///< Server is out of resources.
};

#endif
