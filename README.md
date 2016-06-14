# vuled8

##Arduino VU Meter with Analog Multiplexer

This code allows an analog signal such as audio to be metered directly from an analog input pin (although the input will need to be at least biased with a voltage divider and isolated with a coupling cap).

Unlike other Arduino based VU meters, this code can reliably switch to and read other analog inputs infrequently which can be non-trivial. See the Infrequently Switching Analog Inputs section for details.

## VU Meter Details

A VU meter is modeled by a full-wave rectifier, 2nd order LP filter of ~8 Hz, scaling and log conversion such as described in "A model of the VU (volume-unit) meter, with speech applications" Lobdell 2006.
It turns out that this lends itself quite well to an efficient software implementation as it can use a standard recursion filter and does not require square root.
This Arduino software based 8 LED VU meter implements this model although it uses undersampling, it simplifies the scaling and log conversion by using a threshold table and it completely ignores finer details such as rectifier non-linearity.
Undersampling at less than 10 kHz was measured to be accurate to within 1 dB over 200 Hz to 18 kHz.

Additionally, general purpose multiplexing code (adcmux.c) is included to allow the user to also read relatively infrequently but regularly and *reliably* from other analog inputs concurrently such as those connected to potentiometers, slow sensors or similar.

Currently this code is somewhat specific to the ATmega32U4 as it directly manipulates ADC registers and uses chip specific pin tables but porting to other chips is a relatively simple matter of looking up datasheet values.

Although the code is organized so that multiple analog signals could be metered in the same sketch, this would need to be explored carefully because the ADC is already limited to a sample rate of about 9.5 kHz when reading primarily just one pin.

The vuled8.{h,c} and adcmux.{h,c} files can be integrated into an application but a working VuMeter.ino sketch for the Arduino Micro is provided.
This particular sketch uses "bar" mode to indicate the level as well as a "dot" mode to hold the peak of the level (currently hardcoded at 4000 samples which is ~420 ms).
This sketch also reads from one potentiometer that cycles the VU meter behavior between states:

    off
      0 dBu ref bar
      0 dBu ref bar w/ peak hold dot
    off
     +4 dBu ref bar
     +4 dBu ref bar w/ peak hold dot
    off
    +12 dBu ref bar
    +12 dBu ref bar w/ peak hold dot

### Input Circuit

The input circuit is just a coupling cap w/ series resistor into voltage divider to both step down and bias the input to 1/2 of the digital supply.
The voltage divider should be no larger than 22K||22K to satisfy the maximum source impedance for the ADC of 10K. The series resistor can be adjusted to step down the input voltage as necessary. For example 100n and 68K into 11K (22K||22K) might be used to step down from pro-audio levels.

## The adcmux API

This code includes a general purpose multiplexer module (adcmux.{h,c}) that can be used with any project and is not specific to a VU meter application.
Aside from using Free Running and High Speed modes (see Infrequently Switching Analog Inputs below for why this is important), this module provides a simple way to instruct the reading function to evenly distribute reads. This is particularly useful (perhaps only really useful) when reading most samples from one analog input whilst also occasionally reading from one or more or possibly many other analog inputs.

The desired behavior is specified by passing a `struct adcmux_chan` array to `adcmux_init` before calling `adcmux_read`. Consider the following example:

    struct adcmux_chan chans[5] = {
    	{ 0, 1 },   // VU meter
    	{ 1, 50 },  // gain reduction circuit
    	{ 4, 400 }, // potentiometer 0
    	{ 2, 400 }, // potentiometer 1
    	{ 6, 400 }, // potentiometer 2
    };

The above example means that 1 in 1 samples will be read from pin 0, 1 in 50 samples will be read from pin 1 and 1 in every 400 samples will be read from pins 4, 2 and 6. Of course these ratios are not exact. The ratio value is actually a modulus that is compared to a sample count. So when count % modu == 0 one or more subsequent samples will be diverted to other pins. Meaning there will be gaps in the data. So in this example, the channel for pin 0 will actually only get 389 of 400 samples (11 in 400 samples will go to other pins).

Note that the pins can be any analog pin. So you can easily switch the VU meter source by simply changing the pin number in the array.

## Infrequently Switching Analog Inputs

The ATmega32U4 only has one ADC. So the chip uses gates to connect the desired pin to the ADC for reading. Meaning you can only read one pin at a time. This can lead to unexpected behavior.
After some experimentation it was determined to be vitally important to set ADHSM of the ADCSRB register when *infrequently* switching analog inputs with the ATmega32U4.
Specifically, if you try to read samples mostly from one input but *also* from other inputs much less occasionally (like say 200 to 1 or even 5 to 1), it is very possible to read a garbage value from the analog input being read less frequently.
The severity of the issue increases with the time between reads and noise level of surrounding circuitry (it could be that the sample and hold circuit is drifting while it's gate is left open?).
Anyway, after turning on ADHSM, the issue of reading invalid data when infrequently switching analog inputs was completely resolved in this case. Reading a potentiometer once every 200 samples works flawlessly.

Note that, ironically, the High Speed mode does not result in higher speed in this case. In practice it is actually slower. The exact sampling rate actually depends more on the amount of code executed in the loop. At initial check-in, the sketch reads samples about every 105 us or ~9.5 kHz. The ADC prescaler does not appear to have any effect in High Speed mode (currently set to 32 which without switching was measured at almost 42 kHz).
So if your application is simply reading from one pin or alternating reads between two pins, it is not necessary to use adcmux and it may not be necessary or desirable to use ADHSM.

## TODO:

Instead of scaling the thresholds to switch reference level, it might be better numerically, require less code and make calibration easier to just use a fixed log table and instead adjust the multiplier used to scale incoming sample values (currently hardcoded at 100).

