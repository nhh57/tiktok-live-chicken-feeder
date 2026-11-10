# Test ESP32 Connection and Feed Command
# ========================================

$ESP32_IP = "172.20.10.2"
$ESP32_URL = "http://$ESP32_IP"

Write-Host "`n=== ESP32 CONNECTION TEST ===" -ForegroundColor Cyan

# Test 1: Ping ESP32
Write-Host "`n[1] Testing network connectivity..." -ForegroundColor Yellow
$ping = Test-Connection -ComputerName $ESP32_IP -Count 2 -Quiet

if ($ping) {
    Write-Host "✓ Ping successful! ESP32 is reachable" -ForegroundColor Green
} else {
    Write-Host "✗ Ping failed! ESP32 not reachable" -ForegroundColor Red
    Write-Host "  -> Check if computer is connected to 'Hoang Hai's iPhone'" -ForegroundColor Yellow
    Write-Host "  -> Check if ESP32 is powered on and WiFi connected" -ForegroundColor Yellow
    exit
}

# Test 2: HTTP GET /
Write-Host "`n[2] Testing HTTP server (GET /)..." -ForegroundColor Yellow
try {
    $response = Invoke-WebRequest -Uri "$ESP32_URL/" -Method GET -TimeoutSec 5
    Write-Host "✓ HTTP server is running!" -ForegroundColor Green
    Write-Host "  Status: $($response.StatusCode)" -ForegroundColor Gray
} catch {
    Write-Host "✗ HTTP server not responding: $_" -ForegroundColor Red
    exit
}

# Test 3: POST /feed (cam motor)
Write-Host "`n[3] Testing POST /feed (cam motor)..." -ForegroundColor Yellow
try {
    $body = @{
        motor = "cam"
        duration = 1000
    } | ConvertTo-Json

    $response = Invoke-RestMethod -Uri "$ESP32_URL/feed" -Method POST -ContentType "application/json" -Body $body -TimeoutSec 5
    
    Write-Host "✓ Feed command sent successfully!" -ForegroundColor Green
    Write-Host "  Response: $($response | ConvertTo-Json)" -ForegroundColor Gray
    Write-Host "`n  -> Check Serial Monitor for ESP32 logs" -ForegroundColor Yellow
    Write-Host "  -> Check if relay/LED blinks on ESP32" -ForegroundColor Yellow
} catch {
    Write-Host "✗ Feed command failed: $_" -ForegroundColor Red
}

# Test 4: Multiple motors
Write-Host "`n[4] Testing multiple motors (cam + ngo)..." -ForegroundColor Yellow
try {
    $body = @{
        motors = @("cam", "ngo")
        duration = 500
    } | ConvertTo-Json

    $response = Invoke-RestMethod -Uri "$ESP32_URL/feed" -Method POST -ContentType "application/json" -Body $body -TimeoutSec 5
    
    Write-Host "✓ Multiple motors command sent!" -ForegroundColor Green
    Write-Host "  Response: $($response | ConvertTo-Json)" -ForegroundColor Gray
} catch {
    Write-Host "✗ Multiple motors failed: $_" -ForegroundColor Red
}

Write-Host "`n=== TEST COMPLETE ===" -ForegroundColor Cyan
Write-Host "Check Serial Monitor (Arduino IDE) for detailed ESP32 logs`n" -ForegroundColor Yellow
