"""
Pytest configuration and shared fixtures for backend tests.
Ensures 'backend' is importable when running from repo root or src.
"""
import sys
from pathlib import Path

# Add src to path so "from backend.xxx" works when running pytest from repo root
_src = Path(__file__).resolve().parent.parent
if str(_src) not in sys.path:
    sys.path.insert(0, str(_src))
