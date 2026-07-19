#!/bin/bash

echo "=== Testing X11 Forwarding from Windows/PowerShell ==="

# Try different DISPLAY values
for display in localhost:10.0 localhost:11.0 localhost:12.0 :0.0; do
    echo "Testing DISPLAY=$display"
    export DISPLAY=$display
    echo "Current DISPLAY: $DISPLAY"
    
    # Test basic X11 connectivity
    timeout 3 xset q 2>/dev/null && echo "✓ X11 connection successful with $display" && break
    echo "✗ X11 connection failed with $display"
done

echo ""
echo "Final DISPLAY value: $DISPLAY"

if [ -z "$DISPLAY" ]; then
    echo ""
    echo "❌ X11 forwarding not working from Windows PowerShell"
    echo ""
    echo "SOLUTIONS for Windows users:"
    echo "1. Install X11 server on Windows (VcXsrv, Xming)"
    echo "2. Use Windows Terminal with WSL2"
    echo "3. Use VNC instead of X11 forwarding"
    echo ""
    exit 1
else
    echo ""
    echo "✅ X11 forwarding appears to be working!"
    echo "Proceeding with SfS visualization test..."
fi