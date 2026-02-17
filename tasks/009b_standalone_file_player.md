# Task 009b: Standalone File Player & Recorder

## Objective

Add audio file playback, waveform visualization, and real-time recording to the GRAIN standalone application. This enables A/B comparison between dry and processed signals without requiring a DAW or external loopback routing.

---

## Prerequisites

- Task 009 completed (Standalone application with microphone permission)
- Task 008 completed (Plugin editor with functional layout)

---

## Design Rationale

### Why a File Player?

The standalone app (Task 009) processes live audio input only, requiring a loopback driver (BlackHole) for testing with audio files. A built-in file player removes this friction and enables:

1. **A/B comparison:** Hear dry vs. processed signal instantly
2. **Reproducible testing:** Same source file, different parameter settings
3. **Export:** Record the processed output to WAV for offline analysis

### Architecture Decision: Standalone-Only

The file player lives entirely in the standalone target. The `PluginProcessor` (shared with VST3) is not modified — instead, a standalone-specific `FilePlayerSource` injects audio into the processor's input, replacing the device input when a file is loaded and playing.

---

## Feature Summary

| Feature | Description |
|---------|-------------|
| **File loading** | Open WAV/AIFF via dialog or drag & drop |
| **Transport** | Play, Stop, Loop, Seek (click on waveform/progress bar) |
| **Waveform** | Dry waveform (pre-rendered on load) + wet waveform (real-time), superimposed with distinct colors/opacity |
| **Recording** | Real-time capture of processed output to WAV |
| **Export** | "Export" button that plays file from start to end while recording, then opens Save As dialog |
| **UI** | Transport bar below current footer area |

---

## Architecture

### Audio Signal Flow (Standalone with File Player)

```
┌─────────────────────────────────────────────────────────────┐
│                    GRAIN Standalone                          │
│                                                             │
│  ┌──────────────┐                                           │
│  │ Audio Device  │──┐                                       │
│  │    Input      │  │  (muted when file is playing)         │
│  └──────────────┘  │                                        │
│                     ▼                                       │
│  ┌──────────────┐  ┌──────────┐  ┌──────────────────────┐  │
│  │ FilePlayer   │─▶│ Selector │─▶│  PluginProcessor     │  │
│  │ Source       │  │          │  │  (same DSP pipeline)  │  │
│  └──────────────┘  └──────────┘  └──────────┬───────────┘  │
│                                              │              │
│                                    ┌─────────▼─────────┐   │
│                                    │   Audio Device     │   │
│                                    │     Output         │   │
│                                    └─────────┬─────────┘   │
│                                              │              │
│                                    ┌─────────▼─────────┐   │
│                                    │   WAV Recorder     │   │
│                                    │   (when active)    │   │
│                                    └───────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### Key Components

| Component | Location | Responsibility |
|-----------|----------|----------------|
| `FilePlayerSource` | `Source/Standalone/FilePlayerSource.h/.cpp` | Load audio files, manage transport (play/stop/loop/seek), provide samples to processor |
| `WaveformDisplay` | `Source/Standalone/WaveformDisplay.h/.cpp` | Draw dry waveform (pre-rendered) + wet waveform (real-time overlay), handle seek clicks |
| `TransportBar` | `Source/Standalone/TransportBar.h/.cpp` | UI component: Open, Play/Stop, Loop, Export buttons + progress bar |
| `AudioRecorder` | `Source/Standalone/AudioRecorder.h/.cpp` | Capture processed output to WAV file in real-time |
| `PluginEditor` | `Source/PluginEditor.cpp` | Extended with transport bar area (standalone only) and drag & drop support |

### Standalone Detection

The editor detects standalone mode at runtime:

```cpp
// In PluginEditor — standalone detection
bool isStandalone = (juce::PluginHostType().getPluginLoadedAs() ==
                     juce::AudioProcessor::wrapperType_Standalone);
```

When running as VST3/AU, the transport bar and file player UI are hidden. The editor height remains 350px for plugins and grows for standalone.

---

## Subtasks

### Subtask 1: Audio File Loader

**Objective:** Load WAV/AIFF files and make them available as an audio source.

**Files to create:**
- `Source/Standalone/FilePlayerSource.h`
- `Source/Standalone/FilePlayerSource.cpp`

**Scope:**
- `AudioFormatManager` configured for WAV and AIFF
- Load file from `juce::File` (used by both Open dialog and drag & drop)
- Store loaded audio in `AudioFormatReaderSource`
- Expose file metadata: sample rate, length in samples, number of channels
- Generate `AudioThumbnail` for dry waveform on load
- Handle sample rate mismatch (resample if file SR ≠ device SR)
- Validate file: reject non-audio files, show error for unsupported formats

**Unit tests:**
- Load valid WAV → verify sample count, channel count, sample rate
- Load valid AIFF → verify same
- Reject invalid file → verify error state
- Thumbnail generation completes without crash
- Sample rate mismatch handling produces valid output

---

### Subtask 2: Audio Transport

**Objective:** Play, stop, loop, and seek through loaded audio files. Inject file audio into the processor, replacing device input.

**Files to modify/create:**
- `Source/Standalone/FilePlayerSource.h/.cpp` (extend with transport)
- `Source/PluginProcessor.h/.cpp` (add file audio injection point for standalone)

**Scope:**
- `AudioTransportSource` wrapping the reader source
- Play / Stop / Loop toggle
- Seek to position (in seconds)
- When playing: file audio replaces device input in `processBlock`
- When stopped: device input resumes (live audio passthrough)
- Pipeline reset on seek to avoid filter artifacts
- Thread-safe state communication between audio thread and UI

**Unit tests:**
- Play starts from position 0
- Stop pauses at current position
- Loop restarts at beginning when reaching end
- Seek sets position correctly
- Pipeline reset on seek produces no NaN/Inf
- File audio replaces device input (output matches processed file, not silence)

---

### Subtask 3: Transport Bar UI

**Objective:** Add a transport bar below the footer with playback controls and progress indicator.

**Files to modify/create:**
- `Source/Standalone/TransportBar.h`
- `Source/Standalone/TransportBar.cpp`
- `Source/PluginEditor.h/.cpp` (extend layout for standalone)

**Scope:**
- Buttons: Open File, Play/Stop (toggle), Loop (toggle), Export
- Progress bar showing current position / total duration
- Click on progress bar to seek
- Time display (current / total in MM:SS format)
- Standalone detection: transport bar only visible in standalone mode
- Editor height increases when in standalone mode (350 → ~550px)
- Visual style consistent with existing GrainColours palette

**Unit tests:**
- Transport bar hidden in non-standalone mode (editor height = 350)
- Transport bar visible in standalone mode (editor height > 350)
- Button states reflect transport state (play → shows stop icon, etc.)

---

### Subtask 4: Waveform Display

**Objective:** Show dry waveform (pre-rendered) and wet waveform (real-time) superimposed with distinct visual treatment.

**Files to create:**
- `Source/Standalone/WaveformDisplay.h`
- `Source/Standalone/WaveformDisplay.cpp`

**Scope:**
- Dry waveform: rendered from `AudioThumbnail` on file load, static, semi-transparent color
- Wet waveform: accumulated in real-time from processed output, different color/opacity
- Both waveforms superimposed in the same area
- Playback cursor (vertical line) showing current position
- Click to seek (sends position to `FilePlayerSource`)
- Scroll/zoom not required for V1 (full file visible at all times)

**Unit tests:**
- Waveform component renders without crash for empty state
- Waveform component renders without crash for loaded file
- Click position maps correctly to file time position
- Wet waveform buffer accumulates samples correctly

---

### Subtask 5: Drag & Drop

**Objective:** Allow dragging audio files onto the standalone window to load them.

**Files to modify:**
- `Source/PluginEditor.h/.cpp` (add `FileDragAndDropTarget`)

**Scope:**
- `PluginEditor` implements `juce::FileDragAndDropTarget`
- Accept `.wav` and `.aiff`/`.aif` extensions
- Reject other file types (show visual feedback: red border)
- Visual feedback on drag hover (highlight border)
- On drop: load file via `FilePlayerSource`
- Only active in standalone mode

**Unit tests:**
- `isInterestedInFileDrag` returns true for .wav
- `isInterestedInFileDrag` returns true for .aiff/.aif
- `isInterestedInFileDrag` returns false for .mp3, .txt, etc.

---

### Subtask 6: Real-Time Recorder & Export

**Objective:** Record the processed output to a WAV file in real-time, with an Export workflow that plays the full file while recording.

**Files to create:**
- `Source/Standalone/AudioRecorder.h`
- `Source/Standalone/AudioRecorder.cpp`

**Scope:**
- `AudioRecorder` class that writes processed output samples to a WAV file
- Recording happens in the audio thread (lock-free write to a `TimeSliceThread` writer)
- Export workflow:
  1. User clicks "Export"
  2. Save As dialog opens → user picks destination
  3. Playback rewinds to start and begins playing
  4. Recorder captures all processed output
  5. When playback reaches end (or user stops): file is finalized and closed
- WAV output: same sample rate and channel count as the loaded file
- Thread-safe start/stop from UI thread

**Unit tests:**
- Recorder produces valid WAV file header
- Recorded file length matches source file length (within tolerance)
- Recorded file is not silent (RMS > 0)
- Recorder handles stop mid-file gracefully (no corrupt file)
- Export workflow: output file sample rate matches input file sample rate

---

## Window Layout (Standalone)

```
┌──────────────────────────────────────────────┐
│  Header (50px): "GRAIN" + BYPASS             │  ← existing
├──────────────────────────────────────────────┤
│  Main (200px): GRAIN + WARMTH knobs          │  ← existing
│  [IN meters]              [OUT meters]       │
├──────────────────────────────────────────────┤
│  Footer (100px): INPUT | MIX | FOCUS | OUT   │  ← existing
├──────────────────────────────────────────────┤
│  Waveform (120px):                           │  ← NEW
│  [dry waveform (semi-transparent)]           │
│  [wet waveform (overlay, real-time)]         │
│  [playback cursor]                           │
├──────────────────────────────────────────────┤
│  Transport (50px):                           │  ← NEW
│  [Open] [▶/■] [↻] [Export]  00:12 / 01:45  │
│  [═══════════●════════════════════]          │
└──────────────────────────────────────────────┘
```

**Standalone editor size:** 500 × 520px (350 existing + 120 waveform + 50 transport)

---

## Files to Create/Modify

| File | Action | Subtask |
|------|--------|---------|
| `Source/Standalone/FilePlayerSource.h` | Create | 1, 2 |
| `Source/Standalone/FilePlayerSource.cpp` | Create | 1, 2 |
| `Source/Standalone/TransportBar.h` | Create | 3 |
| `Source/Standalone/TransportBar.cpp` | Create | 3 |
| `Source/Standalone/WaveformDisplay.h` | Create | 4 |
| `Source/Standalone/WaveformDisplay.cpp` | Create | 4 |
| `Source/Standalone/AudioRecorder.h` | Create | 6 |
| `Source/Standalone/AudioRecorder.cpp` | Create | 6 |
| `Source/PluginProcessor.h` | Modify | 2 |
| `Source/PluginProcessor.cpp` | Modify | 2 |
| `Source/PluginEditor.h` | Modify | 3, 5 |
| `Source/PluginEditor.cpp` | Modify | 3, 5 |
| `Source/Tests/FilePlayerTest.cpp` | Create | 1, 2, 6 |
| `Source/Tests/WaveformTest.cpp` | Create | 4 |
| `Source/Tests/DragDropTest.cpp` | Create | 5 |
| `GRAIN.jucer` | Modify | 1 (add Standalone group + files) |
| `GRAINTests.jucer` | Modify | 1 (add test files) |

---

## Acceptance Criteria

### Functional
- [ ] Open WAV/AIFF via file dialog
- [ ] Open WAV/AIFF via drag & drop
- [ ] Play/Stop/Loop controls work
- [ ] Seek by clicking on waveform or progress bar
- [ ] Audio from file passes through GRAIN DSP pipeline
- [ ] Dry waveform displayed on file load
- [ ] Wet waveform updates in real-time during playback
- [ ] Export produces a valid WAV file with processed audio
- [ ] Device input muted during file playback, resumes on stop

### Quality
- [ ] No audio glitches during playback
- [ ] No clicks on seek (pipeline reset)
- [ ] No UI lag during waveform rendering
- [ ] Exported WAV matches source duration and sample rate
- [ ] Drag & drop rejects non-audio files with visual feedback

### Platform
- [ ] All features work on macOS Apple Silicon (ARM64)
- [ ] Transport bar hidden when running as VST3/AU
- [ ] Window resizes correctly for standalone mode

---

## Branching Strategy

```
main
 └── feature/009b-file-player                    ← main task branch
      ├── feature/009b-file-player/001-file-loader    ← subtask 1
      ├── feature/009b-file-player/002-transport       ← subtask 2
      ├── feature/009b-file-player/003-transport-ui    ← subtask 3
      ├── feature/009b-file-player/004-waveform        ← subtask 4
      ├── feature/009b-file-player/005-drag-drop       ← subtask 5
      └── feature/009b-file-player/006-recorder        ← subtask 6
```

Each subtask branch is created from `feature/009b-file-player`.
PRs go from subtask branch → `feature/009b-file-player`.
Final PR goes from `feature/009b-file-player` → `main`.

---

## Out of Scope

- MP3/FLAC/OGG format support
- Offline bounce (4× oversampling export)
- Waveform zoom/scroll
- Multiple file loading / playlist
- A/B switch button (dry vs wet monitoring)
- Audio file metadata display (bit depth, etc.)
