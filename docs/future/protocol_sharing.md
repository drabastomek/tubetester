# Protocol sharing: single source of truth (future)

**Status:** Parked for future implementation.

## Goal

Define the VTTester binary protocol (see `docs/protocol/VTTester_Protocol_v0.3.txt`) in a **single machine-readable file** that both:

- **Python** (desktop protocol implementation) can use **at runtime** (load JSON, build/parse frames).
- **Firmware** (C) can use via a **pre-compilation step** that turns the spec into a C header (or generated source) so constants and layouts are never hand-duplicated.

When the protocol changes, we update one file; Python and firmware stay in sync without editing two codebases.

## Format: JSON

**JSON** is a good fit:

- Python can read it at runtime with the standard library (`json`).
- No parser needed on the MCU: a small **codegen script** (e.g. Python) runs before the firmware build and emits a C header (e.g. `protocol_spec.h`) with `#define`s, frame layout, and any lookup tables. The firmware only compiles the generated output.
- Widely understood and tool-friendly. Comments are not allowed in strict JSON; use a separate `_comment` / `description` fields in the spec, or a JSON-with-comments variant (e.g. JSON5) if the codegen supports it.

## What the spec would contain

A single JSON file (e.g. `docs/protocol/protocol_spec.json` or under `tools/`) would describe at least:

- **Metadata:** protocol version, description.
- **Frame:** length (8), byte roles (CTRL, INDEX, P1–P5, CRC), CRC parameters (polynomial 0x07, init, bytes covered).
- **Request CTRL:** command codes (SET, STATUS, RESET) and values (0x00, 0x40, 0x80); reserved bits.
- **Response CTRL:** status codes (ACK, BUSY, ERROR, ALARM), DATA bit (bit 5), STATE bits; example values (e.g. 0x20 for DATA frame).
- **INDEX:** TUBE_TYPE (bits 7–4), SYSTEM (bits 3–0).
- **SET:** P1–P5 semantics (references or short descriptions; full encoding may stay in the human spec).
- **RESET:** P1 values (0x01 = Ua/Ug2/Ug1 only, 0x02 = full including Uh).
- **DATA frames:** four frames, field layout per frame (byte offsets, names), scales for Uh, Ih, Ua, Ia, Ug2, Ig2, Ug1; alarm byte (bits 7–4).
- **Error codes:** R1 values (OUT_OF_RANGE, CRC_ERROR, INVALID_CMD, HARDWARE).
- **Alarm bits:** OVERTE, OVERIG, OVERIA, OVERIH (e.g. bit positions or mask).

Exact keys and structure can be designed when we implement.

## Workflow

1. **Authoritative spec:** The JSON file is the machine-readable definition of the protocol (optionally kept in sync with the human-readable `VTTester_Protocol_v0.3.txt`).
2. **Python:** Loads the JSON at startup (or from a path). Uses it to build request frames, parse response/DATA frames, validate, and map error/alarm codes. No codegen required for Python if we’re happy to interpret the spec at runtime.
3. **Firmware:** As part of the build (e.g. `make` or a CMake step), run a small script that reads the same JSON and emits C (e.g. `protocol_spec.h` with `#define CMD_SET 0x00`, `#define DATA_BIT 5`, frame offsets, CRC table if desired). Firmware `#include "protocol_spec.h"` and does not parse JSON on the device.
4. **Changes:** Edit the JSON (and the human doc if desired); re-run the firmware codegen and rebuild. Python picks up the new JSON on next run. One change, two consumers.

## Why parked

- Current protocol (v0.3) is stable and documented; duplication between Python and firmware is manageable for now.
- Implementing the JSON schema and codegen is non-trivial (design, testing, CI). This doc records the intent so we can adopt the approach later without re-inventing the idea.

## References

- Protocol definition: `docs/protocol/VTTester_Protocol_v0.3.txt`
- Diff v0.2→v0.3: `docs/protocol/VTTester_Protocol_v0.2_to_v0.3_diff.txt`
