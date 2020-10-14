#include "SensorNode.hpp"

#include "xmpp/helpers/PubSubHelper.hpp"

//---------------------------------------------------------------------------
namespace esp32jura::xmpp::messages::xep_iot {
//---------------------------------------------------------------------------
const std::string SensorNode::XMPP_IOT_SENSORS = "xmpp.iot.sensors";

SensorNode::SensorNode(const std::string&& id, const std::string&& fieldType, const std::string&& unit) : AbstractNode(std::move(id), std::move(fieldType)), unit(std::move(unit)) {}

NodeType SensorNode::get_node_type() const { return NodeType::SENSOR; }

void SensorNode::gen_sensor_node(tinyxml2::XMLDocument& doc, const xmpp::Jid& from, const std::string& iot_ns) const {
    tinyxml2::XMLElement* itemNode = helpers::PubSubHelper::gen_publish_item_node(doc, from, XMPP_IOT_SENSORS.c_str(), id.c_str());
    tinyxml2::XMLElement* valNode = doc.NewElement("val");
    valNode->SetAttribute("xmlns", iot_ns.c_str());
    valNode->SetAttribute("type", fieldType.c_str());
    valNode->SetAttribute("unit", unit.c_str());
    valNode->SetText(get_value_str().c_str());
    itemNode->InsertEndChild(valNode);
}

const std::string SensorNode::get_unit() const { return unit; }
//---------------------------------------------------------------------------
}  // namespace esp32jura::xmpp::messages::xep_iot
//---------------------------------------------------------------------------
