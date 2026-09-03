# Plan: Asynchrones Pixel-Readback (PBO) für p5cpp

Status: geplant, noch nicht implementiert.
Kontext: entstanden aus einer Diskussion über Ruckler während GIF-Aufnahmen (`libs/p5cpp_gif`).

## 1. Problem

Während einer GIF-Aufnahme ruckelt der Sketch spürbar. Ursache ist **nicht** (mehr) das
GIF-Encoding selbst — das läuft bereits in einem eigenen Worker-Thread pro `GIFRecording`
(siehe `libs/p5cpp_gif/src/p5cpp_gif.cpp`, Queue + `std::thread` in `GIFRecording`).

Die eigentliche Ursache ist `loadPixels()` → `queryPixelData(const Texture&)`
(`libs/p5cpp/src/p5cpp/graphics/texture.cpp`), welches intern `glGetTexImage()` aufruft.
Dieser Call ist **synchron**: Der Treiber muss die GPU-Pipeline bis zu diesem Punkt fertig
abarbeiten lassen (impliziter Flush) und die Daten dann per DMA in den CPU-Speicher kopieren.
Der aufrufende Thread (Render-/Main-Thread) blockiert währenddessen — mehrmals pro Sekunde,
mitten im Draw-Loop. Das verursacht die Ruckler, unabhängig vom bereits vorhandenen
Encoder-Threading.

## 2. Bereits umgesetzt (Vorstufe)

- `GIFRecording` besitzt einen eigenen Worker-Thread + Queue (`std::mutex`,
  `std::condition_variable`, `std::atomic<bool> m_finished`).
- `captureFrame()` schiebt `Pixels` nur noch in die Queue (schnell, nicht blockierend).
- `workerLoop()` verarbeitet `msf_gif_frame_to_file` / `msf_gif_end_to_file` im Hintergrund.
- `GIFRecorder::updateRecordings()` entfernt eine Recording erst, wenn `isFinished()` true ist
  (kein blockierendes `join()` im Draw-Loop).

Diese Vorstufe behebt den Stall durch das **Encoding**, nicht den Stall durch das
**Readback** (`glGetTexImage`). Dieser Plan behandelt den verbleibenden Stall.

## 3. Prior Art / Vergleich mit anderen Frameworks

- **Processing (Core, `PGraphicsOpenGL`)**: `loadPixels()` nutzt striktes synchrones
  `glReadPixels`, kein PBO. Bekanntes, seit Jahren in der Community diskutiertes
  Performance-Problem (z. B. bei Video-Export-/GIF-Libraries) — wird im Core nicht behoben.
- **openFrameworks**: `ofFbo::readToPixels()` unterstützt PBO-basiertes asynchrones
  Readback (`usePBO`) explizit aus diesem Grund.
- **Cinder**: Hat eine eigene `ci::gl::Pbo`-Klasse im Core, dediziert für asynchrones
  Pack/Unpack von Pixel-Daten.
- **Screen-/Game-Capture-Software** (OBS, NVENC/ShadowPlay etc.): Geht noch weiter und
  vermeidet den CPU-Readback komplett (GPU-Textur direkt per Interop an Hardware-Encoder).
  Für uns nicht relevant, da `msf_gif` einen CPU-seitigen RGBA8-Buffer benötigt — zeigt aber,
  dass "GPU→CPU-Sync vermeiden" ein Standard-Pain-Point ist, für den PBO-Pipelining die
  etablierte Lösung ist.

Fazit: PBO-basiertes asynchrones Readback nachzurüsten ist kein Exot, sondern das, was die
meisten ernstzunehmenden GL-Frameworks (außer Processing) bereits tun.

## 4. Design-Entscheidung: Core-Primitive statt Einzellösung

GIF ist voraussichtlich nicht der einzige zukünftige Konsument von Pixel-Readback
(denkbar: Video-Export, Screenshot-/Streaming-Plugins, CV-Hooks, Live-Preview). Die
PBO-Logik soll daher **nicht** in `p5cpp_gif` vergraben werden, sondern als leichtgewichtige,
wiederverwendbare Primitive in `p5cpp` (Core) landen — konsistent mit dem bestehenden Muster
für `Texture`/`Framebuffer`/`Shader` (RAII-Struct + freie Funktionen, vom Aufrufer gehalten).

**Bewusst kein** zentrales Registry-/Service-System für PBOs:
- PBOs sind an eine feste Größe gebunden und werden 1:1 von genau einem Konsumenten genutzt.
- Lifetime ist trivial scope-gebunden (Recorder lebt → Reader lebt), analog dazu, wie
  `Texture`/`Framebuffer` heute schon einfach vom Aufrufer als `std::unique_ptr` gehalten
  werden, nicht zentral verwaltet.
- Eine Registry würde nur Indirektion/Lookup-Overhead bringen, ohne ein reales Problem zu
  lösen.

## 5. Geplante API

### 5.1 Low-Level-Primitive (`p5cpp.hpp`, analog zu `Texture`)

```cpp
struct Pbo
{
    uint32_t id = 0;
    size_t sizeInBytes = 0;

    Pbo() = default;
    Pbo(const Pbo&) = delete;
    Pbo& operator=(const Pbo&) = delete;
    ~Pbo();
};

std::unique_ptr<Pbo> createPbo(size_t sizeInBytes);

// Bindet pbo als GL_PIXEL_PACK_BUFFER und stößt glGetTexImage(...) an (Ziel = PBO,
// nullptr als Datenzeiger). Kehrt sofort zurück, Transfer läuft asynchron per DMA.
void beginAsyncReadback(Pbo& pbo, const Texture& texture);

// Nicht-blockierend (ggf. mit glClientWaitSync(..., 0) o.ä. Absicherung): liefert die
// Bytes, falls der Transfer abgeschlossen ist, sonst std::nullopt. Bei Erfolg intern
// glMapBuffer/glUnmapBuffer.
std::optional<std::vector<uint8_t>> tryMapPbo(Pbo& pbo);
```

Muss auf dem Render-/GL-Thread aufgerufen werden (wie alle bestehenden GL-Funktionen in
`p5cpp`).

### 5.2 Convenience-Schicht: `AsyncPixelReader`

Kapselt den Ringpuffer aus N PBOs (Standard: 3), damit kein Plugin die
Ringpuffer-/Timing-Logik selbst bauen muss:

```cpp
class AsyncPixelReader
{
public:
    AsyncPixelReader(uint32_t width, uint32_t height, uint32_t bufferCount = 3);

    // Pro gewünschtem Capture-Zeitpunkt einmal aufrufen (z. B. einmal pro
    // Frame-Intervall der GIF-Aufnahme). Stößt beginAsyncReadback() auf dem nächsten
    // Ringpuffer-Slot an.
    void capture(const Texture& texture);

    // Nicht-blockierend, im Draw-Loop pollen: liefert das älteste fertige Frame als
    // Pixels, sobald verfügbar (typischerweise 1–2 Frames nach dem zugehörigen
    // capture()-Aufruf), sonst std::nullopt.
    std::optional<Pixels> poll();

private:
    std::vector<std::unique_ptr<Pbo>> m_pbos;
    uint32_t m_width;
    uint32_t m_height;
    size_t m_nextCaptureSlot = 0;
    size_t m_nextPollSlot = 0;
    size_t m_pendingCount = 0;
};
```

## 6. Implementierungsschritte

1. **`Pbo`-Struct + freie Funktionen** in `p5cpp` ergänzen:
   - Deklaration in `libs/p5cpp/include/p5cpp/p5cpp.hpp` (neben `Texture`).
   - Implementierung in neuer Datei `libs/p5cpp/src/p5cpp/graphics/pbo.cpp`
     (`glGenBuffers`, `glBufferData(GL_PIXEL_PACK_BUFFER, size, nullptr, GL_STREAM_READ)`,
     `glBindBuffer`, `glGetTexImage` mit gebundenem PBO, `glMapBuffer`/`glUnmapBuffer`,
     Cleanup via `glDeleteBuffers` im Destruktor).
2. **`AsyncPixelReader`** implementieren (Ringpuffer-Verwaltung), ebenfalls im Core
   (z. B. `libs/p5cpp/src/p5cpp/graphics/async_pixel_reader.cpp` +
   öffentlicher Header-Eintrag in `p5cpp.hpp` oder `graphics/graphics.hpp`, je nachdem wie
   öffentlich die API sein soll).
3. **`GIFRecording` umstellen** (`libs/p5cpp_gif/src/p5cpp_gif.cpp`):
   - Statt `captureFrame(loadPixels())` synchron: pro Frame-Intervall
     `m_pixelReader.capture(texture)` aufrufen.
   - In `update()` zusätzlich `m_pixelReader.poll()` abfragen; bei Ergebnis
     `captureFrame(std::move(pixels))` wie bisher (→ landet in der bestehenden
     Encoder-Queue/Worker-Thread, unverändert).
   - Muss an die aktuelle Textur/Framebuffer-Quelle herankommen (aktuell nutzt
     `loadPixels()` implizit den gerade gebundenen Framebuffer via `graphics()` /
     `Context`) — prüfen, wie das sauber injiziert wird, ohne die bestehende
     `loadPixels()`-Signatur zu verändern.
4. **Drain-Handling am Recording-Ende**: Beim Erreichen von `m_isRecordingComplete` dürfen
   noch ausstehende `capture()`-Aufrufe nicht verloren gehen — alle gepollten Frames müssen
   vor `finish()`/Worker-Stop eingesammelt und in die Queue geschoben werden (kleine
   Erweiterung des bestehenden Drain-Mechanismus).
5. **Kein Fallback-Pfad geplant**: Da wir ausschließlich Desktop-GL nutzen (bereits
   `glGetTexImage`-abhängig, kein GLES), ist PBO-Unterstützung durchgehend gegeben — ein
   Sync-Fallback ist nicht nötig.
6. **Test/Verifikation**:
   - Sketch mit hoher Auflösung/Framerate aufnehmen, `getDeltaTime()`-Ausreißer während der
     Aufnahme loggen/vergleichen (vorher/nachher).
   - Sicherstellen, dass das erzeugte GIF inhaltlich/zeitlich weiterhin korrekt ist (Frame
     Timing durch die 1–2 Frames Verzögerung nicht verfälscht wird — Verzögerung betrifft nur
     *wann* ein Pixel-Snapshot eintrifft, nicht die einprogrammierte GIF-Frame-Dauer).
   - Bestehenden Build (`p5cpp`, `p5cpp_gif`, Beispiel `gif_recorder`) durchbauen.

## 7. Offene Punkte / Diskussionsbedarf bei Umsetzung

- Öffentlichkeitsgrad der neuen API: Vollständig öffentlich in `p5cpp.hpp` (für Plugin-Autoren
  nutzbar) vs. "internal"/detail-Namespace, bis ein zweiter echter Konsument existiert.
- Genaue Anzahl der Ringpuffer-Slots (Start: 3) ggf. experimentell anpassen (Kompromiss
  Latenz vs. Sicherheit, dass der Transfer beim Poll wirklich fertig ist).
- Ob `tryMapPbo` zusätzlich `glFenceSync`/`glClientWaitSync(..., 0)` nutzt, um "ist der
  Transfer wirklich fertig" explizit zu prüfen, statt sich rein auf die Ringpuffer-Verzögerung
  zu verlassen.
