# Task 009: Standalone Application

## Objective

Deliver a minimal standalone application for real-time audio monitoring with GRAIN processing. The app allows testing and basic usage without a DAW.

---

## Prerequisites

- Task 008 completed (Plugin Editor with functional layout)

---

## Design Rationale

### What Is the Standalone App?

From the PRD:
> "Standalone: real-time monitoring with device selector, bypass, and meters."

A simple wrapper around the same DSP engine and editor used in the VST3 plugin. Not a DAW, not a recorder — just a real-time passthrough processor.

### Why After the Editor?

The standalone app uses the same `GrainEditor` component. Having the editor ready (Task 008) means the standalone gets a proper UI for free.

---

## Architecture

### JUCE Standalone Wrapper

JUCE automatically generates a standalone app when enabled in Projucer. It wraps the same `AudioProcessor` in a `StandaloneFilterWindow`:

```
┌──────────────────────────────────────┐
│       GRAIN Standalone Window        │
│                                      │
│  ┌────────────────────────────────┐  │
│  │    Audio Device Settings ⚙️    │  │
│  │  (JUCE built-in dialog)       │  │
│  └────────────────────────────────┘  │
│                                      │
│  ┌────────────────────────────────┐  │
│  │    GrainEditor (same as VST3) │  │
│  │    - Knobs, meters, bypass    │  │
│  │    - All controls functional  │  │
│  └────────────────────────────────┘  │
│                                      │
└──────────────────────────────────────┘
```

### What JUCE Provides Automatically

| Feature | Provided | Custom work |
|---------|----------|-------------|
| Audio device selector | ✅ Built-in dialog | None |
| Audio I/O routing | ✅ Automatic | None |
| Plugin editor display | ✅ Wraps GrainEditor | None |
| Window management | ✅ StandaloneFilterWindow | None |
| Settings persistence | ✅ Saves last device | None |

---

## Implementation

### Step 1: Verify Standalone in Projucer

1. Open `GRAIN.jucer`
2. Click on project settings (top-left)
3. Under **Plugin Formats**, verify **Standalone** is checked
4. Under **Standalone Plugin** settings:

| Setting | Value |
|---------|-------|
| **Enable MIDI Input** | No |
| **Enable MIDI Output** | No |
| **Audio Input Required** | Yes |
| **Audio Output Required** | Yes |

5. Save and re-export to Xcode

### Step 2: macOS Audio Permissions

macOS requires microphone permission for audio input.

1. Open `GRAIN.jucer`
2. Under **Xcode (macOS)** exporter settings
3. **Microphone Access** → Enable
4. **Microphone Usage Description** → "GRAIN needs audio input access for real-time processing"

### Step 3: Verify Device Settings Dialog

JUCE's standalone wrapper includes a settings button. Verify it opens and allows:
- Input device selection
- Output device selection
- Sample rate selection
- Buffer size selection

```cpp
// No custom code needed — JUCE handles this via
// StandalonePluginHolder::showAudioSettingsDialog()
```

### Step 4: Handle Edge Cases

```cpp
// In PluginProcessor.cpp — prepareToPlay
void prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Standalone is always real-time
    // isNonRealtime() returns false
    // Oversampling will be 2× (from Task 007)
    
    // ... existing prepare code ...
}
```

---

## Standalone-Specific Considerations

### Oversampling

Standalone is always real-time, so oversampling is always 2× (Task 007's `isNonRealtime()` returns false).

### No Audio File Playback

From the PRD:
> "Out of scope: Audio file player/recorder in the standalone app."

The standalone processes live audio input only.

### Device Disconnection

JUCE handles device disconnection gracefully. The standalone will show an error dialog if the device is lost. No custom handling needed for V1.

### Sample Rate Changes

If the user changes sample rate in the device settings, JUCE calls `prepareToPlay()` again. All DSP modules will be re-initialized (Task 007 handles this).

---

## Unit Tests

Most testing is already covered by previous tasks (same DSP engine). Standalone-specific tests are minimal:

```cpp
void runStandaloneTests()
{
    beginTest("Standalone: meter level is non-negative");
    {
        juce::AudioBuffer<float> buffer(2, TestConstants::BUFFER_SIZE);

        // Fill with sine wave
        for (int i = 0; i < TestConstants::BUFFER_SIZE; ++i)
        {
            float sample = 0.5f * std::sin(
                2.0f * juce::MathConstants<float>::pi * 440.0f * i / 44100.0f);
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
        }

        float magnitude = buffer.getMagnitude(0, 0, TestConstants::BUFFER_SIZE);
        expect(magnitude >= 0.0f);
        expect(magnitude <= 1.0f);
    }

    beginTest("Standalone: silence produces zero meter level");
    {
        juce::AudioBuffer<float> buffer(2, TestConstants::BUFFER_SIZE);
        buffer.clear();

        float magnitude = buffer.getMagnitude(0, 0, TestConstants::BUFFER_SIZE);
        expectWithinAbsoluteError(magnitude, 0.0f, TestConstants::TOLERANCE);
    }
}
```

Don't forget to call `runStandaloneTests()` from `runTest()`.

---

## Files to Create/Modify

| File | Action |
|------|--------|
| `GRAIN.jucer` | Verify standalone enabled, microphone permission |
| `Source/Tests/DSPTests.cpp` | Add standalone-specific tests |

**Note:** No new source files needed. The standalone uses the same `PluginProcessor` and `PluginEditor`.

---

## Acceptance Criteria

### Functional
- [ ] Standalone app compiles and launches
- [ ] Audio device settings dialog opens
- [ ] Input and output devices selectable
- [ ] Audio passes through with processing
- [ ] All parameters controllable (same UI as VST3)
- [ ] Bypass button works
- [ ] Meters respond to audio

### Quality
- [ ] No audio glitches or dropouts
- [ ] App handles device disconnection gracefully
- [ ] App handles sample rate changes
- [ ] Settings persist between launches

### Platform
- [ ] Runs on macOS Apple Silicon (ARM64)
- [ ] Microphone permission requested on first launch
- [ ] Window displays correctly

---

## Testing Checklist

```bash
# 1. Build standalone
xcodebuild -project Builds/MacOSX/GRAIN.xcodeproj \
  -scheme "GRAIN - Standalone Plugin" \
  -configuration Debug build

# 2. Launch
open /Users/sbrocos/workspace/TFM/GRAIN/Builds/MacOSX/build/Debug/GRAIN.app

# 3. Manual test
# - Verify microphone permission dialog appears
# - Open audio device settings
# - Select input device
# - Select output device
# - Play audio through input (mic or loopback)
# - Verify meters respond
# - Adjust all parameters
# - Toggle bypass
# - Change sample rate → verify no crash
# - Close and reopen → verify settings persist
```

---

## Technical Notes

### Audio Routing for Testing

To test without a microphone, use a loopback device:
- **BlackHole** (free): Routes audio between apps
- **Loopback** (paid): More flexible routing

Route DAW output → BlackHole → GRAIN Standalone input.

### App Bundle Location

After building:
```bash
# Debug build
/Users/sbrocos/workspace/TFM/GRAIN/Builds/MacOSX/build/Debug/GRAIN.app

# Release build
/Users/sbrocos/workspace/TFM/GRAIN/Builds/MacOSX/build/Release/GRAIN.app
```

### Code Signing for Distribution

For development, "Sign to Run Locally" is sufficient. For distribution:
1. Apple Developer account required
2. Notarization needed for macOS Gatekeeper
3. Out of scope for V1 (academic project)

---

## Out of Scope

- Audio file player/recorder (PRD: out of scope for V1)
- MIDI support
- Custom standalone window chrome
- Distribution/notarization (academic project)
