# Calibração e Equações — Monitor Portátil de Qualidade da Água

**Projeto:** Monitor Portátil de Qualidade da Água  
**Disciplina:** Instrumentação Industrial II — UFMT  
**Plataforma:** ESP32 DevKit V3 (WROOM-32) — Arduino / PlatformIO

---

## 1. Visão Geral do Sistema de Calibração

O firmware possui um sistema de calibração que converte tensões analógicas brutas em grandezas físicas (pH, TDS em ppm, Turbidez em NTU). Cada sensor possui sua própria equação de conversão e coeficientes específicos que são determinados durante o procedimento de calibração em laboratório.

### 1.1 Estrutura de Dados — `CalCoef`

Todos os coeficientes de calibração são armazenados em uma estrutura compartilhada definida em `config.h`:

```cpp
// ── Struct de calibração ────────────────────────────────────
struct CalCoef {
  float ph_offset;      // Tensão referência do pH 7.00 (V7)
  float ph_slope_acid;  // Inclinação da faixa ácida (pH 4–7)
  float ph_slope_bas;   // Inclinação da faixa básica (pH 7–10)
  float k_tds;          // Fator de calibração TDS
  float turb_a;         // Coeficiente a — regressão 2º grau
  float turb_b;         // Coeficiente b — regressão 2º grau
  float turb_c;         // Coeficiente c — regressão 2º grau
};

extern CalCoef calib;
```

### 1.2 Tabela de Coeficientes

| Coeficiente | Sensor | Unidade | Valor padrão (fábrica) | Descrição |
|---|---|---|---|---|
| `ph_offset` | pH PH4502C | V | 0.0 | Tensão medida na solução pH 7.00 |
| `ph_slope_acid` | pH PH4502C | pH/V | 1.0 | Inclinação entre pH 4.00 e 7.00 |
| `ph_slope_bas` | pH PH4502C | pH/V | 1.0 | Inclinação entre pH 7.00 e 10.01 |
| `k_tds` | TDS Gravity | — | 1.0 | Fator de calibração do TDS |
| `turb_a` | Turbidez TS-300B | NTU/V² | 0.0 | Coeficiente quadrático |
| `turb_b` | Turbidez TS-300B | NTU/V | 0.0 | Coeficiente linear |
| `turb_c` | Turbidez TS-300B | NTU | 0.0 | Coeficiente angular |

> **Nota:** Valores padrão (slope=1, offset=0) produzem leituras em tensão bruta. É necessário calibrar antes de usar o dispositivo para medições confiáveis.

### 1.3 Inicialização em Código

```cpp
// sensores.cpp — linha 12
CalCoef calib = {0, 1, 1, 1, 0, 0, 0};
```

---

## 2. Cadeia de Sinal — Do Sensor ao Microcontrolador

### 2.1 Arquitetura de Hardware

```
┌─────────────┐    Analógico     ┌──────────────┐    I²C 5V     ┌───────────┐    I²C 3.3V    ┌─────────┐
│  Sensor      │──── (0–5V) ────▶│  ADS1115     │──── SDA/SCL ──▶│ TXS0108E  │──── SDA/SCL ──▶│  ESP32  │
│  Analógico   │                  │  16-bit ADC  │                │  Level    │                │ WROOM-32│
│  (5V)        │                  │  VDD = 5V    │                │  Shifter  │                │  3.3V   │
└─────────────┘                  └──────────────┘                └───────────┘                └─────────┘
```

### 2.2 ADS1115 — Conversor Analógico-Digital

- **Modelo:** ADS1115 (Texas Instruments)
- **Resolução:** 16 bits (sinal complemento de dois)
- **Faixa:** GAIN_ONE → ±4,096V
- **Endereço I²C:** 0x48 (ADDR conectado ao GND)
- **Alimentação:** VDD = 5V (tolera entradas analógicas até ~5,5V)

### 2.3 Mapa de Canais

| Canal | Sensor | Faixa de tensão | Grandeza medida |
|---|---|---|---|
| AIN0 | Turbidez TS-300B | 0 – 3,3V | NTU (regressão polinomial) |
| AIN1 | TDS Gravity Analog | 0 – 2,3V | ppm (compensação térmica) |
| AIN2 | pH PH4502C | ~1,0 – 4,0V | pH (calibração 2 retas) |
| AIN3 | Reserva | — | Disponível para expansão |

### 2.4 Equação de Conversão — Tensão Bruta

O ADC retorna um valor inteiro com sinal de 16 bits. A tensão correspondente é:

$$V = \text{raw} \times \frac{4{,}096}{32\,767}$$

Em código:

```cpp
// sensores.cpp — linhas 50–56
int16_t raw_ph   = ads.readADC_SingleEnded(2);
int16_t raw_tds  = ads.readADC_SingleEnded(1);
int16_t raw_turb = ads.readADC_SingleEnded(0);

float v_ph   = ads.computeVolts(raw_ph);
float v_tds  = ads.computeVolts(raw_tds);
float v_turb = ads.computeVolts(raw_turb);
```

---

## 3. Calibração de pH — 3 Pontos (Duas Retas)

### 3.1 Princípio

O sensor de pH PH4502C produce uma tensão que varia linearmente com o pH, mas com inclinações diferentes nas faixas ácida (pH < 7) e básica (pH > 7). A calibração determina dois pontos de referência (offset e inclinações) usando três soluções tampão.

### 3.2 Procedimento

| Passo | Solução tampão | pH conhecido | Variável | Ação do usuário |
|---|---|---|---|---|
| 1 | Tampão pH 7,00 | 7,00 | V7 | Mergulhar eletrodo → Confirmar |
| 2 | Tampão pH 4,00 | 4,00 | V4 | Mergulhar eletrodo → Confirmar |
| 3 | Tampão pH 10,01 | 10,01 | V10 | Mergulhar eletrodo → Confirmar → Salvar |

### 3.3 Equações de Calibração

**Inclinação da faixa ácida (pH 4,00 → 7,00):**

$$\text{slope}_{\text{ácido}} = \frac{7{,}0 - 4{,}0}{V_7 - V_4}$$

**Inclinação da faixa básica (pH 7,00 → 10,01):**

$$\text{slope}_{\text{básico}} = \frac{7{,}0 - 10{,}0}{V_7 - V_{10}}$$

**Ponto de referência (offset):**

$$\text{offset} = V_7$$

### 3.4 Equação de Medição

A equação de conversão utiliza a inclinação correspondente dependendo se a tensão medida está abaixo ou acima do offset:

$$\text{pH} = \begin{cases} \text{slope}_{\text{ácido}} \cdot (V - V_7) + 7{,}0 & \text{se } V < V_7 \\ \text{slope}_{\text{básico}} \cdot (V - V_7) + 7{,}0 & \text{se } V \geq V_7 \end{cases}$$

### 3.5 Código-fonte — Cálculo

```cpp
// sensores.cpp — linhas 22–28
float calcPH(float v) {
  if (v < calib.ph_offset) {
    return calib.ph_slope_acid * (v - calib.ph_offset) + 7.0f;
  } else {
    return calib.ph_slope_bas * (v - calib.ph_offset) + 7.0f;
  }
}
```

### 3.6 Código-fonte — Calibração (captura de tensões)

```cpp
// maquina.cpp — linhas 68–103 (trecho pH: calibSel == 0)
static void processarCalibracao() {
  int total = (calibSel == 1) ? 1 : 3;
  float vAtual = dados.ph;

  if (calibSel == 0) {                          // pH
    if (calibPasso == 0) calibV7  = vAtual;     // Passo 1: pH 7.00
    else if (calibPasso == 1) calibV4  = vAtual; // Passo 2: pH 4.00
    else if (calibPasso == 2) calibV10 = vAtual; // Passo 3: pH 10.01
  }
  // ... (após último passo)
  if (calibSel == 0) {
    calib.ph_offset    = calibV7;
    calib.ph_slope_acid = (7.0f - 4.0f) / (calibV7 - calibV4);
    calib.ph_slope_bas  = (7.0f - 10.0f) / (calibV7 - calibV10);
  }
}
```

---

## 4. Calibração de TDS — 1 Ponto + Compensação Térmica

### 4.1 Princípio

O sensor TDS Gravity Analog produce uma tensão proporcional à condutividade elétrica (EC) da solução. A temperatura afeta a condutividade, portanto é aplicada uma compensação térmica para referenciar a 25 °C. O fator de calibração `k` é determinado com uma solução padrão.

### 4.2 Procedimento

| Passo | Solução padrão | Condutividade conhecida | Ação |
|---|---|---|---|
| 1 | Solução 1413 µS/cm | 1413 µS/cm | Mergulhar sonda + DS18B20 → Confirmar |

### 4.3 Equações de Calibração

**Condutividade bruta (a partir da tensão):**

$$EC_{\text{bruto}} = \frac{V \times 1000}{2{,}3}$$

> O fator 2,3 corresponde à faixa máxima de tensão do sensor (2,3V = EC maximal).

**Condutividade referida a 25 °C:**

$$EC_{25} = \frac{EC_{\text{bruto}}}{1 + 0{,}02 \cdot (T - 25)}$$

> O coeficiente 0,02 é a variação relativa de EC por grau Celsius (~2%/°C para soluções aquosas diluídas).

**Fator de calibração TDS:**

$$k = \frac{1413}{EC_{25}}$$

### 4.4 Equação de Medição

$$\text{TDS}_{\text{ppm}} = \frac{EC_{\text{bruto}} \cdot k}{2 \cdot \left(1 + 0{,}02 \cdot (T - 25)\right)}$$

> O divisor 2 converte de µS/cm para ppm (relação aproximada 1:2 para soluções comuns).

### 4.5 Código-fonte — Cálculo

```cpp
// sensores.cpp — linhas 30–34
float calcTDS(float v, float temp) {
  float ec_bruto = v * 1000.0f / 2.3f;
  float ec_corr  = ec_bruto * calib.k_tds / (1.0f + 0.02f * (temp - 25.0f));
  return ec_corr / 2.0f;
}
```

### 4.6 Código-fonte — Calibração

```cpp
// maquina.cpp — linhas 89–91 (trecho TDS: calibSel == 1)
} else if (calibSel == 1) {
  float ec25 = (vAtual * 1000.0f / 2.3f) / (1.0f + 0.02f * (dados.temp_c - 25.0f));
  calib.k_tds = 1413.0f / ec25;
}
```

---

## 5. Calibração de Turbidez — 3 Pontos (Regressão Polinomial)

### 5.1 Princípio

O sensor de turbidez TS-300B produce uma tensão que varia de forma não-linear com a turbidez. A relação é aproximada por um polinômio de 2º grau. Três pontos de calibração (soluções com turbidez conhecida) permitem determinar os três coeficientes do polinômio.

### 5.2 Procedimento

| Passo | Solução | Turbidez conhecida | Variável | Ação |
|---|---|---|---|---|
| 1 | Água destilada | ~0 NTU | V0 | Mergulhar sensor → Confirmar |
| 2 | Argila diluída | ~50 NTU | V50 | Mergulhar sensor → Confirmar |
| 3 | Argila concentrada | ~500 NTU | V500 | Mergulhar sensor → Confirmar → Salvar |

### 5.3 Equações de Calibração

Dado três pontos $(x_0, y_0)$, $(x_1, y_1)$, $(x_2, y_2)$ onde $x$ é a tensão e $y$ é a turbidez conhecida:

$$x_0 = V_0, \quad y_0 = 0$$
$$x_1 = V_{50}, \quad y_1 = 50$$
$$x_2 = V_{500}, \quad y_2 = 500$$

Os coeficientes do polinômio $y = ax^2 + bx + c$ são calculados por interpolação:

**Coeficiente $a$ (quadrático):**

$$a = \frac{\dfrac{y_1 - y_0}{x_1 - x_0} - \dfrac{y_2 - y_0}{x_2 - x_0}}{x_1 - x_2}$$

**Coeficiente $b$ (linear):**

$$b = \frac{y_1 - y_0}{x_1 - x_0} - a \cdot (x_0 + x_1)$$

**Coeficiente $c$ (angular):**

$$c = y_0 - a \cdot x_0^2 - b \cdot x_0$$

### 5.4 Equação de Medição

$$\text{NTU} = a \cdot V^2 + b \cdot V + c$$

### 5.5 Código-fonte — Cálculo

```cpp
// sensores.cpp — linhas 36–38
float calcNTU(float v) {
  return calib.turb_a * v * v + calib.turb_b * v + calib.turb_c;
}
```

### 5.6 Código-fonte — Calibração

```cpp
// maquina.cpp — linhas 92–97 (trecho turbidez: calibSel == 2)
} else if (calibSel == 2) {
  float x0 = calibV0, x1 = calibV50, x2 = calibV500;
  float y0 = 0, y1 = 50, y2 = 500;
  calib.turb_a = ((y1 - y0) / (x1 - x0) - (y2 - y0) / (x2 - x0)) / (x1 - x2);
  calib.turb_b = (y1 - y0) / (x1 - x0) - calib.turb_a * (x0 + x1);
  calib.turb_c = y0 - calib.turb_a * x0 * x0 - calib.turb_b * x0;
}
```

---

## 6. Persistência de Calibração

### 6.1 Estratégia de Armazenamento

O sistema utiliza dois mecanismos de persistência para garantir a segurança dos dados:

```
┌──────────────────────────┐
│      NVS (Flash ESP32)   │  ← Primário: rápido, sobrevive reboot
│      Preferences API     │
└────────────┬─────────────┘
             │ fallback
┌────────────▼─────────────┐
│   SD Card (cal_backup.json) │  ← Backup: JSON via ArduinoJson
└──────────────────────────┘
```

### 6.2 Fluxo de Boot

```
carregarCalibracao()
  ├── prefs.begin("calibracao", true)  // modo leitura
  ├── if prefs.isKey("ph_offset")
  │     └── Carrega 7 coeficientes da NVS
  ├── else
  │     └── Abre /cal_backup.json no SD
  │           ├── deserializeJson(doc, f)
  │           └── Carrega 7 coeficientes do JSON
  └── Se nada encontrado → valores padrão (0, 1, 1, 1, 0, 0, 0)
```

### 6.3 Chaves NVS

| Chave NVS | Tipo | Valor padrão | Coeficiente |
|---|---|---|---|
| `ph_offset` | float | 0.0 | Tensão pH 7.00 |
| `ph_slope_acid` | float | 1.0 | Inclinação ácida |
| `ph_slope_bas` | float | 1.0 | Inclinação básica |
| `k_tds` | float | 1.0 | Fator TDS |
| `turb_a` | float | 0.0 | Coef. quadrático turbidez |
| `turb_b` | float | 0.0 | Coef. linear turbidez |
| `turb_c` | float | 0.0 | Coef. angular turbidez |

### 6.4 Formato do Backup JSON

Arquivo: `/cal_backup.json` no cartão SD.

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

### 6.5 Código-fonte — Leitura (NVS → SD → defaults)

```cpp
// nvs_cal.cpp — linhas 8–45
void carregarCalibracao() {
  prefs.begin("calibracao", true);

  if (prefs.isKey("ph_offset")) {
    calib.ph_offset     = prefs.getFloat("ph_offset",     0);
    calib.ph_slope_acid = prefs.getFloat("ph_slope_acid", 1);
    calib.ph_slope_bas  = prefs.getFloat("ph_slope_bas",  1);
    calib.k_tds         = prefs.getFloat("k_tds",         1);
    calib.turb_a        = prefs.getFloat("turb_a",        0);
    calib.turb_b        = prefs.getFloat("turb_b",        0);
    calib.turb_c        = prefs.getFloat("turb_c",        0);
    Serial.println("[NVS] Calibracao carregada da NVS");
  } else {
    Serial.println("[NVS] NVS vazia, tentando backup no SD...");
    File f = SD.open(CAL_BACKUP_FILE, FILE_READ);
    if (f) {
      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, f);
      f.close();
      if (!err) {
        calib.ph_offset     = doc["ph_offset"]     | 0.0f;
        calib.ph_slope_acid = doc["ph_slope_acid"] | 1.0f;
        calib.ph_slope_bas  = doc["ph_slope_bas"]  | 1.0f;
        calib.k_tds         = doc["k_tds"]         | 1.0f;
        calib.turb_a        = doc["turb_a"]        | 0.0f;
        calib.turb_b        = doc["turb_b"]        | 0.0f;
        calib.turb_c        = doc["turb_c"]        | 0.0f;
        Serial.println("[NVS] Calibracao restaurada do backup SD");
      } else {
        Serial.println("[NVS] Erro ao ler backup, usando valores padrao");
      }
    } else {
      Serial.println("[NVS] Sem backup, usando valores padrao");
    }
  }
  prefs.end();
}
```

### 6.6 Código-fonte — Escrita (NVS + SD)

```cpp
// nvs_cal.cpp — linhas 47–75
void salvarCalibracao() {
  prefs.begin("calibracao", false);

  prefs.putFloat("ph_offset",     calib.ph_offset);
  prefs.putFloat("ph_slope_acid", calib.ph_slope_acid);
  prefs.putFloat("ph_slope_bas",  calib.ph_slope_bas);
  prefs.putFloat("k_tds",         calib.k_tds);
  prefs.putFloat("turb_a",        calib.turb_a);
  prefs.putFloat("turb_b",        calib.turb_b);
  prefs.putFloat("turb_c",        calib.turb_c);

  prefs.end();
  Serial.println("[NVS] Calibracao salva na NVS");

  File f = SD.open(CAL_BACKUP_FILE, FILE_WRITE);
  if (f) {
    JsonDocument doc;
    doc["ph_offset"]     = calib.ph_offset;
    doc["ph_slope_acid"] = calib.ph_slope_acid;
    doc["ph_slope_bas"]  = calib.ph_slope_bas;
    doc["k_tds"]         = calib.k_tds;
    doc["turb_a"]        = calib.turb_a;
    doc["turb_b"]        = calib.turb_b;
    doc["turb_c"]        = calib.turb_c;
    serializeJson(doc, f);
    f.close();
    Serial.println("[NVS] Backup salvo no SD (cal_backup.json)");
  }
}
```

---

## 7. Leitura Completa dos Sensores

### 7.1 Fluxo de Leitura (a cada 2 segundos)

```
lerTodos()
  ├── sensors.requestTemperatures()
  ├── dados.temp_c = sensors.getTempCByIndex(0)        // DS18B20
  ├── raw_ph   = ads.readADC_SingleEnded(2)             // AIN2
  ├── raw_tds  = ads.readADC_SingleEnded(1)             // AIN1
  ├── raw_turb = ads.readADC_SingleEnded(0)             // AIN0
  ├── v_ph   = ads.computeVolts(raw_ph)
  ├── v_tds  = ads.computeVolts(raw_tds)
  ├── v_turb = ads.computeVolts(raw_turb)
  ├── dados.ph           = calcPH(v_ph)
  ├── dados.tds_ppm      = calcTDS(v_tds, dados.temp_c)
  ├── dados.turbidez_ntu = calcNTU(v_turb)
  └── verificarAlertas()   // compara com limites
```

### 7.2 Código-fonte — `lerTodos()`

```cpp
// sensores.cpp — linhas 46–69
void lerTodos() {
  sensors.requestTemperatures();
  dados.temp_c = sensors.getTempCByIndex(0);

  int16_t raw_ph   = ads.readADC_SingleEnded(2);
  int16_t raw_tds  = ads.readADC_SingleEnded(1);
  int16_t raw_turb = ads.readADC_SingleEnded(0);

  float v_ph   = ads.computeVolts(raw_ph);
  float v_tds  = ads.computeVolts(raw_tds);
  float v_turb = ads.computeVolts(raw_turb);

  dados.ph           = calcPH(v_ph);
  dados.tds_ppm      = calcTDS(v_tds, dados.temp_c);
  dados.turbidez_ntu = calcNTU(v_turb);

  verificarAlertas();

  Serial.printf("[LEITURA] pH:%.2f TDS:%.1fppm Turb:%.1fNTU Temp:%.1fC | %s %s %s\n",
                dados.ph, dados.tds_ppm, dados.turbidez_ntu, dados.temp_c,
                dados.ph_ok ? "pH_OK" : "pH_NOK",
                dados.tds_ok ? "TDS_OK" : "TDS_NOK",
                dados.turb_ok ? "TURB_OK" : "TURB_NOK");
}
```

### 7.3 Limites de Alerta

```cpp
// sensores.cpp — linhas 40–44
void verificarAlertas() {
  dados.ph_ok   = (dados.ph >= PH_MIN_DEFAULT && dados.ph <= PH_MAX_DEFAULT);
  dados.tds_ok  = (dados.tds_ppm <= TDS_LIMIT_DEFAULT);
  dados.turb_ok = (dados.turbidez_ntu <= TURB_LIMIT_DEFAULT);
}
```

| Parâmetro | Limite inferior | Limite superior | Definição |
|---|---|---|---|
| pH | 6,0 | 9,0 | `PH_MIN_DEFAULT` / `PH_MAX_DEFAULT` |
| TDS | — | 1000 ppm | `TDS_LIMIT_DEFAULT` |
| Turbidez | — | 100 NTU | `TURB_LIMIT_DEFAULT` |

---

## 8. Resumo das Equações (LaTeX para Relatório)

### 8.1 pH

$$\text{slope}_{\text{ácido}} = \frac{7{,}0 - 4{,}0}{V_7 - V_4}$$

$$\text{slope}_{\text{básico}} = \frac{7{,}0 - 10{,}0}{V_7 - V_{10}}$$

$$\text{pH} = \begin{cases} \text{slope}_{\text{ácido}} \cdot (V - V_7) + 7{,}0 & \text{se } V < V_7 \\ \text{slope}_{\text{básico}} \cdot (V - V_7) + 7{,}0 & \text{se } V \geq V_7 \end{cases}$$

### 8.2 TDS

$$EC_{\text{bruto}} = \frac{V \times 1000}{2{,}3}$$

$$EC_{25} = \frac{EC_{\text{bruto}}}{1 + 0{,}02 \cdot (T - 25)}$$

$$k = \frac{1413}{EC_{25}}$$

$$\text{TDS}_{\text{ppm}} = \frac{EC_{\text{bruto}} \cdot k}{2 \cdot \left(1 + 0{,}02 \cdot (T - 25)\right)}$$

### 8.3 Turbidez

$$a = \frac{\dfrac{y_1 - y_0}{x_1 - x_0} - \dfrac{y_2 - y_0}{x_2 - x_0}}{x_1 - x_2}$$

$$b = \frac{y_1 - y_0}{x_1 - x_0} - a \cdot (x_0 + x_1)$$

$$c = y_0 - a \cdot x_0^2 - b \cdot x_0$$

$$\text{NTU} = a \cdot V^2 + b \cdot V + c$$

### 8.4 Conversão ADC

$$V = \text{raw} \times \frac{4{,}096}{32\,767}$$

---

## 9. Equivalência: Código ↔ Equação

| Equação | Função no firmware | Arquivo | Linhas |
|---|---|---|---|
| Conversão ADC | `ads.computeVolts(raw)` | sensores.cpp | 54–56 |
| pH (ácido) | `calcPH(v)` — branch `v < offset` | sensores.cpp | 22–25 |
| pH (básico) | `calcPH(v)` — branch `v >= offset` | sensores.cpp | 25–27 |
| TDS completo | `calcTDS(v, temp)` | sensores.cpp | 30–34 |
| Turbidez | `calcNTU(v)` | sensores.cpp | 36–38 |
| slope_acid | `processarCalibracao()` — calibSel==0 | maquina.cpp | 87 |
| slope_bas | `processarCalibracao()` — calibSel==0 | maquina.cpp | 88 |
| k_tds | `processarCalibracao()` — calibSel==1 | maquina.cpp | 91 |
| turb_a, b, c | `processarCalibracao()` — calibSel==2 | maquina.cpp | 95–97 |
| Salvar NVS | `salvarCalibracao()` | nvs_cal.cpp | 47–75 |
| Carregar NVS | `carregarCalibracao()` | nvs_cal.cpp | 8–45 |
| Alertas | `verificarAlertas()` | sensores.cpp | 40–44 |

---

## 10. Dependências de Bibliotecas

| Biblioteca | Versão mínima | Uso na calibração |
|---|---|---|
| Adafruit ADS1X15 | ≥ 2.5.0 | Leitura do ADC 16-bit |
| OneWire | ≥ 2.3.7 | Comunicação DS18B20 |
| DallasTemperature | ≥ 3.9.0 | Leitura de temperatura |
| ArduinoJson | ≥ 6.21.0 | Backup JSON da calibração |
| Preferences | built-in ESP32 | NVS (armazenamento primário) |
| SD | built-in ESP32 | Cartão SD (backup secundário) |
