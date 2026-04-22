from enum import IntEnum
from dataclasses import dataclass

# ── enums ─────────────────────────────────────────

class Slice(IntEnum):
    LOAF = 0
    ACTUATION = 1
    OPTICS = 2
    POWER = 3
    SENSING = 4

class ActuationPortion(IntEnum):
    BOOT_MECHANISM = 0
    HEATING_ELEMENT = 1
    SALT_SPREADING = 2

class OpticsPortion(IntEnum):
    MOTION_PRESENCE = 0
    SNOW_HEIGHT = 1
    PRECIPITATION = 2
    LIGHT_LEVEL = 3

class PowerPortion(IntEnum):
    POWER_OUTAGE = 0
    POWER_DRAW = 1
    ESTOP = 2

class SensingPortion(IntEnum):
    TEMPERATURE = 0
    HUMIDITY = 1
    BOOT_DETECT = 2
    WEATHER_DATA = 3

PORTIONS = {
    Slice.ACTUATION: ActuationPortion,
    Slice.OPTICS: OpticsPortion,
    Slice.POWER: PowerPortion,
    Slice.SENSING: SensingPortion,
}

# ── helpers ──────────────────────────────────────

def pack_id(slice_type, portion):
    return (slice_type << 4) | portion

def unpack_id(byte):
    return Slice(byte >> 4), byte & 0xF

def resolve_id(byte):
    s, p = unpack_id(byte)
    enum = PORTIONS.get(s)
    return s, enum(p) if enum else None

# ── packet ───────────────────────────────────────

@dataclass
class Packet:
    slice_type: Slice
    portion: IntEnum | None
    payload: bytes = b''

    def to_bytes(self):
        return bytes([
            pack_id(self.slice_type, self.portion or 0),
            len(self.payload)
        ]) + self.payload

    @classmethod
    def from_bytes(cls, data: bytes):
        if len(data) < 2:
            raise ValueError("Too short")

        sid, length = data[:2]
        payload = data[2:2+length]

        if len(payload) != length:
            raise ValueError("Incomplete payload")

        s, p = resolve_id(sid)
        return cls(s, p, payload)

    def __str__(self):
        name = self.portion.name if self.portion else "N/A"
        return f"{self.slice_type.name}.{name}: {self.payload.hex()}"

# from enum import IntEnum
# from dataclasses import dataclass
#
# class Slice(IntEnum):
#     LOAF      = 0x0
#     ACTUATION = 0x1
#     OPTICS    = 0x2
#     POWER     = 0x3
#     SENSING   = 0x4
#
# class ActuationPortion(IntEnum):
#     BOOT_MECHANISM  = 0x0
#     HEATING_ELEMENT = 0x1
#     SALT_SPREADING  = 0x2
#
# class OpticsPortion(IntEnum):
#     MOTION_PRESENCE    = 0x0
#     SNOW_HEIGHT        = 0x1
#     PRECIPITATION      = 0x2
#     LIGHT_LEVEL        = 0x3
#
# class PowerPortion(IntEnum):
#     POWER_OUTAGE = 0x0
#     POWER_DRAW   = 0x1
#     ESTOP        = 0x2
#
# class SensingPortion(IntEnum):
#     TEMPERATURE   = 0x0
#     HUMIDITY      = 0x1
#     BOOT_DETECT   = 0x2
#     WEATHER_DATA  = 0x3
#
# # Map each slice to its portion enum for validation
# PORTION_MAP = {
#     Slice.ACTUATION: ActuationPortion,
#     Slice.OPTICS:    OpticsPortion,
#     Slice.POWER:     PowerPortion,
#     Slice.SENSING:   SensingPortion,
# }
#
# def pack_slice_id(slice_type: Slice, portion: IntEnum) -> int:
#     """Pack slice and portion into a single byte. Upper nibble = slice, lower = portion."""
#     if not 0x0 <= slice_type <= 0xF:
#         raise ValueError(f"Slice type {slice_type} out of range (0x0-0xF)")
#     if not 0x0 <= portion <= 0xF:
#         raise ValueError(f"Portion {portion} out of range (0x0-0xF)")
#     return ((slice_type & 0xF) << 4) | (portion & 0xF)
#
# def unpack_slice_id(slice_id_byte: int) -> tuple[int, int]:
#     """Unpack a slice ID byte into (slice_type, portion)."""
#     slice_type = (slice_id_byte >> 4) & 0xF
#     portion    = slice_id_byte & 0xF
#     return slice_type, portion
#
# def resolve_slice_id(slice_id_byte: int) -> tuple[Slice, IntEnum | None]:
#     """Unpack and resolve to named enums. Returns (Slice, Portion) or (Slice, None)."""
#     raw_slice, raw_portion = unpack_slice_id(slice_id_byte)
#     try:
#         slice_type = Slice(raw_slice)
#     except ValueError:
#         raise ValueError(f"Unknown slice type: 0x{raw_slice:X}")
#
#     portion_enum = PORTION_MAP.get(slice_type)
#     if portion_enum is None:
#         return slice_type, None   # LOAF has no portions yet
#
#     try:
#         portion = portion_enum(raw_portion)
#     except ValueError:
#         raise ValueError(f"Unknown portion 0x{raw_portion:X} for slice {slice_type.name}")
#
#     return slice_type, portion
#
# import struct
#
# @dataclass
# class Packet:
#     slice_type: Slice
#     portion:    IntEnum
#     payload:    bytes = b''
#
#     # ── encoding ──────────────────────────────────────────────
#
#     def to_bytes(self) -> bytes:
#         """Serialize to wire format: [SLICE_ID][LENGTH][PAYLOAD][CRC8]"""
#         slice_id = pack_slice_id(self.slice_type, self.portion)
#         length   = len(self.payload)
#         if length > 255:
#             raise ValueError(f"Payload too large: {length} bytes (max 255)")
#
#         raw = bytes([slice_id, length]) + self.payload
#         return raw
#
#     # ── decoding ──────────────────────────────────────────────
#
#     @classmethod
#     def from_bytes(cls, data: bytes) -> 'Packet':
#         """Deserialize from raw bytes. Raises on bad length or CRC."""
#         if len(data) < 3:
#             raise ValueError("Packet too short")
#
#         slice_id_byte = data[0]
#         length        = data[1]
#         expected_len  = 2 + length   # header + payload
#
#         if len(data) < expected_len:
#             raise ValueError(f"Incomplete packet: got {len(data)}, need {expected_len}")
#
#         payload = data[2:2 + length]
#
#         slice_type, portion = resolve_slice_id(slice_id_byte)
#         return cls(slice_type=slice_type, portion=portion, payload=bytes(payload))
#
#     # ── display ───────────────────────────────────────────────
#
#     def __str__(self) -> str:
#         portion_name = self.portion.name if self.portion else 'N/A'
#         return (
#             f"Packet({self.slice_type.name}.{portion_name}) "
#             f"payload={self.payload.hex()} ({len(self.payload)} bytes)"
#         )