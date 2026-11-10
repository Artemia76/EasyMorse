// Tests of the radio channel: clean keying, band noise and fading
#include <QTest>

#include <cmath>
#include <numbers>
#include <vector>

#include "audio/radiochannel.h"

namespace
{
    constexpr double Rate = 48000.0;
    constexpr double Gain = 0.1;

    std::vector<float> render(RadioChannel& pChannel, double pSeconds)
    {
        std::vector<float> samples(static_cast<std::size_t>(pSeconds * Rate));
        for (float& sample : samples)
            sample = pChannel.next();
        return samples;
    }

    double rms(const std::vector<float>& pSamples, std::size_t pFrom = 0)
    {
        double sum = 0.0;
        for (std::size_t i = pFrom; i < pSamples.size(); ++i)
            sum += double(pSamples[i]) * pSamples[i];
        return std::sqrt(sum / double(pSamples.size() - pFrom));
    }

    // Power of one frequency (Goertzel), averaged over 40 ms blocks
    double power(const std::vector<float>& pSamples, double pHz)
    {
        const double coefficient = 2.0 * std::cos(2.0 * std::numbers::pi * pHz / Rate);
        const std::size_t block = 1920;
        double total = 0.0;
        std::size_t blocks = 0;
        for (std::size_t start = 0; start + block <= pSamples.size(); start += block, ++blocks)
        {
            double s1 = 0.0, s2 = 0.0;
            for (std::size_t i = start; i < start + block; ++i)
            {
                const double s = pSamples[i] + coefficient * s1 - s2;
                s2 = s1;
                s1 = s;
            }
            total += s1 * s1 + s2 * s2 - coefficient * s1 * s2;
        }
        return total / double(blocks);
    }
}

class TestRadioChannel : public QObject
{
    Q_OBJECT

private slots:
    void silentWhenIdle()
    {
        RadioChannel channel(Rate);
        channel.setGain(Gain);
        for (float sample : render(channel, 0.1))
            QCOMPARE(sample, 0.0f);
    }

    void keyingHasNoClicks()
    {
        RadioChannel channel(Rate);
        channel.setGain(Gain);
        channel.setFrequency(700);
        channel.setSidetone(true);
        const auto on = render(channel, 0.2);
        // Soft start, then the full tone after the 5 ms edge
        QVERIFY(std::abs(on[0]) < Gain * 0.01);
        QVERIFY(std::abs(rms(on, 480) - Gain / std::numbers::sqrt2) < Gain * 0.01);

        channel.setSidetone(false);
        const auto off = render(channel, 0.02);
        for (std::size_t i = 240; i < off.size(); ++i)
            QCOMPARE(off[i], 0.0f);
        // No jump larger than the slope of the sine itself
        const double maxStep = Gain * 2.0 * std::numbers::pi * 700 / Rate * 1.05;
        for (std::size_t i = 1; i < off.size(); ++i)
            QVERIFY(std::abs(off[i] - off[i - 1]) <= maxStep);
    }

    void noiseStaysInTheReceiverBand()
    {
        RadioChannel channel(Rate);
        channel.setGain(Gain);
        channel.setFrequency(800);
        channel.setBandwidth(300);
        channel.setNoiseLevel(1.0);
        const auto noise = render(channel, 2.0);
        // Second order filter: about -19 dB at 3 kHz, -12 dB at 400 Hz
        QVERIFY(power(noise, 800) > 40.0 * power(noise, 3000));
        QVERIFY(power(noise, 800) > 8.0 * power(noise, 400));
    }

    void narrowFilterRemovesNoise()
    {
        RadioChannel wide(Rate);
        wide.setGain(Gain);
        wide.setNoiseLevel(1.0);
        wide.setBandwidth(2400);
        // Level 1 in 2.4 kHz: as strong as the tone
        const double wideRms = rms(render(wide, 5.0), 4800);
        QVERIFY2(std::abs(wideRms - Gain / std::numbers::sqrt2) < Gain * 0.1, qPrintable(QString::number(wideRms)));

        RadioChannel narrow(Rate);
        narrow.setGain(Gain);
        narrow.setNoiseLevel(1.0);
        narrow.setBandwidth(600);
        const double narrowRms = rms(render(narrow, 5.0), 4800);
        QVERIFY2(std::abs(narrowRms / wideRms - 0.5) < 0.07, qPrintable(QString::number(narrowRms / wideRms)));
    }

    void noFadingByDefault()
    {
        RadioChannel channel(Rate);
        for (int i = 0; i < 48000 * 20; ++i)
        {
            channel.next();
            QCOMPARE(channel.qsbGain(), 1.0);
        }
    }

    void qsbFadesTheSignalSlowly()
    {
        RadioChannel channel(Rate);
        channel.setQsbDepth(0.8);
        double lowest = 1.0, highest = 0.0, previous = channel.qsbGain();
        for (int i = 0; i < 48000 * 120; ++i)
        {
            channel.next();
            const double gain = channel.qsbGain();
            QVERIFY(gain >= 0.2 - 1e-9 && gain <= 1.0 + 1e-9);
            // Slow: less than 1 % per millisecond
            QVERIFY(std::abs(gain - previous) < 0.01 / 48.0);
            previous = gain;
            lowest = std::min(lowest, gain);
            highest = std::max(highest, gain);
        }
        QVERIFY(lowest < 0.5);
        QVERIFY(highest > 0.85);
    }

    void qsbSparesTheSidetone()
    {
        RadioChannel channel(Rate);
        channel.setGain(Gain);
        channel.setQsbDepth(1.0);
        channel.setSidetone(true);
        render(channel, 0.01);
        for (int second = 0; second < 30; ++second)
            QVERIFY(std::abs(rms(render(channel, 1.0)) - Gain / std::numbers::sqrt2) < Gain * 0.01);

        // The received signal follows the fading
        RadioChannel received(Rate);
        received.setGain(Gain);
        received.setQsbDepth(1.0);
        received.setSignal(true);
        render(received, 0.01);
        double lowest = 1.0;
        for (int second = 0; second < 60; ++second)
            lowest = std::min(lowest, rms(render(received, 0.5)) / (Gain / std::numbers::sqrt2));
        QVERIFY(lowest < 0.5);
    }
};

QTEST_GUILESS_MAIN(TestRadioChannel)
#include "tst_radiochannel.moc"
