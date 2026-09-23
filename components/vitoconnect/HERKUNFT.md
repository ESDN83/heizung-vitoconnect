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
   zum ersten `0x05`; mit 1 s kann jedes neue `0x04` den Zyklus zuruecksetzen.
   Dieser Fix lief frueher als lokale Kopie und ging beim Wechsel auf das
   dannerph-Repo verloren.

   **Nicht** die Ursache fuer das "Einfrieren" der Werte (September 2026). Das war
   Queue-Verhungern: jede KW-Abfrage wartet auf das naechste `0x05` (ca. 3 s pro
   Datenpunkt), `update()` schiebt aber bei jedem `update_interval` die komplette
   Liste in die Queue (48 Plaetze). Mit 30 s und ~37 Datenpunkten lief die Queue
   voll, und nur noch die ersten ~10 Datenpunkte der Liste kamen dran. Loesung in
   der YAML: `update_interval` groesser als Anzahl Datenpunkte x 3 s (jetzt 150 s).
   Belegt per DEBUG-Log vom 23.09.2026.
2. `vitoconnect.cpp`: `check_uart_settings()` entfernt (deprecated, entfaellt mit
   ESPHome 2027.3.0), dafuer `FINAL_VALIDATE_SCHEMA` in `__init__.py` (4800 8E2,
   TX und RX Pflicht).
3. `vitoconnect_optolink.cpp/.h`: die Ueberladungen `onData(void(*)(uint8_t*, uint8_t))`
   und `onError(void(*)(uint8_t))` entfernt. Sie wurden nirgends benutzt und haetten
   per `reinterpret_cast` eine Funktion mit falscher Signatur aufgerufen
   (`-Wcast-function-type`, Zeilen 42 und 50).
4. `vitoconnect_optolinkDP.h/.cpp`: eigener `operator=` (Rule of Three). Die Queue
   nutzte vorher den impliziten, der bei Schreib-Datenpunkten nur den `data`-Zeiger
   kopiert haette (`-Wdeprecated-copy` in `vitoconnect_simpleQueue.h:93`, im
   Ernstfall doppeltes `delete[]`).

## Hinweis zum Geraet

Die Regelung meldet unter `0x00F8` die ID **0x20CB (VScotHO1)**, nicht 0x2098
(V200KW2). Sie spricht das KW-Protokoll, hat aber die Adressbelegung der neueren
Generation. Passende Adressen: openv `vcontrold/xml/300/vito.xml`, Geraet 20CB.
