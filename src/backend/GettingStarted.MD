# Getting started (VTTester serial example)

You will install **Miniconda**, create an isolated environment, install Python packages, and run **`python -m backend.communication`**, which talks to the firmware over USB serial (VTTester v0.4.1-style frames).

## 1. Install Miniconda

1. Download **Miniconda** for your OS:  
   https://docs.conda.io/en/latest/miniconda.html  
2. Run the installer. On Windows, use **Miniconda Prompt** if `conda` is not on your PATH.

## 2. Open a terminal in the project folder

Use **Terminal** (Mac) or **Miniconda Prompt** / PowerShell (Windows).

Go to the **root** of the **tubetester** repo (folder that contains `src/`):

```bash
cd path/to/tubetester
```

You need this so `PYTHONPATH=src` points at the right `src` folder. (If you use an absolute `PYTHONPATH`, you can run from another directory.)

## 3. Create and activate a conda environment

```bash
conda create -n tubetester python=3.12 -y
conda activate tubetester
```

Each new terminal session:

```bash
conda activate tubetester
```

## 4. Install Python packages

**Always:**

```bash
pip install pyserial
```

**Also**, if you will use **`-f` / `--file`** to load a CSV of tubes:

```bash
pip install pandas numpy
```

## 5. Set `PYTHONPATH`

The code lives under `src/` as package **`backend`**.

**Mac / Linux:**

```bash
export PYTHONPATH=src
```

**Windows CMD:**

```cmd
set PYTHONPATH=src
```

**Windows PowerShell:**

```powershell
$env:PYTHONPATH = "src"
```

## 6. Find the serial port

- **Mac:** e.g. `/dev/cu.usbserial-*` or `/dev/cu.SLAB_USBtoUART` (`ls /dev/cu.*` before/after plugging in).
- **Windows:** **Device Manager** → **Ports (COM & LPT)** → e.g. **COM3**.

## 7. Run the demo

**First argument is always the serial port.** Examples:

```bash
python -m backend.communication /dev/cu.SLAB_USBtoUART
```

```bash
python -m backend.communication COM3
```

Default baud is **9600**:

```bash
python -m backend.communication COM3 -b 9600
```

### Two ways to supply tube / step data

**A) No file (synthetic row)** — the script builds **one** row from flags (see table below). **No pandas/numpy** required.

**B) CSV file** — pass **`-f` / `--file`** with path to a CSV. Required columns match the old `tubes.csv` layout: `tube_type`, `system`, `heater_type`, `heater_value`, `anode_voltage`, `screen_voltage`, `grid_voltage`, `heating_time`. Paths are relative to your **current working directory** unless you use an absolute path. Example (from repo root):

```bash
python -m backend.communication COM3 -f src/backend/temp/tubes.csv
```

### Useful CLI flags (synthetic row)

| Flag | Long | Default | Meaning |
|------|------|---------|---------|
| `-b` | `--baud` | 9600 | Serial baud rate |
| `-ua` | `--anode` | 300 | Anode voltage (volts) → SET Ua 12-bit |
| `-ug1` | `--grid1` | 15.0 | Grid1 voltage (volts) → scaled to SET P5 (0.1 V steps) |
| `-ug2` | `--grid2` | 300 | Screen / grid2 voltage (volts) → SET Ug2 |
| `-uh` | `--heater_value` | 63 | Heater P1 (0.1 V steps, parallel heater) |
| `-th` | `--heating_time` | 20 | P6 time index (see `--help` text on your machine) |
| `-tt` | `--tube_type` | 99999 | Tube type field in INDEX; **99999** = “tube change” → **RESET** only for that row |
| `-st` | `--system` | 1 | System (INDEX) |
| `-ht` | `--heater_type` | 0 | Heater wiring: 0 parallel, 1 series (INDEX) |
| `-f` | `--file` | *(none)* | CSV path; if set, flags above are ignored for row data |

**Default `tube_type` (99999):** with a single synthetic row, the script treats it as a **tube change** and sends **RESET** for that step (no SET/measurement for that row). For a quick **SET + measurement**, use e.g. **`-tt 1`** (and matching `-st` / `-ht` as needed).

At the end of the run the script sends a **final RESET** to shut down the meter side.

## Troubleshooting

| Problem | What to try |
|--------|-------------|
| `conda: command not found` | Open **Miniconda Prompt** or fix PATH. |
| `ModuleNotFoundError: backend` | Set `PYTHONPATH=src` and run from repo root (or set `PYTHONPATH` to the full path to `tubetester/src`). |
| `No module named 'pandas'` | Use **`-f`** only after `pip install pandas numpy`, or run without **`-f`** (synthetic row). |
| CSV not found | Path is relative to **current directory**; use repo root + `src/backend/temp/tubes.csv` or an absolute path. |
| Serial open fails (Windows) | Use **COMn** from Device Manager; high ports: try **`\\.\COM12`**. |
| Permission errors (Mac) | Check cable/driver; user must access `/dev/cu.*`. |
