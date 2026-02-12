# Zmiany w firmware i struktura

Skrot refaktoryzacji i obecnego ukladu. Przeplyw petli glownej: [MAIN_LOOP_WALKTHROUGH.md](MAIN_LOOP_WALKTHROUGH.md).

---

## 1. Podsumowanie zmian

### Protokol i petla glowna

- **Protokol komunikacji po RS:** Obsluga protokolu VTTester po laczu szeregowym (UART) zostala dodana i wyniesiona do modulu `protocol/`. Urzadzenie odbiera 8-bajtowe ramki (SET parametrow, zlecenie MEAS itd.), parsuje je z CRC i limitami, stosuje SET w trybie zdalnym (typ==1) i wysyla ramki ACK lub wyniku pomiaru. Szczegoly parsera i ramek w sekcji 3.
- **Jednolita sciezka odpowiedzi:** Petla glowna wysyla jeden `parsed.err_code` (oraz przy SET opcjonalnie param/wartosc bledu) dla kazdej sparsowanej ramki; limity SET sa w `config/config.h`.
- **Ug1:** Przechowywane jako 0..240 w jednostkach 0,1 V (240 = -24 V); parser w tej samej konwencji co legacy (`ug1def = P4*5`).

### Podzial display vs control

- **Display:** Caly wyswietlacz LCD (inicjacja sprzetu, CGRAM, splash i odswiezanie ekranu 4x20 z `buf[]`) jest w `display/`. Petla glowna wywoluje tylko raz `display_init()` i przy kazdym takcie `display_refresh()`.
- **Control:** Menu, katalog, edycja parametrow, obliczenia ADC->wartosc oraz aktualizacje EEPROM/setpointow sa w `control/`. Petla glowna wywoluje `control_update(ascii)` przy kazdym takcie; control wypelnia `buf[]` i aktualizuje globalne (setpointy, `lamptem` itd.).

### Utils i testy

- **int2asc:** Przeniesione do `utils` do wspoldzielenia; wynik to 4 cyfry dziesietne w `ascii[0..3]`.
- **Testy na hoście:** Zestaw w `test/` buduje sie przez `gcc` (bez AVR), testuje protokol, utils, budownicze wierszy display i control. Uruchomienie: `make test` z katalogu firmware.

### EEPROM i config

- **Makra EEPROM:** `EEPROM_READ` / `EEPROM_WRITE` sa zdefiniowane w `config/config.h` (z uzyciem avr-libc), zeby main i control mogly z nich korzystac bez duplikowania definicji.

---

## 2. Struktura katalogow i plikow

```
firmware/
├── TTesterLCD32.c          # Main: init, ISR-y, petla glowna (dispatch protokolu, display_refresh, control_update, safety)
├── Makefile                 # Budowanie AVR; `make test` uruchamia testy na hoście
├── config/
│   ├── config.h             # Konfiguracja sprzetu/aplikacji: makra portow, timing, limity protokolu, makra EEPROM, typ katalog
│   └── config.c             # Katalog lamp (lamprom), dane EEPROM (lampeep, poptyp), tabela AZ, CGRAM cyr*
├── display/
│   ├── display.h            # display_init(), display_refresh(), prymitywy LCD (cmd2lcd, gotoxy, char2lcd, cstr2lcd, str2lcd)
│   └── display.c            # Dostep do portu LCD, ladowanie CGRAM, splash, odswiezanie z buf[] (wiersze 0-3 w zaleznosci od typ)
├── control/
│   ├── control.h            # control_update(unsigned char *ascii)
│   └── control.c            # Numer/nazwa lampy, edycja nazwy, Ug1/Uh/Ih/Ua/Ia/Ug2/Ig2/S/R/K: obliczenia, wypelnianie buf[], enkoder/EEPROM/setpointy; TX do PC
├── protocol/
│   ├── vttester_remote.h     # Dlugosc ramki, kody cmd/bledu, typy parsed, parse_message, send_response, send_measurement
│   └── vttester_remote.c     # CRC-8, parsowanie 8-bajtowej ramki (SET/MEAS/NONE), budowanie ramek ACK i MEAS
├── utils/
│   ├── utils.h               # char2rs, cstr2rs, delay, zersrk, liczug1, int2asc
│   └── utils.c               # Pomocnicze UART/sprzet; przy VTTESTER_HOST_TEST stuby dla testow, wspolna int2asc
├── test/                     # Testy jednostkowe na hoście (bez AVR)
│   ├── config/config.h       # Minimalny config (tylko VTTESTER_SET_*) dla testow protokolu
│   ├── test_common.h         # TEST_ASSERT, liczniki
│   ├── test_control.c        # Testy control
│   ├── test_display.c        # Testy display
│   ├── test_protocol.c       # Parse (CRC, invalid cmd, MEAS, SET OK/OOB P1-P5, Ug1), send_response, send_measurement
│   ├── test_utils.c          # int2asc (0, 7, 42, 123, 9999, 300, 240, 255)
│   ├── main.c                # Uruchamiacz testow; exit 0/1
│   ├── Makefile              # Budowanie test_runner, `make run` uruchamia
│   └── README.md             # Jak uruchamiac i rozszerzac testy
└── docs/
    ├── CHANGES_AND_STRUCTURE.md   # Wersja angielska
    ├── ZMIANY_STRUKTURA.md        # Ten plik (PL)
    ├── MAIN_LOOP_WALKTHROUGH.md   # Petla glowna krok po kroku
    └── AVR_SIZE_EXPLAINED.md      # Uwagi o rozmiarze flash/RAM
```

---

## 3. Rola kazdego modulu

### TTesterLCD32.c (main)

- **Przechowuje:** Stan globalny (buf, adr, typ, setpointy, lamptem, lamprem, stan ADC/pomiarow, bufory RX/TX protokolu itd.).
- **Robi:** Inicjacja portow/timerow/UART/watchdog; ISR-y (enkoder, timer, UART RX/TX, ADC); petla glowna: przy takcie sync obsluga protokolu (parse, zastosowanie SET gdy typ==1, wyslanie ACK/MEAS), wywolanie `display_refresh()`, wlaczenie INT1, sprawdzenie safety, potem `control_update(ascii)`.
- **Nie robi:** Nie zna szczegolow LCD (display) ani sposobu wypelniania buf[] (control).

### config/

- **config.h:** Jedno miejsce na stale sprzetu i aplikacji: makra portow/bitow, timing (MS1, DMAX, ...), definicje ADC/alarmow, rozmiary katalogu (FLAMP, ELAMP), **limity SET protokolu VTTester** (VTTESTER_SET_UA_MAX, VTTESTER_SET_UG1_P4_STEP, ...), EEPROM_READ/EEPROM_WRITE oraz typ `katalog`.
- **config.c:** Katalog lamp w ROM (`lamprom` w PROGMEM), katalog z EEPROM (`lampeep`), `load_lamprom()`, `poptyp`, tabela AZ oraz dane CGRAM cyr* dla LCD.

### display/

- **display.c:** Niski poziom LCD: `cmd2lcd`, `gotoxy`, `char2lcd`, `cstr2lcd`, `str2lcd`; `display_init()` (inicjacja LCD, CGRAM, splash); `display_refresh()` (rysowanie wierszy 0-3 z `buf[]` maina, przy czym wiersz 3 zalezy od typ: lokalne S/R/K lub T=, albo puste dla zasilacza / zdalne "TX/RX: 9600,8,N,1").
- **display.h:** Deklaruje powyzsze. Display uzywa globalnych maina (buf, adr, typ, start, err, dusk0, takt) przez extern; nie jest wlascicielem stanu.

### control/

- **control.c:** Jedna funkcja `control_update(ascii)`. Uzywa globalnych maina (buf, adr, typ, setpointy, lamptem, lamprem, wartosci ADC itd.), zeby: odswiezyc numer/nazwe lampy (adr==0), obsluzyc edycje nazwy (typ>=FLAMP), a nastepnie dla kazdego parametru (Ug1, Uh, Ih, Ua, Ia, Ug2, Ig2, S, R, K) obliczyc wartosc z ADC lub pomiaru, zapisac ASCII do buf[] oraz przy potwierdzeniu enkoderem zapisac setpointy lub EEPROM. Wysyla tez do PC gdy ustawione `txen` (10-bajtowy legacy lub zrzut 63 znakow). Uzywa `int2asc(..., ascii)` z utils.
- **control.h:** Deklaruje `control_update(unsigned char *ascii)`.

### protocol/

- **vttester_remote.c:** CRC-8 (tabela w PROGMEM na AVR), `vttester_parse_message()` (weryfikacja CRC i bajtu cmd, dekodowanie SET z limitami z config, ustawianie `err_code` i pol SET/error_param/error_value), `vttester_send_response()` (ACK z opcjonalnym OUT_OF_RANGE param/value), `vttester_send_measurement()` (ramka wyniku MEAS). Uzywa `config/config.h` tylko dla VTTESTER_SET_*.
- **vttester_remote.h:** Dlugosc ramki, kody polecen i bledow, `vttester_parsed_t` / `vttester_set_params_t` oraz trzy funkcje API.

### utils/

- **utils.c:** Wysylanie UART (`char2rs`, `cstr2rs`), `delay`, `zersrk`, `liczug1` (Ug1 0..240 -> setpoint sprzetowy), `int2asc` (liczba -> 4 cyfry ASCII). Przy budowaniu z `-DVTTESTER_HOST_TEST` kod AVR jest pomijany i kompilowane sa tylko stuby + `int2asc` dla testow na hoście.
- **utils.h:** Deklaruje powyzsze.

### test/

- **Cel:** Tylko testy na hoście; bez toolchaina AVR. Buduje protokol (z configem testowym) i utils (ze stubami), uruchamia wszystkie przypadki testowe, konczy z 0 gdy wszystko OK, w przeciwnym razie 1.
- **test/config/config.h:** Definiuje tylko VTTESTER_SET_*, zeby protokol kompilowal sie bez naglowkow AVR.
- **test_protocol.c:** Parse (CRC, invalid cmd, MEAS, SET poprawne, SET P1-P5 out-of-range, zakres Ug1), send_response (OK i OUT_OF_RANGE), layout send_measurement i CRC.
- **test_utils.c:** int2asc dla 0, 7, 42, 123, 9999, 300, 240, 255.
- **test_display.c:** display_build_row0..3 (zawartosc wierszy LCD).
- **test_control.c:** control_update (numer/nazwa lampy, Ug1 z ADC, ladowanie lamprom).
- **main.c:** Wywoluje `run_protocol_tests()`, `run_utils_tests()`, `run_display_tests()`, `run_control_tests()`, wypisuje wynik, zwraca 0/1.

---

## 4. Przeplyw danych (uproszczony)

1. **UART RX** (ISR) -> 8 bajtow `rx_proto_buf` -> `rx_proto_ready = 1`.
2. **Petla glowna (sync):** Jesli `rx_proto_ready`, parsuj przez `vttester_parse_message(rx_proto_buf, &parsed)`. Przy SET i typ==1 i braku bledu zastosuj sparsowane parametry do setpointow/lamptem; nastepnie wyslij ACK z `parsed.err_code` (+ error_param/error_value dla SET). Przy MEAS wyslij OK i ustaw trigger pomiaru. Przy NONE wyslij `parsed.err_code`.
3. **Display:** `display_refresh()` czyta `buf[]` maina oraz typ/adr/start/err i rysuje LCD 4x20 (bez modyfikacji buf).
4. **Control:** `control_update(ascii)` oblicza wartosci z ADC/stanu, wypelnia `buf[]`, a przy potwierdzeniu enkoderem aktualizuje setpointy lub EEPROM; obsluguje tez TX do PC gdy ustawione `txen`.
5. **Pomiar:** Gdy pomiar sie konczy, main (lub logika timera) wypelnia wynik i wywoluje `vttester_send_measurement()`; ramka 8-bajtowa jest wysylana przez UART.

Czyli: **protokol** parsuje ramki wejsciowe i buduje ACK/MEAS; **display** tylko rysuje z buf; **control** jest wlascicielem zawartosci buf oraz setpointow/EEPROM.

---

## 5. Budowanie i testy

- **Firmware (AVR):** `make` w `firmware/` -> `TTesterLCD32.hex`. `make clean`, `make size` jak zwykle.
- **Testy na hoście:** `make test` w `firmware/` (lub `make run` w `firmware/test/`) -> kompilacja i uruchomienie test_runner; "OK: N tests" i exit 0 oznacza, ze wszystko przeszlo.

Po kazdej zmianie w protokole lub utils uruchom `make test`, zeby potwierdzic zachowanie.
