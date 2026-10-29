# ====================================================================
# Smart Parking C11 Backend API Integration Test
# ====================================================================

$ErrorActionPreference = "Continue"
[System.Net.ServicePointManager]::Expect100Continue = $false

$Port = 8089
$BaseUrl = "http://localhost:$Port"
$ServerExe = "backend\server.exe"

if (-not (Test-Path $ServerExe)) {
    Write-Error "server.exe not found! Please build backend first."
    exit 1
}

Write-Host "====================================================" -ForegroundColor Cyan
Write-Host " Starting C11 HTTP Backend on Port $Port...         " -ForegroundColor Cyan
Write-Host "====================================================" -ForegroundColor Cyan

# Start server process
$proc = Start-Process -FilePath $ServerExe -ArgumentList "$Port" -PassThru -WindowStyle Hidden
Start-Sleep -Seconds 2

$totalTests = 0
$passedTests = 0

function Assert-Endpoint {
    param(
        [string]$Name,
        [string]$Method,
        [string]$Url,
        [string]$Body = $null,
        [int]$ExpectedStatus = 200
    )
    $script:totalTests++
    Write-Host -NoNewline "Testing $Name ($Method $Url) ... "

    try {
        $headers = @{ "Content-Type" = "application/json" }
        if ($Body) {
            $resp = Invoke-RestMethod -Uri $Url -Method $Method -Headers $headers -Body $Body -TimeoutSec 5
        } else {
            $resp = Invoke-RestMethod -Uri $Url -Method $Method -Headers $headers -TimeoutSec 5
        }

        if ($resp.success -eq $true) {
            Write-Host "[PASS]" -ForegroundColor Green
            $script:passedTests++
            return $resp
        } else {
            Write-Host "[FAIL] (success != true)" -ForegroundColor Red
            return $null
        }
    } catch {
        Write-Host "[FAIL] ($($_.Exception.Message))" -ForegroundColor Red
        return $null
    }
}

try {
    # 1. Health check
    Assert-Endpoint -Name "Health Check" -Method "GET" -Url "$BaseUrl/api/health"

    # 2. Facility Details
    Assert-Endpoint -Name "Get Facility" -Method "GET" -Url "$BaseUrl/api/facility"

    # 3. Configure Facility
    $cfgBody = '{"name":"Metropolis Parking","car_slots":20,"bike_slots":10,"ev_slots":5,"car_rate":25.0,"bike_rate":12.0,"ev_rate":30.0}'
    Assert-Endpoint -Name "Config Facility" -Method "POST" -Url "$BaseUrl/api/facility/config" -Body $cfgBody

    # 4. Filter Slots
    Assert-Endpoint -Name "Get Car Slots" -Method "GET" -Url "$BaseUrl/api/slots?category=CAR"

    $rnd = Get-Random -Minimum 1000 -Maximum 9999
    $testPhone = "555-SPIDER-$rnd"
    $testPlate = "WEB-$rnd"

    # 5. Register Customer
    $custBody = "{`"name`":`"Peter Parker`",`"phone`":`"$testPhone`",`"email`":`"peter@dailybugle.com`"}"
    $custResp = Assert-Endpoint -Name "Register Customer" -Method "POST" -Url "$BaseUrl/api/customers" -Body $custBody -ExpectedStatus 201

    $custId = 1
    if ($custResp -and $custResp.data.customer_id) { $custId = $custResp.data.customer_id }

    # 6. Customer Listing
    Assert-Endpoint -Name "List Customers" -Method "GET" -Url "$BaseUrl/api/customers"

    # 7. Register Vehicle
    $vehBody = "{`"customer_id`":$custId,`"license_plate`":`"$testPlate`",`"category`":`"CAR`"}"
    $vehResp = Assert-Endpoint -Name "Register Vehicle" -Method "POST" -Url "$BaseUrl/api/vehicles" -Body $vehBody -ExpectedStatus 201

    # 8. Vehicle Search
    Assert-Endpoint -Name "Search Vehicle by Plate" -Method "GET" -Url "$BaseUrl/api/vehicles/search?plate=$testPlate"

    # 9. Create Reservation
    $resvBody = "{`"customer_id`":$custId,`"vehicle_id`":1,`"slot_id`":1}"
    $resvResp = Assert-Endpoint -Name "Create Reservation" -Method "POST" -Url "$BaseUrl/api/reservations" -Body $resvBody -ExpectedStatus 201

    $bookingId = "BK-TEST"
    if ($resvResp -and $resvResp.data.booking_id) { $bookingId = $resvResp.data.booking_id }

    # 10. Vehicle Entry Gate
    $entryBody = "{`"booking_id`":`"$bookingId`",`"license_plate`":`"$testPlate`",`"category`":`"CAR`",`"customer_name`":`"Peter Parker`",`"phone`":`"$testPhone`"}"
    $entryResp = Assert-Endpoint -Name "Vehicle Check-In Gate" -Method "POST" -Url "$BaseUrl/api/parking/entry" -Body $entryBody

    # 11. Active Parked Vehicles
    Assert-Endpoint -Name "List Active Parked Vehicles" -Method "GET" -Url "$BaseUrl/api/parking/active"

    # 12. Vehicle Exit Gate
    $exitBody = "{`"license_plate`":`"$testPlate`"}"
    $exitResp = Assert-Endpoint -Name "Vehicle Check-Out Gate" -Method "POST" -Url "$BaseUrl/api/parking/exit" -Body $exitBody

    # 13. Process Payment
    $payBody = "{`"session_id`":1,`"booking_id`":`"$bookingId`",`"amount`":25.0,`"payment_method`":`"CARD`"}"
    Assert-Endpoint -Name "Process Payment" -Method "POST" -Url "$BaseUrl/api/payments" -Body $payBody

    # 14. Join Waiting Queue
    $qBody = "{`"customer_id`":$custId,`"license_plate`":`"QUEUED-99`",`"category`":`"CAR`"}"
    Assert-Endpoint -Name "Join Waiting Queue" -Method "POST" -Url "$BaseUrl/api/queue/join" -Body $qBody

    # 15. Get Waiting Queue
    Assert-Endpoint -Name "Get Waiting Queue" -Method "GET" -Url "$BaseUrl/api/queue"

    # 16. Get History Stack
    Assert-Endpoint -Name "Get History Stack" -Method "GET" -Url "$BaseUrl/api/history"

    # 17. Reports & Summary
    Assert-Endpoint -Name "Dashboard Reports Summary" -Method "GET" -Url "$BaseUrl/api/reports/summary"

} finally {
    Write-Host "Stopping C11 test server process..." -ForegroundColor Cyan
    Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
}

Write-Host "====================================================" -ForegroundColor Cyan
Write-Host "API Tests Run: $totalTests | Passed: $passedTests | Failed: $($totalTests - $passedTests)" -ForegroundColor Yellow
Write-Host "====================================================" -ForegroundColor Cyan

if ($totalTests -eq $passedTests) {
    exit 0
} else {
    exit 1
}
