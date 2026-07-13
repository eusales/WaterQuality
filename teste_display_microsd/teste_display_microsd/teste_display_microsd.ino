// ============================================================
// TESTE — Micro SD Card + TFT ST7789 (Adafruit)
// ESP32 DevKit V3
// SPI compartilhado: SCK=18, MOSI=23, MISO=19
// TFT: CS=5, DC=2, RST=4
// SD:  CS=15
// Orientação: Landscape (320x240)
// ============================================================

#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// ── Pinos TFT ────────────────────────────────────────────────
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4

// ── Pino CS do SD ────────────────────────────────────────────
#define SD_CS    15

// ── Nome do arquivo de teste ─────────────────────────────────
#define FILENAME "/teste_sd.txt"
#define CONTEUDO "Monitor de Qualidade da Agua - ESP32\nTeste de escrita e leitura SD\nLinha 3 de teste\n"

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// ── Helpers de display ───────────────────────────────────────
int cursorY = 0;

void tftPrint(String msg, uint16_t color = ST77XX_WHITE, uint8_t size = 1) {
  tft.setTextColor(color);
  tft.setTextSize(size);
  tft.setCursor(6, cursorY);
  tft.println(msg);
  cursorY += size * 10 + 4;
  Serial.println(msg);
}

void tftHeader(String titulo) {
  tft.fillScreen(ST77XX_BLACK);
  cursorY = 0;
  tft.fillRect(0, 0, 320, 20, 0x1A5F);  // azul escuro
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(6, 5);
  tft.print(titulo);
  cursorY = 26;
  Serial.println("=== " + titulo + " ===");
}

void tftStatus(String label, String valor, bool ok) {
  tft.setCursor(6, cursorY);
  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(1);
  tft.print(label + ": ");
  tft.setTextColor(ok ? ST77XX_GREEN : ST77XX_RED);
  tft.println(valor);
  cursorY += 14;
  Serial.println(label + ": " + valor);
}

// ── Setup ────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);

  // Inicializa TFT
  tft.init(240, 320);
  tft.setRotation(3);  // landscape
  tft.fillScreen(ST77XX_BLACK);

  tftHeader("Teste SD Card — ESP32");
  tftPrint("Iniciando...", ST77XX_YELLOW);
  delay(800);

  // ── ETAPA 1: Inicializar SD ──────────────────────────────
  tftHeader("Etapa 1: Inicializacao SD");

  if (!SD.begin(SD_CS)) {
    tftStatus("SD Card", "FALHA", false);
    tftPrint("Verifique:", ST77XX_YELLOW);
    tftPrint("- Cartao inserido?", ST77XX_WHITE);
    tftPrint("- Formato FAT32?", ST77XX_WHITE);
    tftPrint("- CS no GPIO 15?", ST77XX_WHITE);
    while (1) delay(1000);
  }

  tftStatus("SD Card", "OK", true);

  uint8_t cardType = SD.cardType();
  String tipo = "Desconhecido";
  if      (cardType == CARD_MMC)  tipo = "MMC";
  else if (cardType == CARD_SD)   tipo = "SD";
  else if (cardType == CARD_SDHC) tipo = "SDHC";

  tftStatus("Tipo", tipo, true);

  uint64_t totalMB = SD.totalBytes() / (1024 * 1024);
  uint64_t usedMB  = SD.usedBytes()  / (1024 * 1024);
  tftStatus("Total", String(totalMB) + " MB", true);
  tftStatus("Usado", String(usedMB)  + " MB", true);
  delay(2000);

  // ── ETAPA 2: Escrever arquivo ────────────────────────────
  tftHeader("Etapa 2: Escrita");

  // Remove arquivo anterior se existir
  if (SD.exists(FILENAME)) {
    SD.remove(FILENAME);
    tftPrint("Arquivo anterior removido.", ST77XX_YELLOW);
  }

  File arq = SD.open(FILENAME, FILE_WRITE);
  if (!arq) {
    tftStatus("Abrir arquivo", "FALHA", false);
    while (1) delay(1000);
  }

  size_t bytes = arq.print(CONTEUDO);
  arq.close();

  tftStatus("Arquivo", FILENAME, true);
  tftStatus("Bytes gravados", String(bytes) + " B", bytes > 0);
  delay(2000);

  // ── ETAPA 3: Ler arquivo de volta ────────────────────────
  tftHeader("Etapa 3: Leitura");

  File arqLer = SD.open(FILENAME, FILE_READ);
  if (!arqLer) {
    tftStatus("Abrir para leitura", "FALHA", false);
    while (1) delay(1000);
  }

  tftStatus("Tamanho", String(arqLer.size()) + " B", true);
  tftPrint("Conteudo:", ST77XX_CYAN);

  // Lê e exibe linha a linha
  int linhas = 0;
  while (arqLer.available()) {
    String linha = arqLer.readStringUntil('\n');
    linha.trim();
    if (linha.length() > 0) {
      tftPrint(linha, ST77XX_WHITE);
      linhas++;
    }
  }
  arqLer.close();

  tftStatus("Linhas lidas", String(linhas), linhas > 0);
  delay(2000);

  // ── ETAPA 4: Verificação de integridade ─────────────────
  tftHeader("Etapa 4: Verificacao");

  File arqVerif = SD.open(FILENAME, FILE_READ);
  String conteudoLido = "";
  while (arqVerif.available()) {
    conteudoLido += (char)arqVerif.read();
  }
  arqVerif.close();

  bool integro = (conteudoLido == String(CONTEUDO));
  tftStatus("Integridade", integro ? "APROVADO" : "FALHOU", integro);

  if (integro) {
    tftPrint("Conteudo identico ao", ST77XX_GREEN);
    tftPrint("original. SD OK.", ST77XX_GREEN);
  } else {
    tftPrint("Divergencia detectada!", ST77XX_RED);
    tftPrint("Verifique o cartao.", ST77XX_RED);
  }

  delay(2000);

  // ── RESULTADO FINAL ──────────────────────────────────────
  tftHeader("Resultado Final");

  tft.setTextSize(2);
  if (integro) {
    tft.setTextColor(ST77XX_GREEN);
    tft.setCursor(30, 60);
    tft.println("SD CARD OK!");
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(6, 100);
    tft.println("Escrita e leitura validadas.");
    tft.setCursor(6, 116);
    tft.println("Pronto para datalogger.");
  } else {
    tft.setTextColor(ST77XX_RED);
    tft.setCursor(30, 60);
    tft.println("SD COM FALHA");
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(6, 100);
    tft.println("Verifique cartao e conexoes.");
  }

  Serial.println("Teste concluido.");
}

void loop() {
  // Nada no loop — teste completo no setup
}