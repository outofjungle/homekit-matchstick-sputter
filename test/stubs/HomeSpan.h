#pragma once
#include <Arduino.h>  // for boolean

// Minimal HomeSpan stub for native tests.
// Provides just enough surface area for DEV_LedChannel to compile and for
// FSM methods to be exercised without linking the real HomeSpan library.

struct SpanCharacteristic {
    int _val    = 0;
    int _newVal = 0;

    explicit SpanCharacteristic(int v = 0) : _val(v), _newVal(v) {}
    int  getVal()    const { return _val; }
    int  getNewVal() const { return _newVal; }
    void setVal(int v)    { _val = v; }
    void setNewVal(int v) { _newVal = v; }
};

namespace Characteristic {
    struct On         : SpanCharacteristic { explicit On(int v=0)         : SpanCharacteristic(v) {} };
    struct Hue        : SpanCharacteristic { explicit Hue(int v=0)        : SpanCharacteristic(v) {} };
    struct Saturation : SpanCharacteristic { explicit Saturation(int v=0) : SpanCharacteristic(v) {} };
    struct Brightness : SpanCharacteristic { explicit Brightness(int v=0) : SpanCharacteristic(v) {} };
}

namespace Service {
    struct LightBulb {
        virtual boolean update() { return false; }
        virtual void loop() {}
        virtual ~LightBulb() {}
    };
}
