#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <Arduino.h>

const char* htmlDashboard PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Smart Sentry Command Center</title>
    <style>
        body { background-color: #0d0d0d; color: #00ff00; font-family: 'Courier New', Courier, monospace; margin: 0; display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100vh; overflow: hidden; }
        h1 { margin-bottom: 5px; text-transform: uppercase; letter-spacing: 2px; }
        .dashboard { display: flex; gap: 40px; margin-top: 20px; }
        .radar-container { position: relative; width: 400px; height: 400px; border-radius: 50%; border: 2px solid #00ff00; background: radial-gradient(circle, #001a00 0%, #000 70%); box-shadow: 0 0 20px #00ff00; }
        canvas { position: absolute; top: 0; left: 0; border-radius: 50%; }
        .stats { background: #111; border: 1px solid #00ff00; padding: 20px; width: 300px; box-shadow: 0 0 10px #00ff00; }
        .stat-row { display: flex; justify-content: space-between; margin-bottom: 15px; font-size: 1.2rem; border-bottom: 1px dashed #004400; padding-bottom: 5px; }
        .alert { color: #ff0000; font-weight: bold; animation: blink 1s infinite; display: none; }
        @keyframes blink { 0% { opacity: 1; } 50% { opacity: 0; } 100% { opacity: 1; } }
    </style>
</head>
<body>

    <h1>Smart Sentry Command</h1>
    
    <div class="dashboard">
        <div class="radar-container">
            <canvas id="radarCanvas" width="400" height="400"></canvas>
        </div>

        <div class="stats">
            <div class="stat-row"><span>STATUS:</span> <span id="val_status">SCANNING</span></div>
            <div class="stat-row"><span>AZIMUTH:</span> <span id="val_angle">0&deg;</span></div>
            <div class="stat-row"><span>RANGE:</span> <span id="val_dist">-- cm</span></div>
            <div id="alert_box" class="alert">MULTIPLE THREATS DETECTED</div>
            <br>
            <button onclick="initAudio()" style="background:#00ff00; color:#000; padding:10px; width:100%; border:none; cursor:pointer; font-weight:bold;">ACTIVATE VOICE SYSTEM</button>
        </div>
    </div>

    <script>
        const canvas = document.getElementById('radarCanvas');
        const ctx = canvas.getContext('2d');
        const centerX = canvas.width / 2;
        const centerY = canvas.height / 2;
        const radius = canvas.width / 2;
        
        let currentAngle = 0;
        let blips = [];
        let audioEnabled = false;
        let hasSpoken = false;
        let lastThreatTime = 0;

        function initAudio() {
            audioEnabled = true;
            let u = new SpeechSynthesisUtterance("Voice system online. Perimeter active.");
            speechSynthesis.speak(u);
            alert("Voice warnings armed.");
        }

        function drawRadar() {
            // Fade the background slightly to create the sweep trail effect
            ctx.fillStyle = 'rgba(0, 0, 0, 0.05)';
            ctx.fillRect(0, 0, canvas.width, canvas.height);
            
            // Draw Grid
            ctx.strokeStyle = '#004400';
            ctx.lineWidth = 1;
            ctx.beginPath(); ctx.arc(centerX, centerY, radius * 0.33, 0, 2*Math.PI); ctx.stroke();
            ctx.beginPath(); ctx.arc(centerX, centerY, radius * 0.66, 0, 2*Math.PI); ctx.stroke();
            ctx.beginPath(); ctx.arc(centerX, centerY, radius, 0, 2*Math.PI); ctx.stroke();
            ctx.beginPath(); ctx.moveTo(0, centerY); ctx.lineTo(canvas.width, centerY); ctx.stroke();
            ctx.beginPath(); ctx.moveTo(centerX, 0); ctx.lineTo(centerX, canvas.height); ctx.stroke();
            
            // Draw Danger Zone (Red Ring at 150cm / 37.5% of max range)
            ctx.strokeStyle = 'rgba(255, 0, 0, 0.3)';
            ctx.beginPath(); ctx.arc(centerX, centerY, radius * 0.375, 0, 2*Math.PI); ctx.stroke();

            // Draw Sweep Line
            ctx.strokeStyle = '#00ff00';
            ctx.lineWidth = 2;
            ctx.beginPath();
            ctx.moveTo(centerX, centerY);
            let rad = (180 - currentAngle) * Math.PI / 180;
            ctx.lineTo(centerX + radius * Math.cos(rad), centerY - radius * Math.sin(rad));
            ctx.stroke();

            // Draw and fade blips
            for (let i = blips.length - 1; i >= 0; i--) {
                let blip = blips[i];
                let r = (blip.distance / 400.0) * radius; // Max 400cm range
                if (r > radius) r = radius;
                
                let tx = centerX + r * Math.cos((180 - blip.angle) * Math.PI / 180);
                let ty = centerY - r * Math.sin((180 - blip.angle) * Math.PI / 180);
                
                // Red if inside danger zone (150cm), Green if just mapping the room
                ctx.fillStyle = (blip.distance < 150) ? `rgba(255, 0, 0, ${blip.alpha})` : `rgba(0, 255, 0, ${blip.alpha})`;
                
                ctx.beginPath();
                ctx.arc(tx, ty, (blip.distance < 150) ? 6 : 3, 0, 2*Math.PI);
                ctx.fill();

                blip.alpha -= 0.005; // Fade out slowly
                if (blip.alpha <= 0) {
                    blips.splice(i, 1);
                }
            }

            requestAnimationFrame(drawRadar);
        }
        drawRadar();

        // WebSocket Connection
        var gateway = `ws://${window.location.hostname}/ws`;
        var websocket;

        function initWebSocket() {
            websocket = new WebSocket(gateway);
            websocket.onmessage = onMessage;
            websocket.onclose = function() { setTimeout(initWebSocket, 2000); };
        }
        
        function onMessage(event) {
            let data = JSON.parse(event.data);
            
            document.getElementById('val_status').innerText = data.state;
            document.getElementById('val_angle').innerText = data.angle + "°";
            
            if (data.distance < 400) {
                document.getElementById('val_dist').innerText = parseInt(data.distance) + " cm";
                // Plot the new reading on the radar
                blips.push({ angle: data.angle, distance: data.distance, alpha: 1.0 });
            }

            currentAngle = data.angle;
            
            // Voice Logic - Trigger if any threat is detected within 150cm
            if (data.state === "THREAT_DETECTED") {
                document.getElementById('alert_box').style.display = 'block';
                document.getElementById('val_status').style.color = '#ff0000';
                
                let now = Date.now();
                if (audioEnabled && (now - lastThreatTime > 5000)) { // Speak max once every 5 secs
                    let utterance = new SpeechSynthesisUtterance("Perimeter breached. Multiple threats detected.");
                    utterance.rate = 0.9;
                    utterance.pitch = 0.8;
                    speechSynthesis.speak(utterance);
                    lastThreatTime = now;
                }
            } else {
                document.getElementById('alert_box').style.display = 'none';
                document.getElementById('val_status').style.color = '#00ff00';
            }
        }

        window.addEventListener('load', initWebSocket);
    </script>
</body>
</html>
)rawliteral";

#endif
