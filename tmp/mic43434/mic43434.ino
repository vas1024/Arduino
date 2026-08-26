#include <driver/i2s.h>

#define I2S_WS   5
#define I2S_BCK  18
#define I2S_SD   19
#define I2S_PORT I2S_NUM_0

void i2s_init() {
  i2s_config_t cfg = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate          = 44100,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT,
//    .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
    .channel_format       = I2S_CHANNEL_FMT_ONLY_RIGHT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 8,
    .dma_buf_len          = 64,
    .use_apll             = false,
  };

  i2s_pin_config_t pins = {
    .bck_io_num   = I2S_BCK,
    .ws_io_num    = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num  = I2S_SD
  };

  i2s_driver_install(I2S_PORT, &cfg, 0, NULL);
  i2s_set_pin(I2S_PORT, &pins);
  i2s_zero_dma_buffer(I2S_PORT);
}

void setup() {
  Serial.begin(115200);
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  i2s_init();
}

void loop() {
  int32_t samples[32];
  size_t bytes_read = 0;
  i2s_read(I2S_PORT, samples, sizeof(samples), &bytes_read, portMAX_DELAY);
  int count = bytes_read / sizeof(int32_t);
  for (int i = 0; i < count; i++) {
    int32_t sample = samples[i] >> 8;
    Serial.println(sample);
  }
}