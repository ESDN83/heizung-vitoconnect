# Herkunft und Aenderungen

Kopie von [dannerph/esphome_vitoconnect](https://github.com/dannerph/esphome_vitoconnect),
Stand Commit `4485924dfcdccb94db852d10606511b96dab4545` (master, 05.01.2026).
Lizenz: MIT, siehe `LICENSE` in diesem Ordner.

Eingebunden in `heizung-vitoconnect.yaml` ueber:

```yaml
external_components:
  - source: github://ESDN83/heizung-vitoconnect@stand-2026-09-23-selbstheilung
    components: [vitoconnect]
```

## Aenderungen gegenueber dannerph (23.09.2026)

1. `vitoconnect_optolinkKW.cpp`, `_init()`: Reset-Intervall `1000UL` auf `3000UL`.
   Die Vitotronic 200 KW2 (Device 0x2098) braucht nach einem `0x04` 2 bis 3 s bis
   zum ersten `0x05`. Mit 1 s setzt jedes neue `0x04` den Zyklus zurueck, nach dem
   ersten Aussetzer kommt das Component nie mehr aus INIT heraus. Symptom: nach
   jedem Start 2 bis 3 Minuten Werte, danach eingefroren, ESP bleibt erreichbar,
   Schreiben funktioniert weiter. Dieser Fix lief frueher als lokale Kopie und
   ging beim Wechsel auf das dannerph-Repo verloren.
2. `vitoconnect.cpp`: `check_uart_settings()` entfernt (deprecated, entfaellt mit
   ESPHome 2027.3.0), dafuer `FINAL_VALIDATE_SCHEMA` in `__init__.py` (4800 8E2,
   TX und RX Pflicht).
