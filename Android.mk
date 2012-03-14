LOCAL_PATH := $(call my-dir)

# Rules to build liballjoyn.a

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cc

LOCAL_CFLAGS += \
	-DQCC_CPU_ARM \
	-DQCC_OS_ANDROID \
	-DQCC_OS_GROUP_POSIX

LOCAL_C_INCLUDES := \
	external/connectivity/stlport/stlport \
	external/alljoyn/common/inc \
	external/alljoyn/alljoyn_core/inc \
	external/alljoyn/alljoyn_core/src \
	external/alljoyn/alljoyn_core/autogen \
	external/openssl/include \

LOCAL_SRC_FILES := \
	../common/src/XmlElement.cc \
	../common/src/IPAddress.cc \
	../common/src/Logger.cc \
	../common/src/CryptoSRP.cc \
	../common/src/Util.cc \
	../common/src/String.cc \
	../common/src/ASN1.cc \
	../common/src/StringUtil.cc \
	../common/src/BufferedSource.cc \
	../common/src/Stream.cc \
	../common/src/SDPRecord.cc \
	../common/src/ScatterGatherList.cc \
	../common/src/Crypto.cc \
	../common/src/Timer.cc \
	../common/src/GUID.cc \
	../common/src/StreamPump.cc \
	../common/src/Debug.cc \
	../common/src/KeyBlob.cc \
	../common/src/StringSource.cc \
	../common/src/Pipe.cc \
	../common/src/Config.cc \
	../common/src/BigNum.cc \
	../common/src/SocketStream.cc \
	../common/src/BufferedSink.cc \
	../common/src/LockTrace.cc \
	../common/os/posix/Mutex.cc \
	../common/os/posix/Socket.cc \
	../common/os/posix/Event.cc \
	../common/os/posix/atomic.cc \
	../common/os/posix/time.cc \
	../common/os/posix/Environ.cc \
	../common/os/posix/FileStream.cc \
	../common/os/posix/AdapterUtil.cc \
	../common/os/posix/osUtil.cc \
	../common/os/posix/Thread.cc \
	../common/os/posix/IfConfigLinux.cc \
	../common/crypto/openssl/CryptoRand.cc \
	../common/crypto/openssl/CryptoRSA.cc \
	../common/crypto/openssl/CryptoHash.cc \
	../common/crypto/openssl/CryptoAES.cc

LOCAL_SRC_FILES += \
	src/BusUtil.cc \
	src/MethodTable.cc \
	src/DBusStd.cc \
	src/RemoteEndpoint.cc \
	src/CompressionRules.cc \
	src/KeyStore.cc \
	src/Transport.cc \
	src/BusEndpoint.cc \
	src/PermissionDB.cc \
	src/UnixTransport.cc \
	src/TCPTransport.cc \
	src/AuthMechSRP.cc \
	src/SimpleBusListener.cc \
	src/Message_Gen.cc \
	src/Message.cc \
	src/BusAttachment.cc \
	src/TransportList.cc \
	src/PeerState.cc \
	src/AuthMechLogon.cc \
	src/AllJoynPeerObj.cc \
	src/SessionOpts.cc \
	src/AuthMechRSA.cc \
	src/SignalTable.cc \
	src/InterfaceDescription.cc \
	src/DBusCookieSHA1.cc \
	src/EndpointAuth.cc \
	src/ClientRouter.cc \
	src/MsgArg.cc \
	src/BusObject.cc \
	src/ProxyBusObject.cc \
	src/XmlHelper.cc \
	src/SASLEngine.cc \
	src/Message_Parse.cc \
	src/AllJoynCrypto.cc \
	src/AllJoynStd.cc \
	src/LaunchdTransport.cc \
	src/SignatureUtils.cc \
	src/LocalTransport.cc

LOCAL_SRC_FILES += \
	autogen/Status.c \
	autogen/version.cc

LOCAL_SHARED_LIBRARIES := \
	libstlport \
	libcrypto \
	libssl \
	liblog

LOCAL_PRELINK_MODULE := false

LOCAL_REQUIRED_MODULES := \
	external/openssl/crypto/libcrypto \
	external/openssl/ssl/libssl \
	external/connectivity/stlport

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := liballjoyn

include $(BUILD_SHARED_LIBRARY)

# Rules to build alljoyn-daemon

include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cc

LOCAL_CFLAGS += \
	-DQCC_CPU_ARM \
	-DQCC_OS_ANDROID \
	-DQCC_OS_GROUP_POSIX

LOCAL_C_INCLUDES := \
	external/connectivity/stlport/stlport \
	external/alljoyn/common/inc \
	external/alljoyn/alljoyn_core/inc \
	external/alljoyn/alljoyn_core/src \
	external/alljoyn/alljoyn_core/daemon \
	external/alljoyn/alljoyn_core/autogen \
	external/openssl/include \

LOCAL_SRC_FILES := \
	daemon/DaemonRouter.cc \
	daemon/RuleTable.cc \
	daemon/DaemonUnixTransport.cc \
	daemon/NameService.cc \
	daemon/bt_bluez/AdapterObject.cc \
	daemon/bt_bluez/BlueZUtils.cc \
	daemon/bt_bluez/BlueZIfc.cc \
	daemon/bt_bluez/BTAccessor.cc \
	daemon/bt_bluez/BlueZHCIUtils.cc \
	daemon/BTNodeDB.cc \
	daemon/Bus.cc \
	daemon/BusController.cc \
	daemon/AllJoynObj.cc \
	daemon/BTController.cc \
	daemon/ServiceDB.cc \
	daemon/DBusObj.cc \
	daemon/VirtualEndpoint.cc \
	daemon/DaemonLaunchdTransport.cc \
	daemon/BTTransport.cc \
	daemon/NsProtocol.cc \
	daemon/NameTable.cc \
	daemon/PolicyDB.cc \
	daemon/PropertyDB.cc \
	daemon/DaemonTCPTransport.cc \
	daemon/posix/daemon-main.cc \
	daemon/ConfigDB.cc \

LOCAL_SRC_FILES += \
	autogen/Status.c \
	autogen/version.cc

LOCAL_SHARED_LIBRARIES := \
	liballjoyn \
	libcrypto \
	libssl \
	liblog

LOCAL_REQUIRED_MODULES := \
	external/openssl/crypto/libcrypto \
	external/openssl/ssl/libssl \
	external/connectivity/stlport

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := alljoyn-daemon

include $(BUILD_EXECUTABLE)
