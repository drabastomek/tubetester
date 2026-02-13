# VTTester backend (Warstwa 1: driver szeregowy + protokół)

Implementacja w Pythonie z użyciem **pyserial**.

## Zależności

```bash
pip install -r ../requirements.txt
```

(`pyserial`)

## Struktura

- **protocol.py** — klasa **Protocol**: ramki 8-bajtowe, CRC-8, kodowanie/dekodowanie (SET, MEAS, ACK, wynik pomiaru). Można wstrzyknąć własną instancję do `SerialDriver` i `VTTesterClient` (np. w testach).
- **serial_driver.py** — **SerialDriver**: otwieranie/zamykanie portu COM, handshake (weryfikacja, że po drugiej stronie jest miernik), wysłanie jednej komendy, odbiór odpowiedzi z timeoutem.
- **api.py** — **VTTesterClient**: API dla innych modułów i sygnały w górę.

## API (api.VTTesterClient)

- **open(port)**, **close()**, **handshake()** — port i prosty handshake.
- **set_params(heat_idx, ua_v, ug2_v, ug1_def, tuh_ticks)** — komenda SET.
- **start_measurement()** — komenda MEAS; zwraca wynik pomiaru (jedna ramka).
- **read_status()** — placeholder (jeszcze nie w protokole).
- **reset_params()** — placeholder (jeszcze nie w protokole).

Kolejkowanie: jedna komenda na raz, oczekiwanie na odpowiedź (z timeoutem), potem następna.

## Zdarzenia (callbacki)

Przypisz z zewnątrz:

- **on_measurement_result(point)** — wynik pomiaru (słownik z ihlcd, ialcd, ig2lcd, slcd, alarm_bits).
- **on_status_update(status)** — aktualizacja statusu (np. po ACK SET).
- **on_alarm(alarm_info)** — alarm (słownik z alarm_bits, over_ih, over_ia, over_ig, over_te, measurement).

## Użycie

```python
from backend.api import VTTesterClient

client = VTTesterClient(port="/dev/ttyUSB0", timeout_s=2.0)
client.on_measurement_result = lambda p: print("Measurement:", p)
client.on_alarm = lambda a: print("Alarm:", a)

client.open()
if not client.handshake():
    print("Handshake failed")
else:
    client.set_params(heat_idx=1, ua_v=250, ug2_v=250, ug1_def=120, tuh_ticks=10)
    result = client.start_measurement()
    print(result)
client.close()
```

Protokół zgodny z firmware (vttester_remote.c, ramka 8 bajtów, CRC-8 na bajtach 0..6).
