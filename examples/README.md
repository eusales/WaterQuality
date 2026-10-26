# Exemplos e testes isolados

Estes sketches Arduino são independentes do [firmware principal PlatformIO](../monitor_agua/). Compilar `monitor_agua/` não compila os exemplos desta pasta.

| Exemplo | Objetivo |
| --- | --- |
| [teste_ihm](teste_ihm/teste_ihm.ino) | Explorar a interface com valores simulados |
| [teste_displaytft](teste_displaytft/teste_displaytft.ino) | Testar o display TFT isoladamente |
| [teste_display_microsd](teste_display_microsd/teste_display_microsd.ino) | Testar a integração entre display e microSD |
| [legacy/teste_rfid](legacy/teste_rfid/teste_rfid.ino) | Exemplo legado de leitor RFID MFRC522; anteriormente nomeado como teste de turbidez |

Para usar um exemplo na Arduino IDE, abra o arquivo `.ino` na pasta correspondente. Confira os `#include`, as bibliotecas, a placa e os GPIOs do próprio sketch antes de compilar e gravar. A configuração de um teste pode diferir da configuração do firmware principal.

Cada sketch está em uma pasta com o mesmo nome do arquivo `.ino`. A pasta duplicada do teste de display e microSD foi eliminada nesta organização, preservando o conteúdo do código.

O [roteiro de teste da IHM](../docs/ihm/teste_ihm.md) descreve os passos de navegação e os componentes usados nesse teste. Estes exemplos de bancada não constituem uma suíte automatizada do PlatformIO.

O exemplo RFID foi mantido para preservar o material original. Ele não participa do monitor de água e usa sua própria pinagem e monitor serial a 9600 baud. Não copie essa pinagem para o ESP32 sem revisão: o sketch define GPIOs 9 e 10, usados pela flash no ESP32 WROOM-32. Não foi identificado um teste isolado de turbidez no arquivo antigo.
