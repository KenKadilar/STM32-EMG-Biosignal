// Notch : 50 Hz biquad notch on the centered signal, kills mains hum. filter() once per sample.
#ifndef NOTCH_H
#define NOTCH_H

class Notch
{
  public:
    float filter(float x)
    {
        float y = B0 * x + B2 * x2 - A2 * y2;   // a 2-samples-back filter; the y feedback nulls 50 Hz
        x2 = x1; x1 = x;
        y2 = y1; y1 = y;
        return y;
    }

  private:
    // These three constants = a 50 Hz notch at the 200 Hz sample rate (unity gain everywhere else).
    // Recompute them (a filter calculator) for a different rate or mains frequency, e.g. Canada's 60 Hz.
    const float B0 = 0.833333f;
    const float B2 = 0.833333f;
    const float A2 = 0.666667f;
    float x1 = 0, x2 = 0, y1 = 0, y2 = 0;   // filter memory: last two inputs and outputs
};
#endif
