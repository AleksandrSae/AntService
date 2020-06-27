import hrm

class Callable:
    def __init__(self):
        self.tries = 5

    def __call__(self, json):
        print(json)
        self.tries -= 1

        if self.tries <= 0:
            return False # Stop
        return True # Get next value

call_back = Callable()

hrm.attach('/dev/ttyUSB0')

status = hrm.init()
print(f"Initialisation status {status}")

if not status:
    exit(1)

hrm.set_callback(call_back)
