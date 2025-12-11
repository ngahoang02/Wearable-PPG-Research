# v0.1 – PPG Raw Logger (MAX30102 – 100 Hz)

## Summary
This version implements a stable 100 Hz raw PPG acquisition using the MAX30102 sensor and the SparkFun MAX30105 library. Data is streamed over UART in CSV format for dataset creation.

## Key Features
- Hardware 100 Hz sampling using MAX30102 FIFO.
- Pulse width = 118μs, sampleAverage = 1.
- FIFO-based acquisition (no missed samples).
- Timestamped output for dataset synchronization.
- Ideal for dataset creation and waveform inspection.

## Output Format
timestamp_ms,ir,red

## Example
10,14320,9200
20,14318,9195
30,14325,9202
...

## Next Steps (v0.2)
- Add IMU (MPU6050) synchronized with PPG.
- Build multi-sensor dataset pipeline.
