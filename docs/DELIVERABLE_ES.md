# ENTREGA ACADÉMICA — PROYECTO FINAL  
## GRAIN: Procesador de saturación micro-armónica (VST3 + AU + Standalone)

**Autor:** Sergio Brocos
**Programa:** Máster Desarrollo con IA
**Fecha:** [Fecha]  
**Repositorio:** [GitHub URL]  
**Release/Build:** [URL de la release o enlace de build]

---

## 1. Resumen
GRAIN es un procesador de saturación micro-armónica diseñado para enriquecer señales digitales de forma sutil, controlada y estable. Su objetivo es aumentar la densidad y la cohesión percibidas sin introducir distorsión evidente ni alterar los transitorios, el balance tonal o la imagen estéreo — especialmente dentro de rangos de uso habituales (10–20% wet). El sistema se entrega como plugin VST3 y AU para macOS (Apple Silicon e Intel) y como aplicación standalone mínima de monitorización en tiempo real. El diseño DSP se basa en una función de transferencia no lineal suave (tanh), una asimetría leve dependiente del nivel (comportamiento basado en RMS lento) y un énfasis espectral por bandas (Low/Mid/High). El oversampling se gestiona internamente (2× en tiempo real y 4× en renderizado offline) para reducir el aliasing manteniendo una experiencia de usuario simple.

---

## 2. Contexto y motivación

### 2.1 Motivación personal

GRAIN nace en la intersección de dos caminos que siempre han convivido en paralelo en mi vida: el desarrollo de software y la producción musical.

Como desarrollador Senior de Ruby on Rails, llevo años construyendo sistemas robustos y pensando en abstracciones. La producción musical, sin embargo, siempre ha sido mi contrapeso creativo — algo que he perseguido con la misma intensidad pero en una dimensión completamente distinta. Durante mucho tiempo, estos dos mundos coexistieron por separado. Sabía programar y sabía producir música, pero construir un plugin de audio — algo que vive dentro de un DAW y procesa sonido en tiempo real — se me antojaba una disciplina diferente, un terreno al que no sabía cómo acceder.

La IA lo cambió todo. Trabajar con IA como compañero de desarrollo me permitió navegar un dominio desconocido (C++, JUCE, DSP) aplicando la mentalidad de ingeniería que ya tenía. No escribió el proyecto por mí — me dio el mapa para encontrar mi propio camino.

GRAIN es el resultado de esa convergencia: una herramienta que uso en mis propias producciones, construida con habilidades adquiridas en este máster, en un dominio que me apasionaba pero al que nunca pensé que podría llegar.

### 2.2 Motivación técnica

Los flujos de producción digital pueden sonar "fríos", "planos" o sin cohesión en ciertos buses (batería, pads, bus de mezcla). Muchas herramientas de saturación añaden una coloración fuerte, lo cual puede resultar intrusivo en mezcla o en un uso pre-master ligero. GRAIN está concebido como un procesador "seguro": su contribución debería percibirse principalmente al desactivarlo (bypass reveal), aportando densidad y estabilidad sin un color audible marcado.

---

## 3. Objetivos
### 3.1 Objetivo general
Diseñar e implementar un procesador de saturación micro-armónica transparente para uso en buses y mezcla/pre-master, con comportamiento estable y sin artefactos perceptibles.

### 3.2 Objetivos específicos
1. Definir un PRD/PDR centrado en seguridad, estabilidad y una UX simple.  
2. Implementar una cadena DSP basada en:
   - detección RMS lenta,
   - asimetría suave tipo triodo,
   - waveshaping suave con tanh,
   - control espectral discreto (Low/Mid/High).
3. Reducir el aliasing mediante oversampling interno no expuesto al usuario.
4. Garantizar latencia cero en procesamiento en tiempo real.
5. Entregar plugins VST3 y AU, y una aplicación standalone mínima.
6. Validar mediante escucha y comprobaciones técnicas (bypass reveal, estabilidad, sin saltos de nivel).

---

## 4. Alcance y entregables
### 4.1 Alcance (V1)
- Plugins **VST3 y AU** para **macOS (Apple Silicon ARM64 + Intel x86_64)**.
- Frecuencia de muestreo mínima garantizada: **44.1 kHz**.
- Parámetros de usuario: Input Gain, Drive (Grain), Warmth, Focus (Low/Mid/High), Mix, Output Gain, Bypass.
- Oversampling interno: 2× tiempo real; 4× renderizado offline.
- Estéreo: procesamiento enlazado (detector compartido).
- Latencia: 0 muestras en tiempo real.
- Standalone: monitorización en tiempo real con selector de dispositivo, bypass y medidores.

### 4.2 Fuera de alcance (V1)
- Presets y A/B (previsto para V1.1).
- Parámetros técnicos expuestos (oversampling, techos internos).

### 4.3 Entregables
- Documento PRD/PDR final (Markdown).
- Código fuente y documentación (README, pasos de build).
- Binarios o instrucciones de build reproducibles (plugin + standalone).
- Plan de validación y resultados (escucha + comprobaciones técnicas básicas).

---

## 5. Requisitos funcionales y no funcionales
### 5.1 Requisitos funcionales
- Mejora micro-armónica sutil con control de intensidad (Grain).
- Control Warmth muy sutil: leve ajuste del balance par/impar sin cambio perceptible en A/B.
- Focus espectral discreto (Low/Mid/High).
- Mix wet/dry y nivel final manual mediante Output Gain.

### 5.2 Requisitos no funcionales
- **Transparencia al 10–20% wet:** sin cambios perceptibles en transitorios, balance tonal o imagen estéreo.
- **Estabilidad:** sin artefactos, sin inestabilidad en material sostenido, sin saltos de nivel.
- **Rendimiento:** uso de CPU razonable con múltiples instancias (oversampling moderado).
- **Usabilidad:** interfaz simple, sin decisiones técnicas requeridas del usuario.
- **Compatibilidad:** macOS Apple Silicon (ARM64) e Intel (x86_64); mínimo 44.1 kHz.
- **Latencia:** cero en tiempo real.

---

## 6. Diseño técnico (DSP)
### 6.1 Visión general del pipeline
1) Input Gain (-12 a +12 dB)
2) Upsample (2× tiempo real, 4× offline)
3) Detector de nivel RMS lento (envelope sumado mono compartido)
4) Dynamic Bias (asimetría leve guiada por RMS)
5) Micro waveshaper (tanh)
6) Ponderación armónica / Warmth (muy sutil)
7) Focus espectral (Low/Mid/High)
8) Downsample
9) Mix (dry/wet)
10) DC Blocker
11) Output Gain

### 6.2 Justificación de las decisiones DSP
- **tanh**: transición progresiva hacia la no linealidad, adecuada para micro-saturación y comportamiento espectral estable.
- **Detector RMS lento**: evita el seguimiento de transitorios; el comportamiento se vuelve textural y estable.
- **Asimetría suave tipo triodo**: introduce armónicos pares sin coloración fuerte; offset mínimo y efecto acumulativo controlado.
- **Oversampling interno (2×/4×)**: reduce el aliasing sin coste excesivo de CPU y sin exponer controles técnicos.
- **Enlace estéreo**: reduce el riesgo de desplazamientos no deseados de la imagen estéreo en el bus de mezcla.

---

## 7. Metodología y proceso de desarrollo
- Flujo iterativo: requisitos → prototipo DSP → calibración por escucha → optimización → validación final.
- Asistencia de IA: utilizada para la redacción del PRD, revisión de arquitectura, generación de checklists de testing y consistencia de la documentación.  
  *Nota:* las decisiones de diseño finales y la validación por escucha son responsabilidad del autor.

---

## 8. Plan de implementación (alto nivel)
1. Esqueleto del plugin VST3 + AU (procesamiento estéreo, parámetros, estado).
2. DSP principal (tanh + drive + mix).
3. Integración del detector RMS lento y el dynamic bias.
4. Implementación del Focus (3 bandas) con transiciones seguras.
5. Oversampling interno (2× tiempo real; 4× offline).
6. Comprobaciones de estabilidad de nivel (sin auto-gain; salida manual).
7. App standalone mínima: IO de dispositivo + passthrough + controles + medidores.
8. Validación por escucha/técnica y documentación final.

---

## 9. Validación y testing
### 9.1 Validación por escucha
- Al 10–20% wet:
  - sin distorsión percibida,
  - sin cambios en transitorios/tonalidad/estéreo,
  - bypass reveal: densidad/estabilidad reducida al desactivar.

### 9.2 Comprobaciones técnicas básicas
- Sin saltos de nivel al activar/desactivar bypass.
- Material sostenido (pads/mezcla): sin oscilación ni inestabilidad.
- Revisión básica de aliasing (comparación con/sin oversampling).
- CPU: comprobación cualitativa con múltiples instancias.

---

## 10. Riesgos y mitigaciones
- **Aliasing audible:** mitigado con oversampling 2× tiempo real y 4× offline.
- **Desplazamientos estéreo no deseados:** mitigados con detección estéreo enlazada y diseño conservador.
- **Coloración excesiva al forzar el drive:** mitigada con asimetría suave, curvas progresivas y valores por defecto seguros.
- **Uso elevado de CPU:** mitigado con oversampling moderado y sin modos extremos.
- **Percepción demasiado sutil:** mitigada permitiendo un empuje controlado preservando el carácter "seguro".

---

## 11. Consideraciones éticas y de calidad
- Evita comportamientos engañosos (p.ej., auto-gain oculto que sesga la percepción).
- La IA se utiliza como herramienta de apoyo con responsabilidad de autoría clara.
- Prioriza la seguridad y la predictibilidad para el usuario (sin comportamientos inestables o sorpresivos).

---

## 12. Roadmap (V1.1)
- A/B
- Presets
- Enlace/desenlace estéreo opcional (si se valida con casos de uso reales)

---

## 13. Conclusión
GRAIN propone un enfoque de saturación micro-armónica con prioridad en la estabilidad, para buses y uso pre-master ligero. La combinación de un detector RMS lento, una asimetría suave tipo triodo, un waveshaper basado en tanh y un oversampling interno moderado busca aportar densidad y cohesión sutiles sin artefactos ni cambios tonal/estéreo intrusivos. Los entregables incluyen plugins VST3 y AU para macOS (Apple Silicon e Intel) y una aplicación standalone mínima para pruebas y uso básico.

---
