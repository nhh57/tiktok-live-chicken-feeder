# Simple ESP32 Test Script
# ========================

$ESP32_IP = "172.20.10.2"

Write-Host "`n=== ESP32 TEST ===" -ForegroundColor Cyan

# Test 1: Ping
Write-Host "`n[1] Pinging ESP32..." -ForegroundColor Yellow
$ping = Test-Connection -ComputerName $ESP32_IP -Count 2 -Quiet

if ($ping) {
    Write-Host "OK - ESP32 is reachable" -ForegroundColor Green
} else {
    Write-Host "FAIL - Cannot ping ESP32" -ForegroundColor Red
    exit
}

# Test 2: HTTP GET
Write-Host "`n[2] Testing HTTP server..." -ForegroundColor Yellow
try {
    $response = Invoke-WebRequest -Uri "http://$ESP32_IP/" -Method GET -TimeoutSec 5
    Write-Host "OK - HTTP server running (Status: $($response.StatusCode))" -ForegroundColor Green
} catch {
    Write-Host "FAIL - HTTP server not responding" -ForegroundColor Red
    exit
}

# Test 3: POST /feed
Write-Host "`n[3] Testing motor control (cam)..." -ForegroundColor Yellow
try {
    $body = @{
        motor = "cam"
        duration = 1000
    } | ConvertTo-Json

    $headers = @{
        "Content-Type" = "application/json"
    }

    $response = Invoke-RestMethod -Uri "http://$ESP32_IP/feed" -Method POST -Headers $headers -Body $body -TimeoutSec 5
    
    Write-Host "OK - Motor command sent successfully!" -ForegroundColor Green
    Write-Host "Response: $($response | ConvertTo-Json -Compress)" -ForegroundColor Gray
} catch {
    Write-Host "FAIL - $($_.Exception.Message)" -ForegroundColor Red
}

Write-Host "`n=== TEST COMPLETE ===" -ForegroundColor Cyan
Write-Host "Check Serial Monitor for ESP32 logs" -ForegroundColor Yellow
