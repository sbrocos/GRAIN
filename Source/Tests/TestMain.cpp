/*
  ==============================================================================

    TestMain.cpp
    Entry point for the GRAINTests console application.
    Runs all registered juce::UnitTest instances and returns exit code.

  ==============================================================================
*/

#include <JuceHeader.h>

int main()
{
    juce::ScopedJuceInitialiser_GUI const init;
    juce::UnitTestRunner runner;
    runner.runAllTests();

    int failures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        if (runner.getResult(i)->failures > 0)
        {
            failures += runner.getResult(i)->failures;
        }
    }

    if (failures > 0)
    {
        std::cout << "\n*** " << failures << " test(s) FAILED ***\n";
    }
    else
    {
        std::cout << "\n*** All tests PASSED ***\n";
    }

    return failures > 0 ? 1 : 0;
}
