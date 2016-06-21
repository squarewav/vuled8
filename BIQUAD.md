# vuled8 2nd Order LP Filter Details

A real mechanical VU meter can be modeled in software with a full-wave rectifier, 2nd order LP filter of ~8 Hz, scaling and log conversion such as described in "A model of the VU (volume-unit) meter, with speech applications" Lobdell 2006.

The 2nd order LP filter in vuled8 was implemented using decimation to reduce the sampling rate followed by an efficient integer based biquad filter. This is what gives the meter proper "needle ballistics".

## Biquad Filter Calibration

The filter coefficients depend on the sampling rate and the sampling rate depends on how much code is executed in the loop of your sketch. The current sketch yields a sampling period of 71 us or 14.1 kSps. You can print the sample period to the monitor by changing xDEBUG to DEBUG. If this number changes significantly, the biquad will need to be recomputed as follows:

1) Rename vuled8.c to vuled8.cpp so that Serial.print will work.

2) Find the sampling period by setting xDEBUG to DEBUG and look at the monitor.

3) Compute the samping rate with 1 / period. For example, 1 / 105 us = 9.5 kSps.

4) Divide this rate by the decimation factor (currently 32). For example, 9.5 kSps / 32 = 300 Sps.

5) Go to http://www.schwietering.com/jayduino/filtuino/ and select:

  Bessel > Low pass > 2nd order > sample rate 300 > Lower corner 10 Hz > double type > Send

Take the 3 lines like:

    v[2] = (1.430581490647872545e-2 * x)
       + (-0.62932850645198390449 * v[0])
       + (1.57210524682606900271 * v[1]);

and multiply the coefficients by some power of 2 such that the first coefficient is close to 1 so that it can be simply left out. For example, a multiplier of 64 would yield:

    v2 = (vu->dmate - 40 * v0 + 101 * v1) / 64

Everything should be rounded to the nearest integer and variable names need to be adjusted as shown in the example.

Note: With lower sample rates it might be better to decimate with 16 instead of 32 (eg. 9.5 kSps / 16 = 594) and then the multiplier would probably be 128 instead of 64 (although a multiplier that is too large will cause integer overflow).

6) Test your filter with the following stand-alone GNU Octave / Matlab program. Try different sample rates, decimation values and filter coefficients and plot the result util you find a filter that is accurate and stable.

    clear all;
    close all;
   
    N = 32768;
    A = 16; 
    Fs = 14084;
    T = 1/Fs;
    t = 0:T:(N-1)*T;
    f = Fs*(2:(N/2))/N;
    
    % decimated values
    Z = 32; 
    dN = N/Z;
    dFs = Fs/Z;
    dT = 1/dFs;
    df = dFs*(2:(dN/2))/dN;
    
    m = []; 
    
    for mi = 1:A 
        ip=round(randn(1,N)*512 + sin(2*pi*100*t)*256);
        op=[];
    
        n = N;
    
        rval = 0;
        v0 = 0;
        v1 = 0;
        v2 = 0;
        for i = 1:n 
            rval = (rval * (Z-1) + ip(i)) / Z;
            if mod(i, Z) == 0
                v0 = v1; 
                v1 = v2; 
				v2 = (rval - 93 * v0 + 218 * v1) / 128;
                tmp = (v0 + v2) + 2 * v1; 
                op = [op tmp];
            end 
        endfor
    
        n = length(op);
    
        op = fft(op,n);
        m = [m; abs(op(2:n/2))];
    endfor
    
    y = mean(m);
    
    semilogx(df,20*log10(y));
    
    %axis([1e-1 1e+3 10 80]);

7) Replace the one line of code in vuled8.c:vuled8_add_sample().

8) Try it on the actual Arduino circuit.
