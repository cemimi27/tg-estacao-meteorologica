#include <WiFi.h>
#include "DHT.h"
#include <Adafruit_BMP280.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <Wire.h>

// --- Sensores ---
#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define LDRPIN 32

#define BMPSCLPIN 22
#define BMPSDAPIN 21
Adafruit_BMP280 bmp;

// --- Configuração InfluxDB --
#define INFLUXDB_URL "http://192.168.1.79:8086"
#define INFLUXDB_TOKEN "iWOOB8NOp1LgEnxeHN_YfKCPB2CiIsFLbFY9z0Cba8GKLiXlJd_xEz9fvjmWD3SfsTHRn4AKZCeNydYoNTrG4g=="
#define INFLUXDB_ORG "3f81323e7a2dfc7a"
#define INFLUXDB_BUCKET "Teste"

InfluxDBClient influxClient(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
Point weather("weather");

// --- Configuração Wi-Fi ---
const char* SSID = "Dudu-2.4G";
const char* PASSWORD = "27762315";

void initializeConnection(const char* ssid, const char* password) {
  WiFi.begin(ssid, password);

  Serial.print(F("🌐 Conectando à rede Wi-Fi"));

  unsigned long startAttemptTime = millis();
  const unsigned long timeout = 15000;

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout) {
    Serial.print(".");
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print(F("✅ Conectado ao Wi-Fi: "));
    Serial.println(ssid);
  } else {
    Serial.println();
    Serial.println(F("❌ Falha na conexão Wi-Fi."));
    Serial.println(F("🔄 Tentando novamente..."));
    delay(2000);
    initializeConnection(ssid, password);
  }
}

void initializeInfluxDB() {
  unsigned long startAttemptTime = millis();
  const unsigned long timeout = 15000;  // 15 segundos para conectar

  while (!influxClient.validateConnection() && millis() - startAttemptTime < timeout) {
    Serial.println(F("Tentando abrir conexão com InfluxDB..."));
    delay(1000);
  }

  if (influxClient.validateConnection()) {
    Serial.println(F("📊 Conexão aberta com InfluxDB."));
    Serial.print(F("Servidor: "));
    Serial.println(influxClient.getServerUrl());
  } else {
    Serial.println(F("❌ Falha ao conectar com o InfluxDB."));
  }
}

float paToHpa(float pa) {
  float hpa = pa / 100.0;

  Serial.print(F("Hectopascal: "));
  Serial.println(hpa);

  return hpa;
}

float paToAtm(float pa) {
  float atm = pa / 101325.0;

  Serial.print(F("Atmosfera: "));
  Serial.println(atm);

  return atm;
}

float paToMbar(float pa) {
  float mbar = pa / 100.0;

  Serial.print(F("Milibar: "));
  Serial.println(mbar);

  return mbar;
}

char* getLuminosity(float analogValue) {
  static char luminosity[50];

  float tensao = analogValue * (3.3 / 4095.0);

  if (tensao <= 0.01) {
    strcpy(luminosity, "Noite sem lua");
    Serial.println(F("🌑 Noite sem lua"));
    return luminosity;
  }

  float resistor = (3.3 - tensao) * 10000.0 / tensao;
  float lux = 500 / (resistor / 1000.0);

  // Classificação com descrições mais naturais
  if (lux < 0.1) {
    strcpy(luminosity, "Noite sem lua");
    Serial.println(F("🌑 Noite sem lua"));
  } else if (lux >= 0.1 && lux < 1) {
    strcpy(luminosity, "Lua cheia");
    Serial.println(F("🌕 Lua cheia"));
  } else if (lux >= 1 && lux < 100) {
    strcpy(luminosity, "Ambiente interno");
    Serial.println(F("💡 Ambiente interno"));
  } else if (lux >= 100 && lux < 1000) {
    strcpy(luminosity, "Dia nublado");
    Serial.println(F("☁️ Dia nublado"));
  } else {
    strcpy(luminosity, "Ensolarado");
    Serial.println(F("🌞 Ensolarado"));
  }

  return luminosity;
}

void setup() {
  Serial.begin(115200);

  // Inicialização dos pinos
  pinMode(DHTPIN, INPUT);
  pinMode(LDRPIN, INPUT);

  // Inicializa I2C com pinos personalizados (opcional)
  Wire.begin(BMPSDAPIN, BMPSCLPIN);

  // Conexões
  initializeConnection(SSID, PASSWORD);
  initializeInfluxDB();
  weather.setTime(WritePrecision::S);

  // Inicializa sensores
  dht.begin();

  if (!bmp.begin(0x77)) {
    if (!bmp.begin(0x76)) {
      Serial.println(F("❌ Sensor BMP280 não encontrado! Verifique a fiação ou o endereço."));
    } else {
      Serial.println(F("📟 Sensor BMP280 encontrado no endereço 0x76."));
    }
  } else {
    Serial.println(F("📟 Sensor BMP280 encontrado no endereço 0x77."));
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    initializeConnection(SSID, PASSWORD);
  }

  if (!influxClient.validateConnection()) {
    initializeInfluxDB();
  }

  delay(1000);
  weather.clearFields();
  // Leitura do DHT
  /*float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  */

  float humidity = 10.0;     //Valor fixo pra teste
  float temperature = 10.0;  //Valor fixo pra teste

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println(F("❌ Falha ao fazer leitura do sensor DHT!"));
    return;
  }

  float hic = dht.computeHeatIndex(temperature, humidity, false);

  Serial.println(F("*** DHT22 ***"));
  Serial.print(F("💧 Umidade: "));
  Serial.print(humidity);
  Serial.print(F("% \n🌡️ Temperatura: "));
  Serial.print(temperature);
  Serial.print(F("°C\n"));
  Serial.print(F("Sensação Térmica: "));
  Serial.print(hic);
  Serial.println(F("°C\n"));

  weather.addField("humidity", humidity, 2);
  weather.addField("temperature", temperature, 2);

  //float pressure = bmp.readPressure();
  float pressure = 10;  //Valor fixo pra teste

  Serial.println(F("*** BMP280 ***"));
  Serial.print(F("Pascal: "));
  Serial.println(pressure);

  float hpa = paToHpa(pressure);
  float atm = paToAtm(pressure);
  float mbar = paToMbar(pressure);
  Serial.println();

  weather.addField("atmosphere", atm);

  Serial.println(F("*** LDR ***"));
  //int analogValue = analogRead(LDRPIN);
  int analogValue = 10;  //Valor fixo pra teste
  char* luminosity = getLuminosity(analogValue);
  Serial.println();

  weather.addField("luminosity", luminosity);

  if (influxClient.writePoint(weather)) {
    Serial.println(F("Dados enviados para o InfluxDB!"));
  } else {
    Serial.print(F("❌ Falha ao enviar os dados para o InfluxDB: "));
    Serial.println(influxClient.getLastErrorMessage());
  }

  Serial.println();
  
  delay(5000);
}
