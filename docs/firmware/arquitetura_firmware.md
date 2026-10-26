# Arquitetura de Firmware — Monitor Portátil de Qualidade da Água

**Projeto:** Monitor Portátil de Qualidade da Água
**Disciplina:** Instrumentação Industrial II — UFMT
**Plataforma:** ESP32 DevKit V3 (WROOM-32)
**Ambiente recomendado:** PlatformIO (VS Code) — compatível com Arduino IDE 2.x sem mudança de arquivos

---

## 1. Pinout Final Consolidado

### 1.1 Barramento SPI — compartilhado TFT + SD

| GPIO | Função | Destino |
|---|---|---|
| GPIO 18 | SCK | TFT SCL + SD SCK |
| GPIO 23 | MOSI | TFT SDA + SD MOSI |
| GPIO 19 | MISO | SD MISO (TFT não usa) |
| GPIO 5 | CS — TFT ST7789 | Chip Select TFT |
| GPIO 17 | DC — TFT ST7789 | Data/Command TFT ⚠ movido do GPIO 2 |
| GPIO 4 | RST — TFT ST7789 | Reset TFT |
| GPIO 15 | CS — SD Card | Chip Select SD |

> ⚠ DC do TFT usa GPIO 17, não GPIO 2. GPIO 2 tem LED onboard que interfere na inicialização do TFT.

### 1.2 Barramento I²C — ADS1115 via TXS0108E

| GPIO | Função |
|---|---|
| GPIO 21 | SDA → TXS0108E A2 → B2 → ADS1115 SDA |
| GPIO 22 | SCL → TXS0108E A1 → B1 → ADS1115 SCL |

### 1.3 ADS1115 — Mapa de canais

| Canal | Sensor | Faixa esperada | Observação |
|---|---|---|---|
| AIN0 | Turbidez — TS-300B | Curva empírica (regressão 2º grau) | Alimentado 5V, saída direta ao ADS |
| AIN1 | TDS — Gravity Analog | 0 – 2,3 V | Compensação de temperatura via DS18B20 |
| AIN2 | pH — PH4502C | ~1,0 – 4,0 V | Calibração 3 pontos (pH 4, 7, 10) |
| AIN3 | Reserva | — | Disponível para expansão futura |

Configuração: endereço 0x48 (ADDR → GND), VDD = 5V, ganho GAIN_ONE (±4,096V).

### 1.4 Demais periféricos

| GPIO | Periférico | Protocolo | Observação |
|---|---|---|---|
| GPIO 13 | DS18B20 | 1-Wire | Pull-up 4,7 kΩ obrigatório para 3,3V |
| GPIO 32 | Botão Navegar | Digital INPUT_PULLUP | Debounce 20 ms por software |
| GPIO 33 | Botão Confirmar | Digital INPUT_PULLUP | Debounce 20 ms por software |
| GPIO 25 | Botão Voltar | Digital INPUT_PULLUP | Debounce 20 ms por software |
| GPIO 26 | Buzzer ativo | Digital OUTPUT | Alertas sonoros |

### 1.5 GPIOs proibidos e livres

| GPIO(s) | Status | Motivo |
|---|---|---|
| GPIO 6 – 11 | PROIBIDO | Flash interna — trava o boot |
| GPIO 34 – 39 | PROIBIDO | Somente entrada, sem pull-up |
| GPIO 0 | PROIBIDO | Botão de boot |
| GPIO 1 / 3 | PROIBIDO | TX0 / RX0 — Serial/USB |
| GPIO 2 | PROIBIDO (realocado) | LED onboard — DC do TFT foi para GPIO 17 |
| GPIO 12, 14, 16, 27 | LIVRE | Disponíveis para expansão |

---

## 2. Estrutura de Arquivos

### 2.1 PlatformIO

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
    └── web.h      / web.cpp
```

### 2.2 Arduino IDE 2.x (alternativa)

```
monitor_agua/
├── monitor_agua.ino        ← equivale ao main.cpp
├── config.h
├── sensores.h / sensores.cpp
├── display.h  / display.cpp
├── botoes.h   / botoes.cpp
├── maquina.h  / maquina.cpp
├── nvs_cal.h  / nvs_cal.cpp
├── sd_log.h   / sd_log.cpp
├── buzzer.h   / buzzer.cpp
└── web.h      / web.cpp
```

> No Arduino IDE 2.x: botão de três pontinhos ao lado das abas → "New File" para criar cada `.h` e `.cpp` dentro da pasta do sketch.

---

## 3. Responsabilidade de Cada Módulo

### `config.h` — Configuração global

Único arquivo a editar quando há mudança de hardware ou parâmetros de projeto.

Contém:
- Todos os `#define` de GPIO
- Endereço I²C e ganho do ADS1115
- Limites padrão de alerta (pH, TDS, turbidez)
- Nome do arquivo CSV e do backup de calibração
- Flag de fase: `#define FASE2_WEB 0`
- Struct `Leituras` compartilhado com `extern`

```cpp
// Exemplo de conteúdo
#define TFT_CS    5
#define TFT_DC   17
#define TFT_RST   4
#define SD_CS    15
#define SDA_PIN  21
#define SCL_PIN  22
#define ONE_WIRE  13
#define BTN_NAV  32
#define BTN_CONF 33
#define BTN_VOLT 25
#define BUZZER   26

#define FASE2_WEB 0   // 1 = ativa servidor HTTP na Fase 2

struct Leituras {
  float ph, tds_ppm, turbidez_ntu, temp_c;
  bool  ph_ok, tds_ok, turb_ok;
};

extern Leituras dados;
```

### `sensores.h / .cpp` — Aquisição e cálculo

Encapsula todo o hardware analógico e o DS18B20. Não sabe nada sobre display ou botões.

Funções públicas:
- `inicializarSensores()` — Wire, ADS1115, DS18B20
- `lerTodos()` — lê os 4 canais, aplica calibração, preenche `dados`
- `calcPH(float v)` — converte tensão em pH
- `calcTDS(float v, float temp)` — converte com compensação de temperatura
- `calcNTU(float v)` — aplica regressão polinomial

Dependências: `config.h`, Adafruit ADS1X15, OneWire, DallasTemperature.

### `display.h / .cpp` — Renderização

Desenha as 6 telas a partir do estado atual e do struct `dados`. Não lê sensor, não toma decisão de navegação.

Funções públicas:
- `inicializarDisplay()` — init TFT, setRotation(1)
- `desenharTela(Estado e)` — despacha para a função correta
- `desenharTela1_Monitoramento()`
- `desenharTela2_Menu()`
- `desenharTela3_Calibracao()`
- `desenharTela4_Registro()`
- `desenharTela5_Historico()`
- `desenharTela6_Configuracoes()`

Dependências: `config.h`, `maquina.h` (enum Estado), Adafruit ST7789, Adafruit GFX.

> Módulo testável de forma isolada com dados simulados — exatamente como o teste IHM já validado.

### `botoes.h / .cpp` — Entrada física

Lê os três GPIOs com debounce de 20 ms e anti-repetição de 200 ms.

Funções públicas:
- `inicializarBotoes()`
- `verificarBotoes()` — chamado no loop; chama `acaoNavegar/Confirmar/Voltar()` da máquina

Dependências: `config.h`, `maquina.h`.

### `maquina.h / .cpp` — Máquina de estados (IHM)

Cérebro da interface. Contém o `enum Estado`, variáveis de seleção de menu e fluxo de calibração, e as três funções de ação.

Funções públicas:
- `acaoNavegar()`
- `acaoConfirmar()`
- `acaoVoltar()`
- `getEstado()` — retorna o estado atual

Estados:
```cpp
enum Estado {
  MONITORAMENTO,
  MENU_PRINCIPAL,
  CALIBRACAO_MENU,
  CALIBRACAO_FLUXO,
  REGISTRO,
  HISTORICO,
  CONFIGURACOES
};
```

Dependências: `config.h`, `display.h`, `nvs_cal.h`, `sd_log.h`, `buzzer.h`.

### `nvs_cal.h / .cpp` — Persistência de calibração

Abstrai NVS e backup JSON no SD. O restante do código só chama as funções públicas — não sabe que existe NVS ou JSON por baixo.

Funções públicas:
- `carregarCalibracao()` — tenta NVS; se vazia/corrompida, restaura do `cal_backup.json` no SD
- `salvarCalibracao()` — grava na NVS e escreve `cal_backup.json` no SD

Estrutura do `cal_backup.json`:
```json
{
  "ph_offset": 2.500,
  "ph_slope_acid": -5.21,
  "ph_slope_bas": 4.87,
  "k_tds": 1.032,
  "turb_a": -12.5,
  "turb_b": 8.3,
  "turb_c": 0.2
}
```

Dependências: `config.h`, Preferences (NVS), `sd_log.h`.

### `sd_log.h / .cpp` — Log de dados CSV

Gerencia o arquivo de dados. Separado do `nvs_cal` pois são responsabilidades distintas — log de medições vs. persistência de configuração.

Funções públicas:
- `inicializarSD()` — monta cartão, verifica FAT32
- `gravarLinha(const char* rotulo)` — append de uma linha CSV com timestamp
- `lerHistorico(int pagina, Registro* buf, int* total)` — leitura paginada para a Tela 5

Formato CSV:
```
timestamp,rotulo,ph,tds_ppm,turbidez_ntu,temperatura_c
2025-06-15T14:32:00,Torneira,7.02,312.0,4.1,24.3
```

Dependências: `config.h`, SD (built-in ESP32).

### `buzzer.h / .cpp` — Alertas sonoros

Padrões sonoros simples no GPIO 26.

Funções públicas:
- `bipAlerta()` — bip único para valor fora do limite
- `bipConfirmacao()` — sequência curta ao salvar calibração
- `bipErro()` — padrão duplo para falha de SD ou sensor

Dependências: `config.h`.

### `web.h / .cpp` — Servidor HTTP (Fase 2)

Compilado apenas quando `FASE2_WEB 1`. Lê diretamente o struct `dados` — sem duplicar lógica de sensor.

Endpoints planejados:
- `GET /leituras` → JSON com valores atuais de `dados`
- `GET /historico` → JSON com últimas N linhas do CSV
- `POST /registrar` → dispara o mesmo fluxo da Tela 4

Dependências: `config.h`, `sensores.h`, `sd_log.h`, WiFi (built-in ESP32), WebServer.

---

## 4. Fluxo de Dados

```
BOOT
 └── carregarCalibracao()     NVS → coeficientes; se vazia → cal_backup.json SD
      └── inicializarSensores()
           └── inicializarSD()
                └── [FASE2_WEB] inicializarWeb()
                     └── desenharTela(MONITORAMENTO)

LOOP — a cada intervalo configurado (padrão 2 s)
 └── lerTodos()               ADS1115 (AIN0/1/2) + DS18B20 → struct dados
      └── verificarAlertas()  compara dados com limites → bipAlerta() se necessário
           └── se estado == MONITORAMENTO → atualizarDisplay()

LOOP — contínuo
 └── verificarBotoes()        detecta pressão → chama maquina.h
      └── [FASE2_WEB] servirWeb()   atende requisições HTTP
```

---

## 5. Dependências entre Módulos

```
config.h  ←────────────────── todos os módulos
             │
sensores ────┤
display  ────┤──→ maquina ──→ nvs_cal ──→ sd_log
botoes   ────┤         └────→ sd_log
buzzer   ────┤         └────→ buzzer
             │
web ─────────┘ (só Fase 2, lê sensores e sd_log diretamente)
```

Regra: nenhum módulo importa `display.h` exceto `maquina.h`. Nenhum módulo importa `maquina.h` exceto `botoes.h` e `main.cpp`.

---

## 6. Estratégia de Implementação por Etapa

| Etapa | O que fazer | Módulos envolvidos |
|---|---|---|
| 1 | Compilar esqueleto vazio — garantir que todos os arquivos compilam sem erro | todos |
| 2 | Integrar leitura real dos sensores com Serial Monitor | `sensores`, `config` |
| 3 | Ativar display com dados reais (substitui dados simulados do teste IHM) | `display`, `maquina` |
| 4 | Integrar SD card — log CSV funcionando | `sd_log` |
| 5 | Implementar NVS + backup calibração | `nvs_cal` |
| 6 | Implementar fluxos de calibração completos na IHM | `maquina`, `nvs_cal` |
| 7 | Integrar buzzer nos alertas | `buzzer`, `maquina` |
| 8 | Testes integrados completos — Fase 1 fechada | todos |
| 9 | Ativar `FASE2_WEB 1`, implementar `web.cpp` | `web`, `main` |

---

## 7. Bibliotecas Necessárias

| Biblioteca | Versão mínima | Módulo que usa |
|---|---|---|
| Adafruit ST7789 | ≥ 1.9.0 | `display` |
| Adafruit GFX Library | ≥ 1.11.0 | `display` |
| Adafruit ADS1X15 | ≥ 2.5.0 | `sensores` |
| OneWire | ≥ 2.3.7 | `sensores` |
| DallasTemperature | ≥ 3.9.0 | `sensores` |
| SD | built-in ESP32 | `sd_log`, `nvs_cal` |
| SPI | built-in ESP32 | `display`, `sd_log` |
| Wire | built-in ESP32 | `sensores` |
| Preferences | built-in ESP32 | `nvs_cal` |
| WiFi + WebServer | built-in ESP32 | `web` (Fase 2) |
| ArduinoJson | ≥ 6.21.0 | `nvs_cal` (backup JSON), `web` |

---

## 8. Checklist Final de Hardware antes de Compilar

- ESP32: GPIO 17 no pino DC do TFT (não GPIO 2)
- TFT: CS=5, DC=17, RST=4, VCC=3,3V, BLK=3,3V
- SD: CS=15, VCC=3,3V, FAT32 formatado
- ADS1115: VDD=5V, ADDR=GND (0x48), SCL/SDA via TXS0108E
- TXS0108E: VCCA=3,3V, VCCB=5V, OE=3,3V, A1=GPIO22, A2=GPIO21
- DS18B20: VDD=3,3V, DATA=GPIO13, pull-up 4,7 kΩ entre DATA e 3,3V
- Sensores analógicos: VCC=5V, saídas analógicas em AIN0/1/2 do ADS1115
- Botões: GPIO 32/33/25 → GND (sem resistor externo)
- Buzzer ativo: GPIO 26 → GND (verificar polaridade)
- GND único e comum entre ESP32, ADS1115, TXS0108E e todos os sensores
