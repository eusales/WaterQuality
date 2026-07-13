// Incluindo as ferramentas que acabamos de baixar
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

// Aqui estamos avisando ao código onde os fios estão ligados
// (Exatamente como você anotou na sua tabela!)
#define TFT_CS    5
#define TFT_DC    22
#define TFT_RST   4

// O ESP32 já sabe sozinho que o MOSI (Dados) é no 23 e o SCK (Clock) é no 18.
// Então, não precisamos repetir isso para ele.

// Criando o "objeto" do display para podermos dar comandos a ele
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  // Iniciando a comunicação com o computador (para vermos mensagens de erro, se houver)
  Serial.begin(115200);
  Serial.println("Iniciando o display...");

  // Ligando a tela! 
  // Os números 240 e 320 são a quantidade de "pontinhos" (pixels) da tela.
  // Displays de 2 polegadas costumam ter 240x320. Se a imagem ficar cortada, 
  // pode ser que o seu seja 240x240. É só trocar aqui se precisar!
  tft.init(240, 320); 

  // Pinta o fundo da tela de azul
  tft.fillScreen(ST77XX_BLUE);

  // Vamos configurar como será a nossa letra
  tft.setCursor(20, 50);          // Escolhe o lugar na tela (20 pro lado, 50 pra baixo)
  tft.setTextColor(ST77XX_WHITE); // Escolhe a cor da letra (Branco)
  tft.setTextSize(3);             // Escolhe o tamanho da letra

  // Escrevendo a mensagem
  tft.println("Ola, Mundo!");
  
  tft.setTextSize(2);             // Diminuindo a letra um pouco
  tft.setCursor(20, 100);
  tft.println("Teste do ESP32");
  tft.println("funcionando!");
}

void loop() {
  // Como é só um teste, não precisamos colocar nada para repetir aqui.
}