#ifndef KALMAN_FILTER_H
#define KALMAN_FILTER_H

#include <Arduino.h>

class KalmanFilter1D {
private:
    float x_est;  // Estimated distance
    float v_est;  // Estimated velocity
    float P;      // Error covariance
    float Q;      // Process noise
    float R;      // Measurement noise
    unsigned long lastUpdateTime;

public:
    KalmanFilter1D(float processNoise = 0.5, float measurementNoise = 4.0) {
        x_est = 0;
        v_est = 0;
        P = 1.0;
        Q = processNoise;
        R = measurementNoise;
        lastUpdateTime = millis();
    }

    void reset(float initialDistance) {
        x_est = initialDistance;
        v_est = 0;
        P = 1.0;
        lastUpdateTime = millis();
    }

    void update(float measurement) {
        unsigned long now = millis();
        float dt = (now - lastUpdateTime) / 1000.0;
        
        // Guard against massive time jumps or zero dt
        if (dt <= 0 || dt > 1.0) {
            dt = 0.05;
        }
        lastUpdateTime = now;

        // Predict
        float x_pred = x_est + v_est * dt;
        float P_pred = P + Q;

        // Update
        float K = P_pred / (P_pred + R);
        float prev_x = x_est;
        x_est = x_pred + K * (measurement - x_pred);
        
        // Velocity estimate
        v_est = (x_est - prev_x) / dt;
        P = (1 - K) * P_pred;
    }

    float getDistance() const { return x_est; }
    float getVelocity() const { return v_est; }
};

#endif
