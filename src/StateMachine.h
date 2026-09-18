#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

enum SystemState {
    IDLE,
    CALIBRATING,
    SCANNING,
    DETECTING,
    TRACKING,
    LOCKED
};

String stateToString(SystemState state) {
    switch (state) {
        case IDLE: return "IDLE";
        case CALIBRATING: return "CALIBRATING";
        case SCANNING: return "SCANNING";
        case DETECTING: return "DETECTING";
        case TRACKING: return "TRACKING";
        case LOCKED: return "LOCKED_UNAUTHORIZED";
        default: return "UNKNOWN";
    }
}

class ConfidenceScorer {
private:
    float readingBuffer[5];
    int bufferIndex;
    int validDetectionStreak;

public:
    ConfidenceScorer() {
        bufferIndex = 0;
        validDetectionStreak = 0;
        for (int i = 0; i < 5; i++) readingBuffer[i] = 0;
    }

    void addReading(float dist) {
        readingBuffer[bufferIndex] = dist;
        bufferIndex = (bufferIndex + 1) % 5;
        if (dist > 2.0 && dist < 400.0) {
            validDetectionStreak++;
            if (validDetectionStreak > 10) validDetectionStreak = 10;
        } else {
            validDetectionStreak = 0;
        }
    }

    void reset() {
        validDetectionStreak = 0;
    }

    float computeConfidence(bool rcwlActive) {
        float mean = 0, variance = 0;
        for (int i = 0; i < 5; i++) mean += readingBuffer[i];
        mean /= 5.0;
        
        for (int i = 0; i < 5; i++) variance += pow(readingBuffer[i] - mean, 2);
        variance /= 5.0;
        
        float stddev = sqrt(variance);
        float consistency = constrain(1.0 - (stddev / max(mean, 1.0f)), 0, 1.0);
        float persistence = constrain((float)validDetectionStreak / 5.0, 0, 1.0);
        float validity = (mean > 2.0 && mean < 400.0) ? 1.0 : 0.0;
        
        float fusionBonus = (rcwlActive) ? 1.2 : 1.0;
        float confidence = (0.4 * consistency + 0.35 * persistence + 0.25 * validity) * 100.0 * fusionBonus;
        
        return constrain(confidence, 0.0, 100.0);
    }
};

#endif
