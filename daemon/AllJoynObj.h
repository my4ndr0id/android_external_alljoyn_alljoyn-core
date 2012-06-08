/**
 * @file
 * BusObject responsible for implementing the AllJoyn methods (org.alljoyn.Bus)
 * for messages directed to the bus.
 */

/******************************************************************************
 * Copyright 2010-2012, Qualcomm Innovation Center, Inc.
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
#ifndef _ALLJOYN_ALLJOYNOBJ_H
#define _ALLJOYN_ALLJOYNOBJ_H

#include <qcc/platform.h>
#include <vector>

#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/StringMapKey.h>
#include <qcc/Thread.h>
#include <qcc/time.h>
#include <qcc/SocketTypes.h>

#include <alljoyn/BusObject.h>
#include <alljoyn/Message.h>

#include "Bus.h"
#include "NameTable.h"
#include "RemoteEndpoint.h"
#include "Transport.h"
#include "VirtualEndpoint.h"

#if defined(QCC_OS_ANDROID)
#include "PermissionDB.h"
#endif

namespace ajn {

/** Forward Declaration */
class BusController;

/**
 * BusObject responsible for implementing the standard AllJoyn methods at org.alljoyn.Bus
 * for messages directed to the bus.
 */
class AllJoynObj : public BusObject, public NameListener, public TransportListener {
    friend class RemoteEndpoint;

  public:
    /**
     * Constructor
     *
     * @param bus            Bus to associate with org.freedesktop.DBus message handler.
     * @param router         The DaemonRouter associated with the bus.
     * @param busController  Controller that created this object.
     */
    AllJoynObj(Bus& bus, BusController* busController);

    /**
     * Destructor
     */
    ~AllJoynObj();

    /**
     * Initialize and register this DBusObj instance.
     *
     * @return ER_OK if successful.
     */
    QStatus Init();

    /**
     * Called when object is successfully registered.
     */
    void ObjectRegistered(void);

    /**
     * Respond to a bus request to bind a SessionPort.
     *
     * The input Message (METHOD_CALL) is expected to contain the following parameters:
     *   sessionPort  sessionPort    SessionPort identifier.
     *   opts         SessionOpts    SessionOpts that must be agreeable to any joiner.
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   resultCode   uint32   An ALLJOYN_BINDSESSIONPORT_* reply code (see AllJoynStd.h).
     *   sessionPort  uint16   SessionPort (same as input sessionPort unless SESSION_PORT_ANY was specified)
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void BindSessionPort(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Respond to a bus request to unbind a SessionPort.
     *
     * The input Message (METHOD_CALL) is expected to contain the following parameters:
     *   sessionPort  sessionPort    SessionPort to be unbound.
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   resultCode   uint32   An ALLJOYN_UNBINDSESSIONPORT_* reply code (see AllJoynStd.h).
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void UnbindSessionPort(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Respond to a bus request to join an existing session.
     *
     * The input Message (METHOD_CALL) is expected to contain the following parameters:
     *   creatorName  string        Name of session creator.
     *   sessionPort  SessionPort   SessionPort targeted for join request.
     *   opts         SessionOpts   Session options requested by the joiner.
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   resultCode   uint32        A ALLJOYN_JOINSESSION_* reply code (see AllJoynStd.h).
     *   sessionId    uint32        Session identifier.
     *   opts         SessionOpts   Final (negociated) session options.
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void JoinSession(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Respond to a bus request to leave a previously joined or created session.
     *
     * The input Message (METHOD_CALL) is expected to contain the following parameters:
     *   sessionId    uint32   Session identifier.
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   resultCode   uint32   A ALLJOYN_LEAVESESSION_* reply code (see AllJoynStd.h).
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void LeaveSession(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Respond to a bus request to advertise the existence of a remote AllJoyn instance.
     *
     * The input Message (METHOD_CALL) is expected to contain the following parameters:
     *   advertisedName  string   A locally obtained well-known name that should be advertised to external AllJoyn instances.
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   resultCode   uint32   A ALLJOYN_ADVERTISE_* reply code (see AllJoynStd.h)
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void AdvertiseName(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Respond to a bus request to cancel a previous advertisement.
     *
     * The input Message (METHOD_CALL) is expected to contain the following parameters:
     *   advertisedName  string   A previously advertised well-known name that no longer needs to be advertised.
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   resultCode   uint32   A ALLJOYN_ADVERTISE_* reply code (see AllJoynStd.h)
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void CancelAdvertiseName(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Respond to a bus request to look for advertisements from remote AllJoyn instances.
     *
     * The input Message (METHOD_CALL) is expected to contain the following parameters:
     *   namePrefix    string   A well-known name prefix that the caller wants to be notified of (via signal)
     *                          when a remote Bus instance is found that advertises a name that matches the prefix.
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   resultCode   uint32   A ALLJOYN_FINDNAME_* reply code (see AllJoynStd.h)
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void FindAdvertisedName(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Respond to a bus request to cancel a previous (successful) FindName request.
     *
     * The input Message (METHOD_CALL) is expected to contain the following parameters:
     *   namePrefix    string   The well-known name prefix that was used in a successful call to FindName.
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   resultCode   uint32   A ALLJOYN_CANCELFINDNAME_* reply code (see AllJoynStd.h)
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void CancelFindAdvertisedName(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Respond to a bus request to get a (streaming) file descritor for an existing session.
     *
     * The input Message (METHOD_CALL) is expected to contain the following parameters:
     *   sessionId   uint32    A session id that identifies an existing streaming session.
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   sessionFd   handle    The socket file descriptor for session.
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void GetSessionFd(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Respond to a bus request to set the link timeout for a given session.
     *
     * The input Message (METHOD_CALL) is expected to contain the following parameters:
     *   sessionId      uint32    A session id that identifies an existing streaming session.
     *   reqLinkTimeout uint32    Requested max number of seconds that an unresponsive comm link
     *                            will be monitored before delcaring the link dead via SessionLost.
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   resultCode     uint32    ALLJOYN_SETLINKTIMEOUT_REPLY_* value.
     *   actLinkTimeout uint32    Actual link timeout value.
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void SetLinkTimeout(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Add an alias to a Unix User ID
     * The input Message (METHOD_CALL) is expected to contain the following parameter
     *   aliasUID      uint32    The alias ID
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   resultCode    uint32    ALLJOYN_ALIASUNIXUSER_REPLY_* value.
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void AliasUnixUser(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Add a new Bus-to-bus endpoint.
     *
     * @param endpoint  Bus-to-bus endpoint to add.
     * @param ER_OK if successful
     */
    QStatus AddBusToBusEndpoint(RemoteEndpoint& endpoint);

    /**
     * Remove an existing Bus-to-bus endpoint.
     *
     * @param endpoint  Bus-to-bus endpoint to add.
     */
    void RemoveBusToBusEndpoint(RemoteEndpoint& endpoint);

    /**
     * Respond to a remote daemon request to attach a session through this daemon.
     *
     * The input Message (METHOD_CALL) is expected to contain the following parameters:
     *   sessionPort   SessionPort  The session port.
     *   joiner        string       The unique name of the session joiner.
     *   creator       string       The name of the session creator.
     *   optsIn        SesionOpts   The session options requested by the joiner.
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   resultCode    uint32       A ALLJOYN_JOINSESSION_* reply code (see AllJoynStd.h).
     *   sessionId     uint32       The session id (valid if resultCode indicates success).
     *   opts          SessionOpts  The actual (final) session options.
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void AttachSession(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Respond to a remote daemon request to get session info from this daemon.
     *
     * The input Message (METHOD_CALL) is expected to contain the following parameters:
     *   creator       string       Name of attachment that bound the session port.
     *   sessionPort   SessionPort  The sessionPort whose info is being requested.
     *   opts          SesionOpts   The session options requested by the joiner.
     *
     * The output Message (METHOD_REPLY) contains the following parameters:
     *   busAddr       string       The bus address to use when attempting to create
     *                              a connection for the purpose of joining the given session.
     *
     * @param member  Member.
     * @param msg     The incoming message.
     */
    void GetSessionInfo(const InterfaceDescription::Member* member, Message& msg);

    /**
     * Process incoming ExchangeNames signals from remote daemons.
     *
     * @param member        Interface member for signal
     * @param sourcePath    object path sending the signal.
     * @param msg           The signal message.
     */
    void ExchangeNamesSignalHandler(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg);

    /**
     * Process incoming NameChanged signals from remote daemons.
     *
     * @param member        Interface member for signal
     * @param sourcePath    object path sending the signal.
     * @param msg           The signal message.
     */
    void NameChangedSignalHandler(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg);

    /**
     * Process incoming SessionDetach signals from remote daemons.
     *
     * @param member        Interface member for signal
     * @param sourcePath    object path sending the signal.
     * @param msg           The signal message.
     */
    void DetachSessionSignalHandler(const InterfaceDescription::Member* member, const char* sourcePath, Message& msg);

    /**
     * NameListener implementation called when a bus name changes ownership.
     *
     * @param alias     Well-known bus name now owned by listener.
     * @param oldOwner  Unique name of old owner of alias or NULL if none existed.
     * @param newOwner  Unique name of new owner of alias or NULL if none (now) exists.
     */
    void NameOwnerChanged(const qcc::String& alias,
                          const qcc::String* oldOwner,
                          const qcc::String* newOwner);

    /**
     * Receive notification of a new bus instance via TransportListener.
     * Internal use only.
     *
     * @param   busAddr   Address of discovered bus.
     * @param   guid      GUID of daemon that sent the advertisement.
     * @param   transport Transport that received the advertisement.
     * @param   names     Vector of bus names advertised by the discovered bus.
     * @param   ttl       Number of seconds before this advertisement expires
     *                    (0 means expire immediately, numeric_limits<uint8_t>::max() means never expire)
     */
    void FoundNames(const qcc::String& busAddr, const qcc::String& guid, TransportMask transport, const std::vector<qcc::String>* names, uint8_t ttl);

    /**
     * Called when a transport gets a surprise disconnect from a remote bus.
     *
     * @param busAddr       The address of the bus formatted as a string.
     */
    void BusConnectionLost(const qcc::String& busAddr);

    /**
     * Get reference to the daemon router object
     */
    DaemonRouter& GetDaemonRouter() { return router; }

  private:
    Bus& bus;                             /**< The bus */
    DaemonRouter& router;                 /**< The router */
    qcc::Mutex stateLock;                 /**< Lock that protects AlljoynObj state */

    const InterfaceDescription* daemonIface;               /**< org.alljoyn.Daemon interface */

    const InterfaceDescription::Member* foundNameSignal;   /**< org.alljoyn.Bus.FoundName signal */
    const InterfaceDescription::Member* lostAdvNameSignal; /**< org.alljoyn.Bus.LostAdvertisdName signal */
    const InterfaceDescription::Member* sessionLostSignal; /**< org.alljoyn.Bus.SessionLost signal */
    const InterfaceDescription::Member* mpSessionChangedSignal;  /**< org.alljoyn.Bus.MPSessionChanged signal */

    /** Map of open connectSpecs to local endpoint name(s) that require the connection. */
    std::multimap<qcc::String, qcc::String> connectMap;

    /** Map of active advertised names to requesting local endpoint name(s) */
    std::multimap<qcc::String, std::pair<TransportMask, qcc::String> > advertiseMap;

    /** Map of active discovery names to requesting local endpoint name(s) */
    std::multimap<qcc::String, qcc::String> discoverMap;

    /** Map of discovery names to forbidden transports (due to lack of permissions) of local endpoint */
    std::multimap<qcc::String, std::pair<TransportMask, qcc::String> > transForbidMap;

    /** Map of discovered bus names (protected by discoverMapLock) */
    struct NameMapEntry {
        qcc::String busAddr;
        qcc::String guid;
        TransportMask transport;
        uint32_t timestamp;
        uint32_t ttl;

        NameMapEntry(const qcc::String& busAddr, const qcc::String& guid, TransportMask transport, uint32_t ttl) :
            busAddr(busAddr),
            guid(guid),
            transport(transport),
            timestamp(qcc::GetTimestamp()),
            ttl(ttl) { }
    };
    std::multimap<qcc::String, NameMapEntry> nameMap;

    /* Session map */
    struct SessionMapEntry {
        qcc::String endpointName;
        SessionId id;
        qcc::String sessionHost;
        SessionPort sessionPort;
        SessionOpts opts;
        qcc::SocketFd fd;
        RemoteEndpoint* streamingEp;
        std::vector<qcc::String> memberNames;
        bool isInitializing;
        SessionMapEntry() :
            id(0),
            sessionPort(0),
            opts(),
            fd(-1),
            streamingEp(NULL),
            isInitializing(false) { }
    };

    typedef std::multimap<std::pair<qcc::String, SessionId>, SessionMapEntry> SessionMapType;

    SessionMapType sessionMap;  /**< Map (endpointName,sessionId) to session info */

    /*
     * Helper function to get session map interator
     */
    SessionMapEntry* SessionMapFind(const qcc::String& name, SessionId session);

    /*
     * Helper function to get session map range interator
     */
    SessionMapType::iterator SessionMapLowerBound(const qcc::String& name, SessionId session);

    /**
     * Helper function to insert a sesssion map
     */
    void SessionMapInsert(SessionMapEntry& sme);

    /**
     * Helper function to erase a sesssion map
     */
    void SessionMapErase(SessionMapEntry& sme);

    const qcc::GUID128& guid;                               /**< Global GUID of this daemon */

    const InterfaceDescription::Member* exchangeNamesSignal;   /**< org.alljoyn.Daemon.ExchangeNames signal member */
    const InterfaceDescription::Member* detachSessionSignal;   /**< org.alljoyn.Daemon.DetachSession signal member */

    std::map<qcc::String, VirtualEndpoint*> virtualEndpoints;  /**< Map of endpoints that reside behind a connected AllJoyn daemon */

    std::map<qcc::StringMapKey, RemoteEndpoint*> b2bEndpoints;    /**< Map of bus-to-bus endpoints that are connected to external daemons */

    /** NameMapReaperThread removes expired names from the nameMap */
    class NameMapReaperThread : public qcc::Thread {
      public:
        NameMapReaperThread(AllJoynObj* ajnObj) : qcc::Thread("NameMapReaper"), ajnObj(ajnObj) { }

        qcc::ThreadReturn STDCALL Run(void* arg);

      private:
        AllJoynObj* ajnObj;
    };

    NameMapReaperThread nameMapReaper;                   /**< Removes expired names from nameMap */

    /** JoinSessionThread handles a JoinSession request from a local client on a separate thread */
    class JoinSessionThread : public qcc::Thread, public qcc::ThreadListener {
      public:
        JoinSessionThread(AllJoynObj& ajObj, const Message& msg, bool isJoin) :
            qcc::Thread(qcc::String("JoinS-") + qcc::U32ToString(qcc::IncrementAndFetch(&jstCount))),
            ajObj(ajObj),
            msg(msg),
            isJoin(isJoin) { }

        void ThreadExit(Thread* thread);

      protected:
        qcc::ThreadReturn STDCALL Run(void* arg);

      private:
        static int jstCount;
        qcc::ThreadReturn STDCALL RunJoin();
        qcc::ThreadReturn STDCALL RunAttach();

        AllJoynObj& ajObj;
        Message msg;
        bool isJoin;
    };

    std::vector<JoinSessionThread*> joinSessionThreads;  /**< List of outstanding join session requests */
    qcc::Mutex joinSessionThreadsLock;                   /**< Lock that protects joinSessionThreads */
    bool isStopping;                                     /**< True while waiting for threads to exit */
    BusController* busController;                        /**< BusController that created this BusObject */

    /**
     * Acquire AllJoynObj locks.
     */
    void AcquireLocks();

    /**
     * Release AllJoynObj locks.
     */
    void ReleaseLocks();

    /**
     * Called to check whether have enough permissions to use designated transports.
     * @param   sender        The sender's well-known name string
     * @param   transports    The transport mask
     * @param   callerName    The caller that invokes this check
     */
    QStatus CheckTransportsPermission(const qcc::String& sender, TransportMask& transports, const char* callerName);

    /**
     * Utility function used to send a single FoundName signal.
     *
     * @param dest        Unique name of destination.
     * @param name        Well-known name that was found.
     * @param transport   The transport that received the advertisment.
     * @param namePrefix  Well-known name prefix used in call to FindName() that triggered this notification.
     * @return ER_OK if succssful.
     */
    QStatus SendFoundAdvertisedName(const qcc::String& dest,
                                    const qcc::String& name,
                                    TransportMask transport,
                                    const qcc::String& namePrefix);

    /**
     * Utility function used to send LostAdvertisedName signals to each "interested" local endpoint.
     *
     * @param name        Well-known name whose advertisment was lost.
     * @param transport   Transport whose advertisment for name has gone away.
     * @return ER_OK if succssful.
     */
    QStatus SendLostAdvertisedName(const qcc::String& name, TransportMask transport);

    /**
     * Utility method used to invoke SessionAttach remote method.
     *
     * @param sessionPort      SessionPort used in join request.
     * @param src              Unique name of session joiner.
     * @param sessionHost      Unique name of sessionHost.
     * @param dest             Unique name of session creator.
     * @param remoteB2BName    Unique name of directly connected (next hop) B2B endpoint.
     * @param remoteControllerName  Unique name of bus controller at next hop.
     * @param outgoingSessionId     SessionId to use for outgoing AttachSession message. Should
     *                              be 0 for newly created (non-multipoint) sessions.
     * @param busAddr          Destination bus address from advertisement or GetSessionInfo.
     * @param optsIn           Session options requested by joiner.
     * @param replyCode        [OUT] SessionAttach response code
     * @param sessionId        [OUT] session id if reply code indicates success.
     * @param optsOut          [OUT] Actual (final) session options.
     * @param members          [OUT] Array or session members (strings) formatted as MsgArg.
     */
    QStatus SendAttachSession(SessionPort sessionPort,
                              const char* src,
                              const char* sessionHost,
                              const char* dest,
                              const char* remoteB2BName,
                              const char* remoteControllerName,
                              SessionId outgoingSessionId,
                              const char* busAddr,
                              const SessionOpts& optsIn,
                              uint32_t& replyCode,
                              SessionId& sessionId,
                              SessionOpts& optsOut,
                              MsgArg& members);

    /**
     * Utility method used to invoke AcceptSession on device local endpoint.
     *
     * @param sessionPort      SessionPort that received the join request.
     * @param sessionId        Id for new session (if accepted).
     * @param creatorName      Session creator unique name.
     * @param joinerName       Session joiner unique name.
     * @param opts             Session options requsted by joiner
     * @param isAccepted       [OUT] true iff creator accepts session. (valid if return is ER_OK).
     */
    QStatus SendAcceptSession(SessionPort sessionPort,
                              SessionId sessionId,
                              const char* creatorName,
                              const char* joinerName,
                              const SessionOpts& opts,
                              bool& isAccepted);

    /**
     * Utility method used to send SessionLost signal to locally attached endpoint.
     *
     * @param       entry    SessionMapEntry that was lost.
     */
    void SendSessionLost(const SessionMapEntry& entry);

    /**
     * Utility method used to send MPSessionChanged signal to locally attached endpoint.
     *
     * @param   sessionId   The sessionId.
     * @param   name        Unique name of session member that changed.
     * @param   isAdd       true iff member added.
     * @param   dest        Local destination for MPSessionChanged.
     */
    void SendMPSessionChanged(SessionId sessionId, const char* name, bool isAdd, const char* dest);

    /**
     * Utility method used to invoke GetSessionInfo remote method.
     *
     * @param       creatorName    Bus name of session creator.
     * @param       sessionPort    Session port value.
     * @param       opts           Requested session options.
     * @param[out]  busAddrs       Returned busAddrs for session (if return value is ER_OK)
     * @return  ER_OK if successful.
     */
    QStatus SendGetSessionInfo(const char* creatorName,
                               SessionPort sessionPort,
                               const SessionOpts& opts,
                               std::vector<qcc::String>& busAddrs);

    /**
     * Add a virtual endpoint with a given unique name.
     *
     * @param uniqueName          The uniqueName of the virtual endpoint.
     * @param busToBusEndpoint    The bus-to-bus endpoint that "owns" the virtual endpoint.
     * @param changesMade         [OUT] Written to true of virtual endpoint was created (as opposed to already existing).
     * @return The virtual endpoint.
     */
    VirtualEndpoint& AddVirtualEndpoint(const qcc::String& uniqueName, RemoteEndpoint& busToBusEndpoint, bool* changesMade = NULL);

    /**
     * Remove a virtual endpoint.
     *
     * @param endpoint   The virtualEndpoint to be removed.
     */
    void RemoveVirtualEndpoint(VirtualEndpoint& endpoint);

    /**
     * Find a virtual endpoint by its name.
     *
     * @param uniqueName    The name of the endpoint to find.
     * @return The requested virtual endpoint or NULL if not found.
     */
    VirtualEndpoint* FindVirtualEndpoint(const qcc::String& uniqueName);

    /**
     * Internal bus-to-bus remote endpoint listener.
     * Called when any virtual endpoint's remote endpoint exits.
     *
     * @param ep   RemoteEndpoint that is exiting.
     */
    void EndpointExit(RemoteEndpoint* ep);

    /**
     * Send signal that informs remote bus of names available on local daemon.
     * This signal is used only in bus-to-bus connections.
     *
     * @param endpoint    Remote endpoint to exchange names with.
     * @return  ER_OK if successful.
     */
    QStatus ExchangeNames(RemoteEndpoint& endpoint);

    /**
     * Process a request to cancel advertising a name from a given (locally-connected) endpoint.
     *
     * @param uniqueName         Name of endpoint requesting end of advertising
     * @param advertiseName      Well-known name whose advertising is to be canceled.
     * @param transports         Set of transports that should cancel the advertisment.
     * @return ER_OK if successful.
     */
    QStatus ProcCancelAdvertise(const qcc::String& uniqueName, const qcc::String& advertiseName, TransportMask transportMask);

    /**
     * Process a request to cancel discovery of a name prefix from a given (locally-connected) endpoint.
     *
     * @param endpointName         Name of endpoint requesting end of advertising
     * @param namePrefix           Well-known name prefix to be removed from discovery list
     * @return ER_OK if successful.
     */
    QStatus ProcCancelFindName(const qcc::String& endpointName, const qcc::String& namePrefix);

    /**
     * Validate and normalize a transport specification string.  Given a
     * transport specification, convert it into a form which is guaranteed to
     * have a one-to-one relationship with a transport.  (This is just a
     * convenient inline wrapper for internal use).
     *
     * @param inSpec    Input transport connect spec.
     * @param outSpec   [OUT] Normalized transport connect spec.
     * @param argMap    [OUT] Normalized parameter map.
     * @return ER_OK if successful.
     */
    QStatus NormalizeTransportSpec(const char* inSpec,
                                   qcc::String& outSpec,
                                   std::map<qcc::String, qcc::String>& argMap)
    {
        return bus.GetInternal().GetTransportList().NormalizeTransportSpec(inSpec, outSpec, argMap);
    }

    /**
     * Helper method used to shutdown a remote endpoint while preserving it's open file descriptor.
     * This is used for converting a RemoteEndpoint into a raw streaming socket.
     *
     * @param b2bEp    Bus to bus endpoint being shutdown.
     * @param sockFd   [OUT] b2bEp's socket descriptor.
     * @return   ER_OK if successful.
     */
    QStatus ShutdownEndpoint(RemoteEndpoint& b2bEp, qcc::SocketFd& sockFd);

    /**
     * Get a list of the currently advertised names
     *
     * @param names  A vector containing the advertised names.
     */
    void GetAdvertisedNames(std::vector<qcc::String>& names);

    /**
     * Utility function used to clean up the session map when a session participant.
     * leaves a session.
     *
     * @param endpoint  Endpoint (virtual or remote) that has left the session.
     * @param id        Session id.
     */
    void RemoveSessionRefs(BusEndpoint& endpoint, SessionId id);

    /**
     * Utility function used to clean up the session map when a virtual endpoint with a
     * given b2b endpoint leaves a session.
     *
     * This utility is used when the given B2B ep has closed for some reason and we
     * need to clean any virtual endpoints that might have been using that b2b ep
     * from the sessionMap
     *
     * @param vep       Virtual that should be cleaned from sessionMap if it routes
     *                  through b2bEp for a given session.
     * @param b2bEp     B2B endpoint that vep must route through in order to be cleaned.
     */
    void RemoveSessionRefs(const VirtualEndpoint& vep, const RemoteEndpoint& b2bEp);
};

}

#endif
