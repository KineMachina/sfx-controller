#ifndef AUDIO_H_MOCK
#define AUDIO_H_MOCK

#include "Arduino.h"
#include "SD.h"

class Audio {
public:
    void setPinout(int bck, int lrc, int dout) {
        (void)bck; (void)lrc; (void)dout;
    }
    void setVolume(int v) { _volume = v; }
    int getVolume() { return _volume; }
    void stopSong() { _running = false; }
    bool isRunning() { return _running; }
    void loop() {}
    void connecttoFS(SDClass&, const char*) { _running = true; }

    void reset() { _volume = 0; _running = false; }

private:
    int _volume = 0;
    bool _running = false;
};

#endif // AUDIO_H_MOCK
