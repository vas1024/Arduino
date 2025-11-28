import paho.mqtt.client as mqtt
import time;

# эта верси€ дл€ первого варианта, значени€ передавал в сообщении строковыми числами, разделенными пробелом ( можно любыми символами раздел€ть )


# Ќастройки MQTT
MQTT_BROKER = "192.168.1.101"
MQTT_PORT = 1883
MQTT_TOPIC = "test/topic"
NUM_LEDS = 180 

# √енерируем данные дл€ светодиодов (HUE значени€ 0-255)
# Ќапример: 0, 10, 20, 30... дл€ плавного градиента
numbers = [str((i * 10) % 256) for i in range( NUM_LEDS )]
message = " ".join(numbers)

client = mqtt.Client()
client.connect(MQTT_BROKER, MQTT_PORT)
client.publish(MQTT_TOPIC, message)
client.disconnect()

print("ƒанные дл€ светодиодов отправлены!")
