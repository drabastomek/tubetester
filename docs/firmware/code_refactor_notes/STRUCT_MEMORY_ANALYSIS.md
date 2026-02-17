# Struct memory layout analysis (no code changes)

Analysis of all defined structs for padding and alignment. Assumes AVR/typical GCC: `uint8_t` size 1 / alignment 1, `uint16_t` size 2 / alignment 2. No `#pragma pack` or `__attribute__((packed))` is used on these structs.

---

## 1. `vttester_set_params_t` (vttester_remote.h)

**Current order:**

| Field                | Type     | Offset (actual) | Size | Note        |
|----------------------|----------|-----------------|------|-------------|
| voltage_heater_set   | uint8_t  | 0               | 1    |             |
| current_heater_set   | uint8_t  | 1               | 1    |             |
| voltage_g1_set       | uint8_t  | 2               | 1    |             |
| *padding*            | —        | 3               | 1    | for uint16_t|
| voltage_anode_set   | uint16_t | 4               | 2    |             |
| voltage_screen_set  | uint16_t | 6               | 2    |             |
| tuh_ticks            | uint16_t | 8               | 2    |             |
| error_param          | uint8_t  | 10              | 1    |             |
| *padding*            | —        | 11              | 1    | for uint16_t|
| error_value         | uint16_t | 12              | 2    |             |

**Current size: 14 bytes** (2 bytes padding).

**Efficient layout:** Group by size — all `uint8_t` first, then all `uint16_t`:

- 4× uint8_t (voltage_heater_set, current_heater_set, voltage_g1_set, error_param) → bytes 0–3
- 4× uint16_t (voltage_anode_set, voltage_screen_set, tuh_ticks, error_value) → bytes 4–11 (offset 4 is 2‑byte aligned)

**Optimal size: 12 bytes** (0 padding). **Saving: 2 bytes.**

---

## 2. `vttester_parsed_t` (vttester_remote.h)

**Current layout:**

| Field    | Type                    | Offset | Size | Note |
|----------|-------------------------|--------|------|------|
| cmd      | uint8_t                 | 0      | 1    |      |
| index    | uint8_t                 | 1      | 1    |      |
| err_code | uint8_t                 | 2      | 1    |      |
| *padding*| —                       | 3      | 1    | align nested `set` to 2 |
| set      | vttester_set_params_t   | 4      | 14   | current size of set |

**Current size: 18 bytes** (1 byte padding before `set`, plus `set` is 14 bytes).

If `vttester_set_params_t` is reordered to 12 bytes (and still 2‑byte aligned):

- 3 + 1 (pad) + 12 = **16 bytes total** (save 2 bytes).

**Efficient layout for `vttester_parsed_t` itself:** No reorder needed beyond fixing `set`. The only padding is before `set`; we cannot remove that 1 byte without forcing packed layout (or adding another uint8_t field after `err_code` that has semantic use). So the gain for `vttester_parsed_t` comes entirely from shrinking `vttester_set_params_t` to 12 bytes.

**Summary:** **Current 18 → 16 bytes** after reordering `vttester_set_params_t`. **Saving: 2 bytes.**

---

## 3. `katalog` (config.h and test/config.h)

**Current order:**

| Field               | Type        | Offset | Size | Note |
|---------------------|-------------|--------|------|------|
| tube_name           | uint8_t[9]  | 0      | 9    |      |
| voltage_heater_set  | uint8_t     | 9      | 1    |      |
| current_heater_set  | uint8_t     | 10     | 1    |      |
| voltage_g1_set      | uint8_t     | 11     | 1    |      |
| voltage_anode_set   | uint16_t    | 12     | 2    | 12 % 2 == 0 ✓ |
| current_anode_ref   | uint16_t    | 14     | 2    |      |
| voltage_screen_set  | uint16_t    | 16     | 2    |      |
| current_screen_ref  | uint16_t    | 18     | 2    |      |
| slope_ref           | uint16_t    | 20     | 2    |      |
| r_anode_ref         | uint16_t    | 22     | 2    |      |
| k_amplification_ref | uint16_t    | 24     | 2    |      |

**Size: 26 bytes. No padding.** Layout is already efficient (three uint8_t then six uint16_t; offset 12 is naturally 2‑byte aligned).

**No change recommended.**

---

## Summary

| Struct                  | File(s)           | Current size | Optimal size | Padding today | Action        |
|-------------------------|-------------------|-------------|-------------|---------------|---------------|
| vttester_set_params_t   | vttester_remote.h | 14          | 12          | 2 bytes       | Reorder fields|
| vttester_parsed_t       | vttester_remote.h | 18          | 16          | 1 + (2 in set)| Fix set only  |
| katalog                 | config.h, test    | 26          | 26          | 0             | None          |

**Total saving:** 2 bytes per `vttester_set_params_t` and 2 bytes per `vttester_parsed_t`. These structs are used on the stack or in a single instance, so the gain is small but removes unnecessary padding and keeps layouts clear and consistent.

**Recommended reorder for `vttester_set_params_t`** (no semantic change, only order):

1. voltage_heater_set (uint8_t)  
2. current_heater_set (uint8_t)  
3. voltage_g1_set (uint8_t)  
4. error_param (uint8_t)  
5. voltage_anode_set (uint16_t)  
6. voltage_screen_set (uint16_t)  
7. tuh_ticks (uint16_t)  
8. error_value (uint16_t)  

Code that fills or reads the struct must use the same logical fields; only the order in the definition changes.
