/*
  ==============================================================================

    DragDropTest.cpp
    Unit tests for the standalone drag & drop file loading (GT-19).
    Tests file extension filtering via AudioFileUtils::isSupportedAudioFile.

  ==============================================================================
*/

#include "../Standalone/AudioFileUtils.h"

#include <JuceHeader.h>

//==============================================================================
class DragDropTest : public juce::UnitTest
{
public:
    DragDropTest() : juce::UnitTest("GRAIN Drag & Drop") {}

    void runTest() override
    {
        runAcceptWavTest();
        runAcceptAiffTest();
        runRejectUnsupportedTest();
    }

private:
    //==========================================================================
    void runAcceptWavTest()
    {
        beginTest("DragDrop: accepts .wav files");

        expect(AudioFileUtils::isSupportedAudioFile("/path/to/file.wav"), ".wav should be accepted");
        expect(AudioFileUtils::isSupportedAudioFile("/path/to/FILE.WAV"), ".WAV (uppercase) should be accepted");
        expect(AudioFileUtils::isSupportedAudioFile("/Users/test/Music/test.wav"), "Deep path .wav should be accepted");
    }

    //==========================================================================
    void runAcceptAiffTest()
    {
        beginTest("DragDrop: accepts .aiff/.aif files");

        expect(AudioFileUtils::isSupportedAudioFile("/path/to/file.aiff"), ".aiff should be accepted");
        expect(AudioFileUtils::isSupportedAudioFile("/path/to/file.aif"), ".aif should be accepted");
        expect(AudioFileUtils::isSupportedAudioFile("/path/to/FILE.AIFF"), ".AIFF (uppercase) should be accepted");
        expect(AudioFileUtils::isSupportedAudioFile("/path/to/FILE.AIF"), ".AIF (uppercase) should be accepted");
    }

    //==========================================================================
    void runRejectUnsupportedTest()
    {
        beginTest("DragDrop: rejects unsupported file types");

        expect(!AudioFileUtils::isSupportedAudioFile("/path/to/file.mp3"), ".mp3 should be rejected");
        expect(!AudioFileUtils::isSupportedAudioFile("/path/to/file.txt"), ".txt should be rejected");
        expect(!AudioFileUtils::isSupportedAudioFile("/path/to/file.flac"), ".flac should be rejected");
        expect(!AudioFileUtils::isSupportedAudioFile("/path/to/file.ogg"), ".ogg should be rejected");
        expect(!AudioFileUtils::isSupportedAudioFile("/path/to/file.m4a"), ".m4a should be rejected");
        expect(!AudioFileUtils::isSupportedAudioFile("/path/to/file"), "No extension should be rejected");
    }
};

//==============================================================================
static DragDropTest
    dragDropTest;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables,misc-use-anonymous-namespace)
