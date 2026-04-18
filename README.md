# Heizung Vitoconnect

ESPHome-basierter Ersatz für den Viessmann Vitoconnect — liest und schreibt die Heizungsparameter
per Optolink über einen ESP32 mit Ethernet und bindet sie in Home Assistant ein.

## Überblick

- **Heizung:** Viessmann Vitodens mit Vitotronic 200 KW2 (Device-ID `0x2098`, KW-Protokoll)
- **Microcontroller:** WT32-ETH01 (ESP32 + LAN8720A Ethernet-PHY)
- **Interface:** Optolink (optische serielle Schnittstelle am Heizungs-Display)
- **Integration:** Home Assistant via ESPHome API + Ethernet
- **Fallback:** Wenn HA > 1 Stunde offline und Außentemperatur < 15°C, schaltet das Modul
  selbständig beide Heizkreise auf "WW + Heizung", damit das Haus nicht kalt wird.

<img width="1243" height="1555" alt="grafik" src="https://github.com/user-attachments/assets/5fb8ad2a-dad2-407b-b7d2-0336b37e8e98" />


## Hardware

### Board

- **WT32-ETH01** (Standard-Variante, ESP32-D0WD-V3, 4 MB Flash, LAN8720A PHY, 100 Mbit/s Ethernet)
- Versorgung: **3.3 V** am `3V3`-Pin (bei 5 V-Quelle einen Spannungsregler vorsehen)
- Kein USB-Port auf dem Board — zum Flashen wird ein externer USB-UART-Adapter an die
  UART0-Pins benötigt.

### Optolink-Schaltung

```
TX-Pfad:
  ESP32 GPIO2 ── 180 Ω ── SFH 487-3 Kathode (–)
                           SFH 487-3 Anode (+) ── 3.3 V

RX-Pfad:
  3.3 V ── 10 kΩ ── ESP32 GPIO4
                  └── SFH 309 FA Collector
                      SFH 309 FA Emitter ── GND
```

Bauteile:

| Bauteil | Wert/Typ | Funktion |
|---------|----------|----------|
| IR-LED | SFH 487-3 (880 nm) | Sender (TX) |
| Phototransistor | SFH 309 FA (NPN, 730–1120 nm, 24°) | Empfänger (RX) |
| Widerstand TX | 180 Ω | Strombegrenzung der IR-LED |
| Widerstand RX | 10 kΩ | Pull-up am Phototransistor |

UART-Parameter (fest vom KW-Protokoll vorgegeben):
`4800 Baud, 8 Datenbits, EVEN Parität, 2 Stoppbits`

### Ethernet-Pinbelegung (WT32-ETH01 LAN8720)

| Pin | Funktion |
|------|----------|
| GPIO23 | MDC |
| GPIO18 | MDIO |
| GPIO0 | CLK_EXT_IN |
| GPIO16 | PHY Power / Oscillator Enable |
| PHY-Adresse | 1 |

### Mechanischer Aufbau

- **Gehäuse:** Standard-Elektro-Stromverteilerdose (Aufputz)
- **Stromversorgung:** USB-Netzteil an einer Steckdose, Versorgung direkt auf 5 V-Eingang
  des WT32-ETH01
- **Ethernet:** Kabel von der Verteilerdose zum nächsten Switch
- **Optolink-Kopf:** 3D-gedruckter Adapter, sitzt direkt in der Optolink-Mulde der Vitotronic-Front
  (die runde Öffnung, in die sonst der originale Vitoconnect gesteckt wird). Die beiden Dioden
  sitzen im 3D-Druck und leuchten in die Vitotronic-Optik.

## Software

### Abhängigkeiten

- ESPHome 2026.4.0 oder neuer
- Externes Component: [`dannerph/esphome_vitoconnect`](https://github.com/dannerph/esphome_vitoconnect)
  (wird automatisch über `external_components` aus GitHub geladen — kein lokaler Ordner nötig)

### Installation

1. In der zentralen `/config/esphome/secrets.yaml` auf dem HA-Host zwei projektspezifische
   Einträge ergänzen (siehe `secrets.yaml.example`):
   - `heizung_api_encryption_key`
   - `heizung_ota_password`

   Projektspezifische Namen sind nötig, weil andere ESPHome-Geräte im selben Haushalt
   dieselbe `secrets.yaml` nutzen, aber mit anderen Credentials laufen.
2. `heizung-vitoconnect.yaml` in `/config/esphome/` ablegen (oder direkt im ESPHome-Dashboard
   importieren)
3. Initial-Flash über USB-UART-Adapter (siehe unten), danach läuft alles Weitere per OTA
   über Ethernet
4. In Home Assistant wird das Gerät automatisch via ESPHome-Integration entdeckt

### Erstmaliges Flashen (Initial-Flash)

Das WT32-ETH01 hat keinen USB-Port. Verkabelung mit einem externen USB-UART-Adapter
(CH340, CP2102 oder FTDI):

| USB-UART | WT32-ETH01 |
|----------|------------|
| TX | RX0 |
| RX | TX0 |
| GND | GND |
| 5V oder 3.3V | 5V bzw. 3V3 |

Zum Flashen `IO0` auf `GND` ziehen (Boot-Modus), dann `EN` kurz auf `GND` legen (Reset).
Anschließend ESPHome kompilieren und flashen, z. B. per ESPHome-Dashboard oder
`esphome run heizung-vitoconnect.yaml`. Ab dem ersten erfolgreichen Flash läuft alles
Weitere per OTA über Ethernet.

## Home-Assistant-Entities

Das Modul stellt u. a. folgende Entities bereit (vollständige Liste in `heizung-vitoconnect.yaml`):

- **Temperaturen:** Außen (normal + gedämpft), Kessel (ist + soll), Warmwasser, Abgas,
  Vorlauf M1/M2 (ist + soll), Rücklauf, Speicher
- **Brenner:** Starts, Betriebsstunden, Leistung
- **Mischer:** M1, M2 (Stellung in %)
- **Binäre Sensoren:** Brenner, Umwälzpumpen M1/M2, Speicherlade- und Zirkulationspumpe, Störung
- **Selects:** Betriebsart M1/M2 (Abschalt / nur WW / WW+Heizung)
- **Numbers:** WW-Soll, Raumsoll normal/reduziert M1/M2, Party-Temperatur M1/M2,
  Heizkurve Neigung und Niveau für M1/M2
- **Switches:** Spar- und Partybetrieb M1/M2, Zirkulationspumpe

## Besonderheiten der Config

Ein paar Dinge, die beim Nachbau gerne übersehen werden:

- **UART0-Logger deaktiviert** (`baud_rate: 0`) — sonst redet der ESPHome-Logger über dieselben
  Pins wie das Optolink-UART.
- **Heizkurven-Adressen:** `0x27D3`/`0x27D4` (M1) und `0x37D3`/`0x37D4` (M2) statt der in älteren
  Doku-Quellen genannten `0x2305`/`0x2304` — letztere enthielten bei dieser Anlage veraltete Werte.
- **Schreibende Operationen** (Select/Number/Switch) gehen nicht über die `vitoconnect`-Component,
  sondern als Raw-UART-Lambdas direkt in den UART. Pattern: Sync-Byte `0x04` senden, auf
  ACK `0x05` warten (bis zu 2 s), dann Schreib-Frame senden. Ohne dieses ACK-Warten werden
  Writes nicht akzeptiert.
- **Strapping-Pin-Warnungen** für GPIO0/GPIO2 sind zu erwarten und unkritisch — GPIO0 ist der
  Ethernet-Takt und GPIO2 das Optolink-TX.
- **Fallback-Logik:** Implementiert über einen 60-s-Interval-Check. Nur aktiv wenn die API
  länger als eine Stunde keinen Kontakt zu HA hatte *und* die gemessene Außentemperatur
  unter 15 °C liegt — verhindert Einfrieren bei HA-Ausfall.

## Referenzen

- [dannerph/esphome_vitoconnect](https://github.com/dannerph/esphome_vitoconnect) — ESPHome-Component
  (wird als `external_components` geladen)
- [JuergenLeber/home-assistant-optolink](https://github.com/JuergenLeber/home-assistant-optolink) —
  Schwesterprojekt mit WT32-ETH01 EVO (ESP32-C3) und PoE
- [openv/openv Wiki](https://github.com/openv/openv) — Community-Wiki zum Optolink-Protokoll,
  Bauanleitungen für Adapter
- [bertmelis/VitoWiFi](https://github.com/bertmelis/VitoWiFi) — Arduino-Library, Basis von
  `esphome_vitoconnect`
