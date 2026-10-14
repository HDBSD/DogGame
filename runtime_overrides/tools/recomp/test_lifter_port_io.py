"""IN must route port reads with the accumulator width and preserve EFLAGS."""
import unittest

from .disasm import Instruction, Operand
from .lifter import Lifter


class PortReadTests(unittest.TestCase):
    def lift(self, accumulator, port):
        instruction = Instruction(0x1095AD, 1, "in", "", "ec")
        instruction.operands = [Operand(type="reg", reg=accumulator), port]
        return "\n".join(Lifter().lift_instruction(instruction))

    def test_byte_word_and_dword_bus_access(self):
        for accumulator, width in (("al", 1), ("ax", 2), ("eax", 4)):
            with self.subTest(accumulator=accumulator):
                lifted = self.lift(accumulator, Operand(type="reg", reg="dx"))
                self.assertIn(f"xbox_ReadIoPort(LO16(edx), {width})", lifted)
                self.assertNotIn("UNIMPL", lifted)

    def test_immediate_port_and_partial_register_write(self):
        lifted = self.lift("al", Operand(type="imm", imm=0x80))
        self.assertIn("SET_LO8(eax,", lifted)
        self.assertIn("xbox_ReadIoPort(0x80, 1)", lifted)

    def test_port_read_does_not_modify_flags(self):
        lifted = self.lift("al", Operand(type="reg", reg="dx"))
        for flags in ("_cf", "_fa", "_fb", "_flags"):
            self.assertNotIn(flags, lifted)


if __name__ == "__main__":
    unittest.main()
