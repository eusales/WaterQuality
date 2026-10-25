# WaterQuality — Monitor Portátil de Qualidade da Água

Projeto acadêmico de um instrumento portátil com **ESP32**, desenvolvido para acompanhar **pH, sólidos dissolvidos totais (TDS), turbidez e temperatura**. O firmware usa Arduino/C++, display TFT, três botões físicos e cartão microSD.

**Autor:** Matheus Sales
**Disciplina:** Instrumentação Industrial II — Engenharia de Controle e Automação, UFMT
**Estágio:** MVP em desenvolvimento; as pendências conhecidas estão descritas ao final deste README.

## Visão do projeto

![Diagrama de montagem do WaterQuality com ESP32, display TFT, microSD, ADS1115, conversor de nível, sensores e botões](WaterQuality.jpg)

Diagrama de referência disponível no repositório. O arquivo editável é [WaterQuality1.fzz](WaterQuality1.fzz), para abrir no Fritzing. Existem divergências de pinagem entre os materiais e o firmware; confira a tabela abaixo antes de reproduzir a montagem.

## Como o sistema funciona

Os sensores analógicos chegam ao ADS1115, que se comunica com o ESP32 por I²C. O DS18B20 usa o barramento 1-Wire. O ESP32 converte as leituras, atualiza a interface TFT e permite registrar uma amostra no microSD pelo menu.

| Parte | Implementação presente |
| --- | --- |
| Aquisição | ADS1115 nos canais AIN0/AIN1/AIN2 e DS18B20 |
| Monitoramento | pH, TDS (ppm), turbidez (NTU) e temperatura (°C) |
| Interface | Monitoramento, menu, calibração, registro, histórico e configurações |
| Operação | Botões Navegar, Confirmar e Voltar |
| Calibração | Fluxos de pH em três pontos, TDS em um ponto e turbidez em três pontos; precisam das correções listadas abaixo |
| Persistência | Coeficientes na NVS e gravação de backup JSON no SD |
| Registro | Gravação manual de amostras com rótulo em CSV; formato pendente de correção |
| Alertas | Indicadores visuais; módulo de buzzer com rotinas de sinalização |
| Web | Código parcial, desativado por padrão com `FASE2_WEB = 0` |

O intervalo de aquisição usado pelo código é de **2 segundos** (`DEFAULT_INTERVAL_MS`). Alterar o intervalo no menu ainda não muda o ciclo de leitura.

## Organização do repositório

O firmware principal está em **`monitor_agua/`**. Os sketches de teste são projetos separados e não integram automaticamente a compilação principal.

```text
WaterQuality/
├── README.md
├── monitor_agua/                 # Projeto principal PlatformIO
│   ├── platformio.ini           # Placa, framework e dependências
│   ├── include/config.h         # GPIOs, limites, estruturas e flags
│   └── src/                     # Módulos do firmware
├── 3D/                          # Peças do invólucro em STL
├── WaterQuality.jpg             # Diagrama de montagem
├── WaterQuality1.fzz            # Projeto editável no Fritzing
├── pinout.md                    # Tabela de ligações (com divergências)
├── pinout_WaterQuality.xlsx     # Planilha de pinagem
├── arquitetura_firmware.md      # Arquitetura e responsabilidades
├── calibracao_firmware.md       # Equações e procedimentos propostos
├── ihm_monitor_agua.md          # Especificação da interface
├── teste_ihm.md                 # Roteiro de teste da interface
├── teste_ihm/                   # Sketch da IHM com dados simulados
├── teste_displaytft/            # Teste isolado do display
├── teste_display_microsd/       # Teste de display e cartão SD
├── teste_turbidez/              # Teste isolado de turbidez
├── relatorio_testes_sensores.docx
├── include/                     # README auxiliar do template PlatformIO
├── lib/                         # README auxiliar do template PlatformIO
└── test/                        # README auxiliar do template PlatformIO
```

### Módulos do firmware principal

| Arquivo em `monitor_agua/src/` | Responsabilidade |
| --- | --- |
| `main.cpp` | Inicialização e ciclo de aquisição |
| `sensores.h/.cpp` | Aquisição e conversão das grandezas |
| `display.h/.cpp` | Renderização da interface TFT |
| `botoes.h/.cpp` | Leitura dos botões e debounce |
| `maquina.h/.cpp` | Navegação, estados e fluxo de calibração |
| `nvs_cal.h/.cpp` | Leitura e gravação dos coeficientes |
| `sd_log.h/.cpp` | Escrita de amostras e leitura do histórico |
| `buzzer.h/.cpp` | Rotinas de sinalização sonora |
| `web.h/.cpp` | Base para futura interface HTTP |

A separação atual por módulos atende ao MVP. Os arquivos de projeto, documentação, testes e modelos 3D foram mantidos nos caminhos existentes para preservar as referências.

## Hardware e pinagem

| Componente | Modelo / ligação |
| --- | --- |
| Microcontrolador | ESP32 DevKit V3 / WROOM-32; ambiente `esp32dev` |
| ADC | ADS1115, endereço I²C `0x48`, ganho `GAIN_ONE` |
| pH | PH4502C → ADS1115 AIN2 |
| TDS | Gravity Analog TDS → ADS1115 AIN1 |
| Turbidez | TS-300B → ADS1115 AIN0 |
| Temperatura | DS18B20 → 1-Wire; pino pendente de alinhamento |
| Display | TFT ST7789 de 2,0 polegadas; interface em 320 × 240 |
| Armazenamento | Cartão microSD por SPI |
| Conversor de nível | TXS0108E previsto entre I²C de 3,3 V e 5 V |
| Entradas e sinalização | Três botões e buzzer ativo |

### GPIOs definidos no firmware

A tabela registra o conteúdo atual de [`config.h`](monitor_agua/include/config.h), sem apresentar os pontos divergentes como já resolvidos.

| Função | GPIO no código | Observação |
| --- | --- | --- |
| TFT CS | 5 | SPI compartilhado |
| TFT DC | 17 | `pinout.md` ainda indica 2 |
| TFT RST | 4 | Reset do display |
| SD CS | 15 | Seleção do cartão |
| SPI SCK / MOSI / MISO | 18 / 23 / 19 | Barramento utilizado pelo projeto |
| I²C SDA / SCL | 21 / 22 | ADS1115 |
| DS18B20 | **34** | Documentação indica **13**; requer correção antes do uso |
| Navegar / Confirmar / Voltar | 32 / 33 / 25 | Botões com `INPUT_PULLUP` |
| Buzzer | 26 | Saída digital |

O GPIO 34 do ESP32 é somente entrada, conforme a [documentação da Espressif](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/gpio.html). O barramento 1-Wire precisa também transmitir; portanto essa definição não deve ser usada como uma ligação validada. A documentação do projeto propõe GPIO 13 com pull-up de 4,7 kΩ para 3,3 V. Confirme a montagem física antes de ajustar a configuração.

## Como compilar e executar

Este projeto é um **firmware embarcado**: a execução acontece no ESP32 após a gravação por USB.

### 1. Preparar o ambiente

- Instale o Git e o [PlatformIO IDE para VS Code ou PlatformIO Core](https://platformio.org/install).
- Para upload e teste físico, use o ESP32 e um cabo USB de dados.
- Confira o hardware, as divergências de pinagem e as pendências antes de medir amostras.

Clone o projeto:

```bash
git clone https://github.com/eusales/WaterQuality.git
cd WaterQuality/monitor_agua
```

No VS Code com PlatformIO, abra a pasta **`monitor_agua`**, que contém `platformio.ini`. Os comandos abaixo podem ser executados no terminal do PlatformIO.

### 2. Compilar

```bash
pio run -e esp32dev
```

O PlatformIO obtém a plataforma e as bibliotecas declaradas em [`platformio.ini`](monitor_agua/platformio.ini). A primeira compilação precisa de internet e pode demorar mais. Compilar não exige uma placa conectada.

Dependências declaradas: Adafruit ST7735/ST7789, Adafruit GFX, Adafruit ADS1X15, OneWire, DallasTemperature e ArduinoJson. As versões possuem limites mínimos, sem fixação exata; o resultado depende das versões resolvidas pelo PlatformIO.

### 3. Gravar no ESP32

```bash
pio device list
pio run -e esp32dev --target upload
```

Se a porta não for detectada automaticamente, informe a porta exibida por `pio device list`. Exemplo fictício no Windows:

```powershell
pio run -e esp32dev --target upload --upload-port COM5
```

### 4. Acompanhar as leituras

```bash
pio device monitor --baud 115200
```

Para selecionar uma porta explicitamente:

```powershell
pio device monitor --port COM5 --baud 115200
```

Substitua `COM5` pela porta do seu ESP32. Consulte também o [guia oficial do PlatformIO](https://docs.platformio.org/en/latest/core/quickstart.html).

## Operação da interface

1. Após iniciar, o dispositivo exibe o monitoramento dos quatro parâmetros.
2. Pressione **Confirmar** para abrir o menu principal.
3. Use **Navegar** para selecionar Calibração, Histórico, Configurações ou Registrar amostra.
4. Use **Confirmar** para entrar ou executar a ação e **Voltar** para retornar.
5. Em Registro, selecione um rótulo: Destilada, Torneira, Rio, Amostra1 ou Amostra2.

Os nomes de arquivos previstos no cartão são `/agua_log.csv` para amostras e `/cal_backup.json` para coeficientes. O histórico e a restauração do backup possuem as limitações listadas a seguir.

Para explorar apenas a interface com valores simulados, consulte [`teste_ihm.md`](teste_ihm.md) e o sketch [`teste_ihm.ino`](teste_ihm/teste_ihm.ino). Esse teste é separado do firmware principal.

## Calibração e estado atual

Os procedimentos e equações propostos estão em [`calibracao_firmware.md`](calibracao_firmware.md): pH em três pontos, TDS com referência de condutividade e turbidez em três pontos. O fluxo implementado ainda precisa ser corrigido para capturar a tensão bruta do sensor selecionado. A presença das telas de calibração não significa que o resultado foi validado.

### Pendências identificadas na leitura do código

| Ponto | Situação atual | Ajuste necessário |
| --- | --- | --- |
| Pinagem | DS18B20 em GPIO 34 no código e 13 na documentação; TFT DC em 17 no código e 2 em `pinout.md` | Consolidar esquema, documentação e configuração |
| Entrada da calibração | `processarCalibracao()` usa `dados.ph` como `vAtual` para todos os sensores | Capturar a tensão bruta do canal correto e validar os pontos |
| CSV e histórico | Cabeçalho e leitor esperam seis campos; escritor grava cinco, sem timestamp | Definir e aplicar o mesmo formato na gravação e leitura |
| Recuperação do backup | `carregarCalibracao()` é chamado antes de `inicializarSD()` | Montar o SD antes de tentar ler o backup |
| Configurações | Menu altera variáveis, mas alertas usam constantes e aquisição usa intervalo fixo; limites não são persistidos | Conectar parâmetros editados à aquisição, aos alertas e à persistência |
| Alarme sonoro | Existe `bipAlerta()`, mas o fluxo de avaliação dos limites não o chama | Integrar a sinalização ao tratamento de alertas |
| Web | `FASE2_WEB` está desativado; `setup()` não chama `inicializarWeb()` | Completar e testar a inicialização antes de anunciar acesso HTTP |

Esta atualização trata da documentação e da organização. O firmware foi preservado. Não foi realizada nova compilação nem validação em bancada nesta revisão; métricas anteriores de RAM/Flash não são apresentadas como resultados atuais.

## Documentação e arquivos de apoio

| Recurso | Conteúdo |
| --- | --- |
| [Arquitetura de firmware](arquitetura_firmware.md) | Organização dos módulos e projeto da solução |
| [Calibração](calibracao_firmware.md) | Equações e procedimentos propostos |
| [Especificação da IHM](ihm_monitor_agua.md) | Telas e navegação |
| [Teste da IHM](teste_ihm.md) | Procedimento isolado com dados simulados |
| [Pinout](pinout.md) e [planilha](pinout_WaterQuality.xlsx) | Referências de montagem a consolidar |
| [Modelos 3D](3D/) | Arquivos STL do invólucro |
| [Projeto Fritzing](WaterQuality1.fzz) | Diagrama editável |
| [Relatório de testes dos sensores](relatorio_testes_sensores.docx) | Documento de apoio |

Os documentos de projeto descrevem também comportamentos pretendidos. Para avaliar o que está implementado nesta versão, compare-os com o código em `monitor_agua/` e com as pendências acima.

## Autoria e licença

Projeto acadêmico desenvolvido por Matheus Sales na UFMT. O repositório ainda não contém um arquivo `LICENSE` definindo uma licença de distribuição.
