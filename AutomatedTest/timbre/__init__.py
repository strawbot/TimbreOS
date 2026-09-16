"""Test harness for the TimbreOS command line interpreter (ActiveRobot)."""

from .cli import Cli, CommandSpec, StackError
from .codec import Crc16Codec, PlainCodec, build_codec, crc16_ccitt
from .config import DEFAULT, TargetConfig
from .conformance import EffectResult, check_effect, derive_expected, is_probe, probe
from .session import ProtocolError, Response, Session
from .stack import StackParseError, StackSnapshot, is_sentinel, parse_stack_output, sentinel
from .timing import ActionTiming, TimingInstrument, TimingViolation
from .transport import (
    HostTransport,
    SerialTransport,
    Transport,
    TransportError,
    wait_until_responsive,
)

__all__ = [
    "Cli", "CommandSpec", "StackError",
    "Crc16Codec", "PlainCodec", "build_codec", "crc16_ccitt",
    "DEFAULT", "TargetConfig",
    "EffectResult", "check_effect", "derive_expected", "is_probe", "probe",
    "ProtocolError", "Response", "Session",
    "StackParseError", "StackSnapshot", "is_sentinel", "parse_stack_output", "sentinel",
    "ActionTiming", "TimingInstrument", "TimingViolation",
    "HostTransport", "SerialTransport", "Transport", "TransportError",
    "wait_until_responsive",
]
