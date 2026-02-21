# Installation Guide

## VST3 Plugin

1. Locate the `GRAIN.vst3` file
2. Copy to your VST3 folder:
   - System: `/Library/Audio/Plug-Ins/VST3/`
   - User: `~/Library/Audio/Plug-Ins/VST3/`
3. Restart your DAW
4. Scan for new plugins if necessary

## AU Plugin (Audio Unit)

1. Locate the `GRAIN.component` file
2. Copy to your Components folder:
   - System: `/Library/Audio/Plug-Ins/Components/`
   - User: `~/Library/Audio/Plug-Ins/Components/`
3. Restart your DAW
4. The plugin should appear under **BrocosWave > GRAIN**

## Standalone Application

1. Locate the `GRAIN.app` file
2. Copy to `/Applications/` (optional)
3. Double-click to launch
4. Grant microphone permission when prompted
5. Select audio input/output devices

## Troubleshooting

### Plugin not appearing in DAW

- Verify the `.vst3` or `.component` file is in the correct folder
- Rescan plugins in your DAW
- Check DAW's plugin manager for blacklisted plugins

### Audio not passing through standalone

- Check audio device selection
- Verify microphone permissions in System Preferences > Privacy & Security
- Ensure input/output devices are working

### AU not validated

- Run `auval -v aufx Grn1 BrWv` in Terminal to check AU validation
- If validation fails, reinstall the `.component` file
