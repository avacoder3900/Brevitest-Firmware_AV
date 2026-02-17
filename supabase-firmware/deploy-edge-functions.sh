#!/bin/bash
# Deploy Supabase Edge Functions
# Usage: SUPABASE_ACCESS_TOKEN=sbp_xxxxx ./deploy-edge-functions.sh
#
# To get a Personal Access Token (PAT):
#   1. Go to https://supabase.com/dashboard/account/tokens
#   2. Click "Generate new token"
#   3. Name it (e.g., "CLI Deploy") and copy the token (starts with sbp_)
#   4. Set it as: export SUPABASE_ACCESS_TOKEN=sbp_your_token_here
#
# Project: ncyipufvutzghwgcxukg
# URL: https://ncyipufvutzghwgcxukg.supabase.co

set -e

PROJECT_REF="ncyipufvutzghwgcxukg"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
FUNCTIONS_DIR="$SCRIPT_DIR/backend/functions"

if [ -z "$SUPABASE_ACCESS_TOKEN" ]; then
    echo "ERROR: SUPABASE_ACCESS_TOKEN environment variable is not set."
    echo ""
    echo "To get a Personal Access Token:"
    echo "  1. Go to https://supabase.com/dashboard/account/tokens"
    echo "  2. Click 'Generate new token'"
    echo "  3. Copy the token (starts with sbp_)"
    echo "  4. Run: export SUPABASE_ACCESS_TOKEN=sbp_your_token_here"
    echo ""
    exit 1
fi

FUNCTIONS=("validate-cartridge" "load-assay" "upload-test" "reset-cartridge")

echo "=== Deploying Supabase Edge Functions ==="
echo "Project: $PROJECT_REF"
echo "Functions: ${FUNCTIONS[*]}"
echo ""

for func in "${FUNCTIONS[@]}"; do
    echo "--- Deploying: $func ---"

    if [ ! -f "$FUNCTIONS_DIR/$func/index.ts" ]; then
        echo "ERROR: $FUNCTIONS_DIR/$func/index.ts not found!"
        exit 1
    fi

    # Deploy using Management API
    RESPONSE=$(curl -s -w "\n%{http_code}" \
        --request POST \
        --url "https://api.supabase.com/v1/projects/$PROJECT_REF/functions/deploy?slug=$func" \
        --header "Authorization: Bearer $SUPABASE_ACCESS_TOKEN" \
        --header "content-type: multipart/form-data" \
        --form "metadata={\"entrypoint_path\": \"index.ts\", \"name\": \"$func\", \"verify_jwt\": false}" \
        --form "file=@$FUNCTIONS_DIR/$func/index.ts")

    HTTP_CODE=$(echo "$RESPONSE" | tail -1)
    BODY=$(echo "$RESPONSE" | head -n -1)

    if [ "$HTTP_CODE" -ge 200 ] && [ "$HTTP_CODE" -lt 300 ]; then
        echo "SUCCESS: $func deployed (HTTP $HTTP_CODE)"
    else
        echo "FAILED: $func (HTTP $HTTP_CODE)"
        echo "Response: $BODY"
    fi
    echo ""
done

echo "=== Deployment Complete ==="
echo ""
echo "Function URLs:"
for func in "${FUNCTIONS[@]}"; do
    echo "  https://$PROJECT_REF.supabase.co/functions/v1/$func"
done
