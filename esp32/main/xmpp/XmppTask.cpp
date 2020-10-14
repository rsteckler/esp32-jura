#include "XmppTask.hpp"

#include <smooth/core/network/event/ConnectionStatusEvent.h>
#include <smooth/core/task_priorities.h>
#include <tinyxml2.h>

#include <iostream>
#include <string>

#include "xmpp/Jid.hpp"
#include "xmpp/XmppAccount.hpp"

//---------------------------------------------------------------------------
namespace esp32jura::xmpp {
//---------------------------------------------------------------------------
using namespace smooth::core;
//---------------------------------------------------------------------------
const std::string XmppTask::INITIAL_HELLO_MESSAGE = "Hi from the ESP32. Please mirror this message!";

XmppTask::XmppTask(std::shared_ptr<esp::Storage> storage)
    : Task("XMPP Task", 4096, smooth::core::APPLICATION_BASE_PRIO, std::chrono::seconds(3), 1),
      net_status(NetworkStatusQueue::create(2, *this, *this)),
      storage(std::move(storage)),
      client(nullptr),
      iotDevice(nullptr) {}

XmppTask::~XmppTask() {
    if (client) {
        client->unsubscribeFromMessagesListener(this);
    }
}

void XmppTask::init() {
    // Coffee Maker:
    coffeeMaker.init();

    // XMPP:
    std::string jidString = storage->readString(esp::Storage::JID);
    xmpp::Jid jid = xmpp::Jid::fromFullJid(jidString);
    std::string password = storage->readString(esp::Storage::JID_PASSWORD);
    xmpp::XmppAccount account(std::move(jid), std::move(password), std::make_shared<smooth::core::network::IPv4>(jid.domainPart, 5222));
    client = std::make_unique<xmpp::XmppClient>(std::move(account), *this, *this);
    client->subscribeToMessagesListener(this);

    // IoT-Device:
    iotDevice = std::make_unique<messages::xep_iot::Device>(client);
}

void XmppTask::tick() {
    if (client->isConnected()) {
        // pubSubHelper->requestNodeConfigMessage(pubSubHelper->XMPP_IOT_ACTUATORS);
    }
}

void XmppTask::event(const network::NetworkStatus& event) {
    switch (event.get_event()) {
        case network::NetworkEvent::GOT_IP:
            if (client) {
                client->connect();
            }
            break;

        case network::NetworkEvent::DISCONNECTED:
            if (client) {
                client->disconnect();
            }
            break;

        default:
            break;
    }
}

void XmppTask::onReady() { iotDevice->init(); }

void XmppTask::handlePresenceMessages(const tinyxml2::XMLElement* elem) {
    const tinyxml2::XMLAttribute* attrib = elem->FindAttribute("type");
    // For now approve all presence subscription requests:
    if (attrib && !strcmp(attrib->Value(), "subscribe")) {
        attrib = elem->FindAttribute("from");
        std::string bareJid = attrib->Value();
        client->approvePresenceSubscription(bareJid);
    }
}

void XmppTask::handleMessageMessages(const tinyxml2::XMLElement* elem) {
    const tinyxml2::XMLElement* tmp = elem->FirstChildElement("body");
    if (tmp) {
        if (!storage->readBool(esp::Storage::SETUP_DONE)) {
            if (!strcmp(INITIAL_HELLO_MESSAGE.c_str(), tmp->GetText())) {
                std::string to = storage->readString(esp::Storage::JID_SENDER);

                // Add to roster:
                client->addToRoster(to);

                // Send setup done message:
                std::string body = "Setup done!";
                client->sendMessage(to, body);
                storage->writeBool(esp::Storage::SETUP_DONE, true);
                std::cout << "Setup done!\n";
                onReady();
            }
        } else {
            handleIoTMessageMessage(tmp->GetText());
        }
    } else if ((tmp = elem->FirstChildElement("event"))) {
        if ((tmp = tmp->FirstChildElement("items"))) {
            iotDevice->on_pub_sub_event_message(tmp);
        }
    }
}

void XmppTask::handleIoTMessageMessage(const char* msg) {
    /*#ifdef SPEAKER
        if (!strcmp("set speaker 1", msg)) {
            // speaker.set(1000);
        } else if (!strcmp("set speaker 0", msg)) {
            // speaker.set(0);
        }
    #endif  // SPEAKER*/
}

void XmppTask::event(const XmppClientConnectionState& event) {
    if (event == CLIENT_CONNECTED) {
        if (!storage->readBool(esp::Storage::SETUP_DONE)) {
            std::string to = storage->readString(esp::Storage::JID_SENDER);
            client->sendMessage(to, INITIAL_HELLO_MESSAGE);
        } else {
            onReady();
        }
    }
}

void XmppTask::event(messages::Message& event) {
    const tinyxml2::XMLDocument& doc = event.toXmlDoc();
    const tinyxml2::XMLElement* elem = nullptr;
    if ((elem = doc.FirstChildElement("message"))) {
        handleMessageMessages(elem);
    } else if ((elem = doc.FirstChildElement("presence"))) {
        handlePresenceMessages(elem);
    }
}
//---------------------------------------------------------------------------
}  // namespace esp32jura::xmpp
//---------------------------------------------------------------------------
