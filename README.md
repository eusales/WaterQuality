# Monitor Portátil de Qualidade da Água

Monitor portátil e independente de qualidade da água, desenvolvido com ESP32 DevKit V3. Mede **pH**, **TDS** (ppm), **Turbidez** (NTU) e **Temperatura** (°C) em tempo real, com exibição em display TFT colorido, registro em cartão SD e calibração completa.

**Projeto:** WaterQuality  
**Autor:** Matheus Sales  
**Disciplina:** (Instrumentação Industrial II) Engenharia de Controle e Automação - UFMT

---

## Funcionalidades

- Leitura de 4 parâmetros: pH, TDS, Turbidez, Temperatura
- Display TFT ST7789 2.0" (320×240) com 6 telas
- Calibração guiada: pH (3 pontos), TDS (1 ponto), Turbidez (3 pontos)
- Persistência de calibração: NVS flash + backup JSON no SD
- Registro de amostras em CSV no cartão SD
- Alertas visuais (verde/vermelho) e sonoros (buzzer)
- Configuração de limites de alerta via menu
- Splash screen na inicialização
- Servidor HTTP preparado (Fase 2, desativado)

---

## Hardware

| Componente | Modelo | Interface |
|---|---|---|
| Microcontrolador | ESP32 DevKit V3 (WROOM-32) | — |
| ADC | ADS1115 16-bit | I²C @ 0x48 (5V) |
| Sensor pH | PH4502C | Analógico → ADS1115 AIN2 |
| Sensor TDS | Gravity Analog TDS | Analógico → ADS1115 AIN1 |
| Sensor Turbidez | TS-300B | Analógico → ADS1115 AIN0 |
| Sensor Temperatura | DS18B20 | 1-Wire (GPIO 13) |
| Display | TFT ST7789 2.0" | SPI |
| SD Card | Micro SD SPI | SPI |
| Conversor de Nível | TXS0108E | I²C (3.3V ↔ 5V) |
| Buzzer | Ativo | GPIO 26 |
| Botões (3) | Tácteis | GPIO 32/33/25 |

### Pinout

| GPIO | Função |
|---|---|
| 5 | TFT CS |
| 17 | TFT DC |
| 4 | TFT RST |
| 15 | SD CS |
| 18 | SCK (SPI compartilhado) |
| 23 | MOSI (SPI compartilhado) |
| 19 | MISO (SD) |
| 21 | SDA → TXS0108E → ADS1115 |
| 22 | SCL → TXS0108E → ADS1115 |
| 13 | DS18B20 (1-Wire) |
| 32 | Botão Navegar |
| 33 | Botão Confirmar |
| 25 | Botão Voltar |
| 26 | Buzzer |

---

## Estrutura do Projeto

```
monitor_agua/
├── platformio.ini
├── include/
│   └── config.h          # GPIOs, structs, enums, constantes
└── src/
    ├── main.cpp           # Setup + Loop
    ├── sensores.h/.cpp    # Leitura ADS1115 + DS18B20 + cálculo
    ├── display.h/.cpp     # 6 telas TFT + splash
    ├── botoes.h/.cpp      # Debounce + despacho
    ├── maquina.h/.cpp     # Máquina de estados (7 estados)
    ├── nvs_cal.h/.cpp     # Persistência NVS + backup SD
    ├── sd_log.h/.cpp      # Log CSV
    ├── buzzer.h/.cpp      # Alertas sonoros
    └── web.h/.cpp         # Servidor HTTP (Fase 2)
```

---

## Build e Upload

### Pré-requisitos

- [PlatformIO CLI](https://platformio.org/install/cli) ou extensão PlatformIO para VS Code
- ESP32 DevKit V3 conectado via USB

### Comandos

```bash
cd monitor_agua

# Compilar
pio run

# Upload para ESP32
pio run --target upload

# Monitor serial
pio device monitor
```

---

## Calibração

### pH (3 pontos)

1. Tampão pH 7,00 → registra V7
2. Tampão pH 4,00 → registra V4
3. Tampão pH 10,01 → registra V10
4. Calcula inclinações: `slope_acid = (7-4)/(V7-V4)`, `slope_bas = (7-10)/(V7-V10)`

### TDS (1 ponto)

1. Solução padrão 1413 µS/cm + DS18B20
2. Calcula: `EC25 = (V*1000/2.3) / (1+0.02*(T-25))`
3. Fator: `k = 1413/EC25`

### Turbidez (3 pontos)

1. Água destilada (~0 NTU) → V0
2. Argila diluída (~50 NTU) → V50
3. Argila concentrada (~500 NTU) → V500
4. Regressão polinomial: `NTU = a*V² + b*V + c`

Veja mais detalhes em [`calibracao_firmware.md`](calibracao_firmware.md).

---

## Interface (IHM)

| Tela | Descrição |
|---|---|
| Tela 1 | Monitoramento — 4 parâmetros em tempo real |
| Tela 2 | Menu Principal — Calibração, Histórico, Configurações, Registrar |
| Tela 3 | Calibração — Seleção de sensor + fluxo passo a passo |
| Tela 4 | Registro — Seleção de rótulo para gravar no SD |
| Tela 5 | Histórico — Registros paginados do CSV |
| Tela 6 | Configurações — Edição de limites e parâmetros |

**Botões:** Navegar (GPIO 32), Confirmar (GPIO 33), Voltar (GPIO 25)

---

## Documentação

- [`calibracao_firmware.md`](calibracao_firmware.md) — Equações e contexto de calibração
- [`arquitetura_firmware.md`](arquitetura_firmware.md) — Arquitetura de software
- [`ihm_monitor_agua.md`](ihm_monitor_agua.md) — Especificação da interface
- [`pinout.md`](pinout.md) — Pinout consolidado


---

## Resultados

| Métrica | Valor |
|---|---|
| RAM | 7,0% (22.824 / 327.680 bytes) |
| Flash | 30,0% (393.177 / 1.310.720 bytes) |
| Erros de compilação | 0 |
| Warnings | 0 |
| Módulos | 10 |
| Linhas de código | ~980 |

---

## Licença

Projeto acadêmico — UFMT
