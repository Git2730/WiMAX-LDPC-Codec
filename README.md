# WiMAX LDPC Codec — C++ Implementation

A working C++ simulation of the LDPC encoder and decoder from this research paper:

> **"An Improved Codec Design Architecture for Irregular LDPC Codes Applicable to WiMAX"**
> Divita Shri, Arijit Mondal, Shayan Srinivasa Garani — IISc Bengaluru, IEEE ICECS 2022
> DOI: [10.1109/ICECS202256217.2022.9970908](https://doi.org/10.1109/ICECS202256217.2022.9970908)

**Author:** Mallam Tharun Reddy (23EE10038)
B.Tech (Hons.) Electrical Engineering, IIT Kharagpur
Research implementation under departmental supervision, 2025.

---

## What This Project Does

This project simulates how data is sent and received over a noisy wireless channel using a technique called **LDPC coding** — the same error-correction method used in the WiMAX wireless standard (IEEE 802.16e).

Here's the basic idea:

1. You start with **1152 bits** of random data (information bits)
2. The **encoder** adds 1152 extra bits to protect the data, giving a **2304-bit codeword**
3. The codeword is sent over a **noisy channel** (simulated using AWGN — random noise)
4. The **decoder** receives the noisy bits and tries to recover the original 1152 bits
5. We measure how often it succeeds at different noise levels

The results are compared against **Figure 3 of the paper** to confirm the implementation is correct.

---

## How the Pipeline Works

```
 Random data bits (1152)
        │
        ▼
 ┌─────────────┐
 │   Encoder   │  ──── Adds error-correction bits using the RU algorithm
 └──────┬──────┘
        │  2304-bit codeword
        ▼
 ┌─────────────┐
 │ Noisy Ch.   │  ──── Adds random noise (simulates real wireless conditions)
 └──────┬──────┘
        │  Received (noisy) values
        ▼
 ┌─────────────┐
 │   Decoder   │  ──── Tries to find and fix the errors (up to 5 rounds)
 └──────┬──────┘
        │
        ▼
 Recovered bits  ──── Compared against original to measure error rate
```

---

## Code Parameters

| Parameter | Value |
|-----------|-------|
| Standard | WiMAX IEEE 802.16e |
| Code Rate | 1/2 (half the bits are data, half are protection) |
| Codeword Length | 2304 bits |
| Information Bits | 1152 bits |
| Expansion Factor (z) | 96 |
| Base Matrix Size | 12 × 24 |
| Expanded Matrix Size | 1152 × 2304 |
| Max Decoder Rounds | 5 |
| Quantization | 5-bit messages, 8-bit accumulator |

---

## Simulation Results

These results reproduce **Figure 3** of the paper — how often the decoder fails (FER) vs. how noisy the channel is (SNR in dB). Lower FER = better.

```
SNR (dB) | Raw BER    | Decoded BER | FER        | Frames Tested
--------------------------------------------------------------------------
1.0      | 0.103607   | 0.104019    | 1.000000    | 100
1.5      | 0.093789   | 0.061301    | 0.990099    | 101
2.0      | 0.081823   | 0.010343    | 0.813008    | 123
2.5      | 0.071290   | 0.000239    | 0.121655    | 822
3.0      | 0.061038   | 0.000005    | 0.006700    | 10000
3.5      | 0.051679   | 0.000000    | 0.000200    | 10000
4.0      | 0.042926   | 0.000000    | 0.000000    | 10000
```

- **Raw BER** — error rate before decoding (what the noisy channel produces)
- **Decoded BER** — error rate after decoding (should be much lower)
- **FER** — fraction of frames where at least one bit is still wrong after decoding
- The sharp drop in FER between 2–3 dB is the "waterfall" — this is what good LDPC decoding looks like

---

## Encoder Validation

```
=== RU ENCODER VALIDATION ===
Tested 1000 random frames.
Encoder Pass Rate: 1000 / 1000
SUCCESS — Every encoded codeword satisfies H·c = 0 (mod 2).
==============================
```

Every single codeword produced by the encoder satisfies all 1152 parity-check equations — meaning the encoder is mathematically correct.

---

## File Structure

```
ldpc-wimax-cpp/
│
├── base_matrix.hpp          The 12×24 WiMAX parity-check table (shift values)
├── expanded_matrix.hpp      Expands the base matrix to full size, handles routing
│
├── fixed_point_llr.hpp      Integer type with clamping — models hardware registers
├── signed_magnitude_llr.hpp A different number format used inside the decoder
│
├── gf2_math.hpp             Binary (GF2) math: used to pre-compute encoder matrices
├── ru_encoder.hpp           The encoder (Richardson-Urbanke algorithm)
│
├── cnu.hpp                  Check Node Unit — the core piece of the decoder
├── decoder.hpp              Full layered min-sum decoder
│
├── main.cpp                 ← START HERE: full encode → channel → decode pipeline
├── diff_bits.cpp            Decoder-only test with Monte Carlo BER/FER curves
└── test.cpp                 Checks that the encoder always produces valid codewords
```

---

## Build and Run

**You need:** A C++ compiler that supports C++17 (g++ works)

### Run the full pipeline (encode → channel → decode)
```bash
g++ -std=c++17 -O2 -o codec main.cpp -lm
./codec
```

### Run the decoder-only Monte Carlo simulation
```bash
g++ -std=c++17 -O2 -o simulator diff_bits.cpp -lm
./simulator
```

### Run the encoder validation test
```bash
g++ -std=c++17 -O2 -o test_encoder test.cpp -lm
./test_encoder
```

> **Note:** The full Monte Carlo run takes about 3–5 minutes with `-O2` optimization.

---

## What Each Module Does (Plain English)

### `FixedPointLLR<N_BITS>`
Stores numbers in a fixed number of bits, just like real hardware does. If a value gets too big or too small, it clips to the max/min instead of overflowing. N=5 means values stay in the range −15 to +15.

### `RUEncoder`
The encoder. Takes your 1152 information bits and produces a valid 2304-bit codeword using the Richardson-Urbanke method. The hard math (matrix inversion) is done once at startup; actual encoding is just XOR operations.

### `CheckNodeUnit (CNU) with HBN`
The heart of the decoder. For each check equation, it figures out what correction message to send back to each bit based on what all the other connected bits are saying. The "HBN" (Hardware Bypass Network) handles the fact that some connections in the WiMAX matrix simply don't exist (null entries) — those ports are masked out so they don't interfere with the calculation.

### `LDPCDecoder<N_BITS, ACC_BITS>`
The full decoder. Runs up to 5 rounds. In each round it goes through all 1152 check equations one by one, updating its belief about each bit as it goes (this is the "layered" approach — it reuses updated beliefs immediately rather than waiting for the full round to finish). Stops early if all parity checks pass.

---

## References

1. D. Shri, A. Mondal, S. S. Garani — *"An Improved Codec Design Architecture for Irregular LDPC Codes Applicable to WiMAX"*, IEEE ICECS 2022
2. T. J. Richardson, R. L. Urbanke — *"Efficient Encoding of Low-Density Parity-Check Codes"*, IEEE Trans. Inf. Theory, Feb. 2001
3. D. E. Hocevar — *"A Reduced Complexity Decoder Architecture via Layered Decoding of LDPC Codes"*, IEEE Workshop on Signal Processing Systems, 2004
