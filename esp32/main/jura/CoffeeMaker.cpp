#include "CoffeeMaker.hpp"

#include <cassert>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <thread>

#include "JuraCommands.hpp"
#include "smooth/core/task_priorities.h"

//---------------------------------------------------------------------------
namespace esp32jura::jura {
//---------------------------------------------------------------------------
CoffeeMaker::CoffeeMaker() : connection(UART_PORT, UART_TX, UART_RX) {}

void CoffeeMaker::init() {
    std::cout << "Initializing coffee maker...\n";
    connection.init();
    std::cout << "Coffee maker initialized.\n";
}

void CoffeeMaker::switch_page() {
    press_button(jura_button_t::BUTTON_6);
    if (++pageNum >= NUM_PAGES) {
        pageNum = 0;
    }
}

void CoffeeMaker::switch_page(size_t pageNum) {
    if (this->pageNum == pageNum) {
        return;
    }

    press_button(jura_button_t::BUTTON_6);
    if (++pageNum >= NUM_PAGES) {
        pageNum = 0;
    }
    switch_page(pageNum);
}

void CoffeeMaker::brew_coffee(coffee_t coffee) {
    assert(!locked);
    locked = true;

    size_t pageNum = get_page_num(coffee);
    switch_page(pageNum);
    jura_button_t button = get_button_num(coffee);
    press_button(button);
    locked = false;
}

size_t CoffeeMaker::get_page_num(coffee_t coffee) {
    for (const std::pair<coffee_t, size_t>& c : coffee_page_map) {
        if (c.first == coffee) {
            return c.second;
        }
    }
    assert(false);  // Should not happen
    return std::numeric_limits<size_t>::max();
}

CoffeeMaker::jura_button_t CoffeeMaker::get_button_num(coffee_t coffee) {
    for (const std::pair<coffee_t, jura_button_t>& c : coffee_button_map) {
        if (c.first == coffee) {
            return c.second;
        }
    }
    assert(false);  // Should not happen
    return jura_button_t::BUTTON_6;
}

void CoffeeMaker::press_button(jura_button_t button) {
    switch (button) {
        case jura_button_t::BUTTON_1:
            write_and_wait(JURA_BUTTON_1);
            break;

        case jura_button_t::BUTTON_2:
            write_and_wait(JURA_BUTTON_2);
            break;

        case jura_button_t::BUTTON_3:
            write_and_wait(JURA_BUTTON_3);
            break;

        case jura_button_t::BUTTON_4:
            write_and_wait(JURA_BUTTON_4);
            break;

        case jura_button_t::BUTTON_5:
            write_and_wait(JURA_BUTTON_5);
            break;

        case jura_button_t::BUTTON_6:
            write_and_wait(JURA_BUTTON_6);
            break;

        default:
            assert(false);  // Should not happen
            break;
    }

    // Give the coffee maker time to react:
    std::this_thread::sleep_for(std::chrono::milliseconds{500});
}

void CoffeeMaker::brew_custom_coffee(const std::chrono::milliseconds grindTime, const std::chrono::milliseconds waterTime) {
    assert(!locked);
    locked = true;

    std::cout << "Brewing custom coffee with " << std::to_string(grindTime.count()) << " ms grind time and " << std::to_string(waterTime.count()) << " ms water time...\n";

    // Grind:
    std::cout << "Custom coffee grinding...\n";
    write_and_wait(JURA_GRINDER_ON);
    std::this_thread::sleep_for(grindTime);
    write_and_wait(JURA_GRINDER_OFF);
    write_and_wait(JURA_BREW_GROUP_TO_BREWING_POSITION);

    // Compress:
    std::cout << "Custom coffee compressing...\n";
    write_and_wait(JURA_COFFEE_PRESS_ON);
    std::this_thread::sleep_for(std::chrono::milliseconds{500});
    write_and_wait(JURA_COFFEE_PRESS_OFF);

    // Brew step 1:
    std::cout << "Custom coffee brewing...\n";
    write_and_wait(JURA_COFFEE_WATER_PUMP_ON);
    std::this_thread::sleep_for(std::chrono::milliseconds{2000});
    write_and_wait(JURA_COFFEE_WATER_PUMP_OFF);
    std::this_thread::sleep_for(std::chrono::milliseconds{2000});

    // Brew setp 2:
    pump_hot_water(waterTime);

    // Reset:
    std::cout << "Custom coffee finishing up...\n";
    write_and_wait(JURA_BREW_GROUP_RESET);
    std::cout << "Custom coffee done.\n";

    locked = false;
}

bool CoffeeMaker::write_and_wait(const std::string s) {
    connection.flush_read_buffer();
    connection.write_decoded(s);
    return connection.wait_for_ok();
}

void CoffeeMaker::pump_hot_water(const std::chrono::milliseconds waterTime) {
    write_and_wait(JURA_COFFEE_WATER_PUMP_ON);
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now() + waterTime;
    while (std::chrono::steady_clock::now() < end) {
        write_and_wait(JURA_COFFEE_WATER_HEATER_ON);
        std::cout << "Heater turned on.\n";
        std::this_thread::sleep_for(waterTime / 8);
        write_and_wait(JURA_COFFEE_WATER_HEATER_OFF);
        std::cout << "Heater turned off.\n";
        std::this_thread::sleep_for(waterTime / 20);
    }
    write_and_wait(JURA_COFFEE_WATER_PUMP_OFF);
}

bool CoffeeMaker::is_locked() const { return locked; }

//---------------------------------------------------------------------------
}  // namespace esp32jura::jura
//---------------------------------------------------------------------------
