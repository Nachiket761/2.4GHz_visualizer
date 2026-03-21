# 2.4GHz_visualizer

I am doing this as part of PH222 Microprocessor lab project. In this project we will be analyzing all the 128 channels of 2.4GHz band and representing them using color, Intensity. We will use this setup to Intercept BLE device and read what data it is broadcasting

<img width="1765" height="731" alt="image" src="https://github.com/user-attachments/assets/30b7dc86-dc43-404f-bd7a-0d92a34bdf0b" />

 - the first part: Visualizing channels on LED Matrix

Schematic:
<img width="1385" height="1107" alt="image" src="https://github.com/user-attachments/assets/ed46c1ae-7c66-462c-88d2-d335e247f892" />
````
 "connections": [
    [ "uno:6", "matrix1:DIN", "green", [ "v0" ] ],
    [ "uno:5V", "matrix1:VDD", "red", [ "v-124.9", "h71" ] ],
    [ "uno:GND.2", "matrix1:VSS", "black", [ "v-115.3", "h61.5" ] ],
    [ "matrix1:DOUT", "matrix2:DIN", "green", [ "v9.6", "h162" ] ],
    [ "matrix1:VDD", "matrix2:VDD", "red", [ "v19.2", "h191.6" ] ],
    [ "matrix1:VSS", "matrix2:VSS", "black", [ "v28.8", "h191.2" ] ],
    [ "chip1:VCC", "matrix1:VDD", "red", [ "h-19.2", "v96", "h249.6" ] ],
    [ "chip1:GND", "matrix1:VSS", "black", [ "h-28.8", "v96", "h268.8" ] ],
    [ "chip1:SCLK", "uno:13", "green", [ "h-9.6", "v67.2", "h144" ] ],
    [ "chip1:MISO", "uno:11", "green", [ "h9.6", "v48", "h144" ] ],
    [ "chip1:MOSI", "uno:12", "green", [ "v38.4", "h40.21" ] ],
    [ "chip1:CE", "uno:7", "green", [ "h78.61", "v57.6", "h25.2" ] ],
    [ "chip1:CSN", "uno:8", "green", [ "h69.01", "v57.6", "h18.8" ] ]
  ],
````
after uploading the given sketch, what happens is that, nrf24l01+ module starts scanning each channels by tuning its PLL configuration every 130us to change to next channel. the received data signals from each of 126 channels the module can read are mapped to individual LEDs.


https://github.com/user-attachments/assets/5017ae2a-c1e0-4324-a9aa-48e76c01fc4e



What exactly is the received data? to answer that we have to dig a bit deeper into the inner working of nrf24L01+ chip.

this image of functional block diagram of nrf24L01+ is from the official datasheet
<img width="1150" height="854" alt="image" src="https://github.com/user-attachments/assets/79d01bef-67ef-41c2-9ab5-4d2cde3346b6" />
so there are two key takeaways from this picture
1. GFSK
2. Enhanced ShockBurst Baseband Engine

lets start with GFSK
Guassian Frequency shift keying (GFSK) Modullation is an extension to traditional Frequency shift keying (FSK) Modulation. Traditional FSK is implemented using independent local oscillators separately for in-phase and Quadrature components and the oscillators will be switched at the beginning of each symbol period to generate the carrier frequency for modulation. when these independent oscillators are switched to produce the moduated signal, the abrupt changes in phase and frquency make the output band widen and waste transmitted power outside desired frequency range. GFSK introduces a guassian filter which smoothens out the abrupt changes reducing the band widening effects. as the guassian filter frequency is also a guassian the filter can tuned easily to get decent narrow frequecy response band.
I'm adding some sources at the end of this page which can explain way better than I did one can look them up.

Enhanced ShockBurst Baseband Engine
This techonology is created by Nordic Semiconductor, this is a packet based automatic layer which eliminates need for complex data link layers allowing low power high efficient data transmission using low performance microcontrollers. for more details on the protocol refer the datasheet attached.

with this basic understanding out of the way, we were trying to see traffic due to WiFi, Bluetooth(LE) which by now ot is clear that this module cant understand as it works physically different.

what exactly is different?

WiFi is a bit of compilcated thing it uses multiple types of modulations, they are Quadrature Amplitude Modulation (QAM) and Phase Shift Keying (PSK), this inturn increases the amount of data per symbol (like making the modulations more dense). WiFi uses transmission method called orthogonal Frequency Division Multiplexing (OFDM), which sends data on sin and cosine carrier types with the phase of being in the form 2*pi*(fc + 1/Ts) where Ts is the time period of one symbol (you can think of symbol as a chunk of data divided using time axis). Higher QAM -> More data transmission speed.

Bluetooth 

explain what exactly being captured

explain why interception will not work with this module

