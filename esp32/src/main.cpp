#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ctime>
#include <time.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600 * -5, 60000);  // Ajusta la zona horaria según tu ubicación

// Configuración WiFi
const char* ssid = "Axcel";
const char* password = "12345678";


// Define los pines y las constantes
#define LED_PIN 33
#define NUM_LEDS 30
#define JOYSTICK_X_PIN 34
#define JOYSTICK_Y_PIN 35
#define JOYSTICK_BUTTON_PIN 32
#define LCD_ADDRESS 0x27
#define LCD_COLUMNS 20
#define LCD_ROWS 4
#define SS_PIN 5
#define RST_PIN 27
#define API_KEY "nW8NMMyQJUrlNQnqfA8GV2WQFwCIeyuMmsBG4Np6"
#define DATABASE_URL "proyectose-8d6fd-default-rtdb.firebaseio.com" 


MFRC522 rfid(SS_PIN, RST_PIN);
FirebaseData firebaseData;

byte uidTarjeta[10];
int uidLength = 0;
bool uidProcesado = false;
bool tarjeta_encontrada = false;
int menuState = 0;
int seleccionMenu = 1;

bool esPrimeraVezEnMenuJuego = true;
String nombreUsuario = "";
float saldoActual = 0.0;
float valorApostado = 0.0;
int opcionJuego = 1;

bool enPosicionNeutra = true;
bool pantallaInicioMostrada = false;
int ultimaPosicionAleatoria = 0;

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
int intentos = 0;
int ledsSeleccionados[4] = { -1, -1, -1, -1 };
int numeroSelecciones = 0;
int aciertos = 0;
float premio = 0.0;
int ledActual = 0;  // LED actualmente resaltado por el joystick

LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);
const int UMBRAL_SUPERIOR = 3500;          // Valor para moverse hacia abajo
const int UMBRAL_INFERIOR = 500;           // Valor para moverse hacia arriba
const int UMBRAL_NEUTRAL_INFERIOR = 2500;  // Límite inferior del rango neutral
const int UMBRAL_NEUTRAL_SUPERIOR = 2800;  // Límite superior del rango neutral

FirebaseAuth auth;
FirebaseConfig config;

void setup() {
  Serial.begin(115200);
  strip.begin();
  strip.show();
  SPI.begin();
  rfid.PCD_Init();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback; 

  Firebase.reconnectNetwork(true);
  Firebase.begin(&config, &auth);

  lcd.init();
  lcd.backlight();

  pinMode(JOYSTICK_X_PIN, INPUT);
  pinMode(JOYSTICK_Y_PIN, INPUT);
  pinMode(JOYSTICK_BUTTON_PIN, INPUT_PULLUP);

  mostrarBienvenida();
  configTime();
}

void mostrarBienvenida() {
  lcd.clear();
  lcd.setCursor(0, 0);  // Coloca el cursor en la primera fila, primera columna
  lcd.print("Bienvenido a Frutilandia");
  actualizarMenu();
}

void actualizarMenu() {
  lcd.setCursor(0, 2);
  lcd.print(seleccionMenu == 1 ? "> Jugar" : "  Jugar");
  lcd.setCursor(0, 3);
  lcd.print(seleccionMenu == 2 ? "> Ver Saldo" : "  Ver Saldo");
}

void lecturaTarjeta() {
  if (rfid.PICC_IsNewCardPresent() && !uidProcesado) {
    if (rfid.PICC_ReadCardSerial()) {
      MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);

      // Imprimir UID en el Monitor Serial en formato hexadecimal
      Serial.print("UID:");
      uidLength = rfid.uid.size;  // Guardar la longitud del UID
      for (int i = 0; i < rfid.uid.size; i++) {
        uidTarjeta[i] = rfid.uid.uidByte[i];  // Guardar cada byte del UID
      }
      uidProcesado = true;
      verificarUIDEnFirebase();
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
    }
  }
}

String printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return "";
  }
  char buffer[40];
  strftime(buffer, sizeof(buffer), "%Y%m%d-%H%M%S", &timeinfo);
  Serial.println(buffer);
  return String(buffer);
}

void configTime() {
  configTime(-5 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  time_t nowSecs = time(nullptr);

  while (nowSecs < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    nowSecs = time(nullptr);
  }
  Serial.println("");
}



void verificarUIDEnFirebase() {
  String uidString = "";
  if (Firebase.getString(firebaseData, ("/card_user/%s", uidString))) {
    if (firebaseData.dataType() == "string") {
      nombreUsuario = firebaseData.stringData();
      Serial.print("UID encontrado: ");
      Serial.println(nombreUsuario);
    } else {
      Serial.println("UID no asociado a un nombre.");
    }
  }
}

void actualizarSaldoUsuario(String nombreUsuario) {
  if (Firebase.getFloat(firebaseData, ("/usuarios/%s/saldo", nombreUsuario.c_str()))) {
    saldoActual = firebaseData.floatData();
    saldoActual -= 0.5;
    if (saldoActual > 0) {
      valorApostado += 0.5;
      if (Firebase.setFloat(firebaseData, ("/usuarios/%s/saldo", nombreUsuario.c_str()), saldoActual)) {
        Serial.print("Saldo actualizado. Nuevo saldo: ");
        Serial.println(saldoActual);
      }
    } else {
      Serial.println("Saldo insuficiente para realizar la apuesta.");
    }
  }
  uidLength = 0;
  memset(uidTarjeta, 0, sizeof(uidTarjeta));
  uidProcesado = false;
}



void loop() {
  int valorYJoystick = analogRead(JOYSTICK_Y_PIN);

  // Manejo de la navegación en el menú principal
  if (menuState == 0) {
    if (valorYJoystick < UMBRAL_INFERIOR && enPosicionNeutra) {
      seleccionMenu = max(1, seleccionMenu - 1);
      actualizarMenu();
      enPosicionNeutra = false;
    } else if (valorYJoystick > UMBRAL_SUPERIOR && enPosicionNeutra) {
      seleccionMenu = min(2, seleccionMenu + 1);
      actualizarMenu();
      enPosicionNeutra = false;
    } else if (valorYJoystick >= UMBRAL_NEUTRAL_INFERIOR && valorYJoystick <= UMBRAL_NEUTRAL_SUPERIOR && !enPosicionNeutra) {
      enPosicionNeutra = true;
    }

    if (digitalRead(JOYSTICK_BUTTON_PIN) == LOW && enPosicionNeutra) {
      if (seleccionMenu == 1) {
        menuState = 1;  // Cambia al menú de juego
        mostrarMenuJuego();
      } else if (seleccionMenu == 2) {
        menuState = 5;
        mostrarPantallaSaldo();
      }
      delay(200);  // Debounce para el botón
    }
  }

  else if (menuState == 5) {
    lecturaTarjeta();
    Serial.println(uidProcesado);
    if (uidProcesado && nombreUsuario != "") {
      if (Firebase.getFloat(firebaseData, ("/usuarios/%s/saldo", nombreUsuario.c_str()))) {
        saldoActual = firebaseData.floatData();
      }

      Serial.println(saldoActual);
      lcd.setCursor(0, 0);
      lcd.print("Usuario encontrado");

      lcd.setCursor(0, 1);
      lcd.print("Usuario: ");
      
      lcd.setCursor(9, 1);
      lcd.print(nombreUsuario.c_str());

      lcd.setCursor(0, 2);
      lcd.print("Saldo: $");

      lcd.setCursor(7, 2);
      lcd.print(saldoActual);

      lcd.setCursor(0, 3);
      lcd.print("> Regresar");

      uidProcesado = false;
    }

    if (digitalRead(JOYSTICK_BUTTON_PIN) == LOW) {
      mostrarBienvenida();
      menuState = 0;
      nombreUsuario = "";
      saldoActual = 0.0;
      uidLength = 0;
      memset(uidTarjeta, 0, sizeof(uidTarjeta));
      tarjeta_encontrada = false;
      delay(200);
    }
  } else if (menuState == 1) {
    lecturaTarjeta();
    if (uidProcesado && nombreUsuario != "") {
      actualizarSaldoUsuario(nombreUsuario);
    }
    mostrarMenuJuego();
    if (valorYJoystick < UMBRAL_INFERIOR && enPosicionNeutra) {
      opcionJuego = max(1, opcionJuego - 1);
      mostrarMenuJuego();
      enPosicionNeutra = false;
    } else if (valorYJoystick > UMBRAL_SUPERIOR && enPosicionNeutra) {
      opcionJuego = min(2, opcionJuego + 1);
      mostrarMenuJuego();
      enPosicionNeutra = false;
    } else if (valorYJoystick >= UMBRAL_NEUTRAL_INFERIOR && valorYJoystick <= UMBRAL_NEUTRAL_SUPERIOR && !enPosicionNeutra) {
      enPosicionNeutra = true;
    }

    if (digitalRead(JOYSTICK_BUTTON_PIN) == LOW && enPosicionNeutra) {
      if (opcionJuego == 1 && valorApostado > 0) {
        menuState = 2;
        intentos = valorApostado * 2 * 3;
        mostrarMenuJuego();
      } else if (opcionJuego == 2) {
        menuState = 0;
        esPrimeraVezEnMenuJuego = true;
        mostrarBienvenida();
      }
      delay(200);
    }
  }
  if (menuState == 2) {  // Si estamos en "Empezar"
    if (numeroSelecciones == 0) {
      mostrarPantallaInicioJuego();
    }
    if (numeroSelecciones < 4) {
      seleccionarLEDs();  // Llama a la función para seleccionar LEDs
    } else {
      ledSeleccionados();
      ejecutarAnimacionYVerificarAciertos();  // Procede con la animación y verificación
    }
  }
  delay(100);
}

void mostrarPantallaSaldo() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Buscando...");

  lcd.setCursor(0, 1);
  lcd.print("Usuario: N/A");

  lcd.setCursor(0, 2);
  lcd.print("Saldo: $0.00");

  lcd.setCursor(0, 3);
  lcd.print("> Regresar");
}


void mostrarPantallaInicioJuego() {
  if (!pantallaInicioMostrada) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Aciertos: ");

    lcd.setCursor(10, 0);
    lcd.print(String(aciertos));

    lcd.setCursor(0, 1);
    lcd.print("Intentos: ");

    lcd.setCursor(10, 1);
    lcd.print(String(intentos));

    lcd.setCursor(0, 2);
    lcd.print("Premio: $");

    lcd.setCursor(8, 2);
    lcd.print(String(premio, 2));

    lcd.setCursor(0, 3);
    lcd.print("Elige 4 frutas");
    pantallaInicioMostrada = true;
  }
}

void actualizarSaldoYRegistrarMovimiento(String nombreUsuario, float nuevoSaldo) {
  bool exito = false;
  while (!exito) {
    if (Firebase.setFloat(firebaseData, ("/usuarios/%s/saldo", nombreUsuario.c_str()), nuevoSaldo)) {
      Serial.println("Saldo actualizado con éxito.");
      exito = true;
    } else {
      delay(1000);
    }
  }

  String timestamp = printLocalTime();
  FirebaseJson movimiento;
  movimiento.set("apuesta", valorApostado);
  movimiento.set("ganancia", premio);
  movimiento.set("tipo", "ruleta");

  bool exito = false;
  while (!exito) {
    if (Firebase.setJSON(firebaseData, ("/usuarios/%s/movimientos/%s_ruleta", nombreUsuario.c_str(), timestamp.c_str()), movimiento)) {
      Serial.println("Movimiento registrado con éxito.");
      exito = true;
    } else {
      delay(1000);
    }
  }
}

void seleccionarLEDs() {
  int valorYJoystick = analogRead(JOYSTICK_Y_PIN);

  // Navegación del LED actual con el joystick
  if (valorYJoystick < UMBRAL_INFERIOR) {
    ledActual = (ledActual > 0) ? ledActual - 1 : NUM_LEDS - 1;
    mostrarSeleccionLEDs();
    delay(150);  // Retardo para la navegación
  } else if (valorYJoystick > UMBRAL_SUPERIOR) {
    ledActual = (ledActual < NUM_LEDS - 1) ? ledActual + 1 : 0;
    mostrarSeleccionLEDs();
    delay(150);  // Retardo para la navegación
  }

  // Seleccionar LED con el botón del joystick
  if (digitalRead(JOYSTICK_BUTTON_PIN) == LOW) {
    if (numeroSelecciones < 4 && !estaLEDSeleccionado(ledActual)) {
      ledsSeleccionados[numeroSelecciones++] = ledActual;
      mostrarSeleccionLEDs();
    }
    delay(200);  // Debounce para el botón
  }

  if (numeroSelecciones >= 4) {
    // Cambia el mensaje una vez que el usuario ha terminado de seleccionar
    lcd.setCursor(0, 3);
    lcd.print("Jugando...       ");  // Espacios para borrar caracteres previos
  }
}

void mostrarMenuJuego() {
  if (esPrimeraVezEnMenuJuego) {
    lcd.clear();
    esPrimeraVezEnMenuJuego = false;
  }

  lcd.setCursor(0, 0);
  lcd.print("Saldo: ");

  lcd.setCursor(7, 0);
  lcd.print(String(saldoActual));

  lcd.setCursor(0, 1);
  lcd.print("Apuesta: ");

  lcd.setCursor(9, 1);
  lcd.print(String(valorApostado));

  lcd.setCursor(0, 2);
  lcd.print(opcionJuego == 1 ? "> Empezar" : "  Empezar");

  lcd.setCursor(0, 3);
  lcd.print(opcionJuego == 2 ? "> Regresar" : "  Regresar");
}

void mostrarSeleccionLEDs() {
  strip.clear();  // Apaga todos los LEDs

  // Ilumina los LEDs seleccionados
  for (int i = 0; i < 4; i++) {
    if (ledsSeleccionados[i] != -1) {
      strip.setPixelColor(ledsSeleccionados[i], strip.Color(0, 0, 255));  // Verde para seleccionados
    }
  }

  // Resalta el LED actual
  if (numeroSelecciones < 4) {
    strip.setPixelColor(ledActual, strip.Color(255, 165, 0));  // Naranja para el LED actual
  }

  lcd.setCursor(0, 3);
  lcd.print("Seleccionando... ");

  strip.show();
}

void ledSeleccionados() {
  Serial.println("Led seleccionados: ");
  for (int i = 0; i < 4; i++) {
    Serial.print(ledsSeleccionados[i]);
    Serial.print("  ");
  }
}

bool estaLEDSeleccionado(int led) {
  for (int i = 0; i < 4; i++) {

    if (ledsSeleccionados[i] == led) {
      return true;
    }
  }
  return false;
}

void ejecutarAnimacionYVerificarAciertos() {

  for (int ronda = 0; ronda < 4; ronda++) {
    int ledAleatorio = random(NUM_LEDS);  // Genera un LED aleatorio
    int pasosTotales = 30 + ledAleatorio - ultimaPosicionAleatoria;
    int velocidadInicial = 50;    // Velocidad inicial en milisegundos
    int incrementoVelocidad = 5;  // Cantidad en la que aumentará el delay en cada paso

    for (int paso = 1; paso < pasosTotales + 1; paso++) {
      int posicionActual = (ultimaPosicionAleatoria + paso) % NUM_LEDS;

      if (estaLEDSeleccionado(posicionActual)) {
        strip.setPixelColor(posicionActual, strip.Color(255, 0, 0));  // Cambiar a rojo, por ejemplo
      } else {
        strip.setPixelColor(posicionActual, strip.Color(255, 165, 0));  // LED naranja para la animación
      }

      strip.show();
      delay(100);  // Ajusta este delay para cambiar la velocidad de la animación

      // Restaura el color original si el LED actual está seleccionado
      if (estaLEDSeleccionado(posicionActual)) {
        strip.setPixelColor(posicionActual, strip.Color(0, 0, 255));  // Vuelve a azul
      } else {
        strip.setPixelColor(posicionActual, strip.Color(0, 0, 0));  // Apaga el LED
      }

      // Ajusta la velocidad para los últimos pasos hacia el LED aleatorio
      if (paso >= 20) {
        velocidadInicial = min(velocidadInicial + incrementoVelocidad, 500);  // Limita la velocidad máxima
      }
    }
    ultimaPosicionAleatoria = ledAleatorio;  // Actualizar la última posición aleatoria
    Serial.print("Led aleatorio: ");
    Serial.println(ledAleatorio);
    // Verifica si el LED aleatorio coincide con alguna selección
    if (estaLEDSeleccionado(ledAleatorio)) {
      aciertos++;
      lcd.setCursor(0, 0);
      lcd.print("Aciertos: ");

      lcd.setCursor(10, 0);
      lcd.print(String(aciertos));
    }
    delay(1000);  // Un breve retraso antes de la siguiente ronda de animación
  }
  actualizarPremioYIntentos();
  if (intentos <= 0) {
    menuState = 1;  // Regresar al menú principal
    esPrimeraVezEnMenuJuego = true;
  } else {
    prepararSiguienteRonda();
  }
}

int generarLEDaleatorioYAnimacion() {
  int ledAleatorio = random(NUM_LEDS);
  // Código para animación con variación de velocidad.
  // Asegúrate de resaltar ledAleatorio al final de la animación.
  return ledAleatorio;
}

void actualizarPremioYIntentos() {
  Serial.print("Aciertos antes de actualizar premio: ");
  Serial.println(aciertos);

  if (aciertos == 4) {
    premio += 2.0;
  } else if (aciertos == 3) {
    premio += 1.5;
  } else if (aciertos == 1 || aciertos == 2) {
    premio += 0.5;
  } else {
    intentos--;
    Serial.println("No se acertó ninguna posición, restando intento.");
  }

  if (intentos <= 0) {
    // Si no quedan más intentos, apaga todas las luces y regresa al menú principal
    strip.clear();  // Apaga todas las luces LED
    strip.show();

    menuState = 1;  // Cambia al menú principal
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Fin del juego");
    lcd.setCursor(0, 1);
    lcd.print("Vuelve a intentarlo!");
    actualizarSaldoYRegistrarMovimiento(nombreUsuario, saldoActual);

    saldoActual = 0.0;
    valorApostado = 0.0;
    premio = 0.0;
    intentos = 0;
    esPrimeraVezEnMenuJuego = true;

    numeroSelecciones = 0;
    aciertos = 0;
    ledActual = 0;  // LED actualmente resaltado por el joystick

    for (int i = 0; i < 4; i++) {
      ledsSeleccionados[i] = -1;
    }

    delay(1000);
  }

  Serial.print("Premio actualizado: $");
  Serial.println(premio, 2);
  aciertos = 0;  // Reinicia los aciertos para la próxima ronda

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Aciertos: ");

  lcd.setCursor(10, 0);
  lcd.print(String(aciertos));

  lcd.setCursor(0, 1);
  lcd.print("Intentos: ");

  lcd.setCursor(10, 1);
  lcd.print(String(intentos));

  lcd.setCursor(0, 2);
  lcd.print("Premio: $");

  lcd.setCursor(8, 2);
  lcd.print(String(premio, 2));

  lcd.setCursor(0, 3);
  lcd.print(aciertos > 0 ? "¡Ganaste esta vez!" : "Intenta de nuevo");

  delay(1500);
}

void prepararSiguienteRonda() {
  numeroSelecciones = 0;
  for (int i = 0; i < 4; i++) ledsSeleccionados[i] = -1;
  pantallaInicioMostrada = false;
  strip.clear();
  strip.show();
}

void setup();
void loop();

int main() {
    init();             // Inicializar componentes del ESP32
    setup();
    
    while (true) {
        loop();
    }
    
    return 0;  // Nunca se alcanza
}