#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

// ===================== CONFIGURAÇÕES DE REDE =====================
const char* WIFI_SSID  = "Convidado iot";      // ALTERE AQUI
const char* WIFI_SENHA = "mescla@1417";     // ALTERE AQUI
const char* IP_PROXIMO = "192.168.1.178";     // ALTERE AQUI (IP do próximo aluno)
// =================================================================
// ALTERADO VIA AWS - ATIVIDADE M2M //
const unsigned long ESPERA_MS = 2000;  // 2 segundos antes de repassar

ESP8266WebServer server(80);

bool envioPendente = false;     // há um valor aguardando para ser enviado?
long valorParaEnviar = 0;
unsigned long momentoRecebido = 0;

// Rota /relay?valor=X
void handleRelay() {
  if (!server.hasArg("valor")) {
    server.send(400, "text/plain", "Erro: parametro 'valor' ausente");
    return;
  }

  long valorRecebido = server.arg("valor").toInt();
  Serial.print("Valor recebido: ");
  Serial.println(valorRecebido);

  valorParaEnviar = valorRecebido + 1;
  momentoRecebido = millis();
  envioPendente = true;

  // Responde na hora, sem travar o servidor
  server.send(200, "text/plain",
              "Recebido: " + String(valorRecebido) +
              " | Vou enviar: " + String(valorParaEnviar));
}

void enviarParaProximo() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi desconectado, envio cancelado.");
    return;
  }

  WiFiClient client;
  HTTPClient http;

  String url = "http://" + String(IP_PROXIMO) + "/relay?valor=" + String(valorParaEnviar);
  Serial.print("Enviando para: ");
  Serial.println(url);

  if (http.begin(client, url)) {
    http.setTimeout(3000);
    int codigo = http.GET();
    if (codigo > 0) {
      Serial.print("Resposta do proximo: ");
      Serial.println(codigo);
    } else {
      Serial.print("Falha no envio: ");
      Serial.println(http.errorToString(codigo));
    }
    http.end();
  } else {
    Serial.println("Nao foi possivel iniciar a conexao HTTP.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_SENHA);
  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Conectado! Meu IP: ");
  Serial.println(WiFi.localIP());

  server.on("/relay", handleRelay);
  server.begin();
  Serial.println("Servidor iniciado.");
}

void loop() {
  server.handleClient();

  // Controle de tempo não-bloqueante (sem delay)
  if (envioPendente && millis() - momentoRecebido >= ESPERA_MS) {
    envioPendente = false;
    enviarParaProximo();
  }
}
