#!/bin/bash
# Register MQTT broker with Bonjour
# Save as: ~/register-mqtt-broker.sh

# Get your Mac's hostname
HOSTNAME=$(hostname)

# Register the MQTT service
dns-sd -R "MQTT Broker" _mqtt._tcp local 1883 "broker=$HOSTNAME.local"