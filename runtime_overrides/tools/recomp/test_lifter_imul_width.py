"""One-operand IMUL writes registers at the instruction's operand width."""
import unittest
from .disasm import Instruction, Operand
from .lifter import Lifter


class ImulWidthTests(unittest.TestCase):
    def lift(self, register):
        insn = Instruction(0x71FDB, 2, 'imul', register, '')
        insn.operands = [Operand(type='reg', reg=register)]
        return '\n'.join(Lifter().lift_instruction(insn))

    def test_byte_preserves_edx_and_upper_eax(self):
        result = self.lift('dl')
        self.assertIn('(int8_t)LO8(eax)', result)
        self.assertIn('(int8_t)LO8(edx)', result)
        self.assertIn('SET_LO16(eax,', result)
        self.assertNotIn('edx =', result)
        self.assertNotIn('SET_LO16(edx,', result)

    def test_word_preserves_upper_halves(self):
        result = self.lift('dx')
        self.assertIn('(int16_t)LO16(eax)', result)
        self.assertIn('SET_LO16(eax,', result)
        self.assertIn('SET_LO16(edx,', result)

    def test_dword_keeps_full_product(self):
        result = self.lift('edx')
        self.assertIn('int64_t _r', result)
        self.assertIn('edx = (uint32_t)(_r >> 32)', result)


if __name__ == '__main__':
    unittest.main()
