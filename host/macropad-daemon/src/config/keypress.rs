#[derive(Clone, Default, Debug, PartialEq)]
pub struct KeyPress {
    pub mod_byte: u8,
    pub scan_code: u8,
}

impl From<u16> for KeyPress {
    fn from(item: u16) -> KeyPress {
        KeyPress {
            mod_byte: ((0xFF00 & item) >> 8) as u8,
            scan_code: (0x00FF & item) as u8,
        }
    }
}

impl Into<u16> for KeyPress {
    fn into(self) -> u16 {
        (self.mod_byte as u16) << 8 | self.scan_code as u16
    }
}
