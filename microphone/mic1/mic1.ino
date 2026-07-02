#include <driver/i2s.h>

// Привязка к физическим GPIO вашей платы:
#define I2S_WS      25  // На плате подписан как D3
#define I2S_SCK     26  // На плате подписан как D2
#define I2S_SD      33  // На плате подписан как D4

// Выбираем первый порт I2S на чипе
#define I2S_PORT    I2S_NUM_0 

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }
  Serial.println("--- Инициализация микрофона ICS-43434 ---");

  // Настройка конфигурации шины I2S
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX), // ESP32 - мастер, принимает данные
    .sample_rate = 16000,                               // Частота дискретизации 16 кГц
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,       // Микрофон шлет 24 бита в пакете по 32 бита
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,        // Читаем только Левый канал (SEL на GND)
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,  // Стандартный режим Philips (I2S)
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,           // Приоритет прерывания
    .dma_buf_count = 8,                                 // Количество буферов DMA
    .dma_buf_len = 64,                                  // Размер каждого буфера
    .use_apll = false
  };

  // Настройка пинов
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE, // Выход звука нам не нужен
    .data_in_num = I2S_SD
  };

  // Установка драйвера
  if (i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL) != ESP_OK) {
    Serial.println("Ошибка: не удалось установить драйвер I2S!");
    while (1);
  }

  // Применяем пины к порту
  if (i2s_set_pin(I2S_PORT, &pin_config) != ESP_OK) {
    Serial.println("Ошибка: не удалось назначить пины I2S!");
    while (1);
  }

  Serial.println("Драйвер успешно запущен!");
}

void loop() {
  int32_t sample = 0;
  size_t bytes_read = 0;

  // Читаем данные из шины I2S
  // Ждем заполнения буфера максимум 100 миллисекунд
  esp_err_t result = i2s_read(I2S_PORT, &sample, sizeof(sample), &bytes_read, 100 / portTICK_PERIOD_MS);
  
  if (result == ESP_OK && bytes_read > 0) {
    if (sample != 0) {
      // Очищаем звук от лишних нулей (микрофон выдает 24 бита внутри 32-битного контейнера)
      int32_t clean_sample = sample >> 8; 
      
      // Выводим данные для графика в Плоттер
      Serial.println(clean_sample);
    }
  }
}
