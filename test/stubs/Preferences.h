#pragma once
#include <map>
#include <string>

// Stub for ESP32 Preferences (NVS storage).
// All namespaces share one static store so save/load round-trips work correctly.
class Preferences {
public:
    // Reset entire NVS between tests
    static void clearAll() { store().clear(); }

    bool begin(const char* ns, bool readOnly) {
        (void)readOnly;
        ns_ = ns;
        return true;
    }
    void end() {}

    bool isKey(const char* key) {
        auto& m = store()[ns_];
        return m.count(key) > 0;
    }
    void putBool(const char* key, bool v)  { store()[ns_][key] = v ? 1 : 0; }
    void putInt (const char* key, int  v)  { store()[ns_][key] = v; }

    bool getBool(const char* key, bool def = false) {
        auto& m = store()[ns_];
        auto it = m.find(key);
        return (it != m.end()) ? (it->second != 0) : def;
    }
    int getInt(const char* key, int def = 0) {
        auto& m = store()[ns_];
        auto it = m.find(key);
        return (it != m.end()) ? (int)it->second : def;
    }
    void clear() { store()[ns_].clear(); }

private:
    std::string ns_;
    static std::map<std::string, std::map<std::string, int>>& store() {
        static std::map<std::string, std::map<std::string, int>> s;
        return s;
    }
};
