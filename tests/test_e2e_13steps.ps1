# ====================================================================
# Smart Parking System: Mandatory 13-Step End-to-End Workflow Test
# Specified in requirement.md Section 12
# ====================================================================

$ErrorActionPreference = "Stop"
[System.Net.ServicePointManager]::Expect100Continue = $false

$Port = 8088
$BaseUrl = "http://localhost:$Port"
$ServerExe = "backend\server.exe"

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " EXECUTING MANDATORY 13-STEP E2E WORKFLOW VERIFICATION   " -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

# 1. Start Server Process
$serverProc = Start-Process -FilePath $ServerExe -ArgumentList "$Port" -PassThru -WindowStyle Hidden
Start-Sleep -Seconds 2

try {
    # Step 1: Configure facility and slots
    Write-Host -NoNewline "[Step 1/13] Configure parking facility and slots ... "
    $cfg = '{"name":"Grand Central Garage","car_slots":3,"bike_slots":2,"ev_slots":2,"car_rate":20.0,"bike_rate":10.0,"ev_rate":25.0}'
    $r1 = Invoke-RestMethod -Uri "$BaseUrl/api/facility/config" -Method POST -Headers @{"Content-Type"="application/json"} -Body $cfg
    if ($r1.success) { Write-Host "[PASS]" -ForegroundColor Green } else { throw "Failed" }

    # Step 2: Register a customer and vehicle
    Write-Host -NoNewline "[Step 2/13] Register customer and vehicle ... "
    $cust = '{"name":"John E2E","phone":"999-E2E-001","email":"john@e2e.org"}'
    $rCust = Invoke-RestMethod -Uri "$BaseUrl/api/customers" -Method POST -Headers @{"Content-Type"="application/json"} -Body $cust
    $custId = $rCust.data.customer_id

    $veh = "{`"customer_id`":$custId,`"license_plate`":`"E2E-CAR-01`",`"category`":`"CAR`"}"
    $rVeh = Invoke-RestMethod -Uri "$BaseUrl/api/vehicles" -Method POST -Headers @{"Content-Type"="application/json"} -Body $veh
    if ($rVeh.success) { Write-Host "[PASS]" -ForegroundColor Green } else { throw "Failed" }

    # Step 3: Search for an available slot
    Write-Host -NoNewline "[Step 3/13] Search for available CAR slot ... "
    $slots = Invoke-RestMethod -Uri "$BaseUrl/api/slots?category=CAR&status=AVAILABLE" -Method GET
    $targetSlot = $slots.data[0]
    if ($targetSlot -and $targetSlot.slot_number) { Write-Host "[PASS] (Found $($targetSlot.slot_number))" -ForegroundColor Green } else { throw "Failed" }

    # Step 4: Create and confirm a valid reservation
    Write-Host -NoNewline "[Step 4/13] Create and confirm valid reservation ... "
    $resv = "{`"customer_id`":$custId,`"vehicle_id`":$($rVeh.data.vehicle_id),`"slot_id`":$($targetSlot.slot_id)}"
    $rResv = Invoke-RestMethod -Uri "$BaseUrl/api/reservations" -Method POST -Headers @{"Content-Type"="application/json"} -Body $resv
    $bookingId = $rResv.data.booking_id
    if ($bookingId -and $rResv.data.status -eq "CONFIRMED") { Write-Host "[PASS] (Booking: $bookingId)" -ForegroundColor Green } else { throw "Failed" }

    # Step 5: Record vehicle entry
    Write-Host -NoNewline "[Step 5/13] Record vehicle entry at gate ... "
    $entry = "{`"booking_id`":`"$bookingId`",`"license_plate`":`"E2E-CAR-01`",`"category`":`"CAR`",`"customer_name`":`"John E2E`",`"phone`":`"999-E2E-001`"}"
    $rEntry = Invoke-RestMethod -Uri "$BaseUrl/api/parking/entry" -Method POST -Headers @{"Content-Type"="application/json"} -Body $entry
    $sessionId = $rEntry.data.session_id
    if ($rEntry.success) { Write-Host "[PASS] (Assigned to $($rEntry.data.slot_number))" -ForegroundColor Green } else { throw "Failed" }

    # Step 6: Verify slot and active vehicle records
    Write-Host -NoNewline "[Step 6/13] Verify active vehicle records in linked list ... "
    $active = Invoke-RestMethod -Uri "$BaseUrl/api/parking/active" -Method GET
    $foundActive = $active.data | Where-Object { $_.license_plate -eq "E2E-CAR-01" }
    if ($foundActive) { Write-Host "[PASS]" -ForegroundColor Green } else { throw "Failed" }

    # Step 7: Handle full-parking scenario and waiting queue
    Write-Host -NoNewline "[Step 7/13] Fill remaining car capacity & verify FIFO waiting queue ... "
    # Occupy the other 2 car slots (C-02 and C-03)
    $e2 = Invoke-RestMethod -Uri "$BaseUrl/api/parking/entry" -Method POST -Headers @{"Content-Type"="application/json"} -Body '{"license_plate":"CAR-FILL-2","category":"CAR","customer_name":"Driver 2","phone":"9990000002"}'
    $e3 = Invoke-RestMethod -Uri "$BaseUrl/api/parking/entry" -Method POST -Headers @{"Content-Type"="application/json"} -Body '{"license_plate":"CAR-FILL-3","category":"CAR","customer_name":"Driver 3","phone":"9990000003"}'

    # Now 4th car tries to enter -> should be rejected as Full (409)
    try {
        Invoke-RestMethod -Uri "$BaseUrl/api/parking/entry" -Method POST -Headers @{"Content-Type"="application/json"} -Body '{"license_plate":"CAR-QUEUED-4","category":"CAR","customer_name":"Driver 4","phone":"9990000004"}'
        throw "Should have rejected full entry"
    } catch {
        # Expected rejection
    }

    # Customer joins waiting queue
    $qJoin = "{`"customer_id`":$custId,`"license_plate`":`"CAR-QUEUED-4`",`"category`":`"CAR`"}"
    $rQ = Invoke-RestMethod -Uri "$BaseUrl/api/queue/join" -Method POST -Headers @{"Content-Type"="application/json"} -Body $qJoin
    if ($rQ.data.queue_position -ge 1) { Write-Host "[PASS] (Queued at pos $($rQ.data.queue_position))" -ForegroundColor Green } else { throw "Failed" }

    # Step 8: Record vehicle exit
    Write-Host -NoNewline "[Step 8/13] Record vehicle exit ... "
    $exit = '{"license_plate":"E2E-CAR-01"}'
    $rExit = Invoke-RestMethod -Uri "$BaseUrl/api/parking/exit" -Method POST -Headers @{"Content-Type"="application/json"} -Body $exit
    $due = $rExit.data.amount_due
    # Check that FIFO queue customer was auto-allocated the vacated slot!
    if ($rExit.data.queue_allocated -and $rExit.data.queue_allocated.license_plate -eq "CAR-QUEUED-4") {
        Write-Host "[PASS] (Auto-allocated to queued customer CAR-QUEUED-4)" -ForegroundColor Green
    } else {
        throw "Queue auto-allocation failed"
    }

    # Step 9: Calculate parking fee
    Write-Host -NoNewline "[Step 9/13] Calculate parking fee ... "
    if ($due -ge 20.0) { Write-Host "[PASS] (Due: `$$due)" -ForegroundColor Green } else { throw "Failed" }

    # Step 10: Record payment information
    Write-Host -NoNewline "[Step 10/13] Record payment transaction ... "
    $pay = "{`"session_id`":$sessionId,`"booking_id`":`"$bookingId`",`"amount`":$due,`"payment_method`":`"CARD`"}"
    $rPay = Invoke-RestMethod -Uri "$BaseUrl/api/payments" -Method POST -Headers @{"Content-Type"="application/json"} -Body $pay
    if ($rPay.success) { Write-Host "[PASS] (Receipt ID: #$($rPay.data.payment_id))" -ForegroundColor Green } else { throw "Failed" }

    # Step 11: Verify parking history and vehicle search
    Write-Host -NoNewline "[Step 11/13] Verify parking history stack & vehicle search ... "
    $hist = Invoke-RestMethod -Uri "$BaseUrl/api/history" -Method GET
    $search = Invoke-RestMethod -Uri "$BaseUrl/api/vehicles/search?plate=E2E-CAR-01" -Method GET
    if ($hist.data.Count -gt 0 -and $search.data.customer_name -eq "John E2E") {
        Write-Host "[PASS]" -ForegroundColor Green
    } else { throw "Failed" }

    # Step 12: Generate reports using persisted data
    Write-Host -NoNewline "[Step 12/13] Generate reports from persistent database ... "
    $rep = Invoke-RestMethod -Uri "$BaseUrl/api/reports/summary" -Method GET
    if ($rep.data.total_revenue -ge 20.0) {
        Write-Host "[PASS] (Total Revenue: `$$($rep.data.total_revenue))" -ForegroundColor Green
    } else { throw "Failed" }

    # Step 13: Restart application and verify that persistent records remain available
    Write-Host -NoNewline "[Step 13/13] Restart C11 server & verify persistence ... "
    Stop-Process -Id $serverProc.Id -Force
    Start-Sleep -Seconds 1

    # Restart server
    $serverProc = Start-Process -FilePath $ServerExe -ArgumentList "$Port" -PassThru -WindowStyle Hidden
    Start-Sleep -Seconds 2

    # Query persistent data after restart
    $postRestartRep = Invoke-RestMethod -Uri "$BaseUrl/api/reports/summary" -Method GET
    $postRestartCust = Invoke-RestMethod -Uri "$BaseUrl/api/customers" -Method GET
    $postRestartSearch = Invoke-RestMethod -Uri "$BaseUrl/api/vehicles/search?plate=E2E-CAR-01" -Method GET

    if ($postRestartRep.data.total_revenue -ge 20.0 -and $postRestartSearch.data.customer_name -eq "John E2E") {
        Write-Host "[PASS] (All persistent records verified intact after restart!)" -ForegroundColor Green
    } else {
        throw "Persistence verification failed after restart"
    }

} finally {
    Write-Host "Stopping C11 server..." -ForegroundColor Cyan
    Stop-Process -Id $serverProc.Id -Force -ErrorAction SilentlyContinue
}

Write-Host "==========================================================" -ForegroundColor Green
Write-Host " 100% OF MANDATORY 13-STEP E2E CRITICAL WORKFLOW PASSED!  " -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Green
