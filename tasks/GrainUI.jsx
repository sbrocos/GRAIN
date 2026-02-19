import React, { useState, useEffect } from 'react';

const GrainUI = () => {
  const [params, setParams] = useState({
    input: 0,
    grain: 25,
    warmth: 0,
    mix: 15,
    output: 0,
    focus: 'mid',
    bypass: false
  });

  const [meters, setMeters] = useState({ inL: 0.55, inR: 0.52, outL: 0.58, outR: 0.54 });

  useEffect(() => {
    const interval = setInterval(() => {
      setMeters({
        inL: 0.45 + Math.random() * 0.25,
        inR: 0.43 + Math.random() * 0.25,
        outL: 0.47 + Math.random() * 0.25,
        outR: 0.45 + Math.random() * 0.25,
      });
    }, 150);
    return () => clearInterval(interval);
  }, []);

  const updateParam = (key, value) => {
    setParams(prev => ({ ...prev, [key]: value }));
  };

  // Simple dot-style knob like the reference image
  const Knob = ({ label, value, min, max, unit = '', onChange, size = 'normal', labelGap = 4 }) => {
    const range = max - min;
    const normalized = (value - min) / range;
    const angle = -135 + normalized * 270;
    const isLarge = size === 'large';
    const isMedium = size === 'medium';
    const knobSize = isLarge ? 120 : isMedium ? 80 : 60;
    const dotCount = isLarge ? 35 : isMedium ? 24 : 21;
    const dotRadius = isLarge ? 3 : isMedium ? 3.5 : 3;

    return (
      <div className="flex flex-col items-center">
        {/* Label on top */}
        <span
          className="text-sm tracking-wide"
          style={{ color: '#1a1a1a', fontFamily: 'Inter, sans-serif', fontWeight: 500, marginBottom: labelGap }}
        >
          {label}
        </span>

        <div
          className="relative cursor-pointer"
          style={{ width: knobSize + 30, height: knobSize + 30 }}
        >
          {/* Dot scale around the knob */}
          <svg 
            className="absolute inset-0" 
            viewBox="0 0 140 140"
            style={{ width: '100%', height: '100%' }}
          >
            {Array.from({ length: dotCount }).map((_, i) => {
              const dotAngle = 135 + (i / (dotCount - 1)) * 270;
              const rad = (dotAngle * Math.PI) / 180;
              const radius = 64;
              const x = 70 + radius * Math.cos(rad);
              const y = 70 + radius * Math.sin(rad);
              const isActive = (i / (dotCount - 1)) <= normalized;
              return (
                <circle
                  key={i}
                  cx={x}
                  cy={y}
                  r={dotRadius}
                  fill={isActive ? '#d97706' : '#1a1a1a'}
                  style={{
                    transition: 'fill 0.1s ease'
                  }}
                />
              );
            })}
          </svg>
          
          {/* Knob body */}
          <div
            className="absolute rounded-full"
            style={{
              left: 15,
              top: 15,
              width: knobSize,
              height: knobSize,
              background: 'linear-gradient(145deg, #2a2a2a, #1a1a1a)',
              boxShadow: `
                0 4px 8px rgba(0,0,0,0.4),
                0 2px 4px rgba(0,0,0,0.3),
                inset 0 1px 1px rgba(255,255,255,0.1)
              `,
              transform: `rotate(${angle}deg)`,
              transition: 'transform 0.05s ease-out'
            }}
          >
            {/* Outer ring */}
            <div
              className="absolute rounded-full"
              style={{
                inset: 3,
                border: '2px solid #3a3a3a',
              }}
            />
            
            {/* Indicator dot */}
            <div
              className="absolute rounded-full"
              style={{
                width: isLarge ? 11 : isMedium ? 8 : 7,
                height: isLarge ? 11 : isMedium ? 8 : 7,
                backgroundColor: '#ffffff',
                top: isLarge ? 14 : isMedium ? 10 : 8,
                left: '50%',
                transform: 'translateX(-50%)',
                boxShadow: '0 0 4px rgba(255,255,255,0.5)'
              }}
            />
          </div>
          
          {/* Invisible slider */}
          <input
            type="range"
            min={min}
            max={max}
            value={value}
            onChange={(e) => onChange(parseFloat(e.target.value))}
            className="absolute inset-0 w-full h-full opacity-0 cursor-pointer"
            step={max <= 12 ? 0.1 : 1}
          />
        </div>
        
        {/* Value display */}
        <span 
          className="text-sm font-mono mt-1"
          style={{ color: '#1a1a1a' }}
        >
          {value > 0 && unit !== '%' ? `+${value}` : value}{unit}
        </span>
      </div>
    );
  };

  // Focus switch
  const FocusSwitch = ({ value, onChange }) => {
    const options = ['low', 'mid', 'high'];
    const labels = ['LOW', 'MID', 'HIGH'];
    
    return (
      <div className="flex flex-col items-center mt-6">
        <span
          className="text-sm tracking-wide mb-1"
          style={{ color: '#1a1a1a', fontFamily: 'Inter, sans-serif', fontWeight: 500 }}
        >
          FOCUS
        </span>
        <div 
          className="flex rounded-lg overflow-hidden"
          style={{
            background: '#1a1a1a',
            padding: 3,
            gap: 2
          }}
        >
          {options.map((opt, i) => (
            <button
              key={opt}
              onClick={() => onChange(opt)}
              className="px-5 py-2 text-xs font-bold tracking-wider transition-all duration-150"
              style={{
                backgroundColor: value === opt ? '#d97706' : 'transparent',
                color: value === opt ? '#ffffff' : '#888888',
                borderRadius: 4,
                fontFamily: 'Inter, sans-serif'
              }}
            >
              {labels[i]}
            </button>
          ))}
        </div>
      </div>
    );
  };

  // Bypass button
  const BypassButton = ({ active, onClick }) => (
    <button
      onClick={onClick}
      className="w-10 h-10 rounded-full flex items-center justify-center transition-all duration-200"
      style={{
        background: active ? '#1a1a1a' : '#d97706',
        boxShadow: active 
          ? 'inset 0 2px 4px rgba(0,0,0,0.5)' 
          : '0 2px 8px rgba(217,119,6,0.5)'
      }}
    >
      <svg 
        className="w-5 h-5"
        style={{ color: active ? '#666' : '#fff' }}
        fill="none" 
        viewBox="0 0 24 24" 
        stroke="currentColor" 
        strokeWidth={2.5}
      >
        <path strokeLinecap="round" strokeLinejoin="round" d="M5.636 5.636a9 9 0 1012.728 0M12 3v9" />
      </svg>
    </button>
  );

  // Vertical meter bar
  const MeterBar = ({ levelL, levelR, label }) => {
    const segments = 32;
    const litL = Math.round(levelL * segments);
    const litR = Math.round(levelR * segments);

    const getColor = (index, isLit) => {
      if (!isLit) return '#3a3a3a';
      const pos = index / segments;
      if (pos > 0.9) return '#ef4444';
      if (pos > 0.75) return '#eab308';
      return '#22c55e';
    };

    return (
      <div className="flex flex-col items-center gap-1">
        <span
          className="text-sm tracking-wide mb-1"
          style={{ color: '#1a1a1a', fontFamily: 'Inter, sans-serif', fontWeight: 500 }}
        >
          {label}
        </span>
        <div
          className="flex flex-row gap-1 p-2 rounded"
          style={{ background: '#1a1a1a' }}
        >
          {/* Left channel - vertical, bottom to top */}
          <div className="flex flex-col-reverse gap-0.5">
            {Array.from({ length: segments }).map((_, i) => (
              <div
                key={`l-${i}`}
                className="rounded-sm transition-all duration-75"
                style={{
                  width: 8,
                  height: 7,
                  backgroundColor: getColor(i, i < litL),
                  boxShadow: i < litL ? `0 0 4px ${getColor(i, true)}` : 'none'
                }}
              />
            ))}
          </div>
          {/* Right channel - vertical, bottom to top */}
          <div className="flex flex-col-reverse gap-0.5">
            {Array.from({ length: segments }).map((_, i) => (
              <div
                key={`r-${i}`}
                className="rounded-sm transition-all duration-75"
                style={{
                  width: 8,
                  height: 7,
                  backgroundColor: getColor(i, i < litR),
                  boxShadow: i < litR ? `0 0 4px ${getColor(i, true)}` : 'none'
                }}
              />
            ))}
          </div>
        </div>
      </div>
    );
  };

  return (
    <div 
      className="min-h-screen flex items-center justify-center p-8"
      style={{ backgroundColor: '#323232' }}
    >
      <div 
        className="relative rounded-2xl p-6"
        style={{
          backgroundColor: '#B6C1B9',
          boxShadow: `
            0 20px 40px rgba(0,0,0,0.3),
            0 0 0 1px rgba(0,0,0,0.1),
            inset 0 1px 0 rgba(255,255,255,0.3)
          `,
          width: 580
        }}
      >
        {/* Header */}
        <div className="flex items-center justify-between mb-5">
          <div>
            <h1
              className="text-3xl tracking-tight"
              style={{ color: '#1a1a1a', fontFamily: 'Inter, sans-serif', fontWeight: 900, fontStyle: 'italic' }}
            >
              GRAIN
            </h1>
            <p
              className="text-xs tracking-widest"
              style={{ color: '#4a4a4a', fontFamily: 'Inter, sans-serif', fontWeight: 400, fontStyle: 'italic' }}
            >
              MICRO-HARMONIC SATURATION
            </p>
          </div>
          <BypassButton 
            active={params.bypass} 
            onClick={() => updateParam('bypass', !params.bypass)} 
          />
        </div>

        {/* Main controls */}
        <div className={`transition-opacity duration-300 ${params.bypass ? 'opacity-40' : 'opacity-100'}`}>

          {/* Top zone: 3 columns - meters + GRAIN/WARM + FOCUS + oversample */}
          <div className="flex items-stretch gap-2">

            {/* Left column: IN meter */}
            <div className="flex flex-col items-center justify-start" style={{ minWidth: 100 }}>
              <MeterBar
                levelL={params.bypass ? 0.05 : meters.inL}
                levelR={params.bypass ? 0.05 : meters.inR}
                label="IN"
              />
            </div>

            {/* Center column: GRAIN + WARM / FOCUS / oversample */}
            <div className="flex-1 flex flex-col items-center gap-3">
              {/* Row 1: GRAIN + WARM */}
              <div className="flex items-center justify-center" style={{ gap: 20 }}>
                <Knob
                  label="GRAIN"
                  value={params.grain}
                  min={0}
                  max={100}
                  unit="%"
                  onChange={(v) => updateParam('grain', Math.round(v))}
                  size="large"
                />
                <Knob
                  label="WARM"
                  value={params.warmth}
                  min={-100}
                  max={100}
                  unit="%"
                  onChange={(v) => updateParam('warmth', Math.round(v))}
                  size="large"
                />
              </div>

              {/* Row 2: FOCUS selector */}
              <FocusSwitch
                value={params.focus}
                onChange={(v) => updateParam('focus', v)}
              />
            </div>

            {/* Right column: OUT meter */}
            <div className="flex flex-col items-center justify-start" style={{ minWidth: 100 }}>
              <MeterBar
                levelL={params.bypass ? 0.05 : meters.outL}
                levelR={params.bypass ? 0.05 : meters.outR}
                label="OUT"
              />
            </div>

          </div>

          {/* Bottom row: INPUT / MIX / OUTPUT knobs aligned */}
          <div className="flex items-start justify-between mt-3">
            <div style={{ minWidth: 100 }} className="flex justify-center mb-3">
              <Knob
                label="INPUT"
                labelGap= {12}
                value={params.input}
                min={-12}
                max={12}
                unit=" dB"
                onChange={(v) => updateParam('input', Math.round(v * 10) / 10)}
              />
            </div>
            <Knob
              label="MIX"
              value={params.mix}
              min={0}
              max={100}
              unit="%"
              onChange={(v) => updateParam('mix', Math.round(v))}
              size="medium"
            />
            <div style={{ minWidth: 100 }} className="flex justify-center">
              <Knob
                label="OUTPUT"
                labelGap= {12}
                value={params.output}
                min={-12}
                max={12}
                unit=" dB"
                onChange={(v) => updateParam('output', Math.round(v * 10) / 10)}
              />
            </div>
          </div>

        </div>

        {/* Footer */}
        <div className="flex items-center justify-between mt-4 pt-3 border-t" style={{ borderColor: 'rgba(0,0,0,0.1)' }}>
          <span className="text-xs font-medium" style={{ color: '#4a4a4a' }}>v1.0.0</span>
          <span className="text-xs font-medium" style={{ color: '#4a4a4a' }}>Sergio Brocos © 2025</span>
        </div>
      </div>
    </div>
  );
};

export default GrainUI;
