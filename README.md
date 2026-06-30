# TeleCafezinho

Firmware ESP-IDF para o produto TeleCafezinho. O app usa os componentes
TeleSystem como base de conectividade, portal, OTA, status, comandos, MQTT e
indicadores, e adiciona a logica de dominio local em
`components/tele_cafezinho`.

## O que este firmware faz

- Le um sinal local ativo/inativo, inicialmente por GPIO.
- Publica o sinal em um topico MQTT compartilhado do TeleSystem.
- Mantem um cache de peers ativos no mesmo grupo.
- Combina estado local e remoto em `idle`, `local_active`, `remote_active` ou
  `mutual_active`.
- Expoe configuracoes, status e comandos via registries TeleSystem.
- Aciona LEDs por eventos semanticos em `tele_indicator`, sem chamar
  `status_led` diretamente.

## Arquitetura

O app principal fica em `main/` e inicializa energia, NVS, OTA, portal, Wi-Fi,
NTP, indicadores, TeleCafezinho e presence MQTT.

`components/tele_cafezinho` contem a logica especifica do produto:

- `tele_cafezinho.c`: ciclo de vida, estado local/remoto e eventos LED.
- `tele_cafezinho_config.c`: campos `telecafe.*` em `tele_config`.
- `tele_cafezinho_status.c`: campos `telecafe.*` em `tele_status`.
- `tele_cafezinho_mqtt.c`: payload compartilhado e publish/subscribe MQTT.
- `tele_cafezinho_peers.c`: cache fixo de peers e expiracao por TTL.
- `tele_cafezinho_gpio.c`: entrada GPIO com polling e debounce.
- `tele_cafezinho_commands.c`: comandos de teste e operacao.

`tele_mqtt` continua generico. A unica expansao necessaria para este produto e
o getter `tele_mqtt_get_device_id()`, usado para preencher e filtrar payloads.

## Build

Na raiz do repositorio:

```bash
idf.py build
```

Se o diretorio `build/` tiver cache de outro caminho, use um build separado:

```bash
idf.py -B /tmp/telecafezinho-idf-build build
```

## Menuconfig

Abra:

```bash
idf.py menuconfig
```

Configuracoes principais:

- `TeleCafezinho Domain Config`
  - `Default group`: grupo inicial, default `default`.
  - `Default signal source`: fonte inicial, default `gpio`.
  - `Default signal GPIO number`: GPIO de entrada; `-1` desabilita.
  - `Default GPIO active level`: `1` ativo-alto, `0` ativo-baixo.
  - `Default GPIO debounce time`: debounce inicial em ms.
  - `Default shared signal TTL`: TTL de peers remotos.
  - `Default active republish interval`: republicacao enquanto ativo.
  - `Maximum active peer cache entries`: tamanho do cache de peers.
- `Tele MQTT Core Config`: shared topics e limites do core MQTT.
- `MQTT Presence Config`: broker, base topic e device ID prefix.
- `POWER_GOOD GPIO Config` e `VBAT Monitor Config`: energia e bateria.

Os defaults do menuconfig alimentam os campos runtime `telecafe.*`. Depois do
boot, esses campos tambem podem ser alterados via `tele_config` e persistidos em
NVS.

## Configuracoes TeleCafezinho

| Config ID | Tipo | Default |
|---|---:|---|
| `telecafe.group` | string | `CONFIG_TELE_CAFEZINHO_DEFAULT_GROUP` |
| `telecafe.signal_source` | string | `CONFIG_TELE_CAFEZINHO_DEFAULT_SIGNAL_SOURCE` |
| `telecafe.gpio_num` | i32 | `CONFIG_TELE_CAFEZINHO_DEFAULT_GPIO_NUM` |
| `telecafe.gpio_active_level` | i32 | `CONFIG_TELE_CAFEZINHO_DEFAULT_GPIO_ACTIVE_LEVEL` |
| `telecafe.gpio_debounce_ms` | u32 | `CONFIG_TELE_CAFEZINHO_DEFAULT_GPIO_DEBOUNCE_MS` |
| `telecafe.signal_ttl_s` | u32 | `CONFIG_TELE_CAFEZINHO_DEFAULT_SIGNAL_TTL_S` |
| `telecafe.signal_publish_interval_s` | u32 | `CONFIG_TELE_CAFEZINHO_DEFAULT_SIGNAL_PUBLISH_INTERVAL_S` |

## MQTT compartilhado

Topico:

```text
{base_topic}/_shared/telecafezinho/signal
```

Payload:

```json
{
  "schema": "telecafezinho.signal.v1",
  "group": "default",
  "device_id": "TCafe-A1B2C3",
  "signal": "active",
  "source": "gpio",
  "seq": 42,
  "ttl_s": 20
}
```

Mensagens de outro grupo sao ignoradas. Mensagens do proprio `device_id` tambem
sao ignoradas. Peers ativos expiram quando o TTL vence sem republicacao.

## Comandos

- `telecafe/simulate_signal`: define um sinal local simulado.
- `telecafe/clear_peers`: limpa peers remotos.
- `telecafe/get_peers`: retorna o cache de peers.

Exemplo:

```json
{
  "name": "telecafe/simulate_signal",
  "cmd_id": "tc-sim-1",
  "args": {
    "active": true,
    "duration_s": 10
  }
}
```

## Documentacao

Veja tambem:

- `docs/tele_cafezinho.md`
- `docs/arquitetura_index.md`
