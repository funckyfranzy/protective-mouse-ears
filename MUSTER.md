# Muster beschreiben – das JSON-Format (Entwurf)

Idee: Ein Muster ist **eine Farbe (oder mehrere) auf einer Zone**, deren **Helligkeit sich über die Zeit ändert**.
Die Zeitkurve schreibst du als **Stützpunkte**: „nach X Millisekunden soll die Helligkeit Y Prozent sein“.
Dazwischen rechnet die Firmware weich weiter. So lassen sich Atmen, Herzschlag und Zucken einfach hinschreiben.

Die Muster liegen in einer Datei `muster.json`. Später kann das Handy sie per Bluetooth an die Ohren schicken,
sodass du ohne neues Flashen neue Muster ausprobieren kannst.

## Aufbau

```json
{
  "farben": { ... },     // Farb-Namen → Hex-Werte (aus dem 40C3-Styleguide)
  "zonen":  { ... },     // Zonen-Namen → LED-Bereiche [von, bis]
  "muster": { ... },     // die eigentlichen Muster
  "automatik": { ... }   // welches Muster bei welcher Bewegung
}
```

### Farben (40C3-Styleguide)

```json
"farben": {
  "gruen":   "#75B54B",   // 0 – Hauptfarbe
  "petrol":  "#13AEA3",   // 1
  "lavendel":"#999FCD",   // 2
  "flieder": "#B78FBF",   // 3
  "pink":    "#EB5D9D",   // 4
  "rosa":    "#EB6F83",   // 5
  "orange":  "#F28931",   // 6
  "gelb":    "#FAB93E",   // 7
  "salbei":  "#7EAE83",   // 8
  "moos":    "#3B5831",   // 9
  "taupe":   "#B69D93",   // 10
  "mokka":   "#584744",   // 11
  "creme":   "#DFEED5",   // A – Hauptfarbe
  "nacht":   "#031202"    // B – Hauptfarbe (auf LEDs praktisch „aus“)
}
```

**Hinweis LEDs ≠ Papier:** Kräftige Farben (gruen, petrol, pink, rosa, orange, gelb) sehen auf LEDs super aus.
Pastell- und Erdtöne (lavendel, salbei, taupe, mokka, moos) wirken auf LEDs blasser oder nur wie gedimmte Varianten.
Die Firmware rechnet eine Farbkorrektur (Gamma) ein, trotzdem: am Streifen ausprobieren!
Der Styleguide-Klassiker „Tricolore“ (A + B + eine Farbe) heißt auf LEDs: **creme + aus + Lieblingsfarbe**.

### Zonen

```json
"zonen": {
  "ohr_links":  [0, 29],
  "mitte":      [30, 69],
  "ohr_rechts": [70, 99],
  "ohren":      ["ohr_links", "ohr_rechts"],
  "alle":       [0, 99]
}
```

Zahlen sind Platzhalter, bis der echte Streifen gezählt ist. Bei den Ohren zählt die Richtung:
`ohr_links` läuft von unten zur Spitze, `ohr_rechts` je nach Verlegung andersrum. Das trägt dann der Editor ein.

### Muster

Jedes Muster besteht aus **Ebenen** (`ebenen`). Jede Ebene sagt: welche Zone, welche Farbe, welche Bewegung.

| Feld | Bedeutung | Beispiel |
|---|---|---|
| `dauer` | Länge einer Runde in ms, danach wiederholt es sich | `1000` |
| `zone` | Zonen-Name | `"ohren"` |
| `farbe` | Farb-Name, oder Liste für Farbverlauf | `"pink"`, `["petrol","gruen"]` |
| `helligkeit` | Stützpunkte `[ms, Prozent]` | `[[0,10],[500,100],[1000,10]]` |
| `effekt` | wie es sich über die LEDs verteilt (optional) | `"flaeche"`, `"welle"`, `"funkeln"` |
| `richtung` | für `welle`: wohin sie läuft | `"zur_spitze"`, `"zur_mitte"` |
| `einmalig` | `true` = läuft nur einmal (für Zucken), danach zurück zum vorherigen Muster | `true` |

Effekte:
- `flaeche`: alle LEDs der Zone gleich (Standard)
- `welle`: die Helligkeitskurve wandert über die LEDs, wie ein Lauflicht
- `funkeln`: einzelne LEDs blitzen zufällig auf, `helligkeit` gibt die Grundhelligkeit vor

## Beispiele

### Atmen – ruhig sitzen, Vorträge hören

```json
"atmen": {
  "dauer": 4000,
  "ebenen": [
    { "zone": "alle", "farbe": "gruen",
      "helligkeit": [[0, 10], [1800, 80], [2200, 80], [4000, 10]] }
  ]
}
```
Langsam hell werden, kurz oben bleiben, langsam dunkel werden. Ungefähr ein ruhiger Atemzug.

### Herzschlag – „bu-bumm … bu-bumm“

```json
"herzschlag": {
  "dauer": 1100,
  "ebenen": [
    { "zone": "mitte", "farbe": "nacht", "helligkeit": [[0, 0]] },
    { "zone": "ohren", "farbe": "pink",
      "helligkeit": [[0, 15], [80, 100], [180, 30], [280, 85], [450, 15], [1100, 15]] }
  ]
}
```
Zwei Spitzen kurz hintereinander (der typische Doppelschlag), dann Pause. Mit `dauer` stellst du den Puls ein:
1100 ms ≈ 55 Schläge pro Minute (entspannt), 600 ms ≈ 100 (aufgeregt).
Idee: Bei Bewegung schlägt das Herz schneller.

### Ohrenzucken – ausgelöst durch Kopfbewegung

```json
"ohrenzucken": {
  "dauer": 350,
  "einmalig": true,
  "ebenen": [
    { "zone": "ohren", "farbe": "creme", "effekt": "welle", "richtung": "zur_spitze",
      "helligkeit": [[0, 0], [60, 100], [350, 0]] }
  ]
}
```
Ein heller Blitz läuft von unten blitzschnell zur Ohrspitze und verlischt, dann geht das vorherige Muster weiter.

### Automatik

```json
"automatik": {
  "ruhig":       "atmen",
  "bewegung":    "herzschlag",
  "kopfdrehung": "ohrenzucken"
}
```

## Offene Punkte

- Reichen `flaeche`, `welle`, `funkeln` als Effekte, oder fehlt dir etwas?
- Soll der Herzschlag-Puls automatisch mit der Bewegung schneller werden?
- Später: **grafischer Editor** (siehe PLAN.md), der diese Datei erzeugt, sodass niemand JSON von Hand tippen muss
