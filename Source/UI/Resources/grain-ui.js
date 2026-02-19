/* ================================================================
   GRAIN UI — Vanilla JS (ported from GrainUI.jsx)
   Uses JUCE 8 WebBrowserComponent low-level backend API
   ================================================================ */

/* ================================================================
   Relay State Adapters
   Replicated from JUCE's juce_gui_extra/native/javascript/index.js
   using the low-level window.__JUCE__.backend API.
   ================================================================ */

class SliderState {
    constructor(name) {
        this.name = name;
        this.identifier = "__juce__slider" + name;
        this.scaledValue = 0;
        this.properties = {
            start: 0, end: 1, skew: 1, name: "", label: "",
            numSteps: 0, interval: 0, parameterIndex: -1
        };
        this._listeners = [];
        this._propListeners = [];

        window.__JUCE__.backend.addEventListener(this.identifier, (e) => this._handle(e));
    }

    requestInitialUpdate() {
        window.__JUCE__.backend.emitEvent(this.identifier, { eventType: "requestInitialUpdate" });
    }

    getScaledValue() {
        return this.scaledValue;
    }

    getNormalisedValue() {
        var range = this.properties.end - this.properties.start;
        if (range === 0) return 0;
        var proportion = (this.scaledValue - this.properties.start) / range;
        return Math.pow(Math.max(0, Math.min(1, proportion)), this.properties.skew);
    }

    setNormalisedValue(norm) {
        norm = Math.max(0, Math.min(1, norm));
        var range = this.properties.end - this.properties.start;
        var scaled = Math.pow(norm, 1.0 / this.properties.skew) * range + this.properties.start;

        if (this.properties.interval > 0) {
            scaled = Math.round(scaled / this.properties.interval) * this.properties.interval;
        }

        this.scaledValue = scaled;
        window.__JUCE__.backend.emitEvent(this.identifier, {
            eventType: "valueChanged",
            value: this.scaledValue
        });
    }

    sliderDragStarted() {
        window.__JUCE__.backend.emitEvent(this.identifier, { eventType: "sliderDragStarted" });
    }

    sliderDragEnded() {
        window.__JUCE__.backend.emitEvent(this.identifier, { eventType: "sliderDragEnded" });
    }

    addValueListener(fn) {
        this._listeners.push(fn);
    }

    addPropertiesListener(fn) {
        this._propListeners.push(fn);
    }

    _handle(event) {
        if (event.eventType === "valueChanged") {
            this.scaledValue = event.value;
            this._listeners.forEach(function(fn) { fn(); });
        } else if (event.eventType === "propertiesChanged") {
            this.properties = {
                start: event.start !== undefined ? event.start : this.properties.start,
                end: event.end !== undefined ? event.end : this.properties.end,
                skew: event.skew !== undefined ? event.skew : this.properties.skew,
                name: event.name !== undefined ? event.name : this.properties.name,
                label: event.label !== undefined ? event.label : this.properties.label,
                numSteps: event.numSteps !== undefined ? event.numSteps : this.properties.numSteps,
                interval: event.interval !== undefined ? event.interval : this.properties.interval,
                parameterIndex: event.parameterIndex !== undefined ? event.parameterIndex : this.properties.parameterIndex
            };
            this._propListeners.forEach(function(fn) { fn(); });
        }
    }
}

class ToggleState {
    constructor(name) {
        this.name = name;
        this.identifier = "__juce__toggle" + name;
        this.value = false;
        this.properties = { name: "", parameterIndex: -1 };
        this._listeners = [];

        window.__JUCE__.backend.addEventListener(this.identifier, (e) => this._handle(e));
    }

    requestInitialUpdate() {
        window.__JUCE__.backend.emitEvent(this.identifier, { eventType: "requestInitialUpdate" });
    }

    getValue() {
        return this.value;
    }

    setValue(newValue) {
        this.value = newValue;
        window.__JUCE__.backend.emitEvent(this.identifier, {
            eventType: "valueChanged",
            value: this.value
        });
    }

    addValueListener(fn) {
        this._listeners.push(fn);
    }

    _handle(event) {
        if (event.eventType === "valueChanged") {
            this.value = !!event.value;
            this._listeners.forEach(function(fn) { fn(); });
        } else if (event.eventType === "propertiesChanged") {
            this.properties = {
                name: event.name !== undefined ? event.name : this.properties.name,
                parameterIndex: event.parameterIndex !== undefined ? event.parameterIndex : this.properties.parameterIndex
            };
        }
    }
}

class ComboBoxState {
    constructor(name) {
        this.name = name;
        this.identifier = "__juce__comboBox" + name;
        this.normalisedValue = 0;
        this.properties = { name: "", parameterIndex: -1, choices: [] };
        this._listeners = [];
        this._propListeners = [];

        window.__JUCE__.backend.addEventListener(this.identifier, (e) => this._handle(e));
    }

    requestInitialUpdate() {
        window.__JUCE__.backend.emitEvent(this.identifier, { eventType: "requestInitialUpdate" });
    }

    getChoiceIndex() {
        var numChoices = this.properties.choices.length;
        if (numChoices <= 1) return 0;
        return Math.round(this.normalisedValue * (numChoices - 1));
    }

    setChoiceIndex(index) {
        var numChoices = this.properties.choices.length;
        if (numChoices <= 1) {
            this.normalisedValue = 0;
        } else {
            this.normalisedValue = index / (numChoices - 1);
        }
        window.__JUCE__.backend.emitEvent(this.identifier, {
            eventType: "valueChanged",
            value: this.normalisedValue
        });
    }

    addValueListener(fn) {
        this._listeners.push(fn);
    }

    addPropertiesListener(fn) {
        this._propListeners.push(fn);
    }

    _handle(event) {
        if (event.eventType === "valueChanged") {
            this.normalisedValue = event.value;
            this._listeners.forEach(function(fn) { fn(); });
        } else if (event.eventType === "propertiesChanged") {
            this.properties = {
                name: event.name !== undefined ? event.name : this.properties.name,
                parameterIndex: event.parameterIndex !== undefined ? event.parameterIndex : this.properties.parameterIndex,
                choices: event.choices !== undefined ? event.choices : this.properties.choices
            };
            this._propListeners.forEach(function(fn) { fn(); });
        }
    }
}

/* ================================================================
   DotKnob Component
   ================================================================ */

class DotKnob {
    /**
     * @param {HTMLElement} container
     * @param {object} config
     * @param {string} config.relayName   - SliderRelay name
     * @param {string} config.label       - Display label
     * @param {string} config.size        - "large", "medium", or "normal"
     * @param {function} config.displayFn - (scaledValue) => string
     */
    constructor(container, config) {
        this.container = container;
        this.config = config;
        this.size = config.size || "normal";

        var sizeMap = {
            large:  { knob: 120, dots: 35, dotR: 3, indicator: 11, indicatorTop: 14 },
            medium: { knob: 80,  dots: 24, dotR: 3.5, indicator: 8,  indicatorTop: 10 },
            normal: { knob: 60,  dots: 21, dotR: 3, indicator: 7,  indicatorTop: 8 }
        };
        this.sizeConfig = sizeMap[this.size];

        this.state = new SliderState(config.relayName);
        this.dots = [];

        this._buildDOM();

        this.state.addValueListener(() => this._updateVisual());
        this.state.addPropertiesListener(() => this._updateVisual());

        // Request initial value from C++
        this.state.requestInitialUpdate();
    }

    _buildDOM() {
        var sc = this.sizeConfig;
        var wrapperSize = sc.knob + 30;

        // Outer container
        var el = document.createElement("div");
        el.className = "knob-container";

        // Label
        var label = document.createElement("span");
        label.className = "knob-label";
        label.textContent = this.config.label;
        el.appendChild(label);

        // Wrapper (relative, holds SVG + knob body + input)
        var wrapper = document.createElement("div");
        wrapper.className = "knob-wrapper " + this.size;
        wrapper.style.width = wrapperSize + "px";
        wrapper.style.height = wrapperSize + "px";

        // SVG for dot ring
        var svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
        svg.setAttribute("class", "knob-svg");
        svg.setAttribute("viewBox", "0 0 140 140");

        for (var i = 0; i < sc.dots; i++) {
            var dotAngle = 135 + (i / (sc.dots - 1)) * 270;
            var rad = (dotAngle * Math.PI) / 180;
            var radius = 64;
            var x = 70 + radius * Math.cos(rad);
            var y = 70 + radius * Math.sin(rad);

            var circle = document.createElementNS("http://www.w3.org/2000/svg", "circle");
            circle.setAttribute("cx", x);
            circle.setAttribute("cy", y);
            circle.setAttribute("r", sc.dotR);
            circle.setAttribute("fill", "#1a1a1a");
            circle.style.transition = "fill 0.1s ease";
            svg.appendChild(circle);
            this.dots.push(circle);
        }
        wrapper.appendChild(svg);

        // Knob body
        var body = document.createElement("div");
        body.className = "knob-body " + this.size;
        this.knobBody = body;

        // Outer ring
        var ring = document.createElement("div");
        ring.className = "knob-ring";
        body.appendChild(ring);

        // Indicator dot
        var indicator = document.createElement("div");
        indicator.className = "knob-indicator " + this.size;
        body.appendChild(indicator);

        wrapper.appendChild(body);

        // Hidden range input
        var input = document.createElement("input");
        input.type = "range";
        input.className = "knob-range";
        input.min = "0";
        input.max = "1";
        input.step = "0.001";
        input.value = "0";
        this.rangeInput = input;

        var self = this;

        input.addEventListener("mousedown", function() {
            self.state.sliderDragStarted();
        });

        input.addEventListener("input", function() {
            var norm = parseFloat(input.value);
            self.state.setNormalisedValue(norm);
            self._updateVisual();
        });

        input.addEventListener("mouseup", function() {
            self.state.sliderDragEnded();
        });

        input.addEventListener("touchstart", function() {
            self.state.sliderDragStarted();
        });

        input.addEventListener("touchend", function() {
            self.state.sliderDragEnded();
        });

        wrapper.appendChild(input);
        el.appendChild(wrapper);

        // Value label
        var valueLabel = document.createElement("span");
        valueLabel.className = "knob-value";
        valueLabel.textContent = "0";
        this.valueLabel = valueLabel;
        el.appendChild(valueLabel);

        this.container.appendChild(el);
    }

    _updateVisual() {
        var norm = this.state.getNormalisedValue();
        var scaled = this.state.getScaledValue();

        // Update rotation
        var angle = -135 + norm * 270;
        this.knobBody.style.transform = "rotate(" + angle + "deg)";

        // Update dots
        var activeDots = Math.round(norm * (this.dots.length - 1));
        for (var i = 0; i < this.dots.length; i++) {
            this.dots[i].setAttribute("fill", i <= activeDots ? "#d97706" : "#1a1a1a");
        }

        // Update range input (for sync)
        this.rangeInput.value = norm;

        // Update value label
        if (this.config.displayFn) {
            this.valueLabel.textContent = this.config.displayFn(scaled);
        }
    }
}

/* ================================================================
   MeterBar Component
   ================================================================ */

class MeterBar {
    constructor(container, label) {
        this.container = container;
        this.segments = 32;
        this.segsL = [];
        this.segsR = [];

        this._buildDOM(label);
    }

    _buildDOM(label) {
        var el = document.createElement("div");
        el.className = "meter-container";

        var lbl = document.createElement("span");
        lbl.className = "meter-label";
        lbl.textContent = label;
        el.appendChild(lbl);

        var bars = document.createElement("div");
        bars.className = "meter-bars";

        // Left channel
        var chL = document.createElement("div");
        chL.className = "meter-channel";
        for (var i = 0; i < this.segments; i++) {
            var seg = document.createElement("div");
            seg.className = "meter-seg";
            chL.appendChild(seg);
            this.segsL.push(seg);
        }
        bars.appendChild(chL);

        // Right channel
        var chR = document.createElement("div");
        chR.className = "meter-channel";
        for (var i = 0; i < this.segments; i++) {
            var seg = document.createElement("div");
            seg.className = "meter-seg";
            chR.appendChild(seg);
            this.segsR.push(seg);
        }
        bars.appendChild(chR);

        el.appendChild(bars);
        this.container.appendChild(el);
    }

    update(levelL, levelR) {
        var litL = Math.round(levelL * this.segments);
        var litR = Math.round(levelR * this.segments);

        this._updateChannel(this.segsL, litL);
        this._updateChannel(this.segsR, litR);
    }

    _updateChannel(segs, lit) {
        for (var i = 0; i < segs.length; i++) {
            if (i < lit) {
                var pos = i / this.segments;
                if (pos > 0.9) {
                    segs[i].className = "meter-seg red";
                } else if (pos > 0.75) {
                    segs[i].className = "meter-seg yellow";
                } else {
                    segs[i].className = "meter-seg green";
                }
            } else {
                segs[i].className = "meter-seg";
            }
        }
    }
}

/* ================================================================
   FocusSwitch Component
   ================================================================ */

class FocusSwitch {
    constructor(container) {
        this.container = container;
        this.buttons = [];
        this.labels = ["LOW", "MID", "HIGH"];

        this.state = new ComboBoxState("focusCombo");

        this._buildDOM();

        this.state.addValueListener(() => this._updateVisual());
        this.state.addPropertiesListener(() => this._updateVisual());
        this.state.requestInitialUpdate();
    }

    _buildDOM() {
        var el = document.createElement("div");
        el.className = "focus-container";

        var lbl = document.createElement("span");
        lbl.className = "focus-label";
        lbl.textContent = "FOCUS";
        el.appendChild(lbl);

        var btns = document.createElement("div");
        btns.className = "focus-buttons";

        var self = this;

        for (var i = 0; i < this.labels.length; i++) {
            (function(index) {
                var btn = document.createElement("button");
                btn.className = "focus-btn";
                btn.textContent = self.labels[index];
                btn.addEventListener("click", function() {
                    self.state.setChoiceIndex(index);
                    self._updateVisual();
                });
                btns.appendChild(btn);
                self.buttons.push(btn);
            })(i);
        }

        el.appendChild(btns);
        this.container.appendChild(el);
    }

    _updateVisual() {
        var activeIndex = this.state.getChoiceIndex();
        for (var i = 0; i < this.buttons.length; i++) {
            if (i === activeIndex) {
                this.buttons[i].className = "focus-btn active";
            } else {
                this.buttons[i].className = "focus-btn";
            }
        }
    }
}

/* ================================================================
   BypassButton Component
   ================================================================ */

class BypassButton {
    constructor(container) {
        this.container = container;

        this.state = new ToggleState("bypassToggle");

        this._buildDOM();

        this.state.addValueListener(() => this._updateVisual());
        this.state.requestInitialUpdate();
    }

    _buildDOM() {
        var btn = document.createElement("button");
        btn.className = "bypass-button";
        this.button = btn;

        // Power icon SVG
        btn.innerHTML =
            '<svg fill="none" viewBox="0 0 24 24" stroke="currentColor" stroke-width="2.5">' +
            '<path stroke-linecap="round" stroke-linejoin="round" d="M5.636 5.636a9 9 0 1012.728 0M12 3v9" />' +
            '</svg>';

        var self = this;
        btn.addEventListener("click", function() {
            self.state.setValue(!self.state.getValue());
            self._updateVisual();
        });

        this.container.appendChild(btn);
    }

    _updateVisual() {
        var bypassed = this.state.getValue();

        if (bypassed) {
            this.button.className = "bypass-button bypassed";
        } else {
            this.button.className = "bypass-button";
        }

        // Toggle dimming on main controls
        var mainControls = document.getElementById("main-controls");
        if (mainControls) {
            if (bypassed) {
                mainControls.classList.add("bypassed");
            } else {
                mainControls.classList.remove("bypassed");
            }
        }
    }
}

/* ================================================================
   Initialization
   ================================================================ */

document.addEventListener("DOMContentLoaded", function() {
    // Knobs
    var grainKnob = new DotKnob(document.getElementById("knob-grain"), {
        relayName: "grainSlider",
        label: "GRAIN",
        size: "large",
        displayFn: function(v) { return Math.round(v * 100) + "%"; }
    });

    var warmKnob = new DotKnob(document.getElementById("knob-warm"), {
        relayName: "warmSlider",
        label: "WARM",
        size: "large",
        displayFn: function(v) { return Math.round(v * 200 - 100) + "%"; }
    });

    var inputKnob = new DotKnob(document.getElementById("knob-input"), {
        relayName: "inputSlider",
        label: "INPUT",
        size: "normal",
        displayFn: function(v) {
            var s = v >= 0 ? "+" + v.toFixed(1) : v.toFixed(1);
            return s + " dB";
        }
    });

    var mixKnob = new DotKnob(document.getElementById("knob-mix"), {
        relayName: "mixSlider",
        label: "MIX",
        size: "medium",
        displayFn: function(v) { return Math.round(v * 100) + "%"; }
    });

    var outputKnob = new DotKnob(document.getElementById("knob-output"), {
        relayName: "outputSlider",
        label: "OUTPUT",
        size: "normal",
        displayFn: function(v) {
            var s = v >= 0 ? "+" + v.toFixed(1) : v.toFixed(1);
            return s + " dB";
        }
    });

    // Meters
    var inMeter = new MeterBar(document.getElementById("meter-in"), "IN");
    var outMeter = new MeterBar(document.getElementById("meter-out"), "OUT");

    // Focus switch
    var focusSwitch = new FocusSwitch(document.getElementById("focus-switch"));

    // Bypass button
    var bypassBtn = new BypassButton(document.getElementById("bypass-btn"));

    // Meter updates from C++ (custom event, not relay)
    window.__JUCE__.backend.addEventListener("meterUpdate", function(data) {
        inMeter.update(data.inL, data.inR);
        outMeter.update(data.outL, data.outR);
    });
});
