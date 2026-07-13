// Inclui as bibliotecas necessárias para o RFID funcionar
#include <SPI.h>
#include <MFRC522.h>

// Define em quais pinos ligamos o SDA(SS) e o RST
#define PINO_SS 10
#define PINO_RST 9

// Cria o "objeto" do leitor RFID para podermos dar comandos a ele
MFRC522 leitorRfid(PINO_SS, PINO_RST); 

void setup() {
  // Inicia o monitor serial (para vermos o texto no computador)
  Serial.begin(9600);
  
  // Inicia a comunicação SPI (a forma como o Arduino e o módulo conversam)
  SPI.begin();
  
  // Inicia o leitor RFID
  leitorRfid.PCD_Init();
  
  // Manda uma mensagem de boas-vindas para a tela
  Serial.println("Aproxime o seu cartao ou chaveiro do leitor...");
  Serial.println();
}

void loop() {
  // Passo 1: Verifica se existe algum cartão novo perto do leitor
  if (!leitorRfid.PICC_IsNewCardPresent()) {
    return; // Se não tiver cartão, ele para aqui e recomeça o loop
  }

  // Passo 2: Verifica se consegue ler as informações desse cartão
  if (!leitorRfid.PICC_ReadCardSerial()) {
    return; // Se não conseguir ler, ele para e recomeça
  }

  // Se chegou até aqui, é porque encontrou e leu um cartão!
  Serial.print("Cartão detectado! O UID (RG do cartao) é: ");
  
  // O UID é formado por vários pedacinhos (bytes). 
  // Esse bloco 'for' junta todos os pedacinhos para mostrar na tela.
  for (byte i = 0; i < leitorRfid.uid.size; i++) {
    Serial.print(leitorRfid.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(leitorRfid.uid.uidByte[i], HEX); // Mostra no formato Hexadecimal
  }
  
  Serial.println(); // Pula uma linha no texto
  Serial.println("-----------------------------------");
  
  // Manda o cartão "dormir" para não ler o mesmo cartão várias vezes sem parar
  leitorRfid.PICC_HaltA();
}