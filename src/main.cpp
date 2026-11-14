// Protective Mouse Ears – Firmware für ESP32-C3 Super Mini + WS2812B
//
// Liest die Muster aus data/muster.json (Format: MUSTER.md) und spielt sie ab.
// Bedienung zum Testen:
//   - BOOT-Knopf: nächstes Muster
//   - Serielle Konsole (115200): "hilfe" eintippen
// Später dazu: Bluetooth-Steuerung vom Handy, Bewegungssensor.

#include <Arduino.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include <Preferences.h>

#include <vector>

#define LED_PIN 4
#define KNOPF_PIN 9  // BOOT-Knopf auf der Platine
#define MAX_LEDS 400
#define FRAME_MS 16  // ca. 60 Bilder pro Sekunde

extern const char muster_json[] asm("_binary_data_muster_json_start");

// ---------------------------------------------------------------- Datentypen

struct Farbe {
  float r, g, b;  // linear (gamma-korrigiert), 0..1
};

struct Stuetzpunkt {
  float t;  // ms
  float v;  // 0..1
};

struct Strang {
  int von, bis;  // inklusive; von > bis heißt rückwärts
};

enum Effekt { FLAECHE, WELLE, FUNKELN };

struct Ebene {
  std::vector<Strang> straenge;
  std::vector<Farbe> farben;  // 1 Farbe = einfarbig, mehr = Verlauf entlang des Strangs
  std::vector<Stuetzpunkt> kurve;
  Effekt effekt = FLAECHE;
  bool rueckwaerts = false;
  float versatz = 0;  // ms, nur für welle
  float dichte = 0;   // nur für funkeln: Wahrscheinlichkeit pro LED und Bild
};

struct Muster {
  String name;
  float dauer = 1000;
  bool einmalig = false;
  float laenge = 1000;  // bei einmalig: wie lange es insgesamt läuft (dauer + Wellen-Versatz)
  std::vector<Ebene> ebenen;
};

// ---------------------------------------------------------------- Zustand

CRGB leds[MAX_LEDS];
uint8_t funken[MAX_LEDS];
int anzahl = 0;

std::vector<Muster> alleMuster;
int aktuell = -1;     // Index des laufenden Musters
int einmal = -1;      // Index eines gerade ausgelösten einmaligen Musters, sonst -1
uint32_t startAktuell = 0, startEinmal = 0;
uint8_t helligkeit = 60;  // Prozent
Preferences prefs;

// ---------------------------------------------------------------- Laden

static float gamma(float x) { return powf(x, 2.2f); }

static bool leseFarbe(const char* hex, Farbe& f) {
  if (!hex || hex[0] != '#' || strlen(hex) != 7) return false;
  long v = strtol(hex + 1, nullptr, 16);
  f = {gamma(((v >> 16) & 0xFF) / 255.0f), gamma(((v >> 8) & 0xFF) / 255.0f),
       gamma((v & 0xFF) / 255.0f)};
  return true;
}

static void sammleStraenge(JsonVariantConst v, JsonObjectConst zonen, std::vector<Strang>& out,
                           int tiefe = 0) {
  if (tiefe > 8) return;
  if (v.is<const char*>()) {
    JsonVariantConst z = zonen[v.as<const char*>()];
    if (z.isNull()) {
      Serial.printf("! Zone '%s' gibt es nicht\n", v.as<const char*>());
      return;
    }
    sammleStraenge(z, zonen, out, tiefe + 1);
  } else if (v.is<JsonArrayConst>()) {
    JsonArrayConst a = v.as<JsonArrayConst>();
    if (a.size() == 2 && a[0].is<int>() && a[1].is<int>()) {
      int von = constrain(a[0].as<int>(), 0, anzahl - 1);
      int bis = constrain(a[1].as<int>(), 0, anzahl - 1);
      out.push_back({von, bis});
    } else {
      for (JsonVariantConst e : a) sammleStraenge(e, zonen, out, tiefe + 1);
    }
  }
}

static bool ladeMuster(const char* text, String& fehler) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, text);
  if (err) {
    fehler = String("JSON-Fehler: ") + err.c_str();
    return false;
  }

  anzahl = constrain(doc["leds"] | 30, 1, MAX_LEDS);
  JsonObjectConst farbenJ = doc["farben"];
  JsonObjectConst zonen = doc["zonen"];

  alleMuster.clear();
  for (JsonPairConst p : doc["muster"].as<JsonObjectConst>()) {
    Muster m;
    m.name = p.key().c_str();
    JsonObjectConst mj = p.value();
    m.dauer = max(1.0f, mj["dauer"] | 1000.0f);
    m.einmalig = mj["einmalig"] | false;
    m.laenge = m.dauer;

    for (JsonObjectConst ej : mj["ebenen"].as<JsonArrayConst>()) {
      Ebene e;
      sammleStraenge(ej["zone"], zonen, e.straenge);

      JsonVariantConst fj = ej["farbe"];
      auto nimmFarbe = [&](const char* name) {
        Farbe f;
        const char* hex = farbenJ[name] | name;  // Name aus der Palette oder direkt "#RRGGBB"
        if (leseFarbe(hex, f)) e.farben.push_back(f);
        else Serial.printf("! Farbe '%s' unbekannt (Muster %s)\n", name, m.name.c_str());
      };
      if (fj.is<const char*>()) nimmFarbe(fj.as<const char*>());
      else for (JsonVariantConst f : fj.as<JsonArrayConst>()) nimmFarbe(f | "");
      if (e.farben.empty()) e.farben.push_back({1, 1, 1});

      for (JsonArrayConst k : ej["helligkeit"].as<JsonArrayConst>()) {
        e.kurve.push_back({k[0] | 0.0f, constrain((k[1] | 0.0f) / 100.0f, 0.0f, 1.0f)});
      }
      if (e.kurve.empty()) e.kurve.push_back({0, 1});

      const char* eff = ej["effekt"] | "flaeche";
      e.effekt = !strcmp(eff, "welle") ? WELLE : !strcmp(eff, "funkeln") ? FUNKELN : FLAECHE;
      const char* r = ej["richtung"] | "zur_spitze";
      e.rueckwaerts = !strcmp(r, "zur_mitte") || !strcmp(r, "rueckwaerts");
      e.versatz = ej["versatz"] | (m.einmalig ? m.dauer / 2 : m.dauer);
      e.dichte = ej["dichte"] | 0.01f;

      if (e.effekt == WELLE) m.laenge = max(m.laenge, m.dauer + e.versatz);
      m.ebenen.push_back(e);
    }
    alleMuster.push_back(m);
  }
  if (alleMuster.empty()) {
    fehler = "keine Muster gefunden";
    return false;
  }
  return true;
}

// ---------------------------------------------------------------- Abspielen

static float werteKurve(const std::vector<Stuetzpunkt>& k, float t) {
  if (t <= k.front().t) return k.front().v;
  for (size_t i = 1; i < k.size(); i++) {
    if (t < k[i].t) {
      float x = (t - k[i - 1].t) / (k[i].t - k[i - 1].t);
      x = x * x * (3 - 2 * x);  // weich ein- und ausblenden
      return k[i - 1].v + (k[i].v - k[i - 1].v) * x;
    }
  }
  return k.back().v;
}

static Farbe farbeBei(const Ebene& e, float p) {
  if (e.farben.size() == 1) return e.farben[0];
  float pos = p * (e.farben.size() - 1);
  int i = min((int)pos, (int)e.farben.size() - 2);
  float x = pos - i;
  const Farbe &a = e.farben[i], &b = e.farben[i + 1];
  return {a.r + (b.r - a.r) * x, a.g + (b.g - a.g) * x, a.b + (b.b - a.b) * x};
}

static void maleMuster(const Muster& m, float t, bool addieren) {
  for (const Ebene& e : m.ebenen) {
    for (const Strang& s : e.straenge) {
      int n = abs(s.bis - s.von) + 1;
      for (int i = 0; i < n; i++) {
        int idx = s.von <= s.bis ? s.von + i : s.von - i;
        float p = n > 1 ? (float)i / (n - 1) : 0;
        if (e.rueckwaerts) p = 1 - p;

        float tt = t;
        if (e.effekt == WELLE) tt -= p * e.versatz;
        if (!m.einmalig) {
          tt = fmodf(tt, m.dauer);
          if (tt < 0) tt += m.dauer;
        }
        float h = werteKurve(e.kurve, tt);

        if (e.effekt == FUNKELN) {
          if (random(10000) < e.dichte * 10000) funken[idx] = 255;
          else funken[idx] = scale8(funken[idx], 225);
          h = max(h, funken[idx] / 255.0f);
        }

        h = h * h;  // Helligkeit fürs Auge gleichmäßiger machen
        Farbe f = farbeBei(e, p);
        CRGB px(f.r * h * 255 + 0.5f, f.g * h * 255 + 0.5f, f.b * h * 255 + 0.5f);
        if (addieren) leds[idx] += px;
        else leds[idx] = px;
      }
    }
  }
}

// ---------------------------------------------------------------- Bedienung

static int findeMuster(const String& name) {
  for (size_t i = 0; i < alleMuster.size(); i++)
    if (alleMuster[i].name == name) return i;
  return -1;
}

static void setzeMuster(int i, bool speichern = true) {
  if (i < 0 || i >= (int)alleMuster.size()) return;
  if (alleMuster[i].einmalig) {
    einmal = i;
    startEinmal = millis();
    Serial.printf("> %s (einmalig)\n", alleMuster[i].name.c_str());
    return;
  }
  aktuell = i;
  startAktuell = millis();
  memset(funken, 0, sizeof(funken));
  Serial.printf("> Muster: %s\n", alleMuster[i].name.c_str());
  if (speichern) prefs.putString("muster", alleMuster[i].name);
}

static void naechstesMuster() {
  int n = alleMuster.size();
  for (int k = 1; k <= n; k++) {
    int i = (aktuell + k) % n;
    if (!alleMuster[i].einmalig) return setzeMuster(i);
  }
}

static void setzeHelligkeit(int prozent) {
  helligkeit = constrain(prozent, 0, 100);
  FastLED.setBrightness(helligkeit * 255 / 100);
  prefs.putUChar("hell", helligkeit);
  Serial.printf("> Helligkeit: %d %%\n", helligkeit);
}

static void hilfe() {
  Serial.println("Befehle:");
  Serial.println("  liste          alle Muster anzeigen");
  Serial.println("  <name>         Muster starten (einmalige Muster werden ausgeloest)");
  Serial.println("  weiter         naechstes Muster");
  Serial.println("  hell <0-100>   Helligkeit in Prozent");
  Serial.println("  info           Status");
}

static void befehl(String z) {
  z.trim();
  if (z.isEmpty()) return;
  if (z == "hilfe" || z == "?") return hilfe();
  if (z == "liste") {
    for (auto& m : alleMuster)
      Serial.printf("  %s%s\n", m.name.c_str(), m.einmalig ? "  (einmalig)" : "");
    return;
  }
  if (z == "weiter") return naechstesMuster();
  if (z.startsWith("hell")) return setzeHelligkeit(z.substring(4).toInt());
  if (z == "info") {
    Serial.printf("LEDs: %d, Muster: %s, Helligkeit: %d %%, Muster gesamt: %d\n", anzahl,
                  aktuell >= 0 ? alleMuster[aktuell].name.c_str() : "-", helligkeit,
                  (int)alleMuster.size());
    return;
  }
  int i = findeMuster(z);
  if (i >= 0) return setzeMuster(i);
  Serial.printf("? Unbekannt: '%s' (hilfe eintippen)\n", z.c_str());
}

// ---------------------------------------------------------------- Setup / Loop

void setup() {
  Serial.begin(115200);
  pinMode(KNOPF_PIN, INPUT_PULLUP);
  prefs.begin("ohren", false);

  String fehler;
  bool ok = ladeMuster(muster_json, fehler);
  if (!ok) anzahl = 10;

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, anzahl).setCorrection(TypicalLEDStrip);
  JsonDocument d;
  deserializeJson(d, muster_json);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, d["strom_ma"] | 900);
  helligkeit = prefs.getUChar("hell", 60);
  FastLED.setBrightness(helligkeit * 255 / 100);

  if (!ok) {
    // Fehler: erste LEDs blinken rot, Meldung kommt alle 2 s auf der Konsole
    while (true) {
      Serial.println("! " + fehler);
      for (int k = 0; k < 4; k++) {
        fill_solid(leds, anzahl, k % 2 ? CRGB::Black : CRGB(80, 0, 0));
        FastLED.show();
        delay(500);
      }
    }
  }

  int start = findeMuster(prefs.getString("muster", d["start"] | ""));
  if (start < 0 || alleMuster[start].einmalig) start = 0;
  setzeMuster(start, false);
  Serial.printf("Protective Mouse Ears bereit: %d LEDs, %d Muster. 'hilfe' fuer Befehle.\n",
                anzahl, (int)alleMuster.size());
}

void loop() {
  static uint32_t letztesBild = 0;
  static String zeile;
  static bool knopfVorher = true;
  static uint32_t knopfZeit = 0;

  // Serielle Befehle
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      befehl(zeile);
      zeile = "";
    } else if (zeile.length() < 64) {
      zeile += c;
    }
  }

  // BOOT-Knopf: nächstes Muster
  bool knopf = digitalRead(KNOPF_PIN);
  if (!knopf && knopfVorher && millis() - knopfZeit > 200) {
    knopfZeit = millis();
    naechstesMuster();
  }
  knopfVorher = knopf;

  // Bild malen
  uint32_t jetzt = millis();
  if (jetzt - letztesBild < FRAME_MS) return;
  letztesBild = jetzt;

  fill_solid(leds, anzahl, CRGB::Black);
  if (aktuell >= 0) maleMuster(alleMuster[aktuell], jetzt - startAktuell, false);
  if (einmal >= 0) {
    float t = jetzt - startEinmal;
    if (t > alleMuster[einmal].laenge) einmal = -1;
    else maleMuster(alleMuster[einmal], t, true);
  }
  FastLED.show();
}
