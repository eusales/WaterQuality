# Prompt de Codificação — MVP Monitor Portátil de Qualidade da Água

> **Como usar:** Cole este prompt no início de cada nova sessão de codificação.
> Ele estabelece o contexto completo do projeto, a arquitetura definida e as
> regras que o assistente deve seguir ao gerar código.

---

## 1. Contexto do Projeto

Você está auxiliando na codificação do firmware MVP de um **monitor portátil de qualidade da água**, desenvolvido como projeto da disciplina **Instrumentação Industrial II — UFMT**, curso de Engenharia de Controle e Automação.

**Plataforma:** ESP32 DevKit V3 (WROOM-32), framework Arduino, ambiente PlatformIO (VS Code). O código é compatível com Arduino IDE 2.x sem mudança de arquivos — apenas a localização das pastas difere.

**Repositório de referência de estilo e estrutura:** https://github.com/renfernand/WaterQuality
Use-o como referência de organização de projeto PlatformIO para ESP32: estrutura `src/`, `include/`, `lib/`, `platformio.ini`. Adapte o estilo de código C++ observado lá ao hardware e sensores deste projeto.

---

## 2. Pinout Final — IMUTÁVEL

Nunca sugira GPIOs diferentes dos listados abaixo. Este pinout está validado em hardware.

### SPI — TFT ST7789 + SD Card (barramento compartilhado)

| GPIO | Função | Observação |
|---|---|---|
| GPIO 18 | SCK | TFT + SD |
| GPIO 23 | MOSI | TFT + SD |
| GPIO 19 | MISO | SD apenas (TFT não usa) |
| GPIO 5 | CS — TFT ST7789 | |
| GPIO 17 | DC — TFT ST7789 | ⚠ NÃO usar GPIO 2 — tem LED onboard |
| GPIO 4 | RST — TFT ST7789 | |
| GPIO 15 | CS — SD Card | Sem pull-down externo |

### I²C — ADS1115 via TXS0108E

| GPIO | Função |
|---|---|
| GPIO 21 | SDA → TXS0108E A2 → B2 → ADS1115 |
| GPIO 22 | SCL → TXS0108E A1 → B1 → ADS1115 |

### ADS1115 — Canais analógicos

| Canal | Sensor | Faixa |
|---|---|---|
| AIN0 | Turbidez TS-300B | Curva empírica (regressão 2º grau) |
| AIN1 | TDS Gravity Analog | 0 – 2,3 V |
| AIN2 | pH PH4502C | ~1,0 – 4,0 V |
| AIN3 | Reserva | — |

Configuração ADS1115: endereço `0x48` (ADDR → GND), VDD = 5V, ganho `GAIN_ONE` (±4,096V).

### Demais periféricos

| GPIO | Periférico | Observação |
|---|---|---|
| GPIO 13 | DS18B20 (1-Wire) | Pull-up 4,7 kΩ obrigatório para 3,3V |
| GPIO 32 | Botão Navegar | INPUT_PULLUP, debounce 20 ms |
| GPIO 33 | Botão Confirmar | INPUT_PULLUP, debounce 20 ms |
| GPIO 25 | Botão Voltar | INPUT_PULLUP, debounce 20 ms |
| GPIO 26 | Buzzer ativo | OUTPUT |

### GPIOs proibidos — nunca usar

| GPIO(s) | Motivo |
|---|---|
| GPIO 6 – 11 | Flash interna — trava o boot |
| GPIO 34 – 39 | Input only, sem pull-up |
| GPIO 0 | Botão de boot |
| GPIO 1 / 3 | TX0/RX0 — Serial/USB |
| GPIO 2 | LED onboard — DC do TFT foi para GPIO 17 |

---

## 3. Arquitetura de Módulos — ESTRUTURA OBRIGATÓRIA

Todo código gerado deve respeitar esta divisão. Cada módulo tem responsabilidade única e não deve cruzar limites.

### Estrutura de arquivos (PlatformIO)

```
monitor_agua/
├── platformio.ini
├── include/
│   └── config.h
└── src/
    ├── main.cpp
    ├── sensores.h / sensores.cpp
    ├── display.h  / display.cpp
    ├── botoes.h   / botoes.cpp
    ├── maquina.h  / maquina.cpp
    ├── nvs_cal.h  / nvs_cal.cpp
    ├── sd_log.h   / sd_log.cpp
    ├── buzzer.h   / buzzer.cpp
    └── web.h      / web.cpp      ← só ativo quando FASE2_WEB = 1
```

### Responsabilidades por módulo

**`config.h`** — Único arquivo alterado em mudanças de hardware.
- Todos os `#define` de GPIO (TFT_CS, TFT_DC, TFT_RST, SD_CS, SDA_PIN, SCL_PIN, ONE_WIRE, BTN_NAV, BTN_CONF, BTN_VOLT, BUZZER)
- Constantes ADS1115 (endereço, ganho, canais)
- Limites padrão de alerta (pH mín/máx, TDS, turbidez)
- Nome do arquivo CSV e do backup de calibração
- Flag `#define FASE2_WEB 0`
- Struct `Leituras` com `extern Leituras dados`
- Struct `CalCoef` com os coeficientes de calibração

**`sensores.h/.cpp`** — Hardware analógico e DS18B20. Não toca display nem botões.
- Referência: arquivo de leitura de sensores em `src/` do repositório renfernand/WaterQuality
- `inicializarSensores()` — Wire.begin(SDA_PIN, SCL_PIN), ads.begin(0x48), sensors.begin()
- `lerTodos()` — lê AIN0/1/2 + DS18B20, aplica calibração, preenche struct `dados`
- `calcPH(float v)` — calibração 3 pontos (slope_acid, slope_bas, offset)
- `calcTDS(float v, float temp)` — EC bruto → EC25 → ppm com fator k
- `calcNTU(float v)` — regressão: `a*v² + b*v + c`
- `verificarAlertas()` — compara dados com limites de config.h, seta flags `ph_ok`, `tds_ok`, `turb_ok`

**`display.h/.cpp`** — Só renderiza. Recebe estado atual e struct `dados`. Não lê sensor.
- Referência: tela IHM validada no arquivo `teste_ihm.md` — reusar todas as funções `desenharTela*()`, cores e helpers já testados
- `inicializarDisplay()` — tft.init(240,320), tft.setRotation(1)
- `desenharTela(Estado e)` — dispatcher para as 6 funções abaixo
- `desenharTela1_Monitoramento()` — quadrantes 2×2, alertas visuais, verde/vermelho
- `desenharTela2_Menu()` — lista 4 itens com ◉/○
- `desenharTela3_Calibracao()` — submenu + fluxo passo a passo com barra de progresso
- `desenharTela4_Registro()` — lista de rótulos
- `desenharTela5_Historico()` — registros paginados do CSV
- `desenharTela6_Configuracoes()` — edição inline com `< valor >`

**`botoes.h/.cpp`** — Leitura física com debounce. Não desenha nada.
- Referência: lógica de debounce validada no `teste_ihm.md`
- `inicializarBotoes()` — pinMode INPUT_PULLUP nos 3 GPIOs
- `verificarBotoes()` — chamado no loop; chama `acaoNavegar/Confirmar/Voltar()` da máquina
- Debounce: 20 ms; anti-repetição: 200 ms entre pressões aceitas

**`maquina.h/.cpp`** — Cérebro da IHM. Contém enum Estado e as 3 ações.
- Referência: mapa de estados completo em `ihm_monitor_agua.md` — implementar EXATAMENTE os estados e transições descritos lá
- `enum Estado { MONITORAMENTO, MENU_PRINCIPAL, CALIBRACAO_MENU, CALIBRACAO_FLUXO, REGISTRO, HISTORICO, CONFIGURACOES }`
- `acaoNavegar()` — comportamento circular por estado
- `acaoConfirmar()` — transições de estado + fluxo de calibração
- `acaoVoltar()` — sempre sobe um nível
- `getEstado()` — retorna estado atual
- Regra: é o ÚNICO módulo que importa `display.h`. Também importa `nvs_cal.h`, `sd_log.h`, `buzzer.h`

**`nvs_cal.h/.cpp`** — Persistência de calibração. NVS principal + backup JSON no SD.
- Referência: procedimentos de calibração descritos em `relatorio_testes_sensores.docx` — coeficientes por sensor
- `carregarCalibracao()` — tenta NVS (Preferences); se vazia/corrompida → restaura de `cal_backup.json` no SD
- `salvarCalibracao()` — grava na NVS e reescreve `cal_backup.json` no SD
- Estrutura JSON: `{ "ph_offset", "ph_slope_acid", "ph_slope_bas", "k_tds", "turb_a", "turb_b", "turb_c" }`
- Biblioteca: ArduinoJson ≥ 6.21.0 para serialização/deserialização

**`sd_log.h/.cpp`** — Log CSV de dados. Separado do nvs_cal (responsabilidades distintas).
- Referência: teste de SD card validado em `teste_sd_card.md` — reusar lógica de inicialização e escrita
- `inicializarSD()` — SD.begin(SD_CS), verificar FAT32
- `gravarLinha(const char* rotulo)` — append CSV: `timestamp,rotulo,ph,tds_ppm,turbidez_ntu,temperatura_c`
- `lerHistorico(int pagina, Registro* buf, int* total)` — leitura paginada para Tela 5
- Formato CSV: `2025-06-15T14:32:00,Torneira,7.02,312.0,4.1,24.3`

**`buzzer.h/.cpp`** — Padrões sonoros no GPIO 26.
- `bipAlerta()` — 1 bip curto (valor fora do limite)
- `bipConfirmacao()` — 2 bips rápidos (calibração salva)
- `bipErro()` — 3 bips lentos (falha SD ou sensor)

**`web.h/.cpp`** — Servidor HTTP. Compilado APENAS quando `FASE2_WEB 1`.
- `#if FASE2_WEB` em torno de todo o código deste módulo
- `inicializarWeb()` — WiFi.begin(), WebServer na porta 80
- `servirWeb()` — server.handleClient() no loop
- `GET /leituras` → JSON com struct `dados` atual
- `GET /historico` → JSON com últimas N linhas do CSV
- `POST /registrar` → dispara `gravarLinha()` do sd_log

**`main.cpp`** — Apenas setup() e loop(). Máximo 60 linhas.

```cpp
#include <Arduino.h>
#include "config.h"
#include "sensores.h"
#include "display.h"
#include "botoes.h"
#include "maquina.h"
#include "nvs_cal.h"
#include "sd_log.h"
#include "buzzer.h"
#if FASE2_WEB
#include "web.h"
#endif

Leituras dados;  // definição global — extern em config.h

void setup() {
  Serial.begin(115200);
  inicializarBotoes();
  inicializarDisplay();
  inicializarSensores();
  carregarCalibracao();
  inicializarSD();
  #if FASE2_WEB
  inicializarWeb();
  #endif
  desenharTela(MONITORAMENTO);
}

void loop() {
  verificarBotoes();
  // lerTodos() e atualizarDisplay() chamados pela maquina.h no intervalo configurado
  #if FASE2_WEB
  servirWeb();
  #endif
}
```

---

## 4. Regras de Dependência entre Módulos

```
config.h  ←────────── todos os módulos (todos incluem config.h)

sensores  ──────────→ (nenhum módulo interno)
display   ──────────→ (nenhum módulo interno — recebe dados via extern)
botoes    ──────────→ maquina
maquina   ──────────→ display, nvs_cal, sd_log, buzzer
nvs_cal   ──────────→ sd_log
sd_log    ──────────→ (nenhum módulo interno)
buzzer    ──────────→ (nenhum módulo interno)
web       ──────────→ sensores, sd_log  (só Fase 2)
main      ──────────→ todos
```

**Regra absoluta:** nenhum módulo importa `display.h` exceto `maquina.h`. Nenhum módulo importa `maquina.h` exceto `botoes.h` e `main.cpp`.

---

## 5. Bibliotecas (platformio.ini)

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps =
    adafruit/Adafruit ST7789@>=1.9.0
    adafruit/Adafruit GFX Library@>=1.11.0
    adafruit/Adafruit ADS1X15@>=2.5.0
    paulstoffregen/OneWire@>=2.3.7
    milesburton/DallasTemperature@>=3.9.0
    bblanchon/ArduinoJson@>=6.21.0
```

SD, SPI, Wire e Preferences são built-in do core ESP32 — não declarar em lib_deps.

---

## 6. Dados Simulados vs. Dados Reais

O firmware evoluiu por etapas. O teste IHM (`teste_ihm.md`) usava dados simulados (constantes `sim_ph`, `sim_tds`, etc.). No MVP final:

- Os dados simulados são **removidos**
- `lerTodos()` em `sensores.cpp` preenche o struct `dados` com leituras reais do ADS1115 e DS18B20
- `display.cpp` lê de `extern Leituras dados` — sem mudança de interface

---

## 7. Calibração — Equações por Sensor

Implementar exatamente como descrito em `relatorio_testes_sensores.docx`:

**pH — 3 pontos (coeficientes salvos em NVS):**
```cpp
slope_acid  = (7.0 - 4.0)  / (V7 - V4);
slope_basic = (7.0 - 10.0) / (V7 - V10);
pH = (voltage < V7) ? slope_acid*(voltage - V7) + 7.0
                    : slope_basic*(voltage - V7) + 7.0;
```

**TDS — 1 ponto + compensação temperatura (fator k salvo em NVS):**
```cpp
ec_bruto = voltage * 1000.0 / 2.3;
ec25     = ec_bruto / (1.0 + 0.02 * (temp - 25.0));
k        = 1413.0 / ec25;          // calculado na calibração
ec_corr  = ec_bruto * k / (1.0 + 0.02 * (temp - 25.0));
TDS_ppm  = ec_corr / 2.0;
```

**Turbidez — regressão polinomial 2º grau (a, b, c salvos em NVS):**
```cpp
NTU = a * v * v + b * v + c;
```

---

## 8. IHM — Estados e Transições

Implementar o mapa de estados completo de `ihm_monitor_agua.md`. Resumo das transições:

| Estado atual | Navegar | Confirmar | Voltar |
|---|---|---|---|
| MONITORAMENTO | sem efeito | → MENU_PRINCIPAL | sem efeito |
| MENU_PRINCIPAL | percorre 4 itens (circular) | → item selecionado | → MONITORAMENTO |
| CALIBRACAO_MENU | percorre 3 sensores | → CALIBRACAO_FLUXO | → MENU_PRINCIPAL |
| CALIBRACAO_FLUXO | sem efeito | avança passo / salva NVS no último | → CALIBRACAO_MENU |
| REGISTRO | percorre 5 rótulos | grava CSV → MENU_PRINCIPAL | → MENU_PRINCIPAL |
| HISTORICO | percorre registros | exibe detalhe | → MENU_PRINCIPAL |
| CONFIGURACOES | seleciona / incrementa em edição | alterna modo edição | → MENU_PRINCIPAL |

---

## 9. Fluxo de Boot e Loop

```
BOOT
  inicializarBotoes()
  inicializarDisplay()          → tela de splash 2s
  inicializarSensores()         → Wire, ADS1115, DS18B20
  carregarCalibracao()          → NVS → coeficientes; fallback: cal_backup.json SD
  inicializarSD()               → SD.begin(SD_CS)
  [FASE2_WEB] inicializarWeb()
  desenharTela(MONITORAMENTO)

LOOP — a cada intervalo (padrão 2s)
  lerTodos()                    → ADS1115 + DS18B20 → struct dados
  verificarAlertas()            → seta flags ok/nok → bipAlerta() se necessário
  se estado == MONITORAMENTO → redesenhar Tela 1

LOOP — contínuo
  verificarBotoes()             → detecta pressão → chama maquina.h
  [FASE2_WEB] servirWeb()
```

---

## 10. Etapas de Implementação — Ordem Obrigatória

Sempre implemente nesta sequência. Não pule etapas.

| Etapa | Objetivo | Módulos | Validação |
|---|---|---|---|
| 1 | Esqueleto vazio compila sem erro | todos | `pio run` sem erros |
| 2 | Leitura real dos sensores no Serial Monitor | `sensores`, `config` | valores no Serial 115200 |
| 3 | Display com dados reais (substitui simulados do IHM) | `display`, `maquina` | Tela 1 com leituras reais |
| 4 | SD card — log CSV funcionando | `sd_log` | arquivo criado e lido |
| 5 | NVS + backup JSON de calibração | `nvs_cal` | coeficientes persistem após reboot |
| 6 | Fluxos de calibração completos na IHM | `maquina`, `nvs_cal` | todas as telas de calibração |
| 7 | Buzzer nos alertas | `buzzer`, `maquina` | bip ao ultrapassar limite |
| 8 | Testes integrados — Fase 1 fechada | todos | todas as 6 telas funcionando |
| 9 | Ativar FASE2_WEB, implementar web.cpp | `web`, `main` | GET /leituras retorna JSON |

---

## 11. Restrições Absolutas

- **Nunca** colocar o TXS0108E na cadeia analógica — ele é digital, fica só no I²C
- **Nunca** conectar saída analógica de sensor diretamente ao ESP32 — sempre via ADS1115
- **Nunca** usar GPIO 6–11, 34–39, 0, 1, 3 ou 2 para nada novo
- **Nunca** reabrir a discussão de arquitetura de tensão — está resolvida (ADS1115 em 5V)
- **Nunca** usar `delay()` no loop principal para temporização de leituras — usar `millis()`
- **Nunca** gerar arquivo `.ino` monolítico — respeitar a divisão em módulos
- **Nunca** ativar WiFi ou servidor HTTP na Fase 1 (`FASE2_WEB 0`)

---

## 12. Documentos de Referência do Projeto

| Arquivo | Conteúdo | Usar ao implementar |
|---|---|---|
| `arquitetura_firmware.md` | Pinout, estrutura de arquivos, responsabilidades, fluxo | Qualquer módulo |
| `ihm_monitor_agua.md` | Mapa de estados completo, layout de cada tela, lógica de navegação | `maquina.cpp`, `display.cpp` |
| `teste_ihm.md` | Código IHM completo validado com dados simulados | `display.cpp`, `botoes.cpp` — reusar diretamente |
| `relatorio_testes_sensores.docx` | Equações de calibração, pinagem, procedimentos por sensor | `sensores.cpp`, `nvs_cal.cpp` |
| `teste_sd_card.md` | Código validado de escrita/leitura SD | `sd_log.cpp` — reusar diretamente |
| `pinout_WaterQuality.xlsx` | Pinout final consolidado | `config.h` |

---

## 13. Como Pedir Código em Cada Sessão

Use os comandos abaixo para solicitar entregas específicas:

- `"Etapa 1 — gere o esqueleto completo"` → todos os `.h` e `.cpp` com stubs vazios, pronto para compilar
- `"Etapa 2 — implemente sensores.cpp"` → leitura real ADS1115 + DS18B20, referência: relatorio_testes_sensores.docx
- `"Etapa 3 — integre display com dados reais"` → adaptar teste_ihm.md removendo simulados, conectar ao struct dados
- `"Etapa 4 — implemente sd_log"` → reusar teste_sd_card.md + gravarLinha() + lerHistorico()
- `"Etapa 5 — implemente nvs_cal"` → Preferences + ArduinoJson + backup SD
- `"Etapa 6 — implemente fluxos de calibração"` → maquina.cpp seguindo ihm_monitor_agua.md
- `"Etapa 7 — implemente buzzer"` → buzzer.cpp simples GPIO 26
- `"Etapa 8 — teste integrado"` → revisão geral, checklist de hardware
- `"Etapa 9 — Fase 2 web"` → ativar FASE2_WEB, implementar web.cpp
