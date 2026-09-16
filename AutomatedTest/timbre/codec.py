"""
Layer 0.5: the text CRC.

Kept deliberately small and swappable. The only thing that matters is that
`Crc16Codec.compute` produces byte-identical output to the firmware's
implementation. Prove that before trusting anything above it: see
tests/test_link.py::test_crc_agrees_with_target.

A CRC mismatch is never silently tolerated. Corruption that gets treated as
a test result is worse than a dropped link, because it looks like a bug in
the firmware under test.
"""

from __future__ import annotations

import re
from abc import ABC, abstractmethod
from dataclasses import dataclass


@dataclass
class Decoded:
    payload: str
    crc_present: bool
    crc_ok: bool


class Codec(ABC):
    @abstractmethod
    def encode(self, line: str) -> str: ...

    @abstractmethod
    def decode(self, line: str) -> Decoded: ...


class PlainCodec(Codec):
    """No CRC. Use while bringing the harness up, not in CI."""

    def encode(self, line: str) -> str:
        return line

    def decode(self, line: str) -> Decoded:
        return Decoded(payload=line, crc_present=False, crc_ok=True)


def crc16_ccitt(data: bytes, init: int = 0xFFFF, poly: int = 0x1021) -> int:
    """CRC-16/CCITT-FALSE: init 0xFFFF, no input/output reflection, no final XOR.

    TODO: confirm this matches the firmware. If TimbreOS uses a reflected
    variant, a different init, or a final XOR, change it here only.
    """
    crc = init
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ poly) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


class Crc16Codec(Codec):
    def __init__(self, separator: str = "*", digits: int = 4):
        self.separator = separator
        self.digits = digits
        self._pattern = re.compile(
            rf"^(?P<payload>.*?)\s*{re.escape(separator)}\s*"
            rf"(?P<crc>[0-9A-Fa-f]{{{digits}}})\s*$"
        )

    def compute(self, payload: str) -> str:
        value = crc16_ccitt(payload.encode("ascii", "replace"))
        return f"{value:0{self.digits}X}"

    def encode(self, line: str) -> str:
        return f"{line} {self.separator}{self.compute(line)}"

    def decode(self, line: str) -> Decoded:
        match = self._pattern.match(line)
        if not match:
            return Decoded(payload=line, crc_present=False, crc_ok=True)
        payload = match.group("payload")
        expected = self.compute(payload)
        return Decoded(
            payload=payload,
            crc_present=True,
            crc_ok=match.group("crc").upper() == expected,
        )


def build_codec(cfg) -> Codec:
    if cfg.crc_enabled:
        return Crc16Codec(cfg.crc_separator, cfg.crc_digits)
    return PlainCodec()
