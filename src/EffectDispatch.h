#ifndef EFFECT_DISPATCH_H
#define EFFECT_DISPATCH_H

#include "SettingsController.h"
#include "StripLEDController.h"
#include "MatrixLEDController.h"

/**
 * Resolve an LED effect name and start it on the appropriate controller.
 * @return true if the effect was started, false on null pointers or unrecognized name
 */
inline bool dispatchLEDEffect(SettingsController* settings, const char* name,
                              StripLEDController* strip, MatrixLEDController* matrix) {
    if (!settings || !name) return false;

    bool isStrip = false;
    int enumVal = 0;
    if (!settings->getLEDEffectFromName(name, &isStrip, &enumVal)) return false;

    if (isStrip) {
        if (!strip) return false;
        strip->startEffect((StripLEDController::EffectType)enumVal);
    } else {
        if (!matrix) return false;
        matrix->startEffect((MatrixLEDController::EffectType)enumVal);
    }
    return true;
}

/**
 * Resolve an LED effect name and check if it is still running.
 * @return true if the effect is running, false on null pointers or unrecognized name
 */
inline bool isLEDEffectRunning(SettingsController* settings, const char* name,
                               StripLEDController* strip, MatrixLEDController* matrix) {
    if (!settings || !name) return false;

    bool isStrip = false;
    int enumVal = 0;
    if (!settings->getLEDEffectFromName(name, &isStrip, &enumVal)) return false;

    if (isStrip) {
        return strip && strip->isEffectRunning();
    } else {
        return matrix && matrix->isEffectRunning();
    }
}

#endif // EFFECT_DISPATCH_H
