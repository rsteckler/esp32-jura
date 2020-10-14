#pragma once

#include <string>

#include "ActuatorNode.hpp"

//---------------------------------------------------------------------------
namespace esp32jura::xmpp::messages::xep_iot {
//---------------------------------------------------------------------------
class ButtonNode : public ActuatorNode {
   private:
    const std::string title;
    std::string value{"0"};

   public:
    ButtonNode(const std::string&& id, const std::string&& title);
    ~ButtonNode() override = default;

    const std::string get_value_str() const override;
    bool on_value_changed(const tinyxml2::XMLElement* valNode) override;

   protected:
    Type get_type() const override;
    void to_ui_field(tinyxml2::XMLDocument& doc, tinyxml2::XMLElement* xNode) const override;
};
//---------------------------------------------------------------------------
}  // namespace esp32jura::xmpp::messages::xep_iot
//---------------------------------------------------------------------------