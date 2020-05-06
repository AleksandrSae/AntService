import hrm


class Callable:

    def __init__(self):
        self.tries = 100.0

    def __call__(self, device_number, p0, p1, p2, p3, p4, p5, p6, p7):
        print(f"Device: {device_number}, Payload: {p0:#04x} {p1:#04x} {p2:#04x} {p3:#04x} {p4:#04x} {p5:#04x} {p6:#04x} {p7:#04x}")
        self.tries = self.tries - 1.0
        return self.tries

call_back = Callable()

hrm.attach('/dev/ttyUSB0')
hrm.init()
hrm.hrm_callback(call_back)
