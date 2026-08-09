# 2.4GHz_visualizer

Part of the PH222 Microprocessor Lab project. The goal is to sweep across all 128 channels of the 2.4 GHz ISM band and represent the activity on each channel using color and intensity on an LED matrix — effectively building a low-cost, low-resolution spectrum analyzer. A secondary goal was to see whether this same hardware could be pushed further to intercept BLE (Bluetooth Low Energy) advertising traffic. Spoiler: it can *see* BLE energy, but it can't *decode* BLE packets — and the "why" turns out to be the most interesting part of this project. More on that below.

<img width="1765" height="731" alt="image" src="https://github.com/user-attachments/assets/30b7dc86-dc43-404f-bd7a-0d92a34bdf0b" />

## Part 1 — Visualizing channels on an LED matrix

**Schematic:**

<img width="1385" height="1107" alt="image" src="https://github.com/user-attachments/assets/ed46c1ae-7c66-462c-88d2-d335e247f892" />

```json
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
]
```

After uploading the sketch, the nRF24L01+ steps through its channels by retuning its PLL roughly every 130 µs, dwelling briefly on each 1 MHz-wide channel from 2.400–2.525 GHz (channels 0–125). Whatever activity it senses on a given channel is mapped to the brightness/color of the corresponding LED, giving a live, scrolling picture of how crowded the band is.

https://github.com/user-attachments/assets/5017ae2a-c1e0-4324-a9aa-48e76c01fc4e

## What is actually being received?

To understand what's driving the LEDs, it helps to look at the nRF24L01+'s internal architecture.

<img width="1150" height="854" alt="image" src="https://github.com/user-attachments/assets/79d01bef-67ef-41c2-9ab5-4d2cde3346b6" />

Two blocks matter most for this project:

1. **GFSK modulation** (the radio front end)
2. **Enhanced ShockBurst** (the baseband packet engine)

### GFSK

Gaussian Frequency Shift Keying (GFSK) is an extension of classic FSK. In plain FSK, the transmitter switches between two carrier frequencies to represent binary 1s and 0s. Switching abruptly between frequencies produces sharp phase discontinuities, which spread energy into adjacent frequencies (spectral splatter) and waste transmit power outside the intended channel. GFSK fixes this by passing the bitstream through a Gaussian low-pass filter *before* it drives the frequency modulator. Because a Gaussian pulse has no sharp edges in either the time or frequency domain, the resulting frequency transitions are smoothed out, which narrows the occupied bandwidth and reduces adjacent-channel interference — at the cost of introducing a small amount of controlled inter-symbol interference, which the receiver has to tolerate.

Nordic's nRF24L01+ radio front end uses GFSK. So, notably, does classic Bluetooth and Bluetooth Low Energy — this shared modulation is exactly what makes this project's second half interesting (and exactly why it hits a wall).

### Enhanced ShockBurst

Enhanced ShockBurst (ESB) is Nordic's proprietary packet-handling engine sitting on top of the GFSK radio. It automates the parts of a data link layer that would otherwise have to be handled in software: packet framing, address matching, CRC generation/checking, automatic acknowledgements, and retransmission. This lets a cheap 8-bit microcontroller run a reliable wireless link without implementing a MAC layer itself. Critically for us, this also means the chip's hardware is *only* willing to hand the MCU a "packet" if that packet matches Nordic's own ESB framing — preamble, address, payload, CRC. Anything that doesn't match this framing is invisible to the packet-received side of the chip, even if the raw RF energy was received just fine. (Full framing details are in the nRF24L01+ Product Specification.)

With that background, the natural next question was: since GFSK is shared with Bluetooth, could this same setup pick up WiFi and BLE traffic? Short answer: WiFi, no — the physical layer is fundamentally different. BLE, partially — the module can *sense* it but not *decode* it, and the reasons why turn out to be a nice case study in what "same modulation" does and doesn't buy you.

### Why WiFi doesn't fit

WiFi's physical layer is considerably more complex than a simple two-frequency FSK scheme. Depending on data rate, WiFi uses Quadrature Amplitude Modulation (QAM) and Phase Shift Keying (PSK) to pack more bits into each symbol — higher-order QAM constellations mean more bits per symbol and therefore higher throughput, at the cost of requiring a much cleaner signal-to-noise ratio to distinguish the symbols reliably. On top of that, WiFi uses Orthogonal Frequency Division Multiplexing (OFDM): instead of one carrier, it transmits over many orthogonally-spaced subcarriers simultaneously, each an independent sine/cosine pair. Because the subcarriers are orthogonal, they can overlap in frequency without interfering, which is what lets OFDM pack so much data into a fixed bandwidth (typically 20/22 MHz for 2.4 GHz WiFi channels). This is worlds apart from the single-carrier GFSK the nRF24L01+ can demodulate — the nRF24L01+ has no OFDM receiver, no QAM constellation decoder, nothing that can make sense of a WiFi waveform. All it can do is what it does for every channel on the sweep: report whether *some* RF energy above a threshold is present. Because a WiFi channel is roughly 20 MHz wide, it will light up a whole contiguous *block* of adjacent LEDs on the matrix (spanning ~20 of the 1 MHz nRF24 channels) rather than a single narrow spike the way a GFSK transmission does — that block pattern is actually a nice, visually distinct fingerprint for "this is WiFi, not something narrowband."

### Bluetooth (and BLE): a closer, but still incomplete, match

Bluetooth Classic and Bluetooth Low Energy both use GFSK at a 1 MHz channel spacing, with LE's fastest PHY (LE 1M) running at a 1 Mbps air data rate — the same modulation type, channel width, and bit rate that the nRF24L01+ itself supports natively. On paper, this looks like the closest possible match of any 2.4 GHz technology to what our module can do.

BLE advertising in particular is what shows up most obviously on the visualizer: advertising packets (the "here I am" beacons a BLE device broadcasts before any connection is made) go out redundantly on three fixed channels — 37, 38, and 39 in BLE's own numbering, which map to 2402, 2426, and 2480 MHz. Once a connection is established, BLE hops pseudorandomly across the other 37 data channels roughly every connection interval, which is why an active BLE link tends to show up as energy scattered unpredictably across much of the band rather than sitting still.

So the LED matrix genuinely does react to BLE traffic — you'll see brief spikes at the advertising channels (and elsewhere, once devices connect and start hopping) whenever a nearby BLE device is active. What it *cannot* do is turn that spike into readable data.

### Why the module can't actually intercept BLE payloads

This comes down to what the RPD register actually measures versus what a real "packet received" event requires:

- **The channel-sweep display is built on raw carrier detection, not packet demodulation.** The nRF24L01+ exposes a Received Power Detector (RPD) bit that simply goes high when RF energy above roughly ‑64 dBm is present on the currently tuned channel, regardless of whether that energy is a valid ESB packet. The LED matrix visualizer is reading *this* bit as it sweeps channels — it's a coarse carrier/energy detector, not a demodulator. That's genuinely all it needs to be to draw a spectrum picture, and it's also all it's capable of telling you about BLE traffic: "something is transmitting here," never "here's what it said."
- **Getting an actual decoded packet out of the chip is a different, much stricter path**, gated entirely by the Enhanced ShockBurst hardware. For the chip to hand the MCU a payload, the incoming bits have to match Nordic's own framing: a specific preamble byte, an address field the receiver is configured to match, and a CRC (8 or 16-bit) that has to check out — or the ESB engine silently discards the packet. BLE's own link-layer framing looks superficially similar (preamble, then a 4-byte Access Address instead of Nordic's configurable address, then payload, then CRC) but the actual bit patterns, address length/values, and CRC polynomial are all defined by the Bluetooth Core Specification, not by Nordic — so BLE packets fail ESB's address and CRC checks and are never surfaced as "received."
- **BLE also whitens its payload** (XORs it with a per-channel pseudorandom sequence to avoid long runs of 0s/1s that would mess with GFSK's DC balance), and the ESB engine doesn't know to undo that whitening — so even in a hypothetical world where framing matched, the payload bytes would still come out scrambled.
- **BLE's CRC is 24 bits**, while the nRF24L01+ hardware CRC only supports 8 or 16-bit CRCs — there's no register setting that makes the two compatible.
- **Timing is also against it.** BLE frequency-hops per connection event, and its PLL/receiver timing budget assumes a purpose-built BLE radio; the nRF24L01+'s channel-retune time and RX window aren't tuned for that cadence, so even catching the right channel at the right moment is unreliable outside of the fixed advertising channels.

People have gotten *around* several of these limits (not on this hardware setup, but worth knowing about, since it explains exactly where the real ceiling is): by disabling Enhanced ShockBurst and auto-CRC and reading the raw bitstream directly from the chip's shift register, it's possible to fake a "promiscuous" nRF24 and pull out raw Nordic ESB traffic in software instead of relying on the hardware packet engine. A similar raw-bitstream trick using an SDR (not an nRF24L01+) has been used to pull recognizable Bluetooth LE bit patterns out of the air, since the underlying GFSK modulation is genuinely compatible — but that requires software demodulation of the raw RF, not the nRF24L01+'s own packet hardware, and still needs the CRC/whitening/hopping handled separately in software. The nRF24L01+ was also shown to be capable of *transmitting* fake BLE advertising packets, since 1 Mbps GFSK broadcast is well within its native abilities — but the same project ran straight into the receive-side limits above (no 24-bit CRC support, PLL retuning too slow for BLE's hop rate, and a 32-byte payload ceiling) when trying to go the other direction and receive real BLE connections.

**Bottom line:** the visualizer is honestly and correctly showing you *where* in the band BLE devices are transmitting — that part works exactly as advertised. Turning those spikes into readable advertising data would require either switching to a purpose-built BLE-capable radio (e.g., an nRF52-series SoC, CC2540/CC2640, or an SDR with a software BLE decoder), or dropping down to raw-bitstream capture on the nRF24L01+ and writing a custom software demodulator/de-whitener — a genuinely fun follow-up project, but a different one from what Enhanced ShockBurst was designed to do.

## Sources / further reading

- Nordic Semiconductor, *nRF24L01+ Product Specification* (RPD register, Enhanced ShockBurst framing, GFSK radio front end)
- [Faking Bluetooth LE With An NRF24L01+ Module — Hackaday](https://hackaday.com/2024/01/11/faking-bluetooth-le-with-an-nrf24l01-module/)
- [Cyber Explorer — Sniffing and decoding NRF24L01+ and Bluetooth LE packets for under $30](http://blog.cyberexplorer.me/2014/01/sniffing-and-decoding-nrf24l01-and.html)
- [Embedded Innovation — nRF24L01+ sniffer, part 1](https://yveaux.blogspot.com/2014/07/nrf24l01-sniffer-part-1.html) (raw-bitstream / promiscuous-mode capture technique)
- Nordic DevZone Q&A — using the RPD/CD bit as a poor man's carrier detector
- Bluetooth Core Specification — LE physical layer, advertising channels, CRC, and whitening

## Possible next steps

- Log RPD hits per channel over time to build a heatmap instead of an instantaneous snapshot
- Add a second, BLE-capable radio (nRF52840 dongle or similar) running Sniffle/Wireshark alongside the LED matrix, so the visualizer becomes a "where to look" pointer and the second radio does the actual decode
- Try the raw-bitstream/promiscuous-mode trick on the nRF24L01+ itself to decode nearby Nordic-protocol devices (mice, keyboards, other ESB gadgets) that *are* within its native framing
