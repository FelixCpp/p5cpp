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
| Abprallschuss | ✓ | 30 % Chance, 8 m Sprungweite, 5 Sprünge | Beim Treffer besteht eine Chance, dass ein Blitz vom getroffenen Gegner zum nächsten Gegner in Sprungweite springt, von dort weiter bis zur maximalen Sprunganzahl. |
| Schnellfeuer | ✓ | 20 % Chance, 3 s Dauer | Jeder Schuss hat eine Chance, einen „Bullet-Burst“ auszulösen: Für die Dauer feuert der Turm mit ×4 Angriffstempo. Schüsse während eines Bursts lösen keinen neuen aus. |
| Mehrfachschuss | ✓ | 50 % Chance, 8 Ziele | Ein Schuss hat eine Chance, gleichzeitig mehrere Kugeln abzufeuern – jede auf einen **anderen** Gegner, nie zwei auf denselben. |
| Kritischer Treffer | ✓ | 50 % Chance | Chance auf einen kritischen Treffer mit erhöhtem Schaden. Eigene Popup-Farbe/-Größe für Crits. |
| Krit-Multiplikator | ✓ | ×5 | Schadensfaktor kritischer Treffer (Start ×1,5). Getrennt von der Chance, damit beides einzeln skaliert. |
| Rückstoß | Idee (C) | – | Getroffene Gegner werden ein Stück vom Turm weggestoßen. Schwere Gegner (Tank) werden weniger weit gestoßen. |
| Hinrichtung | Idee (C) | 10 % | Gegner, deren Leben unter X % ihres Maximums fällt, sterben sofort. Macht Tanks und Bosse am Ende schneller. |
| Flächenschaden | Idee (C) | 50 % Schaden, 3 m Radius | Ein Treffer verursacht zusätzlich einen Teil des Schadens an allen Gegnern im Radius um das Ziel. |
| Verlangsamung | Idee (C) | 40 %, 2 s | Getroffene Gegner werden für kurze Zeit verlangsamt. Kein Stapeln – neue Treffer frischen nur die Dauer auf. |
| Schaden pro Meter | Idee (C, angepasst von F) | – | Zusätzlicher Schaden pro Meter Abstand zwischen Turm und Ziel, gemessen beim Abschuss. Belohnt Reichweiten-Builds und Gegner früh abzufangen. |
| Orbits | Idee (F, früher besprochen) | 4 Orbits | Kugeln, die um den Turm kreisen und jeden Gegner beschädigen, den sie berühren. Eigene Stats: Anzahl, Radius, Drehgeschwindigkeit, Schaden. |

## Abwehr

| Stat | Herkunft | Maximum | Beschreibung |
|---|---|---|---|
| Max. Leben | ✓ | – | Lebenspunkte des Turms. |
| Regeneration | ✓ | – | Leben pro Sekunde. |
| Absolute Verteidigung | ✓ | 10 | Fester Wert, der von jedem Gegnertreffer abgezogen wird. |
| Prozentuale Verteidigung | ✓ | 50 % | Prozentuale Minderung jedes Gegnertreffers. |
| Dornenschaden | ✓ | 50 % | Ein Gegner, der den Turm angreift, erleidet selbst X % des Schadens, den er verursacht. |
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
| Auto-Upgrade Angriff | Idee (F) | 20 % | Am Ende jeder Welle besteht eine Chance, dass ein zufälliges Angriffs-Upgrade gratis um ein Level steigt. |
| Auto-Upgrade Abwehr | Idee (F) | 20 % | Wie oben, für ein zufälliges Abwehr-Upgrade. |
| Auto-Upgrade Utility | Idee (F) | 20 % | Wie oben, für ein zufälliges Utility-Upgrade. |
| Zinsen | Idee (C) | 10 %, gedeckelt | Am Wellenende gibt es X % des aktuellen Golds als Bonus (mit Obergrenze pro Welle). Macht Sparen zu einer echten Entscheidung. |
| Rabatt | Idee (C) | 30 % | Alle Gold-Upgrades in der Runde werden günstiger. |
| Startgold | Idee (C) | – | Gold, mit dem jede Runde beginnt. Nur als Coin-Upgrade sinnvoll. |
| Kopfgeld | Idee (C) | ×3 | Mehr Gold und Coins für Tanks und Bosse. |

---

## Technische Hinweise

**Alle Zufallswürfe beim Abschuss.** Krit, Mehrfachschuss und Schnellfeuer müssen beim Abfeuern
ausgewürfelt werden, damit der vorgemerkte Schaden (`pendingDamage`) stimmt und nie überflüssige Kugeln
fliegen. Die Ergebnisse wandern als Felder in `Hit` (z. B. `isCrit`, `knockback`, `bouncesLeft`).

**Abprallschuss** wird erst beim Aufprall ausgelöst. Das nächste Ziel wird in diesem Moment gewählt
(nächster nicht „doomed“ Gegner in Sprungweite, der von dieser Kette noch nicht getroffen wurde), sein Schaden wird erst dann
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

**Meter.** 1 m = 10 px (`pixelsPerMeter` in `meters.cpp`). Die Turmreichweite startet bei 15 m,
Gegner sind 1,6–4 m groß, die Welt ist 60 m breit.

**Schaden pro Meter** wird beim Abschuss berechnet (Abstand Turm → Ziel) und landet fertig in `Hit.damage` –
so stimmt die Vormerkung. Braucht ebenfalls die Meter-Umrechnung.

**Auto-Upgrades** würfeln beim `WaveCompletedEvent` je Kategorie einmal. Ausgemaxte Upgrades werden bei der
Auswahl übersprungen; sind alle einer Kategorie ausgemaxt, passiert nichts. Ein Popup zeigt, was gratis gestiegen ist.

## Ultimative Waffen

Idee (F): Große Spezialwaffen, die man im Spiel freischaltet und die dann eigenständig, mit eigenem Cooldown,
auslösen. Jede hat eigene Upgrades (Schaden, Dauer, Cooldown, Größe …). Offen: wie man sie freischaltet
(Coins im Menü, ab Welle X, Boss-Drop?) und ob mehrere gleichzeitig aktiv sein dürfen.

| Waffe | Herkunft | Beschreibung |
|---|---|---|
| Giftfelder | Idee (F) | Auf dem Spielfeld entstehen Giftfelder, die Gegnern darin Schaden pro Sekunde zufügen. Upgrades: Anzahl, Radius, Schaden, Dauer. |
| Todeslaser | Idee (F) | Ein Laser fährt für 0,5 s (aufrüstbar) einmal im Kreis um den Turm und tötet alle Gegner, die er trifft. Upgrades: Dauer/Umdrehungen, Länge, Cooldown. |
| Asteroidenhagel | Idee (F) | Asteroiden fallen vom Himmel und schlagen mit Flächenschaden ein. Ziele zufällig oder auf Gegnergruppen. Upgrades: Anzahl, Schaden, Einschlagradius. |
| Kettenblitz-Sturm | Idee (C) | Für einige Sekunden schlagen Blitze in zufällige Gegner ein und springen weiter – wie Abprallschuss, aber unabhängig von Treffern. |
| Schwarzes Loch | Idee (C) | Ein Wirbel zieht Gegner in der Nähe zu einem Punkt zusammen und beschädigt sie dort. Kombiniert stark mit Flächenschaden und Asteroiden. |
| Zeitstopp | Idee (C) | Alle Gegner frieren für kurze Zeit ein, der Turm feuert normal weiter. |
| Sperrfeuer | Idee (C) | Der Turm feuert eine Salve auf **alle** Gegner in Reichweite gleichzeitig. |
| Frostnova | Idee (C) | Eine Welle vom Turm aus verlangsamt alle Gegner auf dem Feld stark und beschädigt sie leicht. |

**Technik.** Wie bei den anderen Schadensquellen ohne Vormerkung senken die Ultimativen Waffen nur das Leben.
Der Todeslaser braucht dieselbe Kollisionsabfrage über den überstrichenen Winkel wie die Orbits – beide
sollten sich den Code teilen.

## Wellen

Wellen laufen auf einem **festen Timer** statt darauf zu warten, dass alle Gegner tot sind. Die Gegner einer
Welle werden gleichmäßig über ihre Dauer verteilt gespawnt; läuft der Timer ab, beginnt sofort die nächste
Welle – auch wenn noch Gegner leben. Wer zu langsam tötet, bekommt also überlappende Wellen. Gold und Coins
„pro Welle“ gibt es beim Ablauf des Timers.

## Vorgeschlagene Reihenfolge

1. Kritischer Treffer + Krit-Multiplikator – kleinstes Feature, testet `Hit` und Popups
2. Absolute & prozentuale Verteidigung, Dornen – kleine Änderungen an `TowerHitEvent`
3. Mehrfachschuss
4. Abprallschuss (vorher Meter-Umrechnung festlegen)
5. ~~Schnellfeuer~~ ✓
6. Rückstoß, Hinrichtung, Verlangsamung
7. Orbits – eigenes System mit echter Kollisionsabfrage (überstrichener Winkel)
