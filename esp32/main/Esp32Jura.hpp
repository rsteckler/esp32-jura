#pragma once

// #define MODE_SNOOPER
// #define MODE_COFFEE_MAKER
// #define MODE_BRIDGE
// #define MODE_SERIAL
#define MODE_XMPP

#include <smooth/core/Application.h>

#include <memory>

#include "esp/Storage.hpp"
#include "esp/sensor/Button.hpp"

#ifdef MODE_SNOOPER
#include "jura/JuraSnooperTask.hpp"
#endif  // MODE_SNOOPER
#ifdef MODE_COFFEE_MAKER
#include "jura/CoffeeMakerTask.hpp"
#endif  // MODE_COFFEE_MAKER
#ifdef MODE_BRIDGE
#include "utils/SerialJuraBridgeTask.hpp"
#endif  // MODE_BRIDGE
#ifdef MODE_SERIAL
#include "esp/serial/SerialConnectionTask.hpp"
#endif  // MODE_SERIAL
#ifdef MODE_XMPP
#include "esp/WifiTask.hpp"
#include "esp/actuator/RgbLed.hpp"
#include "xmpp/XmppTask.hpp"
#endif  // MODE_XMPP

//---------------------------------------------------------------------------
namespace esp32jura {
//---------------------------------------------------------------------------
#if defined(__cplusplus)
extern "C" {
#endif
void app_main(void);
#if defined(__cplusplus)
}
#endif
//---------------------------------------------------------------------------
class Esp32Jura : public smooth::core::Application {
   private:
    std::shared_ptr<esp::Storage> storage;
    esp::sensor::Button resetButton{GPIO_NUM_13};

#ifdef MODE_SNOOPER
    jura::JuraSnooperTask snooper{};
#endif  // MODE_SNOOPER
#ifdef MODE_COFFEE_MAKER
    jura::CoffeeMakerTask coffeeMaker{};
#endif  // MODE_COFFEE_MAKER
#ifdef MODE_BRIDGE
    utils::SerialJuraBridgeTask bridge{};
#endif  // MODE_BRIDGE
#ifdef MODE_SERIAL
    esp::serial::SerialConnectionTask serialConnection{};
#endif  // MODE_SERIAL
#ifdef MODE_XMPP
    std::shared_ptr<esp::actuator::RgbLed> rgbLed{};
    std::unique_ptr<xmpp::XmppTask> xmpp{};
    std::shared_ptr<esp::WifiTask> wifiTask{};
#endif  // MODE_XMPP

   public:
    Esp32Jura();

    void init() override;
    void tick() override;

   private:
    /**
     * Initialized the NVS with dummy/test values for debugging.
     **/
    void initWithDummyValues();
};
//---------------------------------------------------------------------------
}  // namespace esp32jura
//---------------------------------------------------------------------------
