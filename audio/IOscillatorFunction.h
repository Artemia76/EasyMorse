/**
 * @copyright Copyright (c) 2020 Rico Luedeke
 * @author Rico Luedeke
 * @brief  This file is part of qt-port-synth (A baisc synthesizer using PortAudio and Qt5)
 * @date 2020-05-04
 * @see https://github.com/rluedek/qt-port-synth
 */

#ifndef IOSCILLATOR_FUNCTION_H
#define IOSCILLATOR_FUNCTION_H

class IOscillatorFunction
{
public:

virtual double calculate(double dTime, double dFrequency) = 0;

private:
};

class Sine : public IOscillatorFunction
{
public:
    //ctor
    Sine();

    //dtor
    virtual ~Sine();
  
    double calculate(double dTime, double dFrequency) override;
};

class Square : public IOscillatorFunction
{
public:
    //ctor
    Square();

    //dtor
    virtual ~Square();
  
    double calculate(double dTime, double dFrequency) override;
};

class Triangle : public IOscillatorFunction
{
public:
    //ctor
    Triangle();

    //dtor
    virtual ~Triangle();
  
    double calculate(double dTime, double dFrequency) override;
};

class Saw : public IOscillatorFunction
{
public:
    //ctor
    Saw();

    //dtor
    virtual ~Saw();
  
    double calculate(double dTime, double dFrequency) override;
};

#endif
