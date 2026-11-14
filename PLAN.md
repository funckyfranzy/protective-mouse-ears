# Protective Mouse/Cat Ears – Plan

Blinkende (bzw. leuchtende) Protective Ears für den **40C3**, betrieben mit einer Powerbank.
Muster werden per **WLAN vom Handy** gewechselt (kein Taster nötig).

## Hardware

- **Mikrocontroller:** ESP32-C3 Super Mini (USB-C), am Laptop erkannt als `/dev/ttyACM0`
  - Rotes Licht = nur „Strom ist da“
- **LEDs:** WS2812B-Streifen, **160 LEDs/m** (noch unterwegs)
  - Läuft **durchgehend über den ganzen Haarreif**
  - Zum Testen vorhanden: breitere LED-Streifen (Typ noch prüfen: WS2812B? WS2811? SK6812?)
- **Strom:** Powerbank über USB-C an die Platine

### Verkabelung

| Streifen | ESP32-C3 Super Mini |
|---|---|
| 5V  | 5V |
| GND | G |
| DIN | GPIO 4 (optional 330-Ω-Widerstand dazwischen) |

- Am Ende des Streifens anlöten, von dem die **Pfeile wegzeigen** (Datenrichtung).
- 5V- und GND-Kabel **dicker** als die Datenleitung.

```
Powerbank ──USB-C──► ESP32-C3 ──5V/GND/DIN──► [Ohr 1 | Mittelstück | Ohr 2]  (ein durchgehender Streifen)
```

### Strom-Hinweise

- 160 LEDs/m → über den Haarreif ca. 80–120 LEDs → bei vollem Weiß 5–7 A. Das ist **zu viel**.
- Lösung: WLED-**Strombegrenzer auf ca. 1 A**. WLED dimmt dann automatisch.
- Manche Powerbanks schalten bei zu wenig Stromverbrauch ab, also vorher testen.

## Software

> **Update:** WLED nur zum Testen. Für den Congress kommt eine **eigene Firmware**,
> Anforderungen in [ANFORDERUNGEN.md](ANFORDERUNGEN.md).

### WLED (Test)

- Fertige Open-Source-Firmware mit über 100 Effekten, Paletten und Helligkeitsregelung
- ESP32 macht ein **eigenes WLAN** auf → Steuerung per Handy-Browser/App, auch ohne Internet
- Streifen wird in **Segmente** aufgeteilt:

| Segment | Start-LED | End-LED |
|---|---|---|
| Ohr 1 | ? | ? |
| Mittelstück | ? | ? |
| Ohr 2 | ? | ? |

→ Zahlen eintragen, sobald der endgültige Streifen da ist und aufgeklebt ist.

## Arbeitsteilung

- **Claude:** Werkzeuge installieren (`esptool`), WLED herunterladen und flashen, konfigurieren, Fehlersuche
- **Franzi:** Löten, Verkabeln, LEDs angucken und berichten

## Status / To-do

- [x] `curl` installiert
- [x] `sudo apt install -y python3-venv`
- [x] `sudo usermod -aG dialout $USER` (Zugriff auf USB-Schnittstelle)
- [x] **Danach ab- und wieder anmelden** (oder neu starten), sonst wirkt die Gruppe nicht
  - Prüfen mit `id`: dort muss `dialout` auftauchen
- [x] Typ des Test-Streifens: steht nicht drauf → wir nehmen WS2812B an
- [ ] Teststück (5–10 LEDs) anlöten (siehe „Test-Verkabelung“ unten)
- [x] Claude: `esptool` in `.venv/` hier im Projektordner installiert
- [x] Claude: WLED 16.0.1 für ESP32-C3 geflasht (Dateien in `firmware/`), WLAN „WLED-AP“ ist sichtbar
- [ ] WLED einrichten: LED-Pin GPIO 4, LED-Typ, Anzahl, Strombegrenzer ca. 1 A
- [ ] Mit dem Handy ins WLED-WLAN, erstes Leuchten testen
- [ ] Endgültiger Streifen da: aufkleben, LEDs zählen, Segmente eintragen
- [ ] Lieblingsmuster als Presets speichern
- [ ] Powerbank-Test (schaltet sie ab? wie lange hält sie?)

## Test-Verkabelung (Schritt für Schritt)

Du brauchst: Teststück Streifen (5–10 LEDs), 3 Litzen (~10 cm), optional 330-Ω-Widerstand,
optional Elko 470–1000 µF / ≥6,3 V.

1. **Platine vom USB abziehen.** Nie unter Strom löten.
2. Streifen-Ende finden, an dem die Pfeile **vom Ende weg** zeigen. Dort sind die Pads 5V / DIN / GND.
3. Drei Litzen an den Streifen löten. Farben am besten so:
   - **rot** → 5V
   - **schwarz** → GND
   - **gelb/grün** → DIN
4. Andere Enden an den ESP32-C3 Super Mini löten:

   | Litze | Pin auf der Platine | Hinweis |
   |---|---|---|
   | rot (5V) | **5V** | nicht 3.3! |
   | schwarz (GND) | **G** | |
   | gelb (DIN) | **4** | optional mit 330 Ω in Reihe, möglichst nah an der Platine |

5. Optional: Elko direkt am Streifen zwischen 5V und GND (**Polung beachten**: Minus-Streifen am Elko → GND).
   Schützt die LEDs vor Spannungsspitzen beim Einstecken.
6. Vor dem Einstecken **Sichtprüfung**: keine Lötbrücken, 5V und GND nicht vertauscht.
7. USB-C an den Laptop. Bei 5–10 LEDs und gedrosselter Helligkeit reicht der USB-Port vom Laptop.

Pins **2, 8, 9** nicht für die LEDs benutzen (Strapping-Pins, stören beim Starten).

### Variante: ganzer (langer, wasserfester) Test-Streifen mit Netzteil

- Netzteil: **5 V** (nicht 12 V!), genug Ampere. Faustregel: Anzahl LEDs × 60 mA (WLED begrenzt dann passend)
- Netzteil **direkt an 5V/GND des Streifens**, nicht durch die Platine
- **GND von Netzteil, Streifen und ESP32 verbinden** (gemeinsame Masse, sonst kommen die Daten nicht an)
- ESP32 weiter über USB versorgen; 5V-Pin der Platine dann **nicht** mit dem Netzteil-5V verbinden
- Langer Streifen: 5V/GND zusätzlich am anderen Ende einspeisen, sonst wird er hinten gelb/rot
- Wasserfest: Silikon an den Lötstellen vorsichtig abschälen

## WLED: Zugang und Einrichtung

- WLAN: **WLED-AP**, Passwort **wled1234** (Standard, später ändern)
- Im Browser: **http://4.3.2.1**
- Einrichten (unter Config → LED Preferences): LED-Typ WS2812B, Daten-Pin **GPIO 4**, Anzahl LEDs,
  Strombegrenzer („Automatic Brightness Limiter“) auf ca. 1000 mA

### Neu flashen (falls nötig)

```
cd ~/ProtectiveMouseEars/firmware
../.venv/bin/esptool --chip esp32c3 --port /dev/ttyACM0 write-flash --flash-mode dio --flash-size 4MB \
  0x0 bootloader_c3_8m.bin 0x8000 partitions_c3_4m.bin 0xe000 boot_app0.bin 0x10000 WLED_16.0.1_ESP32-C3.bin
```

## Eigene Firmware (läuft!)

Code in `src/main.cpp`, Muster in `data/muster.json` (Format: [MUSTER.md](MUSTER.md)).

```
cd ~/ProtectiveMouseEars
.venv/bin/pio run -t upload                 # bauen + aufspielen (nach jeder Änderung an muster.json)
.venv/bin/pio device monitor --echo         # Konsole: Befehle eintippen, Beenden mit Strg+C
```

Befehle in der Konsole: `hilfe`, `liste`, `<mustername>`, `weiter`, `hell 40`, `info`.
BOOT-Knopf auf der Platine = nächstes Muster. Muster und Helligkeit bleiben nach dem Ausschalten gespeichert.

- [x] Grundgerüst: JSON-Muster, Zonen, Farben (Gamma-korrigiert), Strombegrenzung, Speichern
- [x] Muster: atmen, herzschlag, tricolore, funkeln, ohrenzucken
- [ ] Am Teststreifen anschauen und Kurven nachjustieren
- [ ] Bluetooth-Steuerung + Steuer-Seite (GitHub Pages)
- [ ] Bewegungssensor

## Später: Muster-Editor

Wenn die **Geometrie des Haarreifs mit Ohren** feststeht (Form, LED-Positionen, welche LED wo sitzt):
Claude baut einen **grafischen Editor** im Browser.
- Zeigt den Haarreif mit Ohren und jede LED als Punkt
- Zonen per Klick festlegen (Ohr links, Mitte, Ohr rechts, Richtung zur Spitze)
- Farben aus dem 40C3-Styleguide wählen, Helligkeitskurve mit der Maus ziehen
- Live-Vorschau der Animation
- Ergebnis ist die `muster.json` (Format: [MUSTER.md](MUSTER.md)), direkt per Bluetooth an die Ohren schickbar

Dafür von Franzi nötig: Maße/Skizze vom Haarreif, LED-Anzahl pro Abschnitt, wie der Streifen verlegt ist.

## GitHub

- Ein Repo für alles: Firmware, Steuer-Seite, Muster
- Steuer-Seite über **GitHub Pages** (dafür muss das Repo öffentlich sein, beim kostenlosen Account)
- Einrichtung:
  - [x] `sudo apt install -y gh` (GitHub-Kommandozeile)
  - [x] `gh auth login` (Franzi meldet sich selbst an, Claude sieht kein Passwort)
  - [x] Claude legt Repo an und lädt hoch: https://github.com/funckyfranzy/protective-mouse-ears

## Weitermachen nach dem Neu-Anmelden

VSCode wieder mit dem Ordner `ProtectiveMouseEars` öffnen und Claude sagen:
„Lies PLAN.md, ich habe mich neu angemeldet, mach weiter.“
