# Task 010: Custom UI (Phase B) — Implementado

## Objetivo

Reemplazar el editor funcional de Phase A con una UI pulida y pixel-perfect basada en el mockup JSX aprobado. La UI añade: knobs de estilo dot-arc, metros LED segmentados con peak hold, indicador bypass LED, campos de valor editables, tipografía Inter embebida, y componentización completa siguiendo principios SOLID.

**Referencia:** `GrainUI.jsx` (React mockup)

**Constraint:** Transformación visual. Mismos parámetros, mismo DSP.

---

## Estado: COMPLETADO ✅

Todos los subtasks implementados en la rama `feature/010-pixel-perfect-ui`.

---

## Dimensiones

| Target | Ancho | Alto |
|--------|-------|------|
| VST3 Plugin | 524 px | 617 px |
| Standalone | 524 px | 787 px (617 + 170 standalone) |

Padding interno: 24 px en todos los lados.

---

## Paleta de colores (`Source/GrainColours.h`)

```cpp
namespace GrainColours
{
    inline const juce::Colour kBackground   {0xffB6C1B9};  // Sage green (fondo principal)
    inline const juce::Colour kSurface      {0xff1a1a1a};  // Cuerpo knob, fondo metros
    inline const juce::Colour kSurfaceLight {0xff3a3a3a};  // Ring knob, dots inactivos
    inline const juce::Colour kAccent       {0xffd97706};  // Dots activos, botón seleccionado
    inline const juce::Colour kText         {0xff1a1a1a};  // Labels sobre fondo claro
    inline const juce::Colour kTextDim      {0xff4a4a4a};  // Subtítulo, footer
    inline const juce::Colour kTextBright   {0xffffffff};  // Texto sobre fondo oscuro
    inline const juce::Colour kTextMuted    {0xff888888};  // Texto botón inactivo
    inline const juce::Colour kMeterGreen   {0xff22c55e};  // 0–75%
    inline const juce::Colour kMeterYellow  {0xffeab308};  // 75–90%
    inline const juce::Colour kMeterRed     {0xffef4444};  // 90–100%
    inline const juce::Colour kMeterOff     {0xff3a3a3a};  // Segmentos inactivos
    inline const juce::Colour kBypassOn     {0xffd97706};  // Procesando (naranja)
    inline const juce::Colour kBypassOff    {0xff1a1a1a};  // Bypass activo (oscuro)
}
```

---

## Arquitectura de componentes

La UI está organizada como un árbol de `juce::Component` independientes, siguiendo SRP:

```
GRAINAudioProcessorEditor  (orquestador de layout — ~175 líneas)
├── HeaderPanel            (título + subtítulo Inter)
├── BypassControl          (botón LED circular + APVTS attachment)
├── ControlPanel           (5 knobs + focus selector + todos los attachments)
├── MeterPanel × 2         (metro LED estéreo IN / OUT con peak hold)
├── FooterPanel            (versión + copyright)
├── StandalonePanel *      (FilePlayer + Transport + Waveform + export)
└── DragDropOverlay *      (feedback visual drag & drop)
```

`*` Solo en modo Standalone (null en modo plugin).

---

## Archivos implementados

### Nuevos

| Archivo | Responsabilidad |
|---------|----------------|
| `Source/UI/GrainLookAndFeel.h/.cpp` | LookAndFeel: knobs dot-arc, bypass LED, focus buttons, TextEditor inline |
| `Source/UI/Components/HeaderPanel.h/.cpp` | Título "GRAIN" (Inter ExtraBold Italic 30px) + subtítulo (Inter Regular 12px) |
| `Source/UI/Components/FooterPanel.h/.cpp` | Separador + "v1.0.0" + "Sergio Brocos © 2025" |
| `Source/UI/Components/MeterPanel.h/.cpp` | 32 segmentos L+R, peak hold, decay (0.85/frame) |
| `Source/UI/Components/BypassControl.h/.cpp` | Botón LED circular + APVTS attachment |
| `Source/UI/Components/ControlPanel.h/.cpp` | 5 knobs + focus selector (3 TextButtons) + APVTS listener |
| `Source/UI/Components/DragDropOverlay.h/.cpp` | Borde naranja/rojo sobre drag hover |
| `Source/UI/Components/StandalonePanel.h/.cpp` | FilePlayerSource + TransportBar + WaveformDisplay + export workflow |
| `Source/Fonts/Inter-ExtraBoldItalic.ttf` | Fuente embebida (BinaryData) |
| `Source/Fonts/Inter-Regular.ttf` | Fuente embebida (BinaryData) |
| `Source/Tests/UIComponentTests.cpp` | Tests: PeakHold (×3) + DragDropOverlay (×4) |

### Modificados

| Archivo | Cambios |
|---------|---------|
| `Source/PluginEditor.h` | Simplificado a orquestador: hereda solo de `AudioProcessorEditor`, `FileDragAndDropTarget`, `Timer` |
| `Source/PluginEditor.cpp` | 175 líneas (vs. 728 originales): constructor + resized() + timerCallback() + drag & drop routing |
| `GRAIN.jucer` | Grupo `UI/Components` con todos los nuevos .cpp; grupo `Fonts` con TTFs como binary resources |
| `GRAINTests.jucer` | Añadido `UIComponentTests.cpp` y archivos UI necesarios para compilar los tests |

---

## Subtasks

### Subtask 1: GrainLookAndFeel + Knobs Dot-Arc ✅

**Archivos:** `Source/UI/GrainLookAndFeel.h/.cpp`

**Knob sizes:**

| Tier | Condición | Dots | Dot radius | Knob radius | Uso |
|------|-----------|------|------------|-------------|-----|
| Large | width ≥ 140 | 35 | 3.0 px | 60 px | GRAIN, WARM |
| Medium | width ≥ 100 | 24 | 3.5 px | 40 px | MIX |
| Small | width < 100 | 21 | 3.0 px | 25 px | INPUT, OUTPUT |

**Implementado via** `KnobSizeParams::fromWidth(int)` en anonymous namespace.

**LookAndFeel overrides:**
- `drawRotarySlider` — dot arc + body gradient + indicator
- `drawLabel` — skip cuando TextEditor activo
- `createSliderTextBox` — `onEditorShow` callback: border(0), indents(0,0), centred
- `fillTextEditorBackground` — transparente
- `drawTextEditorOutline` — outline naranja sutil solo en foco
- `drawButtonBackground` — dispatch por `componentID`: "bypass" (LED circular), "focusButton" (rounded rect naranja)
- `drawButtonText` — "bypass" (power icon via juce::Path), "focusButton" (texto 12px Bold)

---

### Subtask 2: Campos de valor editables ✅

**Comportamiento:**
- `setTextBoxIsEditable(true)` en cada slider
- TextEditor inline: fondo transparente, sin borde por defecto
- Outline naranja sutil (`kAccent.withAlpha(0.5f)`) solo cuando tiene foco
- Zero border + zero indents en `onEditorShow` para evitar desplazamiento del texto

---

### Subtask 3: Metros LED segmentados con peak hold ✅

**Componente:** `Source/UI/Components/MeterPanel.h/.cpp`

**Spec:**
- 32 segmentos por canal (L + R), 8×7 px, gap 2 px, canal gap 4 px
- Fondo: `kSurface` rounded rect (4px)
- Zonas: verde < 75%, amarillo < 90%, rojo ≥ 90%
- Glow effect en segmentos encendidos (`colour.withAlpha(0.15f)`)
- Peak hold: 30 frames (~1s a 30 FPS), luego decay × 0.95

**`PeakHold` struct** definido en `MeterPanel.h`:
```cpp
struct PeakHold { float peakLevel; int holdCounter; void update(float); void reset(); };
```

**Actualización:** `MeterPanel::updateLevels()` llamado desde `timerCallback()` del editor a 30 FPS. Cada metro `repaint()` solo su propio bounds.

---

### Subtask 4: Botón Bypass LED ✅

**Componente:** `Source/UI/Components/BypassControl.h/.cpp`

- Botón circular 40×40 px, `componentID = "bypass"`
- **Procesando** (`toggleState = false`): cuerpo naranja + glow exterior
- **Bypass activo** (`toggleState = true`): cuerpo oscuro + inset shadow
- Power icon: línea vertical + arco 270° via `juce::Path`
- Dimming: `ControlPanel::setBypassAlpha(0.4f)` cuando bypass activo

---

### Subtask 5: Focus selector 3-way ✅

**Componente:** parte de `Source/UI/Components/ControlPanel.h/.cpp`

- 3 `TextButton` (LOW, MID, HIGH), `componentID = "focusButton"`
- Comportamiento radio manual (no radioGroupId) para evitar conflictos con APVTS
- `ControlPanel` hereda de `APVTS::Listener` para sincronizar con automatización DAW
- Parámetro "focus" mapeado: 0 → 0.0f, 1 → 0.5f, 2 → 1.0f (AudioParameterChoice)
- Fondo contenedor: `kSurface` rounded rect 8px, padding 3px

---

### Subtask 6: Layout, tipografía y polish ✅

**Tipografía (embebida via BinaryData):**

| Texto | Fuente | Tamaño | Kerning |
|-------|--------|--------|---------|
| "GRAIN" | Inter ExtraBold Italic 800 | 30 pt | -0.025 (tight) |
| "MICRO-HARMONIC SATURATION" | Inter Regular 400 | 12 pt | +0.1 (wide) |
| Footer | Sistema default | 12 pt | — |

**Layout (resized):**
```
bounds (524×617)
├── standalonePanel (bottom, 170px) — solo standalone
└── content (reduced 24px padding)
    ├── headerArea (60px): headerPanel + bypassControl
    ├── footerArea (36px): footerPanel
    └── mainArea (3 columnas):
        ├── leftColumn (100px): inputMeter
        ├── rightColumn (100px): outputMeter
        └── centerColumn: controlPanel
            ├── knobRow: grainSlider + warmthSlider
            └── focusRow: focusLow + focusMid + focusHigh + focusLabel
```

**ControlPanel internal layout:**
```
bounds
├── bottomRow (100px): inputSlider | mixSlider | outputSlider
└── mainArea
    ├── knobRow: grainSlider | warmthSlider (con gap 8px)
    └── focusRow (50px): botones + label
```

---

## Componentización (SOLID)

La refactorización extrajo 7 componentes independientes del `PluginEditor` original (728 líneas → 175 líneas):

| Principio | Aplicación |
|-----------|-----------|
| SRP | Cada componente tiene una sola responsabilidad |
| OCP | Nuevos tipos de meter/botón se añaden sin modificar PluginEditor |
| LSP | Todos los componentes son `juce::Component` sustituibles |
| ISP | `ControlPanel` implementa solo `APVTS::Listener`; `StandalonePanel` solo `TransportBar::Listener` + `FilePlayerSource::Listener` |
| DIP | Editor depende de abstracciones (`juce::Component`), no de implementaciones concretas |

---

## Tests

**Archivo:** `Source/Tests/UIComponentTests.cpp`

| Test | Descripción |
|------|-------------|
| `PeakHold: nueva peak capturada` | update(0.5f) → peakLevel=0.5, holdCounter=30 |
| `PeakHold: decay tras hold` | Después de 30 frames, multiplica × 0.95 |
| `PeakHold: reset` | peakLevel=0, holdCounter=0 |
| `DragDropOverlay: estado inicial` | No hovering, no accepted |
| `DragDropOverlay: estado aceptado` | setDragState(true, true) |
| `DragDropOverlay: estado rechazado` | setDragState(true, false) |
| `DragDropOverlay: limpieza` | setDragState(false, false) tras drag exit |

**Total tests proyecto:** 102 (95 anteriores + 7 nuevos)

---

## Verificación

```bash
# Build VST3
xcodebuild -project Builds/MacOSX/GRAIN.xcodeproj \
  -scheme "GRAIN - VST3" -configuration Debug build

# Build Standalone
xcodebuild -project Builds/MacOSX/GRAIN.xcodeproj \
  -scheme "GRAIN - Standalone Plugin" -configuration Debug build

# Tests (102 tests)
./scripts/run_tests.sh

# pluginval
/Applications/pluginval.app/Contents/MacOS/pluginval \
  --skip-gui-tests --validate ~/Library/Audio/Plug-Ins/VST3/GRAIN.vst3
```

**Resultados:** VST3 ✅ · Standalone ✅ · Tests ✅ · pluginval SUCCESS ✅

---

## Decisiones de diseño

| Decisión | Valor | Justificación |
|----------|-------|---------------|
| Oversample expuesto | **No** | Interno, filosofía "safe" |
| Etiqueta calentamiento | **WARM** | Más compacto visualmente |
| Focus selector | **3 TextButtons** | Mejor UX que ComboBox |
| Peak hold | **Sí** | Estándar en plugins pro |
| Campos editables | **Sí** | Precisión para usuarios avanzados |
| Tooltips | **No** | Sin contenido claro que añadir |
| Componentización | **7 componentes** | SRP / evaluación académica |
| Fuente | **Inter** (embebida) | Pixel-perfect con mockup JSX |
| Dimensiones | **524×617** | Extraídas del mockup JSX |
| Padding | **24 px** | Extraído del mockup JSX |

---

*GRAIN TFM Project — Task 010 completado*
