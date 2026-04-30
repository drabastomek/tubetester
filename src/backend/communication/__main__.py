import argparse
import sys

from backend.communication.mcu_serial import VTTesterSerial
from backend.communication.protocol import ResetKind

def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(description="VTTester communication package")
    p.add_argument("port", help="Serial device (e.g. /dev/cu.SLAB_USBtoUART or COM3)")
    p.add_argument("-b", "--baud", type=int, default=9600, help="Baud rate")
    p.add_argument("--a1", type=str, default="0", help="A1/A2 selector (byte 1)")
    p.add_argument("--ug", type=int, default=100, help="UG setpoint (byte 2, 0–255)")
    p.add_argument("--uh", type=int, default=63, help="UH setpoint (byte 3)")
    p.add_argument("--ih", type=int, default=0, help="IH setpoint (byte 4)")
    p.add_argument("--ua", type=int, default=300, help="Ua uint16 (bytes 5–6 LE)")
    p.add_argument("--ue", type=int, default=0, help="Ue uint16 (bytes 7–8 LE)")
    args = p.parse_args(argv)

    try:
        with VTTesterSerial(args.port, baudrate=args.baud) as link:
            link.drain()
            print('SENDING SET REQUEST...')
            
            ack = link.send_set(a1_a2=args.a1, ug=args.ug, uh=args.uh, ih=args.ih, ua=args.ua, ue=args.ue)
            print(ack)

            link.drain()
            
            print('\nSENDING STATUS REQUEST...')
            data = link.get_status()
            print(data)

            link.drain()

            print('\nSENDING RESET REQUEST...')
            reset = link.send_reset(reset_kind=ResetKind.RESET_FULL)
            print(reset)

    except serial.SerialException as e:
        print(e, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
