# Deploy Supabase Edge Functions - PowerShell Script
#
# Usage:
#   $env:SUPABASE_ACCESS_TOKEN = "sbp_your_token_here"
#   .\deploy-edge-functions.ps1
#
# To get a Personal Access Token (PAT):
#   1. Go to https://supabase.com/dashboard/account/tokens
#   2. Click "Generate new token"
#   3. Name it (e.g., "CLI Deploy") and copy the token (starts with sbp_)
#
# Project: ncyipufvutzghwgcxukg
# URL: https://ncyipufvutzghwgcxukg.supabase.co

param(
    [string]$Token = $env:SUPABASE_ACCESS_TOKEN
)

$ErrorActionPreference = "Stop"

$PROJECT_REF = "ncyipufvutzghwgcxukg"
$SCRIPT_DIR = Split-Path -Parent $MyInvocation.MyCommand.Path
$FUNCTIONS_DIR = Join-Path $SCRIPT_DIR "backend\functions"

if ([string]::IsNullOrEmpty($Token)) {
    Write-Host "ERROR: No Supabase Access Token provided." -ForegroundColor Red
    Write-Host ""
    Write-Host "To get a Personal Access Token:" -ForegroundColor Yellow
    Write-Host "  1. Go to https://supabase.com/dashboard/account/tokens"
    Write-Host "  2. Click 'Generate new token'"
    Write-Host "  3. Copy the token (starts with sbp_)"
    Write-Host ""
    Write-Host "Then run:" -ForegroundColor Yellow
    Write-Host '  $env:SUPABASE_ACCESS_TOKEN = "sbp_your_token_here"'
    Write-Host "  .\deploy-edge-functions.ps1"
    Write-Host ""
    Write-Host "Or:" -ForegroundColor Yellow
    Write-Host '  .\deploy-edge-functions.ps1 -Token "sbp_your_token_here"'
    exit 1
}

$functions = @("validate-cartridge", "load-assay", "upload-test", "reset-cartridge")

Write-Host "=== Deploying Supabase Edge Functions ===" -ForegroundColor Cyan
Write-Host "Project: $PROJECT_REF"
Write-Host "Functions: $($functions -join ', ')"
Write-Host ""

$successCount = 0
$failCount = 0

foreach ($func in $functions) {
    Write-Host "--- Deploying: $func ---" -ForegroundColor Yellow

    $indexPath = Join-Path $FUNCTIONS_DIR "$func\index.ts"

    if (-not (Test-Path $indexPath)) {
        Write-Host "ERROR: $indexPath not found!" -ForegroundColor Red
        $failCount++
        continue
    }

    $uri = "https://api.supabase.com/v1/projects/$PROJECT_REF/functions/deploy?slug=$func"

    try {
        # Read file content
        $fileBytes = [System.IO.File]::ReadAllBytes($indexPath)

        # Build multipart form data
        $boundary = [System.Guid]::NewGuid().ToString()
        $LF = "`r`n"

        $metadata = @{
            entrypoint_path = "index.ts"
            name = $func
            verify_jwt = $false
        } | ConvertTo-Json -Compress

        $bodyLines = @(
            "--$boundary",
            "Content-Disposition: form-data; name=`"metadata`"",
            "Content-Type: application/json",
            "",
            $metadata,
            "--$boundary",
            "Content-Disposition: form-data; name=`"file`"; filename=`"index.ts`"",
            "Content-Type: application/typescript",
            "",
            [System.Text.Encoding]::UTF8.GetString($fileBytes),
            "--$boundary--"
        )

        $body = $bodyLines -join $LF

        $headers = @{
            "Authorization" = "Bearer $Token"
        }

        $response = Invoke-RestMethod -Uri $uri -Method Post -Headers $headers `
            -ContentType "multipart/form-data; boundary=$boundary" `
            -Body $body

        Write-Host "SUCCESS: $func deployed" -ForegroundColor Green
        $successCount++
    }
    catch {
        Write-Host "FAILED: $func" -ForegroundColor Red
        Write-Host "Error: $($_.Exception.Message)" -ForegroundColor Red
        if ($_.ErrorDetails.Message) {
            Write-Host "Details: $($_.ErrorDetails.Message)" -ForegroundColor Red
        }
        $failCount++
    }

    Write-Host ""
}

Write-Host "=== Deployment Complete ===" -ForegroundColor Cyan
Write-Host "  Success: $successCount / $($functions.Count)" -ForegroundColor $(if ($successCount -eq $functions.Count) { "Green" } else { "Yellow" })
if ($failCount -gt 0) {
    Write-Host "  Failed:  $failCount / $($functions.Count)" -ForegroundColor Red
}
Write-Host ""
Write-Host "Function URLs:" -ForegroundColor Cyan
foreach ($func in $functions) {
    Write-Host "  https://$PROJECT_REF.supabase.co/functions/v1/$func"
}
