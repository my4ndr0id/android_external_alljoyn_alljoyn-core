/**
 * @file
 * This program tests the basic features of Alljoyn.It uses google test as the test
 * automation framework.
 */
/******************************************************************************
 * Copyright 2009-2011, Qualcomm Innovation Center, Inc.
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

#include "ServiceSetup.h"
#include "ClientSetup.h"
#include "ServiceTestObject.h"
#include "ajTestCommon.h"

#include <qcc/time.h>
/* Header files included for Google Test Framework */
#include <gtest/gtest.h>

/* client waits for this event during findAdvertisedName */
static Event g_discoverEvent;

/* Service available flag :
 * true -- service available, client can start
 * false-- service not available
 */
static bool IsServiceReady = false;

/* Common service setup function, each client test has to run at the beginning */
static QStatus ServiceSetup()
{
    QStatus status = ER_OK;
    InterfaceDescription* regTestIntf = NULL;
    BusAttachment* serviceBus = NULL;

    /* Do service setup only once */
    if (!IsServiceReady) {
        serviceBus = new BusAttachment("bbtestservices", true);

        if (!serviceBus->IsStarted()) {
            status = serviceBus->Start();
        }

        if (ER_OK == status && !serviceBus->IsConnected()) {
            /* Connect to the daemon and wait for the bus to exit */
            status = serviceBus->Connect(getConnectArg().c_str());
        }
        ServiceObject* myService = new ServiceObject(*serviceBus, "/org/alljoyn/test_services");

        /* Invoking the Bus Listener */
        MyBusListener* myBusListener = new MyBusListener();
        serviceBus->RegisterBusListener(*myBusListener);


        status = serviceBus->CreateInterface(myService->getAlljoynInterfaceName(), regTestIntf);
        assert(regTestIntf);

        /* Adding a signal to no */
        status = regTestIntf->AddSignal("my_signal", "s", NULL, 0);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = regTestIntf->AddSignal("my_signal_string", "us", NULL, 0);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = regTestIntf->AddMember(MESSAGE_METHOD_CALL, "my_ping", "s",  "s", "o,i", 0);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = regTestIntf->AddMember(MESSAGE_METHOD_CALL, "my_sing", "s",  "s", "o,i", 0);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = regTestIntf->AddMember(MESSAGE_METHOD_CALL, "my_param_test", "ssssssssss", "ssssssssss", "iiiiiiiiii,oooooooooo", 0);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        regTestIntf->Activate();
        status = myService->AddInterfaceToObject(regTestIntf);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        InterfaceDescription* valuesIntf = NULL;
        status = serviceBus->CreateInterface(myService->getAlljoynValuesInterfaceName(), valuesIntf);
        assert(valuesIntf);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = valuesIntf->AddProperty("int_val", "i", PROP_ACCESS_RW);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = valuesIntf->AddProperty("str_val", "s", PROP_ACCESS_RW);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = valuesIntf->AddProperty("ro_str", "s", PROP_ACCESS_READ);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = valuesIntf->AddProperty("prop_signal", "s", PROP_ACCESS_RW);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        valuesIntf->Activate();
        status = myService->AddInterfaceToObject(valuesIntf);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        /* Populate the signal handler members */
        myService->PopulateSignalMembers();
        status = myService->InstallMethodHandlers();
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        status =  serviceBus->RegisterBusObject(*myService);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        /* Request a well-known name */
        status = serviceBus->RequestName(myService->getAlljoynWellKnownName(),
                                         DBUS_NAME_FLAG_REPLACE_EXISTING | DBUS_NAME_FLAG_DO_NOT_QUEUE);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        /* Creating a session */
        SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY,
                         TRANSPORT_ANY);
        SessionPort sessionPort = 550;
        status = serviceBus->BindSessionPort(sessionPort, opts, *myBusListener);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        /* Advertising this well known name */
        status = serviceBus->AdvertiseName(myService->getAlljoynWellKnownName(), TRANSPORT_ANY);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        /* Adding the second object */
        status = ER_OK;
        ServiceTestObject* serviceTestObject = new ServiceTestObject(*serviceBus,
                                                                     myService->getServiceObjectPath());

        InterfaceDescription* regTestIntf2 = NULL;
        status = serviceBus->CreateInterface(myService->getServiceInterfaceName(), regTestIntf2);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        assert(regTestIntf2);

        /* Adding a signal to no */
        status = regTestIntf2->AddSignal("my_signal", "s", NULL, 0);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = regTestIntf2->AddMember(MESSAGE_METHOD_CALL, "my_ping", "s",  "s", "o,i", 0);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = regTestIntf2->AddMember(MESSAGE_METHOD_CALL, "ByteArrayTest", "ay", "ay", "i,o", 0);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = regTestIntf2->AddMember(MESSAGE_METHOD_CALL, "my_sing", "s",  "s", "o,i", 0);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = regTestIntf2->AddMember(MESSAGE_METHOD_CALL, "my_king", "s", "s", "i,o", 0);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = regTestIntf2->AddMember(MESSAGE_METHOD_CALL, "DoubleArrayTest", "ad", "ad", "i,o", 0);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        regTestIntf2->Activate();
        status = serviceTestObject->AddInterfaceToObject(regTestIntf2);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        InterfaceDescription* valuesIntf2 = NULL;
        status = serviceBus->CreateInterface(myService->getServiceValuesInterfaceName(), valuesIntf2);
        assert(valuesIntf2);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = valuesIntf2->AddProperty("int_val", "i", PROP_ACCESS_RW);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = valuesIntf2->AddProperty("str_val", "s", PROP_ACCESS_RW);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
        status = valuesIntf2->AddProperty("ro_str", "s", PROP_ACCESS_READ);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        valuesIntf2->Activate();
        status = serviceTestObject->AddInterfaceToObject(valuesIntf2);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        /* Populate the signal handler members */
        serviceTestObject->PopulateSignalMembers(myService->getServiceInterfaceName());
        status = serviceTestObject->InstallMethodHandlers(myService->getServiceInterfaceName());
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        status =  serviceBus->RegisterBusObject(*serviceTestObject);
        EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

        //Set to True
        IsServiceReady = true;

    }

    return status;
}

TEST(PerfTest, Introspect_CorrectParameters)
{
    ASSERT_EQ(ER_OK, ServiceSetup());

    ClientSetup testclient(ajn::getConnectArg().c_str());

    ProxyBusObject remoteObj(*(testclient.getClientMsgBus()),
                             testclient.getClientWellknownName(),
                             testclient.getClientObjectPath(),
                             0);

    QStatus status = remoteObj.IntrospectRemoteObject();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
}

TEST(PerfTest, ErrorMsg_Error_invalid_path) {
    ASSERT_EQ(ER_OK, ServiceSetup());

    ClientSetup testclient(ajn::getConnectArg().c_str());

    /* Invalid path  - does not begin with '/' */
    ProxyBusObject remoteObj(*(testclient.getClientMsgBus()),
                             //testclient.getClientWellknownName(),
                             "org.alljoyn.test_services",
                             "org/alljoyn/alljoyn_test1",
                             0);
    QStatus status = remoteObj.IntrospectRemoteObject();
    ASSERT_EQ(ER_BUS_BAD_OBJ_PATH, status) << "  Actual Status: " << QCC_StatusText(status);


}

TEST(PerfTest, ErrorMsg_Error_no_such_object) {
    ASSERT_EQ(ER_OK, ServiceSetup());

    ClientSetup testclient(ajn::getConnectArg().c_str());

    BusAttachment* test_msgBus = testclient.getClientMsgBus();

    /* Valid path but-existant  */
    ProxyBusObject remoteObj(*test_msgBus,
                             testclient.getClientWellknownName(),
                             "/org/alljoyn/alljoyn_test1",
                             0);

    QStatus status = remoteObj.IntrospectRemoteObject();
    EXPECT_EQ(ER_BUS_REPLY_IS_ERROR_MESSAGE, status) << "  Actual Status: " << QCC_StatusText(status);

    /* Instead of directly making an introspect...make a method call and get the error reply */
    const InterfaceDescription* introIntf = test_msgBus->GetInterface(ajn::org::freedesktop::DBus::Introspectable::InterfaceName);
    remoteObj.AddInterface(*introIntf);
    ASSERT_TRUE(introIntf);

    /* Attempt to retrieve introspection from the remote object using sync call */
    Message reply(*test_msgBus);
    const InterfaceDescription::Member* introMember = introIntf->GetMember("Introspect");
    ASSERT_TRUE(introMember);
    status = remoteObj.MethodCall(*introMember, NULL, 0, reply, 5000);
    ASSERT_EQ(ER_BUS_REPLY_IS_ERROR_MESSAGE, status) << "  Actual Status: " << QCC_StatusText(status);

    String errMsg;
    reply->GetErrorName(&errMsg);

    EXPECT_STREQ(QCC_StatusText(ER_BUS_NO_SUCH_OBJECT), errMsg.c_str());
}

TEST(PerfTest, ErrorMsg_does_not_exist_interface) {
    ASSERT_EQ(ER_OK, ServiceSetup());

    ClientSetup testclient(ajn::getConnectArg().c_str());

    BusAttachment* test_msgBus = testclient.getClientMsgBus();

    /* Valid well known name - But does not exist  */
    ProxyBusObject remoteObj(*test_msgBus,
                             "org.alljoyn.alljoyn_test.Interface1",
                             testclient.getClientObjectPath(),
                             0);
    QStatus status = remoteObj.IntrospectRemoteObject();
    EXPECT_EQ(ER_BUS_REPLY_IS_ERROR_MESSAGE, status) << "  Actual Status: " << QCC_StatusText(status);

    /* Instead of directly making an introspect...make a method call and get the error reply */
    const InterfaceDescription* introIntf = test_msgBus->GetInterface(ajn::org::freedesktop::DBus::Introspectable::InterfaceName);
    remoteObj.AddInterface(*introIntf);
    ASSERT_TRUE(introIntf);

    /* Attempt to retrieve introspection from the remote object using sync call */
    Message reply(*test_msgBus);
    const InterfaceDescription::Member* introMember = introIntf->GetMember("Introspect");
    ASSERT_TRUE(introMember);
    status = remoteObj.MethodCall(*introMember, NULL, 0, reply, 5000);
    EXPECT_EQ(ER_BUS_REPLY_IS_ERROR_MESSAGE, status) << "  Actual Status: " << QCC_StatusText(status);
    String errMsg;
    reply->GetErrorName(&errMsg);
    EXPECT_STREQ("Unknown bus name: org.alljoyn.alljoyn_test.Interface1", errMsg.c_str());
}

TEST(PerfTest, ErrorMsg_MethodCallOnNonExistantMethod) {
    ASSERT_EQ(ER_OK, ServiceSetup());

    ClientSetup testclient(ajn::getConnectArg().c_str());

    BusAttachment* test_msgBus = testclient.getClientMsgBus();

    ProxyBusObject remoteObj(*test_msgBus, testclient.getClientWellknownName(), testclient.getClientObjectPath(), 0);
    QStatus status = remoteObj.IntrospectRemoteObject();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    Message reply(*test_msgBus);
    MsgArg pingStr("s", "Test Ping");
    status = remoteObj.MethodCall(testclient.getClientInterfaceName(), "my_unknown", &pingStr, 1, reply, 5000);
    EXPECT_EQ(ER_BUS_INTERFACE_NO_SUCH_MEMBER, status) << "  Actual Status: " << QCC_StatusText(status);
}


/* Test LargeParameters for a synchronous method call */

TEST(PerfTest, MethodCallTest_LargeParameters) {
    ASSERT_EQ(ER_OK, ServiceSetup());

    ClientSetup testclient(ajn::getConnectArg().c_str());
    QStatus status = testclient.MethodCall(100, 2);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

}

/* Test for synchronous method call */

TEST(PerfTest, MethodCallTest_SimpleCall) {
    ASSERT_EQ(ER_OK, ServiceSetup());

    ClientSetup testclient(ajn::getConnectArg().c_str());

    QStatus status = testclient.MethodCall(1, 1);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

}

TEST(PerfTest, MethodCallTest_EmptyParameters) {
    ASSERT_EQ(ER_OK, ServiceSetup());

    ClientSetup testclient(ajn::getConnectArg().c_str());

    QStatus status = testclient.MethodCall(1, 3);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

}

TEST(PerfTest, MethodCallTest_InvalidParameters) {
    ASSERT_EQ(ER_OK, ServiceSetup());

    ClientSetup testclient(ajn::getConnectArg().c_str());

    QStatus status = testclient.MethodCall(1, 4);
    EXPECT_EQ(ER_BUS_UNEXPECTED_SIGNATURE, status) << "  Actual Status: " << QCC_StatusText(status);
}

/* Test signals */
TEST(PerfTest, Properties_SimpleSignal) {
    ASSERT_EQ(ER_OK, ServiceSetup());

    ClientSetup testclient(ajn::getConnectArg().c_str());

    BusAttachment* test_msgBus = testclient.getClientMsgBus();

    ProxyBusObject remoteObj(*test_msgBus, testclient.getClientWellknownName(), testclient.getClientObjectPath(), 0);
    QStatus status = remoteObj.IntrospectRemoteObject();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    MsgArg newName("s", "New returned name");
    status = remoteObj.SetProperty(testclient.getClientValuesInterfaceName(), "prop_signal", newName);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

}

TEST(PerfTest, Properties_SettingNoSuchProperty) {
    ASSERT_EQ(ER_OK, ServiceSetup());
    ClientSetup testclient(ajn::getConnectArg().c_str());

    BusAttachment* test_msgBus = testclient.getClientMsgBus();

    ProxyBusObject remoteObj(*test_msgBus, testclient.getClientWellknownName(), testclient.getClientObjectPath(), 0);
    QStatus status = remoteObj.IntrospectRemoteObject();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    Message reply(*test_msgBus);
    MsgArg inArgs[3];
    MsgArg newName("s", "New returned name");
    size_t numArgs = ArraySize(inArgs);
    MsgArg::Set(inArgs, numArgs, "ssv", testclient.getClientValuesInterfaceName(), "prop_signall", &newName);
    const InterfaceDescription* propIface = test_msgBus->GetInterface(ajn::org::freedesktop::DBus::Properties::InterfaceName);

    status = remoteObj.MethodCall(*(propIface->GetMember("Set")), inArgs, numArgs, reply, 5000, 0);
    EXPECT_EQ(ER_BUS_REPLY_IS_ERROR_MESSAGE, status) << "  Actual Status: " << QCC_StatusText(status);
    String errMsg;
    reply->GetErrorName(&errMsg);
    EXPECT_STREQ(QCC_StatusText(ER_BUS_NO_SUCH_PROPERTY), errMsg.c_str());

}

TEST(PerfTest, Properties_SettingReadOnlyProperty) {
    ASSERT_EQ(ER_OK, ServiceSetup());
    ClientSetup testclient(ajn::getConnectArg().c_str());

    BusAttachment* test_msgBus = testclient.getClientMsgBus();

    ProxyBusObject remoteObj(*test_msgBus, testclient.getClientWellknownName(), testclient.getClientObjectPath(), 0);
    QStatus status = remoteObj.IntrospectRemoteObject();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    Message reply(*test_msgBus);
    MsgArg newName("s", "New returned name");
    MsgArg inArgs[3];
    size_t numArgs = ArraySize(inArgs);
    MsgArg::Set(inArgs, numArgs, "ssv", testclient.getClientValuesInterfaceName(), "ro_str", &newName);
    const InterfaceDescription* propIface = test_msgBus->GetInterface(ajn::org::freedesktop::DBus::Properties::InterfaceName);

    status = remoteObj.MethodCall(*(propIface->GetMember("Set")), inArgs, numArgs, reply, 5000, 0);
    EXPECT_EQ(ER_BUS_REPLY_IS_ERROR_MESSAGE, status) << "  Actual Status: " << QCC_StatusText(status);
    String errMsg;
    reply->GetErrorName(&errMsg);
    EXPECT_STREQ(QCC_StatusText(ER_BUS_PROPERTY_ACCESS_DENIED), errMsg.c_str());
}

TEST(PerfTest, Signals_With_Two_Parameters) {
    ASSERT_EQ(ER_OK, ServiceSetup());
    ClientSetup testclient(ajn::getConnectArg().c_str());

    testclient.setSignalFlag(0);
    QStatus status = testclient.SignalHandler(0, 1);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    //Wait upto 2 seconds for the signal to complete.
    for (int i = 0; i < 200; ++i) {
        qcc::Sleep(10);
        if (testclient.getSignalFlag() != 0) {
            break;
        }
    }
    ASSERT_EQ(5, testclient.getSignalFlag());

}


TEST(PerfTest, Signals_With_Huge_String_Param) {
    ASSERT_EQ(ER_OK, ServiceSetup());
    ClientSetup testclient(ajn::getConnectArg().c_str());

    testclient.setSignalFlag(0);
    QCC_StatusText(testclient.SignalHandler(0, 2));
    //Wait upto 2 seconds for the signal to complete.
    for (int i = 0; i < 200; ++i) {
        qcc::Sleep(10);
        if (testclient.getSignalFlag() != 0) {
            break;
        }
    }
    ASSERT_EQ(4096, testclient.getSignalFlag());

}



/* Test Asynchronous method calls */
TEST(PerfTest, AsyncMethodCallTest_SimpleCall) {
    ASSERT_EQ(ER_OK, ServiceSetup());
    ClientSetup testclient(ajn::getConnectArg().c_str());

    testclient.setSignalFlag(0);
    QStatus status = testclient.AsyncMethodCall(1000, 1);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    //Wait upto 2 seconds for the AsyncMethodCalls to complete;
    for (int i = 0; i < 200; ++i) {
        qcc::Sleep(10);
        if (testclient.getSignalFlag() == 1000) {
            break;
        }
    }
    ASSERT_EQ(1000, testclient.getSignalFlag());

}

TEST(PerfTest, BusObject_ALLJOYN_328_BusObject_destruction)
{
    ASSERT_EQ(ER_OK, ServiceSetup());
    ClientSetup testclient(ajn::getConnectArg().c_str());



    qcc::String clientArgs = testclient.getClientArgs();

    /* Create a Bus Attachment Object */
    BusAttachment*serviceBus = new BusAttachment("ALLJOYN-328", true);
    serviceBus->Start();

    /* Dynamically create a BusObject and register it with the Bus */
    BusObject*obj1 = new  BusObject(*serviceBus, "/home/narasubr", true);

    QStatus status = serviceBus->RegisterBusObject(*obj1);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = serviceBus->Connect(clientArgs.c_str());
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    /*Delete the bus object..Now as per fix for ALLJOYN-328 deregisterbusobject will be called */
    BusObject*obj2 = obj1;
    obj1 = NULL;
    delete obj2;
    //TODO nothing is checked after Deleting the busObject what should this test be looking for?
    //status = serviceBus->Stop();
    //ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    // why is this sleep here when nothing is pending
    //qcc::Sleep(2000);
    /* Clean up msg bus */
    if (serviceBus) {
        BusAttachment* deleteMe = serviceBus;
        serviceBus = NULL;
        delete deleteMe;
    }


}

// TODO: enable this test when ALLJOYN-394 is resolved
TEST(PerfTest, DISABLED_BusObject_GetChildTest) {
    QStatus status = ER_OK;
    ClientSetup testclient(ajn::getConnectArg().c_str());


    ASSERT_EQ(ER_OK, ServiceSetup());

    /* The Client side */
    BusAttachment* client_msgBus = testclient.getClientMsgBus();

    /* No session required since client and service on same daemon */
    ProxyBusObject remoteObj = ProxyBusObject(*client_msgBus,
                                              "org.alljoyn.test_services",
                                              "/",
                                              0);

    status = remoteObj.IntrospectRemoteObject();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    ProxyBusObject*wrongPathChildObjPtr = remoteObj.GetChild("/org");
    ProxyBusObject*correctPathChildObjPtr = remoteObj.GetChild("org");

    /* "//org" path is invalid */
    EXPECT_TRUE(wrongPathChildObjPtr == NULL);

    // "/org" path is valid
    EXPECT_TRUE(correctPathChildObjPtr != NULL);


    // RemoveChild can take relative path
    status = remoteObj.RemoveChild("org");
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_TRUE(remoteObj.GetChild("org") == NULL);

}

TEST(PerfTest, Security_ALLJOYN_294_AddLogonEntry_Without_EnablePeerSecurity)
{
    ClientSetup testclient(ajn::getConnectArg().c_str());



    qcc::String clientArgs = testclient.getClientArgs();

    /* Create a Bus Attachment Object */
    BusAttachment*serviceBus = new BusAttachment("ALLJOYN-294", true);
    serviceBus->Start();

    QStatus status = serviceBus->Connect(clientArgs.c_str());
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = serviceBus->AddLogonEntry("ALLJOYN_SRP_LOGON", "sleepy", "123456");
    ASSERT_EQ(status, ER_BUS_KEYSTORE_NOT_LOADED);

    status = serviceBus->AddLogonEntry("ALLJOYN_SRP_LOGON", "happy", "123456");
    ASSERT_EQ(status, ER_BUS_KEYSTORE_NOT_LOADED);

    /* Clean up msg bus */
    if (serviceBus) {
        BusAttachment* deleteMe = serviceBus;
        serviceBus = NULL;
        delete deleteMe;
    }

}

TEST(PerfTest, Marshal_ByteArrayTest) {
    ASSERT_EQ(ER_OK, ServiceSetup());
    ClientSetup testclient(ajn::getConnectArg().c_str());


    BusAttachment* test_msgBus = testclient.getClientMsgBus();

    /* Create a remote object */
    ProxyBusObject remoteObj(*test_msgBus, testclient.getClientWellknownName(), "/org/alljoyn/service_test", 0);
    QStatus status = remoteObj.IntrospectRemoteObject();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    /* Call the remote method */
    Message reply(*test_msgBus);

    const size_t max_array_size = 1024 * 128;
    /* 1. Testing the Max Array Size  */
    uint8_t* big = new uint8_t[max_array_size];
    MsgArg arg;
    status = arg.Set("ay", max_array_size, big);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = remoteObj.MethodCall("org.alljoyn.service_test.Interface", "ByteArrayTest", &arg, 1, reply, 500000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    int res = memcmp(big, (uint8_t*)reply->GetArg(0)->v_string.str, max_array_size);

    ASSERT_EQ(0, res);
    delete [] big;

    /* Testing The Max Packet Length  */
    MsgArg arg1;
    double* bigd = new double[max_array_size];
    status = arg1.Set("ad", max_array_size, bigd);
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = remoteObj.IntrospectRemoteObject();
    EXPECT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    status = remoteObj.MethodCall("org.alljoyn.service_test.Interface", "DoubleArrayTest", &arg1, 1, reply, 500000);
    ASSERT_EQ(ER_BUS_BAD_BODY_LEN, status) << "  Actual Status: " << QCC_StatusText(status);

    delete [] bigd;
}

/** Client Listener to receive advertisements  */
class ClientBusListener : public BusListener {
  public:
    ClientBusListener() : BusListener() { }

    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
        //QCC_SyncPrintf("FoundAdvertisedName(name=%s, transport=0x%x, prefix=%s)\n", name, transport, namePrefix);
        g_discoverEvent.SetEvent();
    }

    void LostAdvertisedName(const char* name, const char* guid, const char* prefix, const char* busAddress)
    {
        //QCC_SyncPrintf("LostAdvertisedName(name=%s, guid=%s, prefix=%s, addr=%s)\n", name, guid, prefix, busAddress);
    }
};

TEST(PerfTest, FindAdvertisedName_MatchAll_Success)
{
    QStatus status = ER_OK;
    ASSERT_EQ(ER_OK, ServiceSetup());

    ClientSetup testclient(ajn::getConnectArg().c_str());

    BusAttachment* client_msgBus = testclient.getClientMsgBus();

    g_discoverEvent.ResetEvent();

    /* Initializing the bus listener */
    ClientBusListener*clientListener = new ClientBusListener();
    client_msgBus->RegisterBusListener(*clientListener);

    /* find every name */
    status = client_msgBus->FindAdvertisedName("");
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = Event::Wait(g_discoverEvent, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

}

TEST(PerfTest, FindAdvertisedName_MatchExactName_Success)
{
    QStatus status = ER_OK;
    Timespec startTime;
    Timespec endTime;

    ASSERT_EQ(ER_OK, ServiceSetup());

    ClientSetup testclient(ajn::getConnectArg().c_str());

    BusAttachment* client_msgBus = testclient.getClientMsgBus();

    g_discoverEvent.ResetEvent();

    /* Initializing the bus listener */
    ClientBusListener*clientListener = new ClientBusListener();
    client_msgBus->RegisterBusListener(*clientListener);

    /* find every name */
    //GetTimeNow(&startTime);
    status = client_msgBus->FindAdvertisedName("org.alljoyn.test_services");
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = Event::Wait(g_discoverEvent, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    //GetTimeNow(&endTime);
    //QCC_SyncPrintf("FindAdvertisedName takes %d ms\n", (endTime - startTime));
}

TEST(PerfTest, FindAdvertisedName_InvalidName_Fail)
{
    QStatus status = ER_OK;
    ASSERT_EQ(ER_OK, ServiceSetup());

    ClientSetup testclient(ajn::getConnectArg().c_str());

    BusAttachment* client_msgBus = testclient.getClientMsgBus();

    g_discoverEvent.ResetEvent();

    /* Initializing the bus listener */
    ClientBusListener*clientListener = new ClientBusListener();
    client_msgBus->RegisterBusListener(*clientListener);

    /* invalid name find */
    status = client_msgBus->FindAdvertisedName("org.alljoyn.test_invalid");
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    QCC_SyncPrintf("Waiting FoundAdvertisedName 3 seconds...\n");
    status = Event::Wait(g_discoverEvent, 3000);
    ASSERT_EQ(ER_TIMEOUT, status);

}

TEST(PerfTest, JoinSession_BusNotConnected_Fail)
{
    QStatus status = ER_OK;
    BusAttachment* client_msgBus = NULL;

    ASSERT_EQ(ER_OK, ServiceSetup());

    client_msgBus = new BusAttachment("clientSetup", true);
    client_msgBus->Start();

    /* Join session failed because not connected yet*/
    SessionId sessionid;
    SessionOpts qos(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    status = client_msgBus->JoinSession("org.alljoyn.invalid_services", 550, NULL, sessionid, qos);
    ASSERT_EQ(ER_BUS_NOT_CONNECTED, status) << "  Actual Status: " << QCC_StatusText(status);

    if (client_msgBus) {
        delete client_msgBus;
    }

}
TEST(PerfTest, JoinSession_InvalidPort_Fail)
{
    QStatus status = ER_OK;
    BusAttachment* client_msgBus = NULL;

    ASSERT_EQ(ER_OK, ServiceSetup());
    // Have to wait for some time till service well-known name is ready
    //qcc::Sleep(5000);

    ClientSetup testclient(ajn::getConnectArg().c_str());

    client_msgBus = testclient.getClientMsgBus();

    /* Join the session */
    SessionId sessionid;
    SessionOpts qos(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);

    /* port 450 is invalid */
    status = client_msgBus->JoinSession("org.alljoyn.test_services", 450, NULL, sessionid, qos);
    ASSERT_EQ(ER_ALLJOYN_JOINSESSION_REPLY_NO_SESSION, status);

}

TEST(PerfTest, JoinSession_RecordTime_Success)
{
    QStatus status = ER_OK;
    BusAttachment* client_msgBus = NULL;
    Timespec startTime;
    Timespec endTime;

    ASSERT_EQ(ER_OK, ServiceSetup());

    ClientSetup testclient(ajn::getConnectArg().c_str());

    client_msgBus = testclient.getClientMsgBus();

    /* Join the session */
    SessionId sessionid;
    SessionOpts qos(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);

    //GetTimeNow(&startTime);
    status = client_msgBus->JoinSession("org.alljoyn.test_services", 550, NULL, sessionid, qos);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_NE(0, sessionid) << "SessionID should not be '0'";

    //GetTimeNow(&endTime);
    //QCC_SyncPrintf("JoinSession takes %d ms\n", (endTime - startTime));

    status = client_msgBus->LeaveSession(sessionid);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

}

TEST(PerfTest, ClientTest_BasicDiscovery) {
    QStatus status = ER_OK;

    ASSERT_EQ(ER_OK, ServiceSetup());

    ClientSetup testclient(ajn::getConnectArg().c_str());

    BusAttachment* client_msgBus = testclient.getClientMsgBus();

    g_discoverEvent.ResetEvent();

    /* Initializing the bus listener */
    ClientBusListener* clientListener = new ClientBusListener();
    client_msgBus->RegisterBusListener(*clientListener);

    //QCC_SyncPrintf("Finding AdvertisedName\n");
    /* Do a Find Name  */
    status = client_msgBus->FindAdvertisedName("org.alljoyn.test_services");
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    status = Event::Wait(g_discoverEvent, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    /* Join the session */
    SessionId sessionid;
    SessionOpts qos(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, TRANSPORT_ANY);
    status = client_msgBus->JoinSession("org.alljoyn.test_services", 550, NULL, sessionid, qos);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    EXPECT_NE(0, sessionid) << "SessionID should not be '0'";
    /* Checking id name is on the bus */
    bool hasOwner = false;
    status = client_msgBus->NameHasOwner("org.alljoyn.test_services", hasOwner);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_EQ(hasOwner, true);

    ProxyBusObject remoteObj = ProxyBusObject(*client_msgBus, "org.alljoyn.test_services", "/org/alljoyn/test_services", sessionid);
    status = remoteObj.IntrospectRemoteObject();
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);

    Message replyc(*client_msgBus);
    MsgArg pingStr("s", "Hello World");
    status = remoteObj.MethodCall("org.alljoyn.test_services.Interface", "my_ping", &pingStr, 1, replyc, 5000);
    ASSERT_EQ(ER_OK, status) << "  Actual Status: " << QCC_StatusText(status);
    ASSERT_STREQ("Hello World", replyc->GetArg(0)->v_string.str);
}
