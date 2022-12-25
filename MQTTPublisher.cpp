#include "MQTTPublisher.h"
#include "Settings.h"

#pragma once

WiFiClient espClient;
PubSubClient client(espClient);

MQTTPublisher::MQTTPublisher(String identifier)
{
  _identifier = identifier;
  randomSeed(micros());
  logger = Logger("MQTTPublisher");
}

MQTTPublisher::MQTTPublisher() {}

bool MQTTPublisher::reconnect()
{
  lastConnectionAttempt = millis();

  logger.debug("Attempt connection to server: " + String(MQTT_BROKER));

  // Attempt to connect
  bool clientConnected;
  if (String(MQTT_USER_NAME).length())
  {
    logger.info("Connecting with credientials");
    clientConnected = client.connect(_identifier.c_str(), MQTT_USER_NAME, MQTT_PASSWORD);
  }
  else
  {
    logger.info("Connecting without credentials");
    clientConnected = client.connect(_identifier.c_str());
  }

  if (clientConnected)
  {
    logger.debug("connected");

    hasMQTT = true;

    // Once connected, publish an announcement...

    client.publish(String(_identifier + "/status").c_str(), String("online").c_str());
    return true;
  }
  else
  {
    logger.warn("failed, rc=" + client.state());
  }

  return false;
}

void MQTTPublisher::start()
{

  if (String(MQTT_BROKER).length() == 0 || MQTT_PORT == 0)
  {
    logger.warn("disabled. No hostname or port set.");
    return; //not configured
  }

  logger.debug("enabled. Connecting.");

  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setKeepAlive(60);
  client.setBufferSize(2048);

  reconnect();
  isStarted = true;
}

void MQTTPublisher::stop()
{
  isStarted = false;
}

void MQTTPublisher::handle()
{
  if (!isStarted)
    return;

  if (!client.connected() && millis() - lastConnectionAttempt > RECONNECT_TIMEOUT)
  {
    hasMQTT = false;
    if (!reconnect())
      return;
  }
}

bool MQTTPublisher::publish(String topic, String msg, bool addIdentifier)
{
  if (addIdentifier)
    topic = _identifier + "/" + topic;
  logger.debug("Publish to: " + topic + ": " + msg);

  auto retVal = client.publish(topic.c_str(), msg.c_str(), true);

  yield();
  if (!retVal)
    logger.debug("!error : " + String(client.state()));

  return retVal;
}
