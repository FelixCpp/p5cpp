# Stat-Ideen

Sammlung möglicher Upgrades. Jeder Stat wird wie die bestehenden über `UpgradeDefinition` angelegt
(Gold-Kosten in der Runde, Coin-Kosten im Menü, `maxLevel` als Obergrenze). Die genannten Maximalwerte
sind die Werte bei `maxLevel`.

Legende: **✓** = existiert bereits · **Idee (F)** = von Felix · **Idee (C)** = Vorschlag von Claude

---

## Angriff

| Stat | Herkunft | Maximum | Beschreibung |
|---|---|---|---|
| Schaden | ✓ | – | Schaden pro Kugel. |
| Angriffstempo | ✓ | – | Schüsse pro Sekunde. |
| Reichweite | ✓ | Lv 8 | Radius, in dem der Turm Ziele auswählt. |
| Abprallschuss | Idee (F) | 30 % Chance, 8 m Sprungweite, 5 Sprünge | Beim Treffer besteht eine Chance, dass ein Blitz vom getroffenen Gegner zum nächsten Gegner in Sprungweite springt, von dort weiter bis zur maximalen Sprunganzahl. |
| Schnellfeuer | Idee (F) | 20 % Chance, 3 s Dauer | Jeder Schuss hat eine Chance, einen „Bullet-Burst“ auszulösen: Für die Dauer feuert der Turm stark erhöht (z. B. ×4 Angriffstempo). |
| Mehrfachschuss | Idee (F) | 50 % Chance, 8 Ziele | Ein Schuss hat eine Chance, gleichzeitig mehrere Kugeln abzufeuern – jede auf einen **anderen** Gegner, nie zwei auf denselben. |
| Kritischer Treffer | Idee (C) | 50 % Chance | Chance auf einen kritischen Treffer mit erhöhtem Schaden. Eigene Popup-Farbe/-Größe für Crits. |
| Krit-Multiplikator | Idee (C) | ×5 | Schadensfaktor kritischer Treffer (Start ×1,5). Getrennt von der Chance, damit beides einzeln skaliert. |
| Rückstoß | Idee (C) | – | Getroffene Gegner werden ein Stück vom Turm weggestoßen. Schwere Gegner (Tank) werden weniger weit gestoßen. |
| Hinrichtung | Idee (C) | 10 % | Gegner, deren Leben unter X % ihres Maximums fällt, sterben sofort. Macht Tanks und Bosse am Ende schneller. |
| Flächenschaden | Idee (C) | 50 % Schaden, 3 m Radius | Ein Treffer verursacht zusätzlich einen Teil des Schadens an allen Gegnern im Radius um das Ziel. |
| Verlangsamung | Idee (C) | 40 %, 2 s | Getroffene Gegner werden für kurze Zeit verlangsamt. Kein Stapeln – neue Treffer frischen nur die Dauer auf. |
| Distanzbonus | Idee (C) | +50 % | Mehr Schaden gegen weit entfernte Gegner (linear bis zur Reichweitengrenze). Belohnt Reichweiten-Builds. |
| Orbits | Idee (F, früher besprochen) | 4 Orbits | Kugeln, die um den Turm kreisen und jeden Gegner beschädigen, den sie berühren. Eigene Stats: Anzahl, Radius, Drehgeschwindigkeit, Schaden. |

## Abwehr

| Stat | Herkunft | Maximum | Beschreibung |
|---|---|---|---|
| Max. Leben | ✓ | – | Lebenspunkte des Turms. |
| Regeneration | ✓ | – | Leben pro Sekunde. |
| Absolute Verteidigung | Idee (F) | – | Fester Wert, der von jedem Gegnertreffer abgezogen wird. |
| Prozentuale Verteidigung | Idee (F) | 50 % | Prozentuale Minderung jedes Gegnertreffers. |
| Dornenschaden | Idee (F) | 50 % | Ein Gegner, der den Turm angreift, erleidet selbst X % des Schadens, den er verursacht. |
| Lebensraub | Idee (C) | 5 % | Der Turm heilt sich um einen Teil des Schadens, den seine Kugeln verursachen. |
| Schild | Idee (C) | – | Zusätzliche Schicht über dem Leben, die zuerst Schaden nimmt und sich nach X Sekunden ohne Treffer vollständig wieder auflädt. |
| Abwehrchance | Idee (C) | 25 % | Chance, einen Gegnertreffer komplett zu ignorieren. |
| Schockwelle | Idee (C) | alle 5 s | Der Turm sendet regelmäßig eine Druckwelle aus, die Gegner im Nahbereich zurückstößt und leicht beschädigt. |
| Letzte Rettung | Idee (C) | 1× pro Runde | Fällt das Leben auf 0, überlebt der Turm einmalig mit X % Leben. Besonders interessant als reines Coin-Upgrade. |

## Utility

| Stat | Herkunft | Maximum | Beschreibung |
|---|---|---|---|
| Gold pro Welle | ✓ | – | Gold-Bonus am Ende jeder Welle. |
| Gold pro Gegner | ✓ | – | Zusätzliches Gold pro Kill. |
| Coins pro Welle | ✓ | – | Coins am Ende jeder Welle. |
| Coins pro Gegner | ✓ | – | Coin-Bruchteile pro Kill. |
| Spieltempo | ✓ | Lv 4 | Beschleunigt das Spiel. |
| Automatisches Upgrade | Idee (F, früher besprochen) | 20 % | Am Ende jeder Welle besteht eine Chance, dass ein zufälliges Runden-Upgrade gratis um ein Level steigt. |
| Zinsen | Idee (C) | 10 %, gedeckelt | Am Wellenende gibt es X % des aktuellen Golds als Bonus (mit Obergrenze pro Welle). Macht Sparen zu einer echten Entscheidung. |
| Rabatt | Idee (C) | 30 % | Alle Gold-Upgrades in der Runde werden günstiger. |
| Startgold | Idee (C) | – | Gold, mit dem jede Runde beginnt. Nur als Coin-Upgrade sinnvoll. |
| Kopfgeld | Idee (C) | ×3 | Mehr Gold und Coins für Tanks und Bosse. |
| Kürzere Pause | Idee (C) | –50 % | Verkürzt die Pause zwischen den Wellen. Schnellerer Fortschritt, weniger Zeit zum Reagieren. |

---

## Technische Hinweise

**Alle Zufallswürfe beim Abschuss.** Krit, Mehrfachschuss und Schnellfeuer müssen beim Abfeuern
ausgewürfelt werden, damit der vorgemerkte Schaden (`pendingDamage`) stimmt und nie überflüssige Kugeln
fliegen. Die Ergebnisse wandern als Felder in `Hit` (z. B. `isCrit`, `knockback`, `bouncesLeft`).

**Abprallschuss** wird erst beim Aufprall ausgelöst. Das nächste Ziel wird in diesem Moment gewählt
(nächster nicht „doomed“ Gegner in Sprungweite, nicht der gerade getroffene), sein Schaden wird erst dann
vorgemerkt. Der Blitz ist eine eigene, sehr schnelle Kugel mit Flugzeit statt Kollision – das bestehende
System trägt also.

**Mehrfachschuss** ruft `find_target` mehrfach auf und schließt bereits gewählte Gegner explizit aus.
Der vorgemerkte Schaden allein reicht dafür nicht, weil ein Gegner nach einer Kugel noch nicht „doomed“ sein muss.

**Schadensquellen ohne Vormerkung** (Dornen, Flächenschaden, Orbits, Schockwelle, Hinrichtung) senken
nur das Leben. Gegner sterben dadurch früher als geplant – Kugeln, die noch auf sie unterwegs sind,
verpuffen. Das ist unvermeidbar und vom Code bereits abgefangen.

**Hinrichtung und „doomed“.** Ein Gegner gilt dann schon als doomed, wenn
`current - pendingDamage <= maxHealth * executeThreshold`. Sonst feuert der Turm unnötige Kugeln auf
Gegner, die ohnehin hingerichtet werden.

**Reihenfolge der Verteidigung** muss festgelegt werden. Vorschlag:
`Schaden → Abwehrchance → prozentuale Verteidigung → absolute Verteidigung (min. 0) → Schild → Leben`.
Dornen beziehen sich auf den Schaden **vor** der Minderung, damit sich beide Stats nicht gegenseitig entwerten.

**Rückstoß** verschiebt Gegner während Kugeln unterwegs sind. Die Kugel schwenkt zum Schluss auf die
echte Position ein – bei starkem Rückstoß wird das sichtbar. Ggf. Rückstoß erst beim Aufprall anwenden
(ist ohnehin so) und Einschwenken weicher machen.

**Meter.** `Meters::toPixels()` rechnet aktuell 1:1, die Turmreichweite liegt bei 150 „Metern“.
Für Angaben wie „8 m Sprungweite“ brauchen wir eine echte Umrechnung (z. B. 1 m = 20 px,
dann wäre die Reichweite 7,5 m). Das sollte vor dem ersten Meter-basierten Stat festgelegt werden.

## Vorgeschlagene Reihenfolge

1. Kritischer Treffer + Krit-Multiplikator – kleinstes Feature, testet `Hit` und Popups
2. Absolute & prozentuale Verteidigung, Dornen – kleine Änderungen an `TowerHitEvent`
3. Mehrfachschuss
4. Abprallschuss (vorher Meter-Umrechnung festlegen)
5. Schnellfeuer
6. Rückstoß, Hinrichtung, Verlangsamung
7. Orbits – eigenes System mit echter Kollisionsabfrage (überstrichener Winkel)
