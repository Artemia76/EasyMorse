// Tests of the band simulator: receiver filter, noises, fading, AGC, stations
#include <QTest>

#include <cmath>
#include <numbers>
#include <vector>

#include "audio/band/bandsimulator.h"

using band::BandSimulator;
using band::StationScript;
using P = BandSimulator::Parameter;

namespace
{
    constexpr double Rate = 48000.0;
    constexpr double Gain = 0.1;
    const double ToneRms = Gain / std::numbers::sqrt2;

    std::vector<float> render(BandSimulator& pBand, double pSeconds)
    {
        std::vector<float> samples(static_cast<std::size_t>(pSeconds * Rate));
        for (float& sample : samples)
            sample = pBand.next();
        return samples;
    }

    double rms(const std::vector<float>& pSamples, std::size_t pFrom = 0, std::size_t pTo = 0)
    {
        if (pTo == 0)
            pTo = pSamples.size();
        double sum = 0.0;
        for (std::size_t i = pFrom; i < pTo; ++i)
            sum += double(pSamples[i]) * pSamples[i];
        return std::sqrt(sum / double(pTo - pFrom));
    }

    // Power of one frequency (Goertzel on Hann windowed 40 ms blocks, averaged)
    double power(const std::vector<double>& pSamples, double pHz)
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
                const double window = 0.5 - 0.5 * std::cos(2.0 * std::numbers::pi * double(i - start) / block);
                const double s = window * pSamples[i] + coefficient * s1 - s2;
                s2 = s1;
                s1 = s;
            }
            total += s1 * s1 + s2 * s2 - coefficient * s1 * s2;
        }
        return total / double(blocks);
    }
    double power(const std::vector<float>& pSamples, double pHz)
    {
        return power(std::vector<double>(pSamples.begin(), pSamples.end()), pHz);
    }

    BandSimulator quietBand(std::uint64_t pSeed = 1)
    {
        BandSimulator band(Rate, pSeed);
        band.setGain(Gain);
        band.setFrequency(700);
        return band;
    }

    // A background station holding its key down for pSeconds
    StationScript carrier(int pSlot, double pOffsetHz, double pLevelDb, double pSeconds)
    {
        StationScript script;
        script.slot = pSlot;
        script.offsetHz = static_cast<float>(pOffsetHz);
        script.levelDb = static_cast<float>(pLevelDb);
        script.count = 2;
        script.durations[0] = 0;
        script.durations[1] = static_cast<std::int32_t>(pSeconds * Rate);
        return script;
    }
}

class TestBandSimulator : public QObject
{
    Q_OBJECT

private slots:
    void silentWhenIdle()
    {
        auto band = quietBand();
        for (float sample : render(band, 0.2))
            QCOMPARE(sample, 0.0f);
    }

    void signalAndSidetoneWithoutClicks()
    {
        auto band = quietBand();
        band.setSignal(true);
        const auto on = render(band, 0.3);
        QVERIFY(std::abs(on[0]) < Gain * 0.01);
        QVERIFY(std::abs(rms(on, 4800) - ToneRms) < ToneRms * 0.02);
        band.setSignal(false);
        const auto off = render(band, 0.1);
        QVERIFY(rms(off, 2400) < ToneRms * 0.001);

        auto own = quietBand();
        own.setSidetone(true);
        const auto sidetone = render(own, 0.2);
        QVERIFY(std::abs(rms(sidetone, 480) - ToneRms) < ToneRms * 0.01);
        const double maxStep = Gain * 2.0 * std::numbers::pi * 700 / Rate * 1.05;
        for (std::size_t i = 1; i < sidetone.size(); ++i)
            QVERIFY(std::abs(sidetone[i] - sidetone[i - 1]) <= maxStep);
    }

    void filterSkirtsAreSteep()
    {
        // 500 Hz filter: -3 dB at 250 Hz from the tone, below -40 dB at 1 kHz
        const auto heard = [](double pOffset) {
            auto band = quietBand();
            band.play(carrier(0, pOffset, 0.0, 1.0));
            return rms(render(band, 0.8), 9600) / ToneRms;
        };
        QVERIFY(std::abs(heard(0.0) - 1.0) < 0.02);
        QVERIFY(std::abs(heard(250.0) - std::numbers::sqrt2 / 2) < 0.03);
        QVERIFY(std::abs(heard(-250.0) - std::numbers::sqrt2 / 2) < 0.03);
        QVERIFY(heard(1000.0) < 0.01);
        QVERIFY(heard(-1000.0) < 0.01);
    }

    void noiseLevelFollowsTheBandwidth()
    {
        const auto noise = [](double pBandwidth) {
            auto band = quietBand();
            band.set(P::NoiseLevel, 1.0);
            band.set(P::Bandwidth, pBandwidth);
            return rms(render(band, 5.0), 4800);
        };
        // Level 1: as strong as the tone in 500 Hz; four times wider, twice louder
        const double narrow = noise(500);
        QVERIFY2(std::abs(narrow / ToneRms - 1.0) < 0.1, qPrintable(QString::number(narrow / ToneRms)));
        QVERIFY2(std::abs(noise(2000) / narrow - 2.0) < 0.2, qPrintable(QString::number(noise(2000) / narrow)));
    }

    void noiseStaysAroundTheTone()
    {
        auto band = quietBand();
        band.set(P::NoiseLevel, 1.0);
        const auto noise = render(band, 4.0);
        QVERIFY(power(noise, 700) > 1000.0 * power(noise, 1700));
        // 500 Hz away = twice the cutoff: about -24 dB
        QVERIFY(power(noise, 700) > 150.0 * power(noise, 200));
    }

    void qsbFadesSlowly()
    {
        auto band = quietBand();
        band.set(P::QsbDepth, 0.8);
        band.set(P::QsbRate, 0.2);
        double lowest = 10.0, highest = 0.0, previous = -1.0;
        for (int i = 0; i < 48000 * 120; ++i)
        {
            band.next();
            const double gain = band.mainQsbGain();
            QVERIFY(gain >= 0.2 - 1e-9 && gain <= 1.8 + 1e-9);
            if (previous >= 0.0)
                QVERIFY(std::abs(gain - previous) < 0.002);
            previous = gain;
            lowest = std::min(lowest, gain);
            highest = std::max(highest, gain);
        }
        QVERIFY(lowest < 0.5);
        QVERIFY(highest > 1.1);
    }

    void flutterRoughensTheTone()
    {
        // Spread of the level measured over 10 ms windows
        const auto roughness = [](double pFlutter) {
            auto band = quietBand();
            band.set(P::Flutter, pFlutter);
            band.setSignal(true);
            const auto tone = render(band, 3.0);
            std::vector<double> levels;
            for (std::size_t start = 4800; start + 480 <= tone.size(); start += 480)
                levels.push_back(rms(tone, start, start + 480));
            double mean = 0.0, square = 0.0;
            for (double level : levels)
                mean += level / levels.size();
            for (double level : levels)
                square += (level - mean) * (level - mean) / levels.size();
            return std::sqrt(square) / mean;
        };
        QVERIFY(roughness(0.0) < 0.02);
        QVERIFY(roughness(25.0) > 0.15);
    }

    void staticCrashesComeAndGo()
    {
        auto band = quietBand();
        band.set(P::QrnLevel, 1.0);
        band.set(P::QrnRate, 30.0);
        const auto noise = render(band, 30.0);
        double peak = 0.0;
        for (float sample : noise)
        {
            QVERIFY(std::isfinite(sample));
            peak = std::max(peak, double(std::abs(sample)));
        }
        // Loud crashes over a much weaker crackle
        QVERIFY2(peak > 4.0 * rms(noise), qPrintable(QString::number(peak / rms(noise))));
        QVERIFY(peak < 30.0 * Gain);
    }

    void mainsBuzzAt100Hz()
    {
        auto band = quietBand();
        band.set(P::ManMade, 1.0);
        const auto noise = render(band, 4.0);
        // Envelope of the buzz: impulses every 10 ms
        std::vector<double> envelope;
        double smooth = 0.0;
        for (float sample : noise)
            envelope.push_back(smooth += (std::abs(sample) - smooth) * 0.05);
        QVERIFY(power(envelope, 100) > 10.0 * power(envelope, 170));
    }

    void agcLiftsAQuietBandAndTamesStrongSignals()
    {
        auto off = quietBand(3);
        off.set(P::NoiseLevel, 0.05);
        auto on = quietBand(3);
        on.set(P::NoiseLevel, 0.05);
        on.set(P::Agc, 1);
        QVERIFY(rms(render(on, 3.0), 48000) > 2.0 * rms(render(off, 3.0), 48000));

        auto strong = quietBand();
        strong.set(P::Agc, 1);
        strong.play(carrier(1, 0.0, 12.0, 2.0));
        const auto loud = render(strong, 1.5);
        QVERIFY(std::abs(rms(loud, 24000) - ToneRms) < ToneRms * 0.1);
    }

    void agcDoesNotPopAfterAWordGap()
    {
        auto band = quietBand();
        band.set(P::NoiseLevel, 0.1);
        band.set(P::Agc, 1);
        double steady = 0.0, peak = 0.0;
        for (int word = 0; word < 6; ++word)
        {
            band.setSignal(true);
            const auto tone = render(band, 0.3);
            if (word > 0)
                for (std::size_t i = 0; i < 960; ++i)
                    peak = std::max(peak, double(std::abs(tone[i])));
            steady = rms(tone, 4800) * std::numbers::sqrt2;
            band.setSignal(false);
            render(band, 0.45);        // word gap at 15 wpm
        }
        QVERIFY2(peak < 1.3 * steady, qPrintable(QString::number(peak / steady)));
    }

    void stationsReportTheEndOfTheirMessage()
    {
        auto band = quietBand();
        QCOMPARE(band.takeFinished(), -1);
        StationScript script = carrier(3, 300.0, 0.0, 0.1);
        script.durations[0] = 4800;        // 0.1 s of silence first
        band.play(script);
        render(band, 0.15);
        QCOMPARE(band.takeFinished(), -1);
        render(band, 0.1);
        QCOMPARE(band.takeFinished(), 3);
        QCOMPARE(band.takeFinished(), -1);

        // An empty script silences the slot at once
        band.play(carrier(5, 0.0, 0.0, 10.0));
        render(band, 0.1);
        StationScript stop;
        stop.slot = 5;
        band.play(stop);
        render(band, 0.1);
        QVERIFY(rms(render(band, 0.1)) < ToneRms * 0.001);
        QCOMPARE(band.takeFinished(), -1);
    }
};

QTEST_GUILESS_MAIN(TestBandSimulator)
#include "tst_bandsimulator.moc"
