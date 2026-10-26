# IHM — Monitor Portátil de Qualidade da Água

> **Projeto:** Monitor Portátil de Qualidade da Água  
> **Disciplina:** Instrumentação Industrial II — UFMT  
> **Plataforma:** ESP32 DevKit V3 + TFT ST7789 2.0" (Adafruit ST7789, landscape 320×240)  
> **Navegação:** 3 botões físicos (Voltar + Navegar + Confirmar)

---

## 1. Botões físicos

| Botão | GPIO | Função |
|---|---|---|
| Navegar | GPIO 32 | Percorre itens do menu (circular, cima/baixo) |
| Confirmar | GPIO 33 | Seleciona item / salva valor / abre menu |
| Voltar | GPIO 25 | Retorna ao nível anterior em qualquer tela |

**Ligação elétrica:** um terminal no GPIO, outro no GND.  
**Configuração:** `INPUT_PULLUP` por software — sem resistor externo necessário.  
**Debounce:** 20 ms por software.

> GPIOs 32, 33 e 25 são ADC/TOUCH livres — sem conflito com SPI (TFT/SD), I²C (ADS1115) ou 1-Wire (DS18B20).

---

## 2. Regras de navegação

- `Navegar` percorre os itens do menu para baixo. Ao chegar no último item, volta para o primeiro (comportamento circular).
- `Confirmar` entra na opção selecionada, salva um valor editado ou, na tela de monitoramento, abre o menu principal.
- `Voltar` sobe um nível em qualquer tela. Na tela principal de monitoramento, não faz nada (já é o nível raiz).
- Em telas de edição de valor (Configurações), `Navegar` incrementa/decrementa o valor selecionado em vez de mover o cursor — o item em edição é destacado com `◀▶`.

---

## 3. Mapa de estados

```
[BOOT]
  └─→ Tela 1: Monitoramento  ← tela principal, sempre ativa ao ligar

[Tela 1 — Monitoramento]
  Confirmar → Tela 2: Menu principal
  Navegar   → (sem efeito na tela principal)
  Voltar    → (sem efeito — nível raiz)

[Tela 2 — Menu principal]
  Navegar   → seleciona item (circular)
  Confirmar → entra no item selecionado
  Voltar    → Tela 1: Monitoramento

  Itens do menu:
  ├─ Calibração   → Tela 3: Submenu Calibração
  ├─ Histórico    → Tela 5: Histórico
  ├─ Configurações→ Tela 6: Configurações
  └─ Registrar    → Tela 4: Registro de amostra

[Tela 3 — Submenu Calibração]
  Navegar   → seleciona sensor (pH / TDS / Turbidez)
  Confirmar → inicia fluxo passo a passo do sensor
  Voltar    → Tela 2: Menu principal

[Tela 3a — Calibração pH (passo a passo, 3 etapas)]
  Passo 1: pH 7,00 → aguarda leitura estável → Confirmar grava V7
  Passo 2: pH 4,00 → aguarda leitura estável → Confirmar grava V4
  Passo 3: pH 10,01→ aguarda leitura estável → Confirmar grava V10
  → Calcula slope_acid e slope_basic → salva na NVS → exibe SALVO
  Voltar (qualquer passo) → cancela e retorna ao submenu

[Tela 3b — Calibração TDS (1 ponto)]
  Passo 1: solução 1413 µS/cm + DS18B20 → Confirmar grava k_tds → salva NVS
  Voltar → cancela

[Tela 3c — Calibração Turbidez (3 amostras empíricas)]
  Passo 1: água destilada (~0 NTU)     → Confirmar grava V0
  Passo 2: argila diluída (~50 NTU)    → Confirmar grava V50
  Passo 3: argila concentrada (~500 NTU)→ Confirmar grava V500
  → Calcula regressão a, b, c → salva NVS → exibe SALVO
  Voltar → cancela

[Tela 4 — Registro de amostra]
  Navegar   → seleciona rótulo da amostra
  Confirmar → grava snapshot no SD (CSV) com timestamp
  Voltar    → cancela, volta ao menu

  Rótulos disponíveis:
  ├─ Água destilada
  ├─ Água torneira
  ├─ Rio / córrego
  ├─ Amostra 1
  └─ Amostra 2

[Tela 5 — Histórico]
  Navegar   → percorre registros do CSV
  Confirmar → exibe detalhe do registro selecionado
  Voltar    → Tela 2: Menu principal

[Tela 6 — Configurações]
  Navegar   → seleciona parâmetro / incrementa valor em edição
  Confirmar → entra em modo edição do parâmetro selecionado / salva
  Voltar    → Tela 2: Menu principal (sem salvar se não confirmado)

  Parâmetros configuráveis:
  ├─ Limite pH mín     (padrão: 6,0)
  ├─ Limite pH máx     (padrão: 9,0)
  ├─ Limite TDS        (padrão: 1000 ppm)
  ├─ Limite turbidez   (padrão: 100 NTU)
  ├─ Intervalo leitura (padrão: 2 s)
  └─ Nome arquivo log  (padrão: agua_log.csv)
```

---

## 4. Descrição de cada tela

### Tela 1 — Monitoramento (tela principal)

Exibe os quatro parâmetros em tempo real, atualizados a cada intervalo configurado (padrão 2 s).

```
┌─────────────────────────────────┐
│ Monitor Qualidade Água        ● │  ← barra azul escura
├─────────────────┬───────────────┤
│ pH              │ TDS           │
│ 7.02            │ 1142 ppm      │  ← TDS em vermelho (acima do limite)
├─────────────────┼───────────────┤
│ Turbidez        │ Temp          │
│ 4.1 NTU         │ 24.3 °C       │
├─────────────────────────────────┤
│ ⚠ TDS acima do limite (1000 ppm)│  ← alerta visível
├─────────────────┬───────────────┤
│   Registrar     │   Menu ▶      │
└─────────────────┴───────────────┘
```

**Alertas visuais:**
- Valor dentro do limite → texto verde
- Valor fora do limite → texto vermelho
- Mensagem de alerta na linha abaixo dos parâmetros

**Ações:**
- `Confirmar` → abre Menu principal
- `Navegar` (pressionamento longo) → aciona Registro rápido direto

---

### Tela 2 — Menu principal

```
┌─────────────────────────────────┐
│ Menu                            │
├─────────────────────────────────┤
│ ◉ Calibração                  › │  ← item selecionado (destacado)
│ ○ Histórico                   › │
│ ○ Configurações               › │
│ ○ Registrar amostra           › │
├─────────────────┬───────────────┤
│   ◀ Voltar      │   Entrar ▶    │
└─────────────────┴───────────────┘
```

---

### Tela 3 — Calibração (passo a passo)

Barra de progresso indica etapa atual. O sistema aguarda leitura estável antes de habilitar `Confirmar`.

```
┌─────────────────────────────────┐
│ Calibração — pH              2/3│
├─────────────────────────────────┤
│ ▓▓▓▓▓▓▓▓▓▓▓▓▓░░░░░░░  66%      │  ← barra de progresso
├─────────────────────────────────┤
│ Passo 2 de 3 — pH 4,00          │
│ Mergulhe e aguarde...           │
│ V4 = 3.812 V  ✓                 │  ← leitura confirmada (verde)
├─────────────────────────────────┤
│ Próximo — pH 10,01              │
│ Aguardando confirmação          │
├─────────────────┬───────────────┤
│   ✕ Cancelar    │   Próximo ▶   │
└─────────────────┴───────────────┘
```

---

### Tela 4 — Registro de amostra

```
┌─────────────────────────────────┐
│ Registrar Amostra               │
├─────────────────────────────────┤
│ Selecione o rótulo:             │
│ ○ Água destilada                │
│ ◉ Água torneira                 │  ← selecionado
│ ○ Rio / córrego                 │
│ ○ Amostra 1                     │
│ ○ Amostra 2                     │
├─────────────────┬───────────────┤
│   ✕ Cancelar    │   Salvar ▶    │
└─────────────────┴───────────────┘
```

**Ao salvar:** grava linha no CSV com timestamp + rótulo + valores atuais.  
**Formato CSV:** `timestamp, etiqueta, pH, TDS_ppm, turbidez_NTU, temperatura_C`

---

### Tela 5 — Histórico

```
┌─────────────────────────────────┐
│ Histórico — agua_log.csv        │
├─────────────────────────────────┤
│ 14:32 15/06 │ Torneira │ pH 7.02│
│ 10:15 15/06 │ Rio      │ pH 6.12│  ← em vermelho (fora do limite)
│ 08:00 15/06 │ Destilada│ pH 7.00│
│ 17:44 14/06 │ Torneira │ pH 7.21│
│ 09:10 14/06 │ Amostra 1│ pH 5.80│
├─────────────────┬───────────────┤
│   ◀ Voltar      │   Ver ▶       │
└─────────────────┴───────────────┘
```

---

### Tela 6 — Configurações

O item em edição fica com destaque `◀▶`. `Navegar` incrementa/decrementa o valor.

```
┌─────────────────────────────────┐
│ Configurações                   │
├─────────────────────────────────┤
│ Limite pH mín       │ 6.0       │
│ Limite pH máx       │ 9.0       │
│ Limite TDS          │◀1000 ppm▶ │  ← em edição
│ Limite turbidez     │ 100 NTU   │
│ Intervalo leitura   │ 2 s       │
│ Arquivo log         │agua_log   │
├─────────────────┬───────────────┤
│   ◀ Voltar      │   Salvar ▶    │
└─────────────────┴───────────────┘
```

---

## 5. Persistência

| Dado | Onde fica | Sobrevive a |
|---|---|---|
| Coeficientes de calibração | NVS (flash ESP32) | Troca do SD card, reboot |
| Limites de alerta | NVS (flash ESP32) | Troca do SD card, reboot |
| Intervalo de leitura | NVS (flash ESP32) | Troca do SD card, reboot |
| Registros de amostras | SD card (`agua_log.csv`) | Reboot |

---

## 6. Lógica de firmware — máquina de estados dos botões

```cpp
// Estados do sistema
enum Estado {
  MONITORAMENTO,
  MENU_PRINCIPAL,
  CALIBRACAO_MENU,
  CALIBRACAO_FLUXO,
  REGISTRO,
  HISTORICO,
  CONFIGURACOES
};

// Botões (INPUT_PULLUP — LOW = pressionado)
#define BTN_NAVEGAR   32
#define BTN_CONFIRMAR 33
#define BTN_VOLTAR    25

// Debounce
#define DEBOUNCE_MS 20

bool lerBotao(int pino) {
  if (digitalRead(pino) == LOW) {
    delay(DEBOUNCE_MS);
    return digitalRead(pino) == LOW;
  }
  return false;
}

// Loop principal
void loop() {
  // Atualiza leituras a cada 2s
  if (millis() - ultimaLeitura > intervaloLeitura) {
    lerSensores();
    atualizarDisplay();
    ultimaLeitura = millis();
  }

  // Verifica botões
  if (lerBotao(BTN_NAVEGAR))   acaoNavegar();
  if (lerBotao(BTN_CONFIRMAR)) acaoConfirmar();
  if (lerBotao(BTN_VOLTAR))    acaoVoltar();
}
```

---

## 7. Checklist de implementação

- [ ] Inicializar GPIOs 32, 33, 25 como `INPUT_PULLUP`
- [ ] Implementar debounce por software (20 ms)
- [ ] Implementar máquina de estados com enum `Estado`
- [ ] Tela de monitoramento: atualização a cada 2 s com alertas visuais
- [ ] Menu principal: navegação circular entre 4 itens
- [ ] Calibração pH: fluxo de 3 passos com progresso e NVS
- [ ] Calibração TDS: 1 ponto + NVS
- [ ] Calibração turbidez: 3 amostras + regressão + NVS
- [ ] Registro: seleção de rótulo + append no CSV do SD
- [ ] Histórico: leitura do CSV e exibição paginada
- [ ] Configurações: edição inline de limites com NVS
- [ ] Carregar coeficientes da NVS no boot
