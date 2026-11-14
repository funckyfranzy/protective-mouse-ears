# Eigene Firmware – Anforderungen (Entwurf)

WLED war zum Testen gut, für den 40C3 bauen wir eine schlanke eigene Firmware.
Status: **Entwurf**, Grundsatzentscheidungen getroffen (Android + BLE, Sensor am Haarreif, kein Knopf).

## Muss

1. **Leuchtmuster**: eine kleine, handverlesene Auswahl (siehe Ideen unten), umschaltbar
2. **Drei Zonen**: Ohr 1, Mittelstück, Ohr 2. Jede Zone kann ein eigenes Muster bzw. eigene Farbe haben
3. **Steuerung per Handy**: einfache Web-Oberfläche mit großen Knöpfen (Modus, Farbe, Helligkeit)
4. **Automatischer Modus-Wechsel** je nach Bewegung (ruhig ↔ unterwegs)
5. **Strombegrenzung** in der Software (ca. 1 A an der Powerbank), Helligkeit regelbar
6. **Startet ohne Handy** mit dem zuletzt benutzten Modus (Einstellungen dauerhaft speichern)
7. **Läuft an der Powerbank** und schaltet nichts ab, was die Powerbank zum Abschalten bringt

## Soll

- „Akku sparen“-Modus (sehr dunkel, wenige LEDs)
- Updates per USB, einfach mit einem Befehl

## Kann (später / Spaß)

- Reaktion auf Kopfbewegungen (Ohren-Zucken bei Kopfdrehen, Nicken)
- Mikrofon: Lautstärke-Anzeige auf den Ohren (passt zu *protective* ears)
- Mehrere Ohren-Träger*innen synchron blinken lassen
- **Grafischer Muster-Editor** (siehe PLAN.md, „Später“)

## Farben

Aus dem **40C3-Styleguide** (`Styleguide.pdf`, „Model Citizens“): Hauptfarben grün `#75B54B`, creme `#DFEED5`,
nacht `#031202` plus 11 Zusatzfarben. Komplette Liste mit Namen in [MUSTER.md](MUSTER.md).

## Muster beschreiben

Muster werden in einer **JSON-Datei** beschrieben (Zonen, Farben, Helligkeitskurven über die Zeit).
Format und Beispiele: [MUSTER.md](MUSTER.md).

**Lieblinge:** ❤️ **Ohrenzucken**, **Atmen**, **Herzschlag**

## Ideen für Muster

| Name | Wirkung | Typisch für |
|---|---|---|
| Atmen ❤️ | langsames sanftes Auf- und Abdimmen | ruhig sitzen, Vorträge |
| Lauflicht | Punkte laufen über den Haarreif von Ohr zu Ohr | unterwegs |
| Herzschlag ❤️ | Doppelschlag „bu-bumm“, Puls einstellbar | Bewegung |
| Ohren-Zucken ❤️ | Blitz läuft zur Ohrspitze, wie ein Zucken | Kopfdrehung |
| Regenbogen | langsamer Farbverlauf | Party |
| Glitzern | zufällige kleine Funkelpunkte | Party / Assembly |
| Statisch | eine Farbe, gedimmt | Akku sparen |

## Technik-Vorschlag

- **Firmware**: Arduino-Framework für ESP32-C3, gebaut mit PlatformIO (Kommandozeile, kein IDE-Zwang)
- **LEDs**: FastLED-Bibliothek, Daten-Pin GPIO 4, mit eingebauter Strombegrenzung
- **Bewegung**: ✅ Bewegungssensor **am Haarreif** (MPU6050 oder LIS3DH, I²C, 4 Drähte: 3.3V, GND, SDA, SCL)
- **Verbindung Handy ↔ Ohren**: ✅ **Bluetooth (BLE)**, Handy ist Android
  - Steuer-Seite läuft in **Chrome auf Android** (Web Bluetooth), Handy bleibt im normalen Netz
  - Web Bluetooth braucht eine HTTPS-Seite: wird einmal im Netz gehostet und als App auf den
    Startbildschirm gelegt, sodass sie auch offline funktioniert
  - Kann später: WLAN-Hotspot als Notlösung für andere Handys

## Offene Fragen

- [x] Handy: **Android** → Bluetooth
- [x] Bewegung: **Sensor am Haarreif**
- [x] Knopf am Haarreif: **nein**
- [x] Sensor: liegt beim Jemand im Büro (Typ noch nachschauen, steht auf dem Chip/Modul)
- [x] Steuer-Seite hosten: **GitHub Pages** (GitHub-Account vorhanden)
- [x] Farben: 40C3-Styleguide
- [x] Lieblingsmuster: Ohrenzucken, Atmen, Herzschlag
- [ ] Muster-Format in MUSTER.md durchsehen
- [ ] Genaue LED-Zahlen der Zonen (wenn der 160/m-Streifen da ist)

## Ablauf (grob)

1. Anforderungen festzurren (diese Datei)
2. Gerüst: Firmware mit 2–3 Mustern über USB flashen, am Teststreifen ausprobieren
3. Handy-Steuerung dazu
4. Bewegungssensor anlöten und einbinden, Schwellwerte für ruhig ↔ Bewegung abstimmen
5. Endgültigen Streifen auf den Haarreif, Zonen einstellen, Powerbank-Dauertest
