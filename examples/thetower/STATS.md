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
| Rückstoß | ✓ | 3 m | Getroffene Gegner werden vom Turm weggestoßen, die Strecke geteilt durch ihr Gewicht – Schwarm ×0,4, Läufer ×0,6, Grunzer ×1, Panzer ×3, Brecher ×5. Treffer hintereinander stapeln sich nur bis zum doppelten Stoß. |
| Verlangsamung | ✓ | 40 %, 2 s | Getroffene Gegner verlieren für die Dauer einen Teil ihres Tempos und werden dabei frostig getönt. Kein Stapeln – neue Treffer frischen nur die Dauer auf. |
| Hinrichtung | ✓ | 10 % | Gegner, deren Leben unter X % ihres Maximums fällt, sterben sofort. Macht Panzer, Brecher und Bosse am Ende schneller. Greift erst, wenn ein Schild gebrochen ist. |
| Flächenschaden | Idee (C) | 50 % Schaden, 3 m Radius | Ein Treffer verursacht zusätzlich einen Teil des Schadens an allen Gegnern im Radius um das Ziel. |
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
| Kopfgeld | Idee (C) | ×3 | Mehr Gold und Coins für Panzer, Brecher und Bosse. |

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
Gegner, die ohnehin hingerichtet werden. Hingerichtet wird nicht im Treffer, sondern einmal pro Frame
über alle Gegner (`enemies_execute`) – so stirbt auch, wer ohne Treffer unter die Schwelle fällt oder
schon darunter lag, als das Upgrade mitten in der Runde gekauft wurde.

**Reihenfolge der Verteidigung** muss festgelegt werden. Vorschlag:
`Schaden → Abwehrchance → prozentuale Verteidigung → absolute Verteidigung (min. 0) → Schild → Leben`.
Dornen beziehen sich auf den Schaden **vor** der Minderung, damit sich beide Stats nicht gegenseitig entwerten.

**Rückstoß** setzt beim Aufprall eine Stoßgeschwindigkeit in `EnemyMovement.push`, die exponentiell
ausläuft; über ihre Lebensdauer trägt sie den Gegner genau die gekaufte Strecke weit. Ein Nahkämpfer am
Turm wird dadurch wirklich weggeschoben: die Kontaktlogik klebt ihn nur noch an die Turmkante, wenn er
nach dem Stoß noch innerhalb der Kontaktdistanz liegt. Die Kugel schwenkt zum Schluss auf die echte
Position ein – bei starkem Rückstoß wird das sichtbar.

**Meter.** 1 m = 10 px (`pixelsPerMeter` in `meters.cpp`). Die Turmreichweite startet bei 15 m,
Gegner sind 1,6–4 m groß, die Welt ist 60 m breit.

**Schaden pro Meter** wird beim Abschuss berechnet (Abstand Turm → Ziel) und landet fertig in `Hit.damage` –
so stimmt die Vormerkung. Braucht ebenfalls die Meter-Umrechnung.

**Auto-Upgrades** würfeln beim `WaveCompletedEvent` je Kategorie einmal. Ausgemaxte Upgrades werden bei der
Auswahl übersprungen; sind alle einer Kategorie ausgemaxt, passiert nichts. Ein Popup zeigt, was gratis gestiegen ist.

## Ultimative Waffen

Idee (F): Große Spezialwaffen, die man im Spiel freischaltet und die dann eigenständig, mit eigenem Cooldown,
auslösen. Jede hat eigene Upgrades (Schaden, Dauer, Cooldown, Größe …).

**Boss-Währung.** Bosse droppen neben Gold und Coins eine eigene, seltene Währung. Mit ihr werden
Ultimative Waffen im Menü freigeschaltet (und ggf. aufgerüstet). Bleibt dauerhaft wie die Coins.

**Slots.** Mehrere Ultimative Waffen dürfen gleichzeitig aktiv sein, anfangs bis zu drei. Die Anzahl der
Slots steigt mit dem Fortschritt (z. B. als weiterer Kauf mit Boss-Währung). Freigeschaltet können mehr Waffen
sein als Slots – vor der Runde wählt man aus, welche mitkommen.

Offen: Wie viel Boss-Währung ein Boss droppt (fest, oder steigend mit der Welle?) und wie genau die Slots
steigen. Hinweis: `bossWaveInterval` und die HUD-Anzeige existieren schon, `bosses` ist in `world.cpp`
aber noch leer – es braucht also zuerst einen Boss-Gegner.

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

## Gegnertypen

Jeder Typ ist eine Form plus eine Farbe, damit man ihn im Gewühl erkennt, ohne eine Legende zu lesen.
Das Roster liegt gesammelt in `enemy_roster.cpp`; Runde und Bestiarium lesen dieselbe Liste, damit beide
nie auseinanderlaufen. Die Werte selbst stehen bei den Typen (`melee_enemy.cpp`, `ranged_enemy.cpp`).

| Typ | Form | Leben | Schaden | Tempo | Kosten | Ab Welle | Besonderheit |
| --- | --- | ---: | ---: | ---: | ---: | ---: | --- |
| Grunzer | Quadrat | 30 | 5 | 8,0 m/s | 1 | 1 | – |
| Läufer | Dreieck | 15 | 3 | 15,0 m/s | 1 | 2 | Schnell, aber dünnhäutig |
| Panzer | Sechseck | 120 | 20 | 4,0 m/s | 4 | 4 | Kaum zu verschieben (Gewicht 3) |
| Schwarm | Raute | 10 | 2 | 19,0 m/s | 1 | 6 | Kommt in Massen, ist sofort am Turm |
| Teiler | Kreuz | 70 | 8 | 7,0 m/s | 3 | 8 | Zerfällt beim Tod in zwei Splitter (je 18 Leben) |
| Wächter | Fünfeck | 90 (+120 Schild) | 16 | 6,0 m/s | 5 | 11 | Schild schluckt Schaden, bis es bricht, und kommt nicht zurück |
| Schütze | Stern | 55 | 12 | 7,0 m/s | 4 | 14 | Hält 15 m Abstand und beschießt den Turm |
| Brecher | Achteck | 300 | 40 | 3,0 m/s | 8 | 18 | Härtester Treffer im Spiel, Gewicht 5 |

**Kosten und Gewicht.** `cost` ist das, was ein Typ aus dem Wellenbudget nimmt (`budgetBase` 8, dazu 3 pro
Welle), `baseWeight` + `growthPerWave` wie oft er gewürfelt wird. Ein Typ, der nicht mehr ins Restbudget
passt, wird gar nicht erst gerollt – so endet eine Welle nicht mit einem halben Brecher.

**Schild.** Das Schild liegt in `EnemyHealth` und nicht im Verhalten, damit die Vormerkung
(`enemy_health_is_doomed`) es mitzählt. Sonst hörte der Turm auf zu schießen, während das Schild noch
alles schluckt. Hinrichtung greift erst nach dem Schild.

**Zerfall.** `DeathSpawn` am `EnemyCore` wandert in das `DeathEvent`; `world.cpp` hängt die Kinder erst an,
nachdem die Toten entfernt wurden – die IDs bleiben dadurch aufsteigend, worauf `enemy_find` sich verlässt.

**Distanzangriff.** Der Schütze schiebt ein `EnemyShotEvent`; daraus wird ein `EnemyShot`, der zum Turmrand
fliegt und beim Aufschlag einen ganz normalen `TowerHitEvent` auslöst. Verteidigung und Dornen wirken
dadurch genauso wie bei einem Nahkämpfer.

## Vorgeschlagene Reihenfolge

1. Kritischer Treffer + Krit-Multiplikator – kleinstes Feature, testet `Hit` und Popups
2. Absolute & prozentuale Verteidigung, Dornen – kleine Änderungen an `TowerHitEvent`
3. Mehrfachschuss
4. Abprallschuss (vorher Meter-Umrechnung festlegen)
5. ~~Schnellfeuer~~ ✓
6. ~~Rückstoß, Hinrichtung, Verlangsamung~~ ✓
7. Orbits – eigenes System mit echter Kollisionsabfrage (überstrichener Winkel)
8. Boss-Gegner und Boss-Währung (Drop, Speichern, Anzeige im Menü)
9. Ultimative Waffen – Slots, Auswahl vor der Runde, dann die Waffen einzeln (Todeslaser teilt Code mit den Orbits)
