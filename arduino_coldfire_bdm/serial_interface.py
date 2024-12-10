import os
import serial
import struct
import time

PATH_TO_ARDUINO_SKETCH = os.path.join(os.path.dirname(__file__), "arduino-coldfire-bdm.ino")

DEBUG_MESSAGE = (
    "Ensure"
    " the Arduino is properly connected on the correct serial port, or try"
    f" re-compiling the Arduino sketch (from '{PATH_TO_ARDUINO_SKETCH}') and"
    " re-uploading with the Arduino IDE."
)


class Response:
    """
    A Coldfire debug response packet, with a status bit and 16-bit data word.
    This is pretty much just a plain old data container, but it also checks
    for invalid responses from the Coldfire.
    """

    def __init__(self, status: int, data: int):
        self.status = status
        self.data = data

        #print("%x"%data)

        if self.status == 1:
            if self.data == 0x0001:
                raise ValueError("Coldfire responded to last-sent command with Error response")
            elif self.data == 0xFFFF:
                raise ValueError("Coldfire responded to last-sent command with Illegal Command")


class ColdfireSerialInterface:
    def __init__(self, serial: serial.Serial):
        """
        Wrap a pySerial object with methods to communicate with a Coldfire BDM port,
        via an Pico serial bridge to provide clocking.

        This class is only useful for communicating with an attached Pico.
        """
        self.serial = serial


        #first_line = self.serial.readline().decode("utf-8").strip()
        #if not "Motorola Coldfire Debug Interface" in first_line:
        #    raise RuntimeError(
        #        "Tried to connect to Arduino, but got unexpected first line:"
        #        f" {first_line}\n{DEBUG_MESSAGE}"
        #    )
        #second_line = self.serial.readline().decode("utf-8").strip()
        #if not "Ready" in second_line:
        #    raise RuntimeError(
        #        "Tried to connect to Arduino, but got unexpected second line:"
        #        f" {second_line}\n{DEBUG_MESSAGE}"
        #    )

        #Wait and flush the input (uses timeout)
        time.sleep(0.15)
        self.serial.reset_input_buffer()

        resp = ""

        while resp != b'N':
            self.serial.write(b"?")
            resp = self.serial.read(1)

        time.sleep(0.1)
        self.serial.reset_input_buffer()
        
        self.test_connection()
        self.test_connection()
        self.test_connection()

    def test_connection(self):
        self.serial.write(b"P")
        response = self.serial.read(4)
        if response != b"PONG":
            raise RuntimeError(
                "Sent ping to Arduino and expected to receive PONG, but got"
                f" {repr(response)}.\n{DEBUG_MESSAGE}"
            )
        self.check_ack()
        
    def check_ack(self):
        """ Checks for 'A' character after a command (ack) """

        for i in range(0, 10):
            data = self.serial.read(1)

            if data == b"A":
                return
            
            if len(data) > 0:
                break
        
        raise IOError("ACK problem, expected 'A', got %d bytes: %s"%(len(data), str(data)))


    def send_packet(self, data: int):
        """
        Send a 16-bit data packet to the attached Coldfire BDM via an Arduino bridge.
        The response will not be read automatically; use `receive_packet` to get the subsequent response.
        """
        self.serial.write(b"s" + struct.pack(">H", data))
        self.check_ack()

    def receive_packet(self) -> Response:
        """
        Receive a 17-bit data packet from the attached Coldfire BDM via an Arduino bridge.
        This implicitly sends a packet full of zeros (a "noop") to the Coldfire, necessary
        to receive a packet in response. This is slower than needed, though; use
        send_and_receive_packet to send a packet while receiving the response from the
        last packet.
        """
        self.serial.write(b"r")
        data = self._receive_packet()
        self.check_ack()
        return data

    def _receive_packet(self) -> Response:
        response = self.serial.read(3)
        # The Arduino serial bridge translates the 17-bit packet into
        # 24 bits for us: 8 bits for the status bit, and then the original
        # 16 bits of the data word.
        if response[0] != ord('N') and response[0] != ord('Y'):
            raise IOError("response[0]: " + str(response[0]))

        status = int(response[0] == ord('Y'))
        data = struct.unpack("<H", response[1:])[0]
        return Response(status, data)

    def send_and_receive_packet(self, data: int) -> Response:
        """
        Send a 16-bit data packet to the attached Coldfire BDM, while also receiving
        the 17-bit data packet that consitutes a response from the last command. This
        can be used to speed up sequences of commands where the response from the last
        command isn't required before sending the next command.
        """
        self.serial.write(b"S" + struct.pack(">H", data))
        data = self._receive_packet()
        self.check_ack()
        return data

    def enter_debug_mode(self, reset=False):
        """
        Pull the Coldfire into BDM (Background Debug Mode) by asserting its BDM pin.
        If `reset` is True, the processor will also be reset during this operation,
        which may reset the program counter and status register.
        """
        if reset:
            # R for Reset
            self.serial.write(b"R")
        else:
            # B for Breakpoint
            self.serial.write(b"B")
        self.check_ack()


class MockColdfireSerialInterface:
    def __init__(self):
        """
        Provide a fake serial interface to use for dry runs and testing.
        """
        self.commands_sent = []

    def test_connection(self):
        self.commands_sent.append(b"P")

    def send_packet(self, data: int):
        """
        Send a 16-bit data packet to the attached Coldfire BDM via an Arduino bridge.
        The response will not be read automatically; use `receive_packet` to get the subsequent response.
        """
        self.commands_sent.append(b"s" + struct.pack(">H", data))

    def receive_packet(self) -> Response:
        self.commands_sent.append(b"r")
        return self._receive_packet()

    def _receive_packet(self) -> Response:
        return Response(0, 0xFFFF)

    def send_and_receive_packet(self, data: int) -> Response:
        self.commands_sent.append(b"S" + struct.pack(">H", data))
        return self._receive_packet()

    def enter_debug_mode(self, reset=False):
        if reset:
            # R for Reset
            self.commands_sent.append(b"R")
        else:
            # B for Breakpoint
            self.commands_sent.append(b"B")
