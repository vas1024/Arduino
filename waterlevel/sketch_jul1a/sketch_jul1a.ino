void setup() {
  Serial.begin(115200); 
}

void loop() {
  int waterLevel = analogRead(A0); // Чтение данных с аналогового пина A0
  int min = 300;
  int max = 550;
  int tmp = constrain(waterLevel, min, max); 
  int percentage = map(tmp, min, max, 0, 100);
  Serial.print("Уровень воды: ");
  Serial.print(waterLevel); 
  Serial.print("     ");
  Serial.print(percentage); 
  Serial.println(" ");
  delay(500); // Задержка полсекунды
}


