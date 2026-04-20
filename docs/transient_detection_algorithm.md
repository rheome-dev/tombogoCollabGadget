# Transient Detection Algorithm Extract

This document provides the complete context and implementation needed to port the transient detection algorithm currently used in the audio analysis worker.

## Dependencies

To use this algorithm in another application, you will need the following dependencies:

1. **`meyda`**: An audio feature extraction library for JavaScript.
   - Installation: `npm install meyda` or `yarn add meyda`
   - Usage: Used for extracting features like `amplitudeSpectrum`, `rms`, etc.
   - Note: The worker uses a stateless implementation of `Meyda.extract()` instead of the standard Meyda Analyzer node, since it runs in a Web Worker without access to the Web Audio API's `AudioContext`.

2. The application needs to be able to extract audio as a `Float32Array` containing the channel data, along with its sample rate, to feed into the algorithm.

## Core Concepts

The transient detection algorithm employs a combination of two primary signals to detect peaks (onsets/transients):
1. **Spectral Flux**: Measures how quickly the power spectrum of a signal is changing, computed by comparing the amplitude spectrum of the current frame with the previous one. Helps identify highly detailed high-frequency changes (like hi-hats or snaps).
2. **Volume / RMS Gate**: Used to heavily influence the peak detection and to reject spurious noise or insignificant quiet events.

It involves a two-pass process:
1. [performFullAnalysis](file:///Users/jasperhall/Desktop/Phonoglyph/apps/web/src/app/workers/audio-analysis.worker.ts#153-276): Uses `meyda` to extract features frame-by-frame and compute spectral flux manually.
2. [performEnhancedAnalysis](file:///Users/jasperhall/Desktop/Phonoglyph/apps/web/src/app/workers/audio-analysis.worker.ts#277-396): Takes the results to identify transient peaks by applying thresholding, smoothing, windowing, and volume gating.

## Implementation Details

Below is the complete, self-contained TypeScript implementation needed to run the algorithm. It is divided into helper methods, the main analysis loop, and the peak picking algorithm.

### 1. Imports and Helpers

```typescript
import Meyda from 'meyda';

/**
 * Helper function to check if a value is an array-like object (regular array or TypedArray)
 * and convert it to a regular array for processing.
 */
function toArray(value: any): number[] | null {
  if (!value) return null;
  if (Array.isArray(value)) return value;
  if (ArrayBuffer.isView(value)) return Array.from(value as unknown as ArrayLike<number>);
  if (typeof value === 'object' && 'length' in value && typeof value.length === 'number') {
    return Array.from(value as unknown as ArrayLike<number>);
  }
  return null;
}
```

### 2. Feature Extraction Pass ([performFullAnalysis](file:///Users/jasperhall/Desktop/Phonoglyph/apps/web/src/app/workers/audio-analysis.worker.ts#153-276))

This extracts all base features and manually computes spectral flux.

```typescript
function performFullAnalysis(
  channelData: Float32Array,
  sampleRate: number
) {
  // Add/remove features as needed, but 'amplitudeSpectrum' and 'rms' are required.
  const featuresToExtract = ['rms', 'amplitudeSpectrum', 'spectralCentroid'];

  const featureFrames: Record<string, any> = {};
  const frameTimes: number[] = [];
  
  featuresToExtract.forEach((f) => { featureFrames[f] = []; });
  featureFrames.spectralFlux = [];
  featureFrames.volume = [];

  const bufferSize = 1024;
  const hopSize = 512;
  let currentPosition = 0;
  let previousSpectrum: number[] | null = null;

  while (currentPosition + bufferSize <= channelData.length) {
    const buffer = channelData.slice(currentPosition, currentPosition + bufferSize);
    let features: any = null;

    try {
      // Use stateless Meyda.extract
      features = (Meyda as any).extract(featuresToExtract, buffer, {
        sampleRate: sampleRate,
        bufferSize: bufferSize,
        windowingFunction: 'hanning'
      });
    } catch (error) {
      features = {}; 
    }

    // --- Manual spectral flux calculation ---
    const currentSpectrum = toArray(features?.amplitudeSpectrum);
    let flux = 0;
    
    if (previousSpectrum && currentSpectrum && previousSpectrum.length > 0 && currentSpectrum.length > 0) {
      const len = Math.min(previousSpectrum.length, currentSpectrum.length);
      for (let i = 0; i < len; i++) {
        // Flux definition: Sum of positive differences between spectra
        const diff = (currentSpectrum[i] || 0) - (previousSpectrum[i] || 0);
        if (diff > 0) flux += diff;
      }
    }
    
    previousSpectrum = currentSpectrum && currentSpectrum.length > 0 ? currentSpectrum : null;
    featureFrames.spectralFlux.push(flux);

    if (features) {
      for (const feature of featuresToExtract) {
        const value = features[feature];
        const arrayValue = toArray(value);
        
        if (arrayValue !== null) {
           featureFrames[feature].push(arrayValue[0] || 0);
        } else if (Array.isArray(value)) {
           featureFrames[feature].push(value[0] || 0);
        } else {
          const sanitizedValue = (typeof value === 'number' && isFinite(value)) ? value : 0;
          featureFrames[feature].push(sanitizedValue);
        }
      }
      featureFrames.volume.push(features.rms || 0);
    } else {
      featuresToExtract.forEach(f => featureFrames[f].push(0));
      featureFrames.volume.push(0);
    }

    const frameStartTime = currentPosition / sampleRate;
    frameTimes.push(frameStartTime);
    currentPosition += hopSize;
  }

  return { ...featureFrames, frameTimes };
}
```

### 3. Transient Detection Pass ([performEnhancedAnalysis](file:///Users/jasperhall/Desktop/Phonoglyph/apps/web/src/app/workers/audio-analysis.worker.ts#277-396))

This function processes the output of [performFullAnalysis](file:///Users/jasperhall/Desktop/Phonoglyph/apps/web/src/app/workers/audio-analysis.worker.ts#153-276) to pick peaks.

```typescript
function performEnhancedAnalysis(
  fullAnalysis: Record<string, any>,
  sampleRate: number,
  analysisParams?: any
): { time: number; intensity: number; type: string }[] {

  const params = Object.assign({
    onsetThreshold: 0.08,   // base normalized threshold (more sensitive)
    peakWindow: 4,          // frames on each side for local max analysis
    peakMultiplier: 1.25,   // how much above local mean a peak must be
  }, analysisParams || {});

  const { frameTimes, spectralFlux, volume, rms } = fullAnalysis;

  if (!spectralFlux || !Array.isArray(spectralFlux) || spectralFlux.length === 0) return [];

  const rawFlux = (spectralFlux as number[]).map(v => (isFinite(v) && v > 0 ? v : 0));
  const finiteFlux = rawFlux.filter(isFinite);
  if (finiteFlux.length === 0) return [];

  // Normalize Flux
  const maxFlux = Math.max(1e-6, ...finiteFlux);
  const normFlux = rawFlux.map(v => v / maxFlux);

  // 1. Lightweight smoothing on the Flux (reduces spurious tiny peaks like hi-hat flams)
  const smoothFlux: number[] = new Array(normFlux.length);
  const smoothRadius = 1; // Frames to smooth over
  for (let i = 0; i < normFlux.length; i++) {
    let sum = 0, count = 0;
    for (let k = -smoothRadius; k <= smoothRadius; k++) {
      const idx = i + k;
      if (idx >= 0 && idx < normFlux.length) {
        sum += normFlux[idx];
        count++;
      }
    }
    smoothFlux[i] = count > 0 ? sum / count : normFlux[i];
  }

  // 2. Volume Gating Preparation
  // We use Volume/RMS to gate and weight the peaks to better capture low/mid hits.
  const volArray: number[] = Array.isArray(volume) 
    ? volume.map(v => (isFinite(v) && v > 0 ? v : 0)) 
    : (Array.isArray(rms) ? rms.map(v => (isFinite(v) && v > 0 ? v : 0)) : []);
    
  const volFinite = volArray.filter(isFinite);
  const avgVolume = volFinite.length ? volFinite.reduce((a, b) => a + b, 0) / volFinite.length : 0;
  const volumeGate = avgVolume * 0.15; // Requires at least 15% of average loudness

  // Normalize volume to blend correctly with spectral flux
  const volMax = volFinite.length ? Math.max(1e-6, ...volFinite) : 1e-6;
  const normVol: number[] = volArray.map(v => v / volMax);

  // 3. Combine Signals
  // Combined onset strength: spectral flux (captures detail) + volume (captures LF/mid hits)
  const combinedFlux: number[] = smoothFlux.map((f, i) => {
    const nv = normVol[i] ?? 0;
    // Slightly favor spectral flux but still give volume strong influence
    return f * 0.7 + nv * 0.4;
  });

  const peaks: { frameIndex: number; time: number; intensity: number }[] = [];
  const w = Math.max(1, params.peakWindow | 0);
  
  // 4. Peak Picking Iteration
  for (let i = w; i < combinedFlux.length - w; i++) {
    const f = combinedFlux[i];
    
    // Ignore anything under the strict onset threshold
    if (f < params.onsetThreshold) continue;

    // Calculate the Local Mean around `i`
    let localSum = 0, localCount = 0;
    for (let j = -w; j <= w; j++) {
      const idx = i + j;
      if (idx >= 0 && idx < combinedFlux.length) {
        localSum += combinedFlux[idx];
        localCount++;
      }
    }
    const localMean = localCount > 0 ? localSum / localCount : 0;
    
    // Compare against the local mean to see if it stands out (Peak Multiplier)
    if (f < localMean * params.peakMultiplier) continue;

    // Reject events that are incredibly quiet (Volume Gate)
    if (volArray.length && (volArray[i] ?? 0) < volumeGate) continue;

    // Ensure it is truly a local maximum in the window
    let isLocalMax = true;
    for (let j = -w; j <= w; j++) {
      const idx = i + j;
      if (idx === i || idx < 0 || idx >= combinedFlux.length) continue;
      if (combinedFlux[i] < combinedFlux[idx]) {
        isLocalMax = false;
        break;
      }
    }
    
    if (isLocalMax) {
      const frameTimesArray = Array.isArray(frameTimes) ? frameTimes : [];
      peaks.push({
        frameIndex: i,
        time: frameTimesArray[i] ?? i * (512 / sampleRate), // Fallback calculation: i * (hopSize / sampleRate)
        intensity: f
      });
      // Skip ahead to avoid double triggering during the same transient
      i += w; 
    }
  }

  return peaks.map(peak => ({
    time: peak.time,
    intensity: peak.intensity,
    type: 'generic'
  }));
}
```
