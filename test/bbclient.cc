/**
 * @file
 * Sample implementation of an AllJoyn client.
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
#include <qcc/platform.h>
#include <qcc/Debug.h>
#include <qcc/Thread.h>

#include <signal.h>
#include <stdio.h>
#include <assert.h>
#include <vector>

#include <qcc/Environ.h>
#include <qcc/Event.h>
#include <qcc/String.h>
#include <qcc/StringUtil.h>
#include <qcc/Util.h>
#include <qcc/time.h>

#include <alljoyn/BusAttachment.h>
#include <alljoyn/DBusStd.h>
#include <alljoyn/AllJoynStd.h>
#include <alljoyn/version.h>

#include <Status.h>

#define QCC_MODULE "ALLJOYN"

#define METHODCALL_TIMEOUT 30000

using namespace std;
using namespace qcc;
using namespace ajn;

/** Sample constants */
namespace org {
namespace alljoyn {
namespace alljoyn_test {
const char* InterfaceName = "org.alljoyn.alljoyn_test";
const char* DefaultWellKnownName = "org.alljoyn.alljoyn_test";
const char* ObjectPath = "/org/alljoyn/alljoyn_test";
const SessionPort SessionPort = 24;   /**< Well-known session port value for bbclient/bbservice */
namespace values {
const char* InterfaceName = "org.alljoyn.alljoyn_test.values";
}
}
}
}

/** Static data */
static BusAttachment* g_msgBus = NULL;
static Event g_discoverEvent;
static String g_wellKnownName = ::org::alljoyn::alljoyn_test::DefaultWellKnownName;

/** AllJoynListener receives discovery events from AllJoyn */
class MyBusListener : public BusListener {
  public:

    MyBusListener(bool stopDiscover) : BusListener(), sessionId(0), stopDiscover(stopDiscover) { }

    void FoundAdvertisedName(const char* name, TransportMask transport, const char* namePrefix)
    {
        QCC_SyncPrintf("FoundAdvertisedName(name=%s, transport=0x%x, prefix=%s)\n", name, transport, namePrefix);

        if (0 == ::strcmp(name, g_wellKnownName.c_str())) {
            /* We found a remote bus that is advertising bbservice's well-known name so connect to it */
            uint32_t disposition = 0;
            SessionOpts opts(SessionOpts::TRAFFIC_MESSAGES, false, SessionOpts::PROXIMITY_ANY, transport);
            QStatus status;

            if (stopDiscover) {
                status = g_msgBus->CancelFindAdvertisedName(g_wellKnownName.c_str(), disposition);
                if ((ER_OK != status) || (ALLJOYN_CANCELFINDADVERTISEDNAME_REPLY_SUCCESS != disposition)) {
                    QCC_LogError(status, ("CancelFindAdvertisedName(%s) failed (%u)", name, disposition));
                }
            }

            status = g_msgBus->JoinSession(name, ::org::alljoyn::alljoyn_test::SessionPort, disposition, sessionId, opts);
            if ((ER_OK != status) || (ALLJOYN_JOINSESSION_REPLY_SUCCESS != disposition)) {
                QCC_LogError(status, ("JoinSession(%s) failed (%u)", name, disposition));
            }

            /* Release the main thread */
            if ((ER_OK == status) && (ALLJOYN_JOINSESSION_REPLY_SUCCESS == disposition)) {
                g_discoverEvent.SetEvent();
            }
        }
    }

    void LostAdvertisedName(const char* name, const char* guid, const char* prefix, const char* busAddress)
    {
        QCC_SyncPrintf("LostAdvertisedName(name=%s, guid=%s, prefix=%s, addr=%s)\n", name, guid, prefix, busAddress);
    }

    void NameOwnerChanged(const char* name, const char* previousOwner, const char* newOwner)
    {
        QCC_SyncPrintf("NameOwnerChanged(%s, %s, %s)\n",
                       name,
                       previousOwner ? previousOwner : "null",
                       newOwner ? newOwner : "null");
    }

    SessionId GetSessionId() const { return sessionId; }

  private:
    SessionId sessionId;
    bool stopDiscover;
};

/** Static bus listener */
static MyBusListener* g_busListener;

/** Signal handler */
static void SigIntHandler(int sig)
{
    if (NULL != g_msgBus) {
        QStatus status = g_msgBus->Stop(false);
        if (ER_OK != status) {
            QCC_LogError(status, ("BusAttachment::Stop() failed"));
        }
    }
}

static void usage(void)
{
    printf("Usage: bbclient [-h] [-c <count>] [-i] [-e] [-r #] [-l | -la | -d[s]] [-n <well-known name>] [-t[a] <delay> [<interval>] | -rt]\n\n");
    printf("Options:\n");
    printf("   -h                    = Print this help message\n");
    printf("   -c [count]            = Number of pings to send to the server\n");
    printf("   -i                    = Use introspection to discover remote interfaces\n");
    printf("   -e[k] [RSA|SRP]       = Encrypt the test interface using specified auth mechanism, -ek means clear keys\n");
    printf("   -a #                  = Max authentication attempts\n");
    printf("   -r #                  = AllJoyn attachment restart count\n");
    printf("   -l                    = launch bbservice if not already running\n");
    printf("   -la                   = launch bbservice if not already running using auto-launch\n");
    printf("   -n <well-known name>  = Well-known bus name advertised by bbservice\n");
    printf("   -d                    = discover remote bus with test service\n");
    printf("   -ds                   = discover remote bus with test service and cancel discover when found\n");
    printf("   -t                    = Call delayed_ping with <delay> and repeat at <interval> if -c given\n");
    printf("   -ta                   = Like -t except calls asynchronously\n");
    printf("   -rt                   = Round trip timer\n");
    printf("   -w                    = Don't wait for service\n");
    printf("   -s                    = Call BusAttachment::WaitStop before exiting");
    printf("\n");
}


static const char x509cert[] = {
    "-----BEGIN CERTIFICATE-----\n"
    "MIIBszCCARwCCQDuCh+BWVBk2DANBgkqhkiG9w0BAQUFADAeMQ0wCwYDVQQKDARN\n"
    "QnVzMQ0wCwYDVQQDDARHcmVnMB4XDTEwMDUxNzE1MTg1N1oXDTExMDUxNzE1MTg1\n"
    "N1owHjENMAsGA1UECgwETUJ1czENMAsGA1UEAwwER3JlZzCBnzANBgkqhkiG9w0B\n"
    "AQEFAAOBjQAwgYkCgYEArSd4r62mdaIRG9xZPDAXfImt8e7GTIyXeM8z49Ie1mrQ\n"
    "h7roHbn931Znzn20QQwFD6pPC7WxStXJVH0iAoYgzzPsXV8kZdbkLGUMPl2GoZY3\n"
    "xDSD+DA3m6krcXcN7dpHv9OlN0D9Trc288GYuFEENpikZvQhMKPDUAEkucQ95Z8C\n"
    "AwEAATANBgkqhkiG9w0BAQUFAAOBgQBkYY6zzf92LRfMtjkKs2am9qvjbqXyDJLS\n"
    "viKmYe1tGmNBUzucDC5w6qpPCTSe23H2qup27///fhUUuJ/ssUnJ+Y77jM/u1O9q\n"
    "PIn+u89hRmqY5GKHnUSZZkbLB/yrcFEchHli3vLo4FOhVVHwpnwLtWSpfBF9fWcA\n"
    "7THIAV79Lg==\n"
    "-----END CERTIFICATE-----"
};

static const char privKey[] = {
    "-----BEGIN RSA PRIVATE KEY-----\n"
    "Proc-Type: 4,ENCRYPTED\n"
    "DEK-Info: AES-128-CBC,0AE4BAB94CEAA7829273DD861B067DBA\n"
    "\n"
    "LSJOp+hEzNDDpIrh2UJ+3CauxWRKvmAoGB3r2hZfGJDrCeawJFqH0iSYEX0n0QEX\n"
    "jfQlV4LHSCoGMiw6uItTof5kHKlbp5aXv4XgQb74nw+2LkftLaTchNs0bW0TiGfQ\n"
    "XIuDNsmnZ5+CiAVYIKzsPeXPT4ZZSAwHsjM7LFmosStnyg4Ep8vko+Qh9TpCdFX8\n"
    "w3tH7qRhfHtpo9yOmp4hV9Mlvx8bf99lXSsFJeD99C5GQV2lAMvpfmM8Vqiq9CQN\n"
    "9OY6VNevKbAgLG4Z43l0SnbXhS+mSzOYLxl8G728C6HYpnn+qICLe9xOIfn2zLjm\n"
    "YaPlQR4MSjHEouObXj1F4MQUS5irZCKgp4oM3G5Ovzt82pqzIW0ZHKvi1sqz/KjB\n"
    "wYAjnEGaJnD9B8lRsgM2iLXkqDmndYuQkQB8fhr+zzcFmqKZ1gLRnGQVXNcSPgjU\n"
    "Y0fmpokQPHH/52u+IgdiKiNYuSYkCfHX1Y3nftHGvWR3OWmw0k7c6+DfDU2fDthv\n"
    "3MUSm4f2quuiWpf+XJuMB11px1TDkTfY85m1aEb5j4clPGELeV+196OECcMm4qOw\n"
    "AYxO0J/1siXcA5o6yAqPwPFYcs/14O16FeXu+yG0RPeeZizrdlv49j6yQR3JLa2E\n"
    "pWiGR6hmnkixzOj43IPJOYXySuFSi7lTMYud4ZH2+KYeK23C2sfQSsKcLZAFATbq\n"
    "DY0TZHA5lbUiOSUF5kgd12maHAMidq9nIrUpJDzafgK9JrnvZr+dVYM6CiPhiuqJ\n"
    "bXvt08wtKt68Ymfcx+l64mwzNLS+OFznEeIjLoaHU4c=\n"
    "-----END RSA PRIVATE KEY-----"
};

class MyAuthListener : public AuthListener {
  public:

    MyAuthListener(const qcc::String& userName, unsigned long maxAuth) : AuthListener(), userName(userName), maxAuth(maxAuth) { }

  private:

    bool RequestCredentials(const char* authMechanism, uint16_t authCount, const char* userId, uint16_t credMask, Credentials& creds) {

        if (authCount > maxAuth) {
            return false;
        }

        if (strcmp(authMechanism, "ALLJOYN_SRP_KEYX") == 0) {
            if (credMask & AuthListener::CRED_PASSWORD) {
                if (authCount == 3) {
                    creds.SetPassword("123456");
                } else {
                    creds.SetPassword("xxxxxx");
                }
                printf("AuthListener returning fixed pin \"%s\" for %s\n", creds.GetPassword().c_str(), authMechanism);
            }
            return true;
        }

        if (strcmp(authMechanism, "ALLJOYN_RSA_KEYX") == 0) {
            if (credMask & AuthListener::CRED_CERT_CHAIN) {
                creds.SetCertChain(x509cert);
            }
            if (credMask & AuthListener::CRED_PRIVATE_KEY) {
                creds.SetPrivateKey(privKey);
            }
            if (credMask & AuthListener::CRED_PASSWORD) {
                creds.SetPassword("123456");
            }
            return true;
        }

        if (strcmp(authMechanism, "ALLJOYN_SRP_LOGON") == 0) {
            if (credMask & AuthListener::CRED_USER_NAME) {
                if (authCount == 1) {
                    creds.SetUserName("Mr Bogus");
                } else {
                    creds.SetUserName(userName);
                }
            }
            if (credMask & AuthListener::CRED_PASSWORD) {
                creds.SetPassword("123456");
            }
            return true;
        }

        return false;
    }

    bool VerifyCredentials(const char* authMechanism, const Credentials& creds) {
        if (strcmp(authMechanism, "ALLJOYN_RSA_KEYX") == 0) {
            if (creds.IsSet(AuthListener::CRED_CERT_CHAIN)) {
                printf("Verify\n%s\n", creds.GetCertChain().c_str());
                return true;
            }
        }
        return false;
    }

    void AuthenticationComplete(const char* authMechanism, bool success) {
        printf("Authentication %s %s\n", authMechanism, success ? "succesful" : "failed");
    }

    void SecurityViolation(const char* error) {
        printf("Security violation %s\n", error);
    }

    qcc::String userName;
    unsigned long maxAuth;
};


class MyMessageReceiver : public MessageReceiver {
  public:
    void PingResponseHandler(Message& message, void* context)
    {
        const InterfaceDescription::Member* pingMethod = static_cast<const InterfaceDescription::Member*>(context);
        if (message->GetType() == MESSAGE_METHOD_RET) {
            QCC_SyncPrintf("%s.%s returned \"%s\"\n",
                           g_wellKnownName.c_str(),
                           pingMethod->name.c_str(),
                           message->GetArg(0)->v_string.str);

        } else {
            // must be an error
            qcc::String errMsg;
            const char* errName = message->GetErrorName(&errMsg);
            QCC_SyncPrintf("%s.%s returned error %s: %s\n",
                           g_wellKnownName.c_str(),
                           pingMethod->name.c_str(),
                           errName,
                           errMsg.c_str());
        }
    }
};



/** Main entry point */
int main(int argc, char** argv)
{
    QStatus status = ER_OK;
    bool useIntrospection = false;
    bool encryptIfc = false;
    bool clearKeys = false;
    qcc::String authMechs;
    qcc::String pbusConnect;
    qcc::String userId;
    unsigned long pingCount = 1;
    unsigned long repCount = 1;
    unsigned long authCount = 1000;
    Environ* env;
    bool startService = false;
    bool autoStartService = false;
    bool discoverRemote = false;
    bool stopDiscover = false;
    bool waitForService = true;
    bool asyncPing = false;
    uint32_t pingDelay = 0;
    uint32_t pingInterval = 0;
    bool waitStop = false;
    bool roundtrip = false;

#ifdef _WIN32
    WSADATA wsaData;
    WORD version = MAKEWORD(2, 0);
    int error = WSAStartup(version, &wsaData);
#endif

    printf("AllJoyn Library version: %s\n", ajn::GetVersion());
    printf("AllJoyn Library build info: %s\n", ajn::GetBuildInfo());

    /* Install SIGINT handler */
    signal(SIGINT, SigIntHandler);

    /* Parse command line args */
    for (int i = 1; i < argc; ++i) {
        if (0 == strcmp("-i", argv[i])) {
            useIntrospection = true;
        } else if ((0 == strcmp("-e", argv[i])) || (0 == strcmp("-ek", argv[i]))) {
            if (!authMechs.empty()) {
                authMechs += " ";
            }
            bool ok = false;
            encryptIfc = true;
            clearKeys |= (argv[i][2] == 'k');
            ++i;
            if (i != argc) {

                if (strcmp(argv[i], "RSA") == 0) {
                    authMechs += "ALLJOYN_RSA_KEYX";
                    ok = true;
                } else if (strcmp(argv[i], "SRP") == 0) {
                    authMechs += "ALLJOYN_SRP_KEYX";
                    ok = true;
                } else if (strcmp(argv[i], "LOGON") == 0) {
                    if (++i == argc) {
                        printf("option %s LOGON requires a user id\n", argv[i - 2]);
                        usage();
                        exit(1);
                    }
                    authMechs += "ALLJOYN_SRP_LOGON";
                    userId = argv[i];
                    ok = true;
                }
            }
            if (!ok) {
                printf("option %s requires an auth mechanism \n", argv[i - 1]);
                usage();
                exit(1);
            }
        } else if (0 == strcmp("-a", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                authCount = strtoul(argv[i], NULL, 10);
            }
        } else if (0 == strcmp("-c", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                pingCount = strtoul(argv[i], NULL, 10);
            }
        } else if (0 == strcmp("-r", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                repCount = strtoul(argv[i], NULL, 10);
            }
        } else if (0 == strcmp("-n", argv[i])) {
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                g_wellKnownName = argv[i];
            }
        } else if (0 == strcmp("-h", argv[i])) {
            usage();
            exit(0);
        } else if (0 == strcmp("-l", argv[i])) {
            startService = true;
        } else if (0 == strcmp("-la", argv[i])) {
            autoStartService = true;
        } else if (0 == strcmp("-d", argv[i])) {
            discoverRemote = true;
        } else if (0 == strcmp("-ds", argv[i])) {
            discoverRemote = true;
            stopDiscover = true;
        } else if (0 == strcmp("-w", argv[i])) {
            waitForService = false;
        } else if ((0 == strcmp("-t", argv[i])) || (0 == strcmp("-ta", argv[i]))) {
            if (argv[i][2] == 'a') {
                asyncPing = true;
            }
            ++i;
            if (i == argc) {
                printf("option %s requires a parameter\n", argv[i - 1]);
                usage();
                exit(1);
            } else {
                pingDelay = strtoul(argv[i], NULL, 10);
                ++i;
                if ((i == argc) || (argv[i][0] == '-')) {
                    --i;
                } else {
                    pingInterval = strtoul(argv[i], NULL, 10);
                }
            }
        } else if (0 == strcmp("-rt", argv[i])) {
            roundtrip = true;
            if (pingCount == 1) {
                pingCount = 1000;
            }
        } else if (0 == strcmp("-s", argv[i])) {
            waitStop = true;
        } else {
            status = ER_FAIL;
            printf("Unknown option %s\n", argv[i]);
            usage();
            exit(1);
        }
    }

    /* Get env vars */
    env = Environ::GetAppEnviron();
#ifdef _WIN32
    qcc::String connectArgs = env->Find("BUS_ADDRESS", "tcp:addr=127.0.0.1,port=9955");
#else
    // qcc::String connectArgs = env->Find("BUS_ADDRESS", "unix:path=/var/run/dbus/system_bus_socket");
    qcc::String connectArgs = env->Find("BUS_ADDRESS", "unix:abstract=alljoyn");
#endif

    /* Create message bus */
    g_msgBus = new BusAttachment("bbclient", true);

    if (!useIntrospection) {
        /* Add org.alljoyn.alljoyn_test interface */
        InterfaceDescription* testIntf = NULL;
        status = g_msgBus->CreateInterface(::org::alljoyn::alljoyn_test::InterfaceName, testIntf, encryptIfc);
        if ((ER_OK == status) && testIntf) {
            testIntf->AddSignal("my_signal", NULL, NULL, 0);
            testIntf->AddMethod("my_ping", "s", "s", "outStr,inStr", 0);
            testIntf->AddMethod("delayed_ping", "su", "s", "outStr,delay,inStr", 0);
            testIntf->AddMethod("time_ping", "uq", "uq", NULL, 0);
            testIntf->Activate();
        } else {
            if (ER_OK == status) status = ER_FAIL;
            QCC_LogError(status, ("Failed to create interface \"%s\"", ::org::alljoyn::alljoyn_test::InterfaceName));
        }

        if (ER_OK == status) {
            /* Add org.alljoyn.alljoyn_test.values interface */
            InterfaceDescription* valuesIntf = NULL;
            status = g_msgBus->CreateInterface(::org::alljoyn::alljoyn_test::values::InterfaceName, valuesIntf, encryptIfc);
            if ((ER_OK == status) && valuesIntf) {
                valuesIntf->AddProperty("int_val", "i", PROP_ACCESS_RW);
                valuesIntf->AddProperty("str_val", "s", PROP_ACCESS_RW);
                valuesIntf->AddProperty("ro_str", "s", PROP_ACCESS_READ);
                valuesIntf->Activate();
            } else {
                if (ER_OK == status) status = ER_FAIL;
                QCC_LogError(status, ("Failed to create interface \"%s\"", ::org::alljoyn::alljoyn_test::values::InterfaceName));
            }
        }
    }

    /* Register a bus listener in order to get discovery indications */
    if (ER_OK == status) {
        g_busListener = new MyBusListener(stopDiscover);
        g_msgBus->RegisterBusListener(*g_busListener);
    }

    for (unsigned long i = 0; i < repCount; i++) {

        unsigned long pings = pingCount;


        /* Start the msg bus */
        if (ER_OK == status) {
            status = g_msgBus->Start();
            if (ER_OK == status) {
                if (encryptIfc) {
                    g_msgBus->EnablePeerSecurity(authMechs.c_str(), new MyAuthListener(userId, authCount));
                    if (clearKeys) {
                        g_msgBus->ClearKeyStore();
                    }
                }
            } else {
                QCC_LogError(status, ("BusAttachment::Start failed"));
            }
        }

        /* Connect to the bus */
        if (ER_OK == status) {
            status = g_msgBus->Connect(connectArgs.c_str());
            if (ER_OK != status) {
                QCC_LogError(status, ("BusAttachment::Connect(\"%s\") failed", connectArgs.c_str()));
            }
        }

        if (ER_OK == status) {
            if (startService) {
                /* Start the org.alljoyn.alljoyn_test service. */
                MsgArg args[2];
                Message reply(*g_msgBus);
                args[0].Set("s", g_wellKnownName.c_str());
                args[1].Set("u", 0);
                const ProxyBusObject& dbusObj = g_msgBus->GetDBusProxyObj();
                status = dbusObj.MethodCall(ajn::org::freedesktop::DBus::InterfaceName,
                                            "StartServiceByName",
                                            args,
                                            ArraySize(args),
                                            reply);
            } else if (discoverRemote) {
                /* Begin discovery on the well-known name of the service to be called */
                uint32_t disposition = 0;
                status = g_msgBus->FindAdvertisedName(g_wellKnownName.c_str(), disposition);
                if ((status != ER_OK) || (disposition != ALLJOYN_FINDADVERTISEDNAME_REPLY_SUCCESS)) {
                    status = (status == ER_OK) ? ER_FAIL : status;
                    QCC_LogError(status, ("FindAdvertisedName failed (disposition=%d)", disposition));
                }
            }
        }

        /*
         * If discovering, wait for the "FoundAdvertisedName" signal that tells us that we are connected to a
         * remote bus that is advertising bbservice's well-known name.
         */
        if (discoverRemote && (ER_OK == status)) {
            status = Event::Wait(g_discoverEvent);
        } else if (waitForService && (ER_OK == status)) {
            /* If bbservice's well-known name is not currently on the bus yet, then wait for it to appear */
            bool hasOwner = false;
            g_discoverEvent.ResetEvent();
            status = g_msgBus->NameHasOwner(g_wellKnownName.c_str(), hasOwner);
            if ((ER_OK == status) && !hasOwner) {
                QCC_SyncPrintf("Waiting for name %s to appear on the bus\n", g_wellKnownName.c_str());
                status = Event::Wait(g_discoverEvent);
                if (ER_OK != status) {
                    QCC_LogError(status, ("Event::Wait failed"));
                }
            }
        }

        if (ER_OK == status) {
            /* Create the remote object that will be called */
            ProxyBusObject remoteObj;
            if (ER_OK == status) {
                remoteObj = ProxyBusObject(*g_msgBus, g_wellKnownName.c_str(), ::org::alljoyn::alljoyn_test::ObjectPath, g_busListener->GetSessionId());
                if (useIntrospection) {
                    status = remoteObj.IntrospectRemoteObject();
                    if (ER_OK != status) {
                        QCC_LogError(status, ("Introspection of %s (path=%s) failed",
                                              g_wellKnownName.c_str(),
                                              ::org::alljoyn::alljoyn_test::ObjectPath));
                    }
                } else {
                    const InterfaceDescription* alljoynTestIntf = g_msgBus->GetInterface(::org::alljoyn::alljoyn_test::InterfaceName);
                    assert(alljoynTestIntf);
                    remoteObj.AddInterface(*alljoynTestIntf);

                    const InterfaceDescription* alljoynTestValuesIntf = g_msgBus->GetInterface(::org::alljoyn::alljoyn_test::values::InterfaceName);
                    assert(alljoynTestValuesIntf);
                    remoteObj.AddInterface(*alljoynTestValuesIntf);
                }
            }

            MyMessageReceiver msgReceiver;
            size_t cnt = 0;
            uint64_t sample = 0;
            uint64_t timeSum = 0;
            uint64_t max_delta = 0;
            uint64_t min_delta = ~0;

            /* Call the remote method */
            while ((ER_OK == status) && pings--) {
                Message reply(*g_msgBus);
                MsgArg pingArgs[2];
                const InterfaceDescription::Member* pingMethod;
                const InterfaceDescription* ifc = remoteObj.GetInterface(::org::alljoyn::alljoyn_test::InterfaceName);
                if (ifc == NULL) {
                    status = ER_BUS_NO_SUCH_INTERFACE;
                    QCC_SyncPrintf("Unable to Get InterfaceDecription for the %s interface\n",
                                   ::org::alljoyn::alljoyn_test::InterfaceName);
                    break;
                }
                char buf[80];

                if (roundtrip) {
                    Timespec now;
                    GetTimeNow(&now);
                    pingMethod = ifc->GetMember("time_ping");
                    pingArgs[0].Set("u", now.seconds);
                    pingArgs[1].Set("q", now.mseconds);
                } else {
                    sprintf(buf, "Ping String %u", static_cast<unsigned int>(++cnt));
                    pingArgs[0].Set("s", buf);

                    if (pingDelay > 0) {
                        pingMethod = ifc->GetMember("delayed_ping");
                        pingArgs[1].Set("u", pingDelay);
                    } else {
                        pingMethod = ifc->GetMember("my_ping");
                    }
                }

                if (!roundtrip && asyncPing) {
                    QCC_SyncPrintf("Sending \"%s\" to %s.%s asynchronously\n",
                                   buf, ::org::alljoyn::alljoyn_test::InterfaceName, pingMethod->name.c_str());
                    status = remoteObj.MethodCallAsync(*pingMethod,
                                                       &msgReceiver,
                                                       static_cast<MessageReceiver::ReplyHandler>(&MyMessageReceiver::PingResponseHandler),
                                                       pingArgs, (pingDelay > 0) ? 2 : 1,
                                                       const_cast<void*>(static_cast<const void*>(pingMethod)),
                                                       pingDelay + 10000);
                    if (status != ER_OK) {
                        QCC_LogError(status, ("MethodCallAsync on %s.%s failed", ::org::alljoyn::alljoyn_test::InterfaceName, pingMethod->name.c_str()));
                    }
                } else {
                    if (!roundtrip) {
                        QCC_SyncPrintf("Sending \"%s\" to %s.%s synchronously\n",
                                       buf, ::org::alljoyn::alljoyn_test::InterfaceName, pingMethod->name.c_str());
                    }
                    status = remoteObj.MethodCall(*pingMethod, pingArgs, (roundtrip || (pingDelay > 0)) ? 2 : 1, reply, pingDelay + 50000);
                    if (ER_OK == status) {
                        if (roundtrip) {
                            Timespec now;
                            GetTimeNow(&now);
                            uint64_t delta = ((static_cast<uint64_t>(now.seconds) * 1000ULL + now.mseconds) -
                                              (static_cast<uint64_t>(reply->GetArg(0)->v_uint32) * 1000ULL + reply->GetArg(1)->v_uint16));
                            if (delta > max_delta) {
                                max_delta = delta;
                                QCC_SyncPrintf("New Max time: %llu ms\n", max_delta);
                            }
                            if (delta < min_delta) {
                                min_delta = delta;
                                QCC_SyncPrintf("New Min time: %llu ms\n", min_delta);
                            }
                            if (delta > ((~0ULL) / pingCount)) {
                                QCC_SyncPrintf("Round trip time &llu ms will overflow average calculation; dropping...\n", delta);
                            } else {
                                timeSum += delta;
                            }
                            QCC_SyncPrintf("DELTA: %llu %llu\n", sample, delta);
                            ++sample;
                        } else {
                            QCC_SyncPrintf("%s.%s ( path=%s ) returned \"%s\"\n",
                                           g_wellKnownName.c_str(),
                                           pingMethod->name.c_str(),
                                           ::org::alljoyn::alljoyn_test::ObjectPath,
                                           reply->GetArg(0)->v_string.str);
                        }
                    } else if (status == ER_BUS_REPLY_IS_ERROR_MESSAGE) {
                        qcc::String errDescription;
                        const char* errName = reply->GetErrorName(&errDescription);
                        QCC_SyncPrintf("MethodCall on %s.%s reply was error %s %s\n", ::org::alljoyn::alljoyn_test::InterfaceName, pingMethod->name.c_str(), errName, errDescription.c_str());
                        status = ER_OK;
                    } else {
                        QCC_LogError(status, ("MethodCall on %s.%s failed", ::org::alljoyn::alljoyn_test::InterfaceName, pingMethod->name.c_str()));
                    }
                }
                if (pingInterval > 0) {
                    qcc::Sleep(pingInterval);
                }
            }

            if (roundtrip) {
                QCC_SyncPrintf("Round trip time MIN/AVG/MAX: %llu/%llu.%03u/%llu ms\n",
                               min_delta,
                               timeSum / pingCount, ((timeSum % pingCount) * 1000) / pingCount,
                               max_delta);
            }

            /* Get the test property */
            if (!roundtrip && (ER_OK == status)) {
                MsgArg val;
                status = remoteObj.GetProperty(::org::alljoyn::alljoyn_test::values::InterfaceName, "int_val", val);
                if (ER_OK == status) {
                    QCC_SyncPrintf("%s.%s ( path=%s) returned \"%s\"\n",
                                   g_wellKnownName.c_str(),
                                   "GetProperty",
                                   ::org::alljoyn::alljoyn_test::ObjectPath,
                                   val.ToString().c_str());
                } else {
                    QCC_LogError(status, ("GetProperty on %s failed", g_wellKnownName.c_str()));
                }
            }

            /* Stop the bus */
            if (g_msgBus) {
                if (waitStop) {
                    g_msgBus->WaitStop();
                } else {
                    QStatus s = g_msgBus->Stop();
                    if (ER_OK != s) {
                        QCC_LogError(s, ("BusAttachment::Stop failed"));
                    }
                }
            }

            if (status != ER_OK) {
                break;
            }
        }
    }

    /* Deallocate bus */
    BusAttachment* deleteMe = g_msgBus;
    g_msgBus = NULL;
    delete deleteMe;

    delete g_busListener;
    g_busListener = NULL;

    printf("bbclient exiting with status %d (%s)\n", status, QCC_StatusText(status));

    return (int) status;
}
