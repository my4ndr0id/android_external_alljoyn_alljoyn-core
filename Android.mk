LOCAL_PATH := $(call my-dir)
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
	../common/os/posix/Socket.cc \
	../common/os/posix/osUtil.cc \
	../common/os/posix/AdapterUtil.cc \
	../common/os/posix/time.cc \
	../common/os/posix/FileStream.cc \
	../common/os/posix/Event.cc \
	../common/os/posix/Environ.cc \
	../common/os/posix/Mutex.cc \
	../common/os/posix/SslSocket.cc \
	../common/os/posix/Thread.cc \
	../common/os/posix/atomic.cc \
	../common/src/Pipe.cc \
	../common/src/GUID.cc \
	../common/src/BufferedSink.cc \
	../common/src/Stream.cc \
	../common/src/KeyBlob.cc \
	../common/src/CryptoSRP.cc \
	../common/src/StringUtil.cc \
	../common/src/CryptoRSA.cc \
	../common/src/CryptoAES.cc \
	../common/src/StringSource.cc \
	../common/src/Config.cc \
	../common/src/Util.cc \
	../common/src/XmlElement.cc \
	../common/src/Timer.cc \
	../common/src/BufferedSource.cc \
	../common/src/IPAddress.cc \
	../common/src/SocketStream.cc \
	../common/src/Debug.cc \
	../common/src/Crypto.cc \
	../common/src/SDPRecord.cc \
	../common/src/ScatterGatherList.cc \
	../common/src/String.cc \
	../common/src/Logger.cc

LOCAL_SRC_FILES += \
	src/XmlHelper.cc \
	src/DBusCookieSHA1.cc \
	src/TCPTransport.cc \
	src/BusUtil.cc \
	src/ProxyBusObject.cc \
	src/BusObject.cc \
	src/SignatureUtils.cc \
	src/AuthMechRSA.cc \
	src/AllJoynCrypto.cc \
	src/DBusStd.cc \
	src/LocalTransport.cc \
	src/BusAttachment.cc \
	src/Message_Gen.cc \
	src/AuthMechSRP.cc \
	src/MethodTable.cc \
	src/EndpointAuth.cc \
	src/KeyStore.cc \
	src/InterfaceDescription.cc \
	src/UnixTransport.cc \
	src/SASLEngine.cc \
	src/ClientRouter.cc \
	src/CompressionRules.cc \
	src/SignalTable.cc \
	src/PeerState.cc \
	src/AuthMechLogon.cc \
	src/RemoteEndpoint.cc \
	src/SessionOpts.cc \
	src/Message.cc \
	src/Transport.cc \
	src/TransportList.cc \
	src/AllJoynPeerObj.cc \
	src/AllJoynStd.cc \
	src/Message_Parse.cc \
	src/MsgArg.cc \
	daemon/ConfigDB.cc \
	daemon/AllJoynObj.cc \
	daemon/DaemonUnixTransport.cc \
	daemon/NameTable.cc \
	daemon/BusController.cc \
	daemon/DBusObj.cc \
	daemon/ServiceDB.cc \
	daemon/NsProtocol.cc \
	daemon/BTTransport.cc \
	daemon/BTController.cc \
	daemon/BTNodeDB.cc \
	daemon/bt_bluez/BlueZHCIUtils.cc \
	daemon/bt_bluez/BlueZIfc.cc \
	daemon/bt_bluez/BlueZUtils.cc \
	daemon/bt_bluez/BTAccessor.cc \
	daemon/DaemonRouter.cc \
	daemon/VirtualEndpoint.cc \
	daemon/DaemonTCPTransport.cc \
	daemon/posix/daemon-main.cc \
	daemon/PropertyDB.cc \
	daemon/PolicyDB.cc \
	daemon/Bus.cc \
	daemon/NameService.cc \
	daemon/RuleTable.cc

# TODO Generated files are a problem: Status.{hc}, version.cc
LOCAL_SRC_FILES += \
	autogen/Status.c \
	autogen/version.cc

LOCAL_SHARED_LIBRARIES := \
	libstlport \
	libcrypto \
	libssl \
	liblog

LOCAL_REQUIRED_MODULES := external/openssl/crypto/libcrypto

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := alljoyn-daemon

include $(BUILD_EXECUTABLE)
